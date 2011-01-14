/*
 * $Id: eval.c,v 1.12 2002/02/18 19:27:11 cssbz Exp $
 *
 *  This file is part of Eden.
 *
 *  Eden is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Eden is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Eden; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

static char rcsid[] = "$Id: eval.c,v 1.12 2002/02/18 19:27:11 cssbz Exp $";

#include <stdio.h>

#include "../../../../../config.h"
#include "eden.h"
#include "yacc.h"

#ifdef DEBUG
#define	DEBUGPRINT(s,t) if (Debug&1)fprintf(stderr,s,t);
#define	DEBUGPRINT2(s,t,u) if (Debug&1)fprintf(stderr,s,t,u);
#else
#define DEBUGPRINT(s,t)
#define DEBUGPRINT2(s,t,u)
#endif

/* for debugging only */
#define print_action_list() {\
	extern void action_list();\
	action_list();\
	print(pop(),stderr,1);\
	printf("\n");\
}

#define address(sp) { Datum d; d.type = SYMBOL | DPTR; d.u.v.x = (Int) &sp->d; d.u.v.y = (Int) sp; push(d); }

symptr_QUEUE formula_queue = EMPTYQUEUE(formula_queue);
symptr_QUEUE action_queue = EMPTYQUEUE(action_queue);

extern void doste_trigger(char*,int);

/* function prototypes */
static void schedule(symptr);
void        schedule_parents_of(symptr);
void        resetLock(void);
void        eval_formula_queue(void);
void        invoke_action_queue(void);
void        eager(void);
void        change(symptr, int);
void        mark_changed(symptr);
void        formula_list(void);
void        action_list(void);
void        reset_eval(void);

/* schedule: adds a given symbol to the appropriate queue depending
   upon its type.  Formulae are added to the formula queue; actions
   are added to the action queue if they have sources (ie if it is a
   "triggered action" -- proc p:v {} is a triggered action, proc p {}
   is not); other types are ignored.

   If it is already in the queue, then move it to the end, so
   duplicates do not appear.  This rule does not apply in dtkeden if
   synchronize is on.  In this case, we may want to faithfully enact
   each redefinition, as they may be coming from another machine, even
   if we have not managed to finish evaluating the queue yet.  Hence
   duplicates are allowed in this case.

   [Ash] */
static void
schedule(symptr sp)
{
    register symptr_ATOM P;
    register symptr_QUEUE *Q;
#ifdef DISTRIB
    extern Int *synchronize;
#endif /* DISTRIB */

    switch (sp->stype) {
    case FORMULA:
        DEBUGPRINT("FQUEUE schedule %s\n", sp->name);
	Q = &formula_queue;
	break;

    case BLTIN:
    case PROCEDURE:
    case FUNCTION:
    case PROCMACRO:
        DEBUGPRINT("AQUEUE schedule %s\n", sp->name);
	Q = &action_queue;
	break;

    default:
        /* If VAR, for example */
        DEBUGPRINT2("schedule: not scheduling %s %s\n", typename(sp->stype),
		    sp->name);
	return;			/* don't queue up */
    }

/*
    if (P = SEARCH_symptr(Q, sp, 1)) {
*/
#ifdef DISTRIB
    if ((P = sp->Qloc) && !*synchronize)
      /* This may raise a problem in a distributed or synchronized
	environment, because some information will be missed. For
	example, if we want to make a train move smoothly, we need to
	show every slight move. But this reschedule may make previous
	movements disappear. So we take synchronization into account
	--sun */
#else
    if ((P = sp->Qloc))
#endif /* not DISTRIB */

    {
	/* Already there --> reschedule */
	DELETE_ATOM(Q, P);
	APPEND_Q(Q, P);
    } else {
	/* Not there --> put it at the end */
	switch (sp->stype) {
	case FORMULA:
	    APPEND_symptr_Q(Q, sp);
	    sp->Qloc = Q->prev;
	    break;

	case BLTIN:
	case PROCEDURE:
        case PROCMACRO:
	case FUNCTION:		/* if it's an action */
	    if (!Q_EMPTY(&sp->sources)) {
		APPEND_symptr_Q(Q, sp);
		sp->Qloc = Q->prev;
	    }
	    break;
	}
    }
}

 /*----------------------------------------------**
 **	queue up all formulae and actions	**
 **	which depends on v 			**
 **----------------------------------------------*/
void
schedule_parents_of(symptr v)
{				/* builtin function */
    register symptr_ATOM P;
    register symptr_QUEUE *Q;

   /* Tell DOSTE about changes to certain observables that it is listening to [Nick] */
   if (((v->stype == 264) || (v->stype == 265))) { /* Removed  && (v->doste == 1) to always tell DOSTE [Nick] */
   	doste_trigger(v->name, v->context);
   }

    Q = &v->targets;
    FOREACH(P, Q) {
      mark_changed(P->obj); /* this target and recursively all of its
                               targets... [Ash] */
      schedule(P->obj);
    }
}

static int  lock = FALSE;	/* prevent nested eval & invoke of
                                   formula and action */

/* resetLock status after error */
void
resetLock(void)
{
  DEBUGPRINT("MCSTAT resetLock\n", 0);
  lock = FALSE;
}

/***** evaluate all formulae in the formula queue *****/
/* eval_formula_queue().  

   This procedure prevents reentry by use of a lock: attempts to
   reenter whilst in execution will do nothing.

   Take one symbol at a time from the front of the formula queue.  If
   it is marked as changed, and if it is ready (all its sources have
   unchanged values), then run its VM code to form its new value.

   Any formulae in the queue which are unchanged or not ready are not
   evaluated: they will presumably be done at the next
   eval_formula_queue, or perhaps they will be added to the end of the
   queue during this process.

   When this process has finished, evaluate the action queue (so that
   the state represented by definitions is presented as consistent to
   the actions).

   [Ash]
 */
void
eval_formula_queue(void)
{				/* internal */
    extern void update(symptr);
    extern int  ready(symptr);
    register symptr_QUEUE *Q;
    register symptr sp;
#ifdef DISTRIB
    extern int  triggeredAction;
#endif /* DISTRIB */

    if (lock)
	return;
    lock = TRUE;
    Q = &formula_queue;

    DEBUGPRINT("MCSTAT|FQUEUE|SYMTBL|AQUEUE eval_formula_queue\n", 0);

    while (!Q_EMPTY(Q)) {
	sp = FRONT(Q)->obj;
	DELETE_FRONT_symptr(Q);
	sp->Qloc = 0;
	if (sp->changed) {
	    if (ready(sp)) {
		DEBUGPRINT("eval_formula_queue: formula %s changed and ready: evaluating it\n", sp->name);

#ifdef DISTRIB
		triggeredAction = 1;
#endif /* DISTRIB */

		/* evaluate the formula by executing its VM code */
		update(sp);

#ifdef DISTRIB
		triggeredAction = 0;
#endif /* DISTRIB */

	    } else {
		DEBUGPRINT("eval_formula_queue: formula %s not ready: not evaluating it\n", sp->name);
	    }
	} else {
	    DEBUGPRINT("eval_formula_queue: formula %s not changed: not evaluating it\n",
		       sp->name);
	}
    }
    lock = FALSE;
    if (!Q_EMPTY(&action_queue))
	invoke_action_queue();
}

/***** invoke all actions in the action queue *****/
/* invoke_action_queue().

   This procedure prevents reentry by use of a lock: attempts to
   reenter whilst in execution will do nothing.

   Take one symbol at a time from the front of the action queue.  If
   it is ready (we don't care if it is marked as changed: actions are
   always evaluated if they are on the queue), call it, then throw
   away the value returned.

   Actions in the queue which are not ready (which have sources which
   are changed) are not evaluated: they will presumably be done at the
   next invoke_action_queue, or perhaps they will be added to the end
   of the queue during this process.

   When this process has finished, evaluate the formula queue if it
   now has items (so that formula and action queues are executed until
   they are empty).

   [Ash]
*/
void
invoke_action_queue(void)
{				/* internal */
    extern void call(symptr, Datum, char *);
    extern int  ready(symptr);
    register symptr_QUEUE *Q;
    register symptr sp;
    Datum       d;

    if (lock)
	return;
    lock = TRUE;
    Q = &action_queue;

    DEBUGPRINT("MCSTAT|AQUEUE|DATSTK|FQUEUE invoke_action_queue\n", 0);

    while (!Q_EMPTY(Q)) {
	sp = FRONT(Q)->obj;
	DELETE_FRONT_symptr(Q);
	sp->Qloc = 0;
	if (ready(sp)) {
	    DEBUGPRINT("invoke_action_queue: invoke action %s\n", sp->name);
	    makearr(0);
	    d = pop();		/* creat null arguments */
	    call(sp, d, sp->name);	/* invoke action */
	    popd();		/* drop the value returned */
	    /* eval_formula_queue(); */
	} else {
	    DEBUGPRINT("invoke_action_queue: action %s not invoked\n",
		       sp->name);
	}
    }
    lock = FALSE;
    if (!Q_EMPTY(&formula_queue))
	eval_formula_queue();
}


/* eager(): force formula and action queue to evaluate (unless
   autocalc off) */
#ifndef TTYEDEN
#include <tk.h>
extern Tcl_Interp *interp;
#endif
void
eager(void)
{				/* builtin C function */
    int         lockvalue;

    DEBUGPRINT("VMOPER|MCSTAT|FQUEUE eager\n", 0);

    lockvalue = lock;
    lock = FALSE;
    eval_formula_queue();
    lock = lockvalue;

#ifndef TTYEDEN
    Tcl_EvalEC(interp, "update");
#endif
}


#ifdef INSTRUMENT
#include <sys/time.h>
#endif

/* change(): put `sp' in schedule and if `autocalc' is enabled
   evaluate it */
void
change(symptr sp, int flag)
{
    extern Int *autocalc;
    register Inst *savepc;
#ifdef INSTRUMENT
    hrtime_t start, end;
    extern hrtime_t insSchedule;
#endif

    DEBUGPRINT2("SYMTBL|FQUEUE|VMEXEC change %s->changed = %d)\n",
		sp->name, flag);

#ifdef INSTRUMENT
    start = gethrtime();
#endif
    sp->changed = flag;
    schedule(sp);
    schedule_parents_of(sp); /* schedule all targets of sp.  Mark all
                                targets of sp as changed.
                                Additionally mark all targets of those
                                targets as changed.  (Why not follow
                                all the way until find already marked
                                as changed or end of the chain?)  [Ash] */
#ifdef INSTRUMENT
    end = gethrtime();
    insSchedule += (end - start);
#endif

    if (*autocalc) {
	savepc = pc;
	eval_formula_queue();
	/* invoke_action_queue(); */
	pc = savepc;
    }
}

/* Recursively mark all targets (and targets of targets, and targets
   of targets of targets etc) as changed [Ash] */
void
mark_changed(symptr sp)
{
    register symptr_QUEUE *P, *Q;

    DEBUGPRINT("SYMTBL mark_changed(%s)\n", sp->name);

    /* Lets DOSTE know about Eden observable changes [Nick] */
    if (((sp->stype == 264) || (sp->stype == 265))) { /* && (sp->doste == 1) */
	doste_trigger(sp->name, sp->context);
    }

    if (!sp->changed) {		/* not already marked */
	sp->changed = TRUE;
	Q = &sp->targets;
	FOREACH(P, Q)
	    mark_changed(P->obj);
    }
}

/* convert formula-queue to a list of pointers */
void formula_list(void)
{				/* builtin function */
    register int count;
    register symptr_ATOM P;
    register symptr_QUEUE *Q;

    count = 0;
    Q = &formula_queue;
    FOREACH(P, Q) {
    	address(P->obj);
    	count++;
    }
    makearr(count);
}

/* convert action-queue to a list of pointers */
void action_list(void)
{				/* builtin function */
    register symptr_ATOM P;
    register symptr_QUEUE *Q;
    register int count;

    count = 0;
    Q = &action_queue;
    FOREACH(P, Q) {
    	address(P->obj);
    	count++;
    }
    makearr(count);
}

void
reset_eval(void)
{
    register symptr_ATOM P;
    register symptr_QUEUE *Q;

    DEBUGPRINT("MCSTAT|FQUEUE|AQUEUE reset_eval\n", 0);

    lock = FALSE;
    Q = &formula_queue;
    FOREACH(P, Q) {
	P->obj->Qloc = 0;
    }
    CLEAR_symptr_Q(&formula_queue);
    Q = &action_queue;
    FOREACH(P, Q) {
	P->obj->Qloc = 0;
    }
    CLEAR_symptr_Q(&action_queue);
}
