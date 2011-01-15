/*
 * $Id: code.c,v 1.24 2002/03/01 23:45:10 cssbz Exp $
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

static char rcsid[] = "$Id: code.c,v 1.24 2002/03/01 23:45:10 cssbz Exp $";

#include <stdio.h>
#include <string.h>

#include "../../../config.h"
#include "eden.h"
#include "yacc.h"
#include "notation.h"

#include "emalloc.h"
#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

#ifdef DEBUG
#define DEBUGPRINT(X,Y,Z) if(Debug&1){fprintf(stderr,X,Y,Z);}
#else
#define DEBUGPRINT(X,Y,Z)
#endif

Datum       UndefDatum = {UNDEF};

Datum       stack[NSTACK];	/* the stack */
Datum      *stackp;		/* next free spot on stack */

Inst        prog[NPROG];	/* the machine */
Inst       *progp = prog;	/* next free spot for code generation */
Inst       *pc;			/* program counter during execurtion */

union compiler_flags compiler_flag;

Frame       frame[NFRAME];
Frame      *fp;			/* frame pointer */

#define MAXENTRY 200
struct t    entry_tbl[MAXENTRY];/* case entry table */
struct t   *entry_ptr;

/* function prototypes */
void        reset_compiler_status(void);
void        initcode(void);
void        stack_overflow_err(void);
Datum       stack_underflow_err(void);
void        execute(Inst *);
void        ret_call(void);
void        call(symptr, Datum, char *);
void        eval(void);
void        change_targets(symptr, symptr_QUEUE *);
void        change_sources(symptr, symptr_QUEUE *);
void        related_by_code(void);
Inst       *code(Inst);
void        codeswitch(struct t *);
void        addentry(Datum *, Inst *);
Inst       *saveprog(Inst *, Inst *);
void        push_text(char *, int);
char       *savetext(char *);
Inst       *code_related_by(symptr);
Inst       *code_related_by_runtimelhs();
Inst       *code_definition(Int, symptr, Inst *, Inst *, Int, char *);
Inst       *code_definition_runtimelhs(Int, Inst *, Inst *, Int, char *);
Inst       *code_eval(Inst *, Inst *);
void       evalDatum(Datum);
#ifdef DISTRIB
int        handle_check(symptr);   /*  for agency  --sun */
int        oracle_check(symptr);
int        handle_check1(char *);
int        oracle_check1(char *);
/* Say we have two definitions, 1) a is b+c, 2) x is y+a.  Say agent
   SUN has a handle on a, but not on x.  Strictly SUN should be
   allowed to change (1), but the change should not propagate to (2) -
   but this would cause inconsistency and so is not implemented in
   dtkeden.  Instead we turn off all handle checking when performing
   triggeredActions [Ash, with Patrick] */
int        triggeredAction = 0;
#endif /* DISTRIB */


/* SUBROUTINE INITIALIZE THE RUN-TIME STATUS */

#define reset_prog_ptr()        (progp=prog)

void
reset_compiler_status(void)
{
    extern void clear_IDlist(void);

    DEBUGPRINT("VMEXEC|MCSTAT reset_compiler_status\n", 0, 0);

    reset_prog_ptr();
    clear_IDlist();
    reset_entry_tbl();
    reset_compiler_flags();
}

void
initcode(void)
{
    reset_frames();
    freeheap();
    reset_stack();
    reset_eval();
    reset_compiler_status();
}

void
stack_overflow_err(void)
{
    error("stack overflow");
}

Datum
stack_underflow_err(void)
{
    error("stack underflow");
    return UndefDatum;		/* dummy */
}

#if defined(DEBUG) && !defined(WEDEN_ENABLED)
/* Normally, push and pop are macros.  With debugging, however, I want
   to be sure that side effect won't be a problem etc, so I'm
   rewriting them as functions.  [Ash] */
/* The normal defintions can be found in eden.h around Line 180 */
inline void push(Datum d) {
  if (stackp>=&stack[NSTACK]) stack_overflow_err();
  else {
#ifdef DEBUG
    if (Debug&1) {
    	fprintf(stderr, "DATSTK push ");
    	fprintf(stdout, "About to run print");
    	print(d, stderr, 1);
    	fprintf(stderr, " (datum at 0x%x, pushed to DATSTK %d)\n", &d, stackp - stack);
    }
#endif
    *stackp++ = d;
  }
}
inline Datum pop(void) {
  Datum d;

  if (stackp == stack) d = stack_underflow_err();
  else {
    d = *--stackp;

#ifdef DEBUG
    if (Debug&1) {
      fprintf(stderr, "DATSTK pop ");
      print(d, stderr, 1);
      fprintf(stderr, " (DATSTK now %d)\n", stackp - stack);
    }
#endif
    
  }
  return d;
}
#endif /* DEBUG and not WebEDEN */

#ifdef DEBUG
struct INSTTBL {
  char *name;
  void (*func) ();
};
struct INSTTBL insttbl[] = {
#define INCLUDE 'T'
#include "inst.h"
#undef INCLUDE
  {0, 0}
};
/* Disassemble: convert a VM opcode into its symbolic name [Ash] */
char * disAss(Inst d) {
  int i;
  static char addr[30]; /* should be big enough */

  for (i = 0; insttbl[i].name; i++)
    if (d == insttbl[i].func) return insttbl[i].name;

  /* Not found */
  sprintf(addr, "0x%x (%d)", d, d);
  return addr;
}
#endif /* DEBUG */

/* RUN-TIME CODE EVALUATE FUNCTION ID(...) WHERE ID = BLTIN, LIB, FUNCTION OR
   PROCEDURE. */
/* also can used to execute a heap of Inst which ents at (rts)  --sun */

/* This virtual machine technique is explained nicely in "The Practice
   of Programming" by Brian W. Kernighan and Rob Pike (1999)
   Addison-Wesley [Ash] */

void execute(Inst * p)
{
    Inst       *resume_point;
    extern int interrupted;

    DEBUGPRINT("VMEXEC|VMREAD execute(0x%x) (to start, %d items on DATSTK)\n",
	       p, stackp - stack);
    resume_point = pc;
    pc = p;
    while (*pc && !interrupted) {
      DEBUGPRINT("VMEXEC|VMREAD execute: next is pc=0x%x, *pc=%s\n",
		 pc, disAss(*pc));
      (*(*pc++)) ();
    }
    pc = resume_point;
    DEBUGPRINT("VMEXEC execute end execute(0x%x) (now %d items on DATSTK)\n",
	       p, stackp - stack);
}

void ret_call(void)
{
    int         i;

    DEBUGPRINT("CALSTK|DATSTK ret_call\n", 0, 0);

    if (fp->sp) {
      switch (fp->sp->stype) {
      case FUNCTION:
      case PROCMACRO:
      case PROCEDURE:
	i = fp->sp->nauto;
	/*
	if (stackp != &fp->stackp[i + 1]) {
	  noticef("stack pointer was inconsistent on returning from %s %s (discarding %d Datums from stack)",
		  typename(fp->sp->stype), fp->sp->name,
		  stackp - &fp->stackp[i + 1]);
	}
	*/
	while (i >= 0)
	  freedatum(fp->stackp[i--]);
	break;
      }
    }
    if (fp->master)
	popMasterStack();
    popEntryStack();
    pc = fp->retpc;
    stackp = fp->stackp;
    --fp;
}

void call(symptr sp, Datum args, char *master)
{
    extern void call_lib(Inst), call_float(Inst);
    int         i;
    Datum       d;
    extern int appAgentName;

    if (fp++ >= &frame[NFRAME - 1]) {
      fp--; /* error handling will try to print out info from current frame */
      errorf("call to %s %s nested too deeply",
	     typename(sp->stype), sp->name);
    }
    DEBUGPRINT("CALSTK|DATSTK call %s: frame level %d\n",
	       sp->name, fp - frame);
    /* appAgentName--; */ /* This statement may cause AgentName can't
                             be prefixed */
    fp->sp = sp;
    fp->retpc = pc;
    fp->stackp = stackp;
    fp->hptr = hptr;
    pushEntryStack(INTERNAL);
    fp->master = master;
    if (master)
      pushMasterStack(master); /* !@!@ perhaps sp->name here? [Ash] */

    switch (sp->stype) {
    case FUNCTION:
    case PROCMACRO:
    case PROCEDURE:

#if defined(DEBUG) && !defined(WEDEN_ENABLED)
      if (Debug & 512) {
	for (i = 0; i < fp - frame; i++)
	  fprintf(stderr, " ");
	fprintf(stderr, "%s %s starting: args ",
		typename(sp->stype), sp->name);
	print(args, stderr, 1);
	fprintf(stderr, "\n");
      }
#endif

	push(newdatum(args));	/* put the arguments back */
	i = sp->nauto;		/* creat local variables */
	while (i--)
	    pushUNDEF();
	execute(sp->inst);

#if defined(DEBUG) && !defined(WEDEN_ENABLED)
      if (Debug & 512) {
	for (i = 0; i < fp - frame; i++)
	  fprintf(stderr, " ");
	fprintf(stderr, "%s %s ending: return ",
		typename(sp->stype), sp->name);
	print(*(stackp-1), stderr, 1);
	fprintf(stderr, "\n");
      }      
#endif

	break;

    case BLTIN:
	push(args);		/* don't copy the whole structure ! */
	((Inst) sp->inst) ();
	break;

    case LIB:
	push(args);		/* don't copy the whole structure ! */
	call_lib((Inst) sp->inst);	/* call the lib interface */
	break;

    case LIB64:
	push(args);		/* don't copy the whole structure ! */
	call_lib64((Inst) sp->inst);	/* call the lib interface */
	break;

    case RLIB:			/* floating point C-function */
	push(args);
	call_float((Inst) sp->inst);
	break;

    default:
	errorf("func/proc/procmacro '%s' needed", sp->name);
	break;
    }

    d = newhdat(pop());		/* preserve function return value */
    ret_call();
    push(d);

    /* appAgentName++; */  /* same as above problem --sun */

    DEBUGPRINT("CALSTK end call %s (level %d)\n", sp->name, fp - frame);
}

void eval(void)
{
    Datum       args;
    Datum       lvalue;

    DEBUGPRINT("VMOPER|CALSTK eval\n", 0, 0);

    args = pop();		/* arguments (a list) */
    lvalue = pop();		/* lvalue */

    switch (lvalue.type) {
    case BLTIN:
    case LIB:
    case LIB64:
    case RLIB:
    case FUNCTION:
    case PROCMACRO:
    case PROCEDURE:
      /* This works in this case...
	 debugMessage("eval %s\n", lvalue.u.sym->name);
	 [Ash] */
	call(lvalue.u.sym, args, 0);
	break;
    default:
	errorf("func/proc/procmacro needed, found %s",
		typename(lvalue.type));
	break;
    }
}

/* SUBROUTINE add a symbol to by_list then change the related objects */
void change_targets(symptr sp, symptr_QUEUE * splist)
{
    void        refer_by(symptr, symptr_QUEUE *);
    symptr_QUEUE *A;

    DEBUGPRINT("SYMTBL|DEFNET change_targets %s\n", sp->name, 0);

    refer_by(sp, splist);
    FOREACH(A, splist) {
	change(A->obj, FALSE /* A->obj->changed */ );
    }
}

/* SUBROUTINE add a symbol to the to_list of formula, function/procedure then
   change the related objects */
void change_sources(symptr sp, symptr_QUEUE * splist)
{
    void        refer_to(symptr, symptr_QUEUE *);
    extern int appAgentName;
#ifdef DISTRIB
    extern void propagate_agency(symptr);    /* for agency  --sun */
#endif /* DISTRIB */

    DEBUGPRINT("SYMTBL|DEFNET change_sources %s\n", sp->name, 0);

    switch (sp->stype) {
    case VAR:
    case FORMULA:
      refer_to(sp, splist); /* record (using sources and triggers)
                               that sp refers to splist [Ash] */
      change(sp, TRUE);
      break;

    case FUNCTION:
    case PROCMACRO:
    case PROCEDURE:
      refer_to(sp, splist); /* record (using sources and triggers)
                               that sp refers to splist [Ash] */
      change(sp, !Q_EMPTY(splist));
      break;

    default:
      error("internal error: change_sources()");
    }

#ifdef DISTRIB
    if (appAgentName > 0) propagate_agency(sp);
#endif /* DISTRIB */

}

/* RUN-TIME CODE check a symbol whether is cyclic defined if ok then add it
   to the by_list */
void related_by_code(void)
{
    extern int  checkok(symptr, symptr_QUEUE *);
    symptr      sp;
    symptr_QUEUE *splist;
    symptr_ATOM P,P2;
    symptr_QUEUE temp;

#ifdef NO_CHECK_CIRCULAR
    extern int  NCC;
#endif				/* NO_CHECK_CIRCULAR */

    DEBUGPRINT("VMOPER|VMREAD|SYMTBL|DEFNET related_by_code\n", 0, 0);

    sp = (symptr) (*pc++);
    splist = (symptr_QUEUE *) (*pc++);

#ifdef NO_CHECK_CIRCULAR
    if (!NCC)
#endif				/* NO_CHECK_CIRCULAR */

	/* Correctly does cyclic check for a ~> [b] operations. [Nick] */
	CLEAN_Q(&temp);
	ALLOC_ATOM(P2);
	P2->obj = sp;
	APPEND_ATOM(&temp,P2);

	FOREACH(P, splist) {
		if (!checkok(P->obj, &temp))
			error2(P->obj->name, ": CYCLIC DEFINITION: ABORTED");
	}

	free(P2);

    change_targets(sp, splist);
}

/* RUN-TIME CODE check a symbol whether is cyclic defined if ok then add it
   to the by_list */
void related_by_code_runtimelhs(void)
{
    extern int  checkok(symptr, symptr_QUEUE *);
    symptr      sp;
    symptr_QUEUE *splist;
    Datum addr;
#ifdef NO_CHECK_CIRCULAR
    extern int  NCC;
#endif				/* NO_CHECK_CIRCULAR */

    DEBUGPRINT("VMREAD|SYMTBL|DEFNET related_by_code_runtimelhs\n", 0, 0);

    addr = pop();
    mustaddr(addr, "related_by_code_runtimelhs");
    sp = symbol_of(addr);

    splist = (symptr_QUEUE *) (*pc++);

#ifdef NO_CHECK_CIRCULAR
    if (!NCC)
#endif				/* NO_CHECK_CIRCULAR */
      if (!checkok(sp, splist))
	error2(sp->name, ": CYCLIC DEFINITION: ABORTED");

    change_targets(sp, splist);
}

/* ENCODER install one instruction or operand */
Inst       *
code(Inst f)
{
    Inst       *oprogp = progp;

#ifdef DEBUG
    if ((Debug & 1) || (Debug & 1024)) {
      debugMessage("VMWRIT code %s to location progp=0x%x\n",
	      disAss(f), progp);
    }
#endif

    if (progp >= &prog[NPROG])
	error("program too big");
    *progp++ = f;
    return oprogp;
}

/* RUN-TIME CODE actual execution code of formula/function/procedure
   definition */

void codeswitch(struct t * tbl)
{
    Inst       *ip, *defp = (Inst *) 0;
    struct t   *t = tbl;

    DEBUGPRINT("VRWRIT codeswitch\n", 0, 0);

    for (; tbl != entry_ptr; tbl++) {
	if (tbl->dp) {
	    ip = code((Inst) tbl->dp);
	    code((Inst) (tbl->ip - ip));
	} else
	    defp = tbl->ip;
    }
    entry_ptr = t;		/* reset */
    ip = code((Inst) 0);
    if (defp)
	code((Inst) (defp - ip));
    else
	code((Inst) 2);		/* there is no default case */
}

/* ENCODER encode ``CASE'' or ``DEFAULT'' entry */
void addentry(Datum * dp, Inst * ip)
{
    if (entry_ptr < &entry_tbl[MAXENTRY]) {
	entry_ptr->dp = dp;
	entry_ptr->ip = ip;
	entry_ptr++;
    } else
	error("no. of 'case' overflow");
}

Inst       *
saveprog(Inst * p_begin, Inst * p_end)
 /* save a piece of program code at a permanent place */
{
    int         size;
    Inst       *p;
    Inst       *q;

    size = ((char *) p_end) - ((char *) p_begin);
    p = (Inst *) emalloc(size);

    /* copy from p_begin to p_end into p (use q as an iterator) [Ash] */
    for (q = p; p_begin != p_end;)
	*q++ = *p_begin++;

    return p;
}

#define LARGE_TEXT      0x20000

char        textcode[LARGE_TEXT];	/* INPUT TEXT BUFFER */
char       *textptr = textcode;	/* end position of text buffer */

void
push_text(char *text, int len)
 /* push some text on textcode */
 /* len - number of char */
{				/* orginal text is temp. stored */
    if (textptr + len >= &textcode[LARGE_TEXT])
	error("text buffer overflow");
    strncpy(textptr, text, len);
    textptr += len;
}

char       *
savetext(char *text)
{				/* save text buffer permanently */
    /* text - start position of text buffer */
    char       *s;

    *textptr = '\0';		/* force a null */
    s = (char *) emalloc((textptr - text) + 1);
    return strcpy(s, text);
}

/* ENCODER encode the definition commands sequences */
/* This is used by the parser for "identifier is expr ';'" and
   proc/procmacro/func defines [Ash] */
Inst       *
code_definition(Int type, symptr id,
		Inst * prog_start, Inst * prog_end, Int nauto, char *text)
{
    Inst       *p_begin, *p_end;
    Inst       *oprogp;
    symptr_QUEUE *splist, *save_IDlist(void);

#ifdef DISTRIB
    if (!handle_check(id)) {          /* for agency  --sun */
      splist = save_IDlist(); /* I get the feeling this should clear
                                 the IDlist instead of saving it: when
                                 will splist be freed?  [Ash] */
        return progp;
    }
#endif /* DISTRIB */

    p_begin = saveprog(prog_start, prog_end); /* save prog_start to
                                                 prog_end in a
                                                 permanent place [Ash] */
    p_end = p_begin + (prog_end - prog_start);
    oprogp = progp = prog_start;
    splist = save_IDlist();

    code((Inst) definition);
    code((Inst) type);
    code((Inst) id);
    code((Inst) splist);
    code((Inst) p_begin);
    code((Inst) p_end);
    code((Inst) nauto);
    code((Inst) savetext(text));
    return oprogp;
}

/* This is used by the parser for "'`'expr'`' is expr ';'", and so id
   is not passed in or saved.  Note the first thing coded is
   definition_runtimelhs.  Other than this, it is the same as code_definition
   above.  This used to (less usefully) be called code_definition1.  [Ash] */
Inst       *
code_definition_runtimelhs(Int type,
			   Inst * prog_start, Inst * prog_end,
			   Int nauto, char *text)
{
    Inst       *p_begin, *p_end;
    Inst       *oprogp;
    symptr_QUEUE *splist, *save_IDlist(void);

    p_begin = saveprog(prog_start, prog_end);
    p_end = p_begin + (prog_end - prog_start);
    oprogp = progp = prog_start;
    splist = save_IDlist();

    code((Inst) definition_runtimelhs); /* note the change from
                                           code_definition */
    code((Inst) type);
    code((Inst) splist);
    code((Inst) p_begin);
    code((Inst) p_end);
    code((Inst) nauto);
    code((Inst) savetext(text));

    return oprogp;
}

/* This is used to implement eval(expr).  expr is evaluated now (at
   code time) and is coded as a constant value.  I think this was
   implemented by Patrick.  It was originally called code_definition2,
   which seems a silly name as an eval has no persistent definition
   sense about it.  [Ash] */
Inst       *
code_eval(Inst * prog_start, Inst * prog_end)
{
    Inst       *p_begin, *p_end;
    Inst       *oprogp;
    Datum     *dp, d;
    char *s;
    int i;
    /* extern void print(Datum); */

    code((Inst) 0);
    execute(prog_start);
    unmarkGarbage(prog_start, progp);
      /* the next line is used to give up these codes
         from prog_start to prog_end --sun */
    oprogp = progp = prog_start;
    /* t_str1(pop()); */
    d = pop();
    dp = (Datum *) emalloc(sizeof(Datum));
    dp->type = d.type;
    switch (d.type) {
       case  STRING:
           s = (char *) emalloc(strlen(d.u.s) + 1);
           dp->u.s = (char *) strcpy(s, d.u.s);
           break;
       case  INTEGER:
           dp->u.i = d.u.i;
           break;
       case  REAL:
           dp->u.r = d.u.r;
           break;
       case  MYCHAR:
           dp->u.i = d.u.i;
           break;
       case  UNDEF:
           dp->u.i = d.u.i;
           break;
       case LIST:  /* need more codes --sun */
       	    dp->u.a[0].u.i = d.u.a[0].u.i;
	    for (i = 1; i <= d.u.a[0].u.i; i++) {
	        dp->u.a[i] = d.u.a[i];
	    }
	    break;

       default:
           error("EVAL error : unsuitable usage of eval syntax");
     }
    evalDatum(d);
    markGarbage(progp, dp);
    code((Inst) constpush);
    code((Inst) dp);
    return oprogp;
}

/* ENCODER code the ``id ~> [...]'' command */
Inst       *
code_related_by(symptr sp)
{
    extern symptr_QUEUE *save_IDlist(void);
    symptr_QUEUE *splist;
    Inst       *ip;

    splist = save_IDlist();
    ip = code((Inst) related_by_code);
    code((Inst) sp);
    code((Inst) splist);
    return ip;
}

Inst * code_related_by_runtimelhs() {
    extern symptr_QUEUE *save_IDlist(void);
    symptr_QUEUE *splist;
    Inst       *ip;

    splist = save_IDlist();
    ip = code((Inst) related_by_code_runtimelhs);
    code((Inst) splist);
    return ip;
}

#ifdef DISTRIB
/* SUBROUTE check agency of a symbol --sun */
/* ------------------------------------------------------------------------ */
int
handle_check(symptr sp)
{
   extern char agentName[128];
   extern char *everyone;

   if (sp==0) return 1;
   if (*agentName ==0) return 1; /* mean own */
   if (triggeredAction) return 1; /* means it is a triggered action */
   /* if (Q_EMPTY(&sp->HandleOf)) return 1;  using evryone to allow everyone's agency */
   if (search_agent_Q(&sp->HandleOf, everyone)) return 1;
   if (search_agent_Q(&sp->HandleOf, "SYSTEM")) return 1;
   if (search_agent_Q(&sp->HandleOf, agentName)) {
      /*printf("handleOK %s", sp->name);*/
      return 1;
   }
   /* printf("HandleNo %s %s", agentName, sp->name);*/
   return 0;

}

int handle_check1(char *name)
{
  extern symptr lookup(char *);

  return handle_check(lookup(name));
}

int oracle_check(symptr sp)
{
   extern char agentName[128];
   extern char *everyone;

   if (sp==0)  return 1;
   if (*agentName ==0) return 1; /* means own */
   if (triggeredAction) return 1; /* means it is a triggered action */
   /* if (Q_EMPTY(&sp->OracleOf)) return 1;   using everyone to allow everyone's agency */
   if (search_agent_Q(&sp->OracleOf, everyone)) return 1;
   if (search_agent_Q(&sp->OracleOf, "SYSTEM")) return 1;
   if (search_agent_Q(&sp->OracleOf, agentName)) {
      /* printf("OracleOK %s", sp->name); */
      return 1;
   }
   /* printf("OracleNo %s %s", agentName, sp->name); */
   return 0;

}

int oracle_check1(char *name)
{
  extern symptr lookup(char *);

  return oracle_check(lookup(name));
}
#endif /* DISTRIB */


void evalDatum(Datum d)
{
    char s2[80];
    int i;

    switch (d.type) {
       case  STRING:
           push_text("\"", 1);
           push_text(d.u.s, strlen(d.u.s));
           push_text("\"", 1);
           break;
       case  INTEGER:
           sprintf(s2, "%d", d.u.i);
           push_text(s2, strlen(s2));
           break;
       case  REAL:
           sprintf(s2, "%f", d.u.r);
           push_text(s2, strlen(s2));
           break;
       case  MYCHAR:
           push_text("'", 1);
           sprintf(s2, "%c", d.u.i);
           push_text(s2, 1);
           push_text("'", 1);
           break;
       case  UNDEF:
           push_text("@", 1);
           break;
       case LIST:
	    push_text("[", 1);
	    for (i = 1; i <= d.u.a[0].u.i; i++) {
		evalDatum(d.u.a[i]);
		if (i < d.u.a[0].u.i)
		    push_text(",", 1);
	    }
	    push_text("]", 1);
	    break;

       default:
           error("EVAL error : unsuitable usage of eval syntax");
     }

}
