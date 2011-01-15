/*
 * $Id: refer.c,v 1.8 2001/07/27 17:52:39 cssbz Exp $
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

static char rcsid[] = "$Id: refer.c,v 1.8 2001/07/27 17:52:39 cssbz Exp $";

/***************************************
 *	REFERENCE LIST MAINTAINER      *
 ***************************************/

#include <stdlib.h>

#include "../../../config.h"
#include "eden.h"
#include "yacc.h"

#include "emalloc.h"
#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

symptr_QUEUE IDlist = EMPTYQUEUE(IDlist);
symptr_QUEUE *lastNullInIDlist = &IDlist;

Int        *autocalc;

#ifdef DEBUG
#define DEBUGPRINT(X,Y) if (Debug&1) { debugMessage(X, Y); }
#else
#define DEBUGPRINT(X,Y)
#endif

#if defined(DEBUG) && !defined(WEDEN_ENABLED)
void printlist(symptr_QUEUE * Q)
{				/* INTERNAL: PRINT A DEPENDENCY LIST */
    /* USED FOR DEBUGGING ONLY */
    register symptr_ATOM P;

    FOREACH(P, Q)
	fprintf(stderr, " %s", P->obj ? P->obj->name : "*");
    printf("\n");
}
#endif /* DEBUG */

void
addID(symptr id)
{				/* INTERNAL: ADD AN IDENTIFIER USED BY
				   PARSER */
  /* Original code here was:

       if (!id || !IN_symptr_Q(&IDlist, id))
         APPEND_symptr_Q(&IDlist, id);

     When all the macros are expanded, this looks like:
       if (!id || !SEARCH_symptr(&IDlist, id, 1)) {
         symptr_ATOM A;
         A = emalloc(sizeof(*A));
         A->obj = NEW_symptr(id);
         ((A)->prev=((A)->next=(&IDlist))->prev)->next=(A);
         (&IDlist)->prev=(A);
       }

     ... which implements the following: append the id to the IDlist,
     unless it is non-zero and we already have an occurrance of it in
     the list.  (Ie we can have multiple nulls within the list, but no
     multiple occurrances of actual ids).

     Notice the OR used in the if, not an AND:
       id==0, idinQ  -> T || F -> T (append)
       id==0, !idinQ -> T || T -> T (append)
       id!=0, idinQ  -> F || F -> F (don't append)
       id!=0, !idinQ -> F || T -> T (append)

     ('clever' but highly confusing code as it is searching for one
     significant case which might be better done using an AND).

     This 'clever' coding is unfortunately incorrect.  addID / IDlist
     is actually used by the parser to maintain lists of identifiers
     for three separate purposes: within 'is' formulae definitions,
     within proc/func/procmacro action definitions, and within ~>
     related_by definitions.  It appears (as the code documentation is
     lacking) that the removal of multiple occurrances of ids is
     intended to stop multiple triggering (eg proc p : a, a... or p is
     a+a leading to [a,a] triggers).  However, we are now discovering
     that these three purposes are not separate: when an 'is' formula
     redefinition is made within a triggered action, the removal of
     multiple occurrances of ids can cause a problem.

     What I believe this function should really implement is a queue
     of ids which are unique between nulls.  Eg b * a b * b is
     allowed, but b * a b b * b is not.

     [Ash] */

  if (id == (symptr)0) {
    /* A null (list terminator): add it */
    APPEND_symptr_Q(&IDlist, id);
    lastNullInIDlist = LAST(&IDlist);
    DEBUGPRINT("addID: id=* ", 0);

  } else {
    /* An id: add it only if it is unique since the last null */
    symptr_QUEUE *P;
    int n = 0;

    P = FRONT(lastNullInIDlist);
    while (P && ((P->obj) != (symptr)0) && (P != &IDlist)) {
      if (EQUAL_symptr(id, P->obj)) n++;
      P = NEXT(lastNullInIDlist, P);
    }

    if (n < 1) {
      /* This is an id which we haven't seen since the last null */
      APPEND_symptr_Q(&IDlist, id);
      DEBUGPRINT("addID: id=%s ", id->name);
#ifdef DEBUG
    } else {
      /* We've seen this id more than once */
      DEBUGPRINT("addID: not adding id=%s ", id->name);
#endif
    }

  }

#if defined(DEBUG) && !defined(WEDEN_ENABLED)
  if (Debug&1) {
    fprintf(stderr, "IDlist now");
    printlist(&IDlist);
  }
#endif
}

void
clear_IDlist(void)
{
    CLEAR_symptr_Q(&IDlist);
    lastNullInIDlist = &IDlist;
}

/* save_IDlist: this seems to work through the items added by addID to
   IDlist, saving them in newly malloc'd memory (well, the first item
   is newly malloc'd), re-linking them as we go, stopping when we find
   a queued item whose obj is null.  A pointer to the new queue is
   returned.  This is used by functions in code.c which code
   redefinitions (is formulae and proc/func/procmacro actions) and the
   related_by operator ~>.  [Ash] */
symptr_QUEUE *
save_IDlist(void)
{
    register symptr_ATOM A;
    register symptr_QUEUE *P, *Q;

    P = &IDlist;
    Q = (symptr_QUEUE *) emalloc(sizeof(symptr_QUEUE));
    CLEAN_Q(Q);
    while ((A = LAST(P))) {
	DELETE_ATOM(P, A);
	if (A->obj == (symptr) 0) {
	    DESTROY_symptr_ATOM(A);
	    break;
	}
	INSERT_Q(Q, A);
    }
    lastNullInIDlist = &IDlist;
    return Q;
}

/* refer_to(): pass this a symbol and its sources.  This seems to
   update information in sp and the Q with the information that sp has
   Q has its sources.  [Ash] */
void
refer_to(symptr sp, symptr_QUEUE * Q)
{				/* INTERNAL: APPEND SYMBOL TO SOURCES */
    register symptr_ATOM Symbol;
    register symptr_QUEUE *SourceTargets, *Sources;

    /* I don't understand this code at all well, so my explanatory
       comments may be totally wrong :( [Ash] */
#if defined(DEBUG) && !defined(WEDEN_ENABLED)
    if (Debug&1) {
      fprintf(stderr, "refer_to: sp=%s, Q=", sp->name);
      printlist(Q);
    }
#endif

    /* Find the symbol's existing list of sources */
    Sources = &sp->sources;

    /* Remove any existing target pointers to this symbol */
    FOREACH(Symbol, Sources) {
	SourceTargets = &Symbol->obj->targets;
	DELETE_FIRST_symptr(SourceTargets, sp);
    }

    /* Now remove any existing list of sources from this symbol */
    CLEAR_symptr_Q(Sources);

    /* Add the sources passed here (Q) as a parameter to this symbol (sp) */
    FOREACH(Symbol, Q) {
	APPEND_symptr_Q(Sources, Symbol->obj);
    }

    /* Make the target pointers of the sources passed here (Q) point to
       this symbol (sp) */
    FOREACH(Symbol, Sources) {
	SourceTargets = &Symbol->obj->targets;
	APPEND_symptr_Q(SourceTargets, sp);
    }
}

void
refer_by(symptr sp, symptr_QUEUE * Q)
{				/* INTERNAL: APPEND SYMBOL TO TARGETS */
    register symptr id;
    register symptr_ATOM Symbol;
    register symptr_QUEUE *Targets, *Sources;

    FOREACH(Symbol, Q) {
	id = Symbol->obj;
	Sources = &id->sources;
	DELETE_FIRST_symptr(Sources, sp);
	APPEND_symptr_Q(Sources, sp);
	Targets = &sp->targets;
	DELETE_FIRST_symptr(Targets, id);
	APPEND_symptr_Q(Targets, id);
    }
}

/* checkok1 is used by checkok below [Ash] */
int
checkok1(symptr sp, symptr_QUEUE * Q)
{				/* INTERNAL: IF SYMBOL IN THE LIST ? */
    /* TRUE  = 1 = NOT IN THE LIST */
    /* FALSE = 0 = ALREADY EXISTED */
    register symptr id;
    register symptr_ATOM P;

    FOREACH(P, Q) {
	id = P->obj;
	if (id->marked)
	    continue;
	id->marked = 1;
	if (id == sp)
	    return FALSE;
	if (!checkok1(sp, &id->sources))
	    return FALSE;
    }
    return TRUE;
}

/* checkok2 is used by checkok below [Ash] */
int
checkok2(symptr sp, symptr_QUEUE * Q)
{				/* INTERNAL: IF SYMBOL IN THE LIST ? */
    /* TRUE  = 1 = NOT IN THE LIST */
    /* FALSE = 0 = ALREADY EXISTED */
    register symptr id;
    register symptr_ATOM P;

    FOREACH(P, Q) {
	id = P->obj;
	if (!id->marked)
	    continue;
	id->marked = 0;
	if (id == sp)
	    return FALSE;
	if (!checkok2(sp, &id->sources))
	    return FALSE;
    }
    return TRUE;
}

#ifdef INSTRUMENT
#include <sys/time.h>
#endif

/* checkok() checks for an attempt to introduce a cyclic definition.  It
   checks that the sp (the symbol we are about to introduce) is not
   referenced by any items in Q (the sources of the symbol we are
   about to introduce), or any of Q's sources.  Looks like it uses a
   kind of mark/sweep algorithm: mark each object as we look at it.
   If we've seen it before, ignore it, else check it and all its
   sources for equality with sp.  When we've found a result one way or
   the other, do the search again, removing the marks we made along
   the way.  [Ash] */
int
checkok(symptr sp, symptr_QUEUE * Q)
{				/* INTERNAL: IF SYMBOL IN THE LIST ? */
    /* TRUE  = 1 = NOT IN THE LIST */
    /* FALSE = 0 = ALREADY EXISTED */
  int result;

#ifdef INSTRUMENT
  extern hrtime_t insCycle;
  hrtime_t start, end;

  start = gethrtime();
#endif

  result = checkok1(sp, Q);

  checkok2(sp, Q);		/* clear the markings by checkok1() */

#ifdef INSTRUMENT
  end = gethrtime();
  insCycle += (end - start);
#endif

  return result;
}

/* ready() is used during evaluation.  It checks the sources of a symbol
   to see if they are marked as having been changed (in which case the
   symbol cannot be evaluated yet).  [Ash] */
int
ready(symptr sp)
{				/* INTERNAL: CHECK CHILDREN OF SYMBOL */
    /* TRUE = ALL CHILDREN ARE UP-TO-DATE */
    register symptr_ATOM P;
    register symptr_QUEUE *Q;

    Q = &sp->sources;
    FOREACH(P, Q) {
	if (P->obj->changed)
	    return FALSE;
    }
    return TRUE;
}
