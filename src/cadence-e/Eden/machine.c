/*
 * $Id: machine.c,v 1.22 2002/07/10 19:24:49 cssbz Exp $
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

static char rcsid[] = "$Id: machine.c,v 1.22 2002/07/10 19:24:49 cssbz Exp $";

/****************************************************************
*								*
*	This files contains most of the implementation of	*
*	most of the pseduo-machine codes.			*
*								*
****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../config.h"
#include "eden.h"
#include "yacc.h"

#include "emalloc.h"
#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

extern int  topEntryStack(void);
extern char *topMasterStack(void);
extern Datum ctos(Datum);
extern int appAgentName;
extern char *typename();
#ifdef DISTRIB
extern int  handle_check(symptr);  /* for agency  --sun  */
extern void propagate_agency(symptr);
#endif /* DISTRIB */
void makearr(Int);

extern int basecontext; /* DOSTE context [Nick] */

#ifdef DEBUG
#define DEBUGPRINT(s,t)		{if(Debug&1)fprintf(stderr,s,t);}
#else
#define DEBUGPRINT(s,t)
#endif

#define dpush(x, y, z)		x.u.i=(Int)(z); x.type = y; push(x);
#define CheckRange(n,low,up)	if(n<low||n>up)out_of_range_error(n,low,up)
/* pop_address used to be (more confusingly) called get_address [Ash] */
#define pop_address(addr,where)	addr=pop();mustaddr(addr,where)
#define is_string(d)		(d.type == MYCHAR || d.type==STRING)
#define is_list(d)		(d.type == LIST)
#define sametype(d1, d2) ((d1.type == d2.type) || (isnum(d1) && isnum(d2)))
#define address(sp) { Datum d; d.type = SYMBOL | DPTR; \
        d.u.v.x = (Int) &(sp->d); d.u.v.y = (Int) sp; push(d); }

#ifdef DISTRIB
#define ChangeSym(addr) if (is_symbol(addr)) {\
        if (appAgentName > 0) propagate_agency(symbol_of(addr));\
	change(symbol_of(addr), FALSE);\
	symbol_of(addr)->entry = topEntryStack();\
	symbol_of(addr)->master = topMasterStack();\
}
#else
#define ChangeSym(addr) if (is_symbol(addr)) {\
	change(symbol_of(addr), FALSE);\
	symbol_of(addr)->entry = topEntryStack();\
	symbol_of(addr)->master = topMasterStack();\
}
#endif /* DISTRIB */

void freedatum(Datum d)
{				/* free the content of the datum, not the
				   datum itself */
    int         i;

    switch (d.type) {
    case STRING:
	if (d.u.s)
	    free(d.u.s);	/* free the string */
	break;

    case LIST:
	for (i = 1; i <= d.u.a->u.i; i++)
	    freedatum(d.u.a[i]);/* free each item */
	free(d.u.a);		/* free the list */
	break;
    }
}

Datum newdatum(Datum d)
{				/* use malloc to duplicate a datum */
    char       *s;
    Datum      *a;
    int         i;

    switch (d.type) {
    case STRING:		/* copy the content of a string */
	if (d.u.s) {		/* a precaution of NULL */
	  /* Allocate memory on the malloc heap for the string in d [Ash] */
	  s = (char *) emalloc(strlen(d.u.s) + 1);
	  /* Copy d.u.s into the newly malloc'd space s,
	     then set d.u.s to point to the new space [Ash] */
	  d.u.s = (char *) strcpy(s, d.u.s);
	}			/* an error ? */
	break;

    case LIST:			/* copy the content of a list */
	a = (Datum *) emalloc((d.u.a->u.i + 1) * sizeof(Datum));
	for (i = 0; i <= d.u.a->u.i; i++)
	    a[i] = newdatum(d.u.a[i]);
	d.u.a = a;
	break;
    }
    return d;
}

Datum newhdat(Datum d)
{				/* ("new *heap* datum" [Ash]) similiar
				   to newdatum, but use heap to allocate
				   space instead */
    char       *s;
    Datum      *a;
    int         i;

    switch (d.type) {
    case STRING:
	if (d.u.s) {
	    s = (char *) getheap(strlen(d.u.s) + 1);
	    d.u.s = (char *) strcpy(s, d.u.s);
	} else
	    s = d.u.s;		/* if null [Ash] */
	break;

    case LIST:
	a = (Datum *) getheap((d.u.a->u.i + 1) * sizeof(Datum));
	for (i = 0; i <= d.u.a->u.i; i++) {
	    a[i] = newhdat(d.u.a[i]);
	}
	d.u.a = a;
	break;
    }

    return d;
}

void out_of_range_error(int i, int low, int up)
{
    errorf("index out of range (%d is outside the range %d...%d)",
	   i, low, up);
}

void mustint(Datum d, char *where)
{
    if (d.type != INTEGER && d.type != MYCHAR)
      errorf("type clash: expecting %s (in %s, got %s)",
	     typename(INTEGER), where, typename(d.type));
}

void mustchar(Datum d, char *where)
{
    if (d.type != INTEGER && d.type != MYCHAR)
      errorf("type clash: expecting %s (in %s, got %s)",
	     typename(MYCHAR), where, typename(d.type));
}

void muststr(Datum d, char *where)
{
    if (d.type != STRING)
      errorf("type clash: expecting %s (in %s, got %s)",
	     typename(STRING), where, typename(d.type));
}

void mustlist(Datum d, char * where)
{
    if (d.type != LIST)
      errorf("type clash: expecting %s (in %s, got %s)",
	     typename(LIST), where, typename(d.type));
}

void mustaddr(Datum d, char *where)
{
    if (!is_address(d))
      errorf("type clash: expecting reference to variable (in %s, got %s)",
	     where, typename(d.type));
}

void address_error(char *where) {
  errorf("type clash: expecting reference to variable (in %s)",
	 where);
}

int get2num(Datum * dp1, Datum * dp2)
{				/* get 2 integer from stack */
    *dp2 = pop();
    *dp1 = pop();

    switch (dp1->type) {
    case MYCHAR:
	dp1->type = INTEGER;
    case INTEGER:
    case REAL:
	break;
    case UNDEF:
	return UNDEF;		/* undefined value */
    default:
        errorf("type clash: number type required (got %s)",
	       typename(dp1->type));
    }

    switch (dp2->type) {
    case MYCHAR:
	dp1->type = INTEGER;
    case INTEGER:
    case REAL:
	break;
    case UNDEF:
	return UNDEF;		/* undefined value */
    default:
        errorf("type clash: number type required (got %s)",
	       typename(dp2->type));
    }

    if (dp1->type != REAL && dp2->type == REAL) {
	/* convert d1 to real */
	dp1->type = REAL;
	dp1->u.r = dp1->u.i;
	return REAL;
    }
    if (dp1->type == REAL && dp2->type != REAL) {
	/* convert d2 to real */
	dp2->type = REAL;
	dp2->u.r = dp2->u.i;
	return REAL;
    }
    return dp1->type;
}

void add(void)
{				/* operator + */
    Datum       d1, d2;

    DEBUGPRINT("VMOPER|DATSTK add\n", 0);

    switch (get2num(&d1, &d2)) {
    case INTEGER:
	d1.u.i += d2.u.i;
	break;
    case REAL:
	d1.u.r += d2.u.r;
	break;
    default:
	pushUNDEF();
	return;
    }
    push(d1);
}

void
sub(void)
{				/* operator - */
    Datum       d1, d2;

    DEBUGPRINT("VMOPER|DATSTK sub\n", 0);

    switch (get2num(&d1, &d2)) {
    case INTEGER:
	d1.u.i -= d2.u.i;
	break;
    case REAL:
	d1.u.r -= d2.u.r;
	break;
    default:
	pushUNDEF();
	return;
    }
    push(d1);
}

void
mul(void)
{				/* operator * */
    Datum       d1, d2;

    DEBUGPRINT("VMOPER|DATSTK mul\n", 0);

    switch (get2num(&d1, &d2)) {
    case INTEGER:
	d1.u.i *= d2.u.i;
	break;
    case REAL:
	d1.u.r *= d2.u.r;
	break;
    default:
	pushUNDEF();
	return;
    }
    push(d1);
}

void divide(void)
{				/* operator / */
    Datum       d1, d2;

    DEBUGPRINT("VMOPER|DATSTK divide\n", 0);

    switch (get2num(&d1, &d2)) {
    case INTEGER:
	if (d2.u.i == 0)
	  errorf("division by zero (trying to calculate %d/0)",
		 d1.u.i);
	d1.u.i /= d2.u.i;
	break;
    case REAL:
	if (d2.u.r == 0.0)
	  errorf("division by zero (trying to calculate %f/0)",
		 d1.u.r);
	d1.u.r /= d2.u.r;
	break;
    default:
	pushUNDEF();
	return;
    }
    push(d1);
}

void mod(void)
{				/* operator % */
    Datum       d1, d2;
    double r;

    DEBUGPRINT("VMOPER|DATSTK mod\n", 0);

    switch (get2num(&d1, &d2)) {
    case INTEGER:
      if (d2.u.i == 0)
	errorf("division by zero (trying to calculate %d%%0)",
	       d1.u.i);
      d1.u.i %= d2.u.i;
      break;
    case REAL:
      d1.u.r = d1.u.r - ((int)(d1.u.r / d2.u.r) * d2.u.r);
      break;
    default:
      pushUNDEF();
      return;
    }
    push(d1);
}

void negate(void)
{				/* unary minus - */
    Datum       d;

    DEBUGPRINT("VMOPER|DATSTK negate\n", 0);

    d = pop();
    switch (d.type) {
    case MYCHAR:
	d.type = INTEGER;
    case INTEGER:
	d.u.i = -d.u.i;
	break;
    case REAL:
	d.u.r = -d.u.r;
	break;
    case UNDEF:
	pushUNDEF();
	return;
    default:
        errorf("type clash on negation: number type required (got %s)",
	       typename(d.type));
	return;
    }
    push(d);
}

void lazy_not(void)
{				/* logical lazy not */
    Datum       d;

    DEBUGPRINT("VMOPER|DATSTK lazy_not\n", 0);

    d = pop();
    if (!isint(d) && !isundef(d))
        errorf("type clash: lazy not: expecting %s (got %s)",
	       typename(INTEGER), typename(d.type));
    switch (d.type) {
    case MYCHAR:
	d.type = INTEGER;
    case INTEGER:
	d.u.i = !d.u.i;
	break;
    case REAL:
    default:
	d.type = INTEGER;
	d.u.i = !d.u.r;
	break;
    }
    push(d);
}

void not(void)
{				/* logical not */
    Datum       d;

    DEBUGPRINT("VMOPER|DATSTK not\n", 0);

    d = pop();
    if (!isint(d) && !isundef(d))
        errorf("type clash: logical not: expecting %s (got %s)",
	       typename(INTEGER), typename(d.type));
    switch (d.type) {
    case MYCHAR:
	d.type = INTEGER;
    case INTEGER:
	d.u.i = !d.u.i;
	break;
    case REAL:
	d.type = INTEGER;
	d.u.i = !d.u.r;
	break;
    default:
	pushUNDEF();
	return;
    }
    push(d);
}

void concat(void)
{				/* string/list concat opeator // */
    Datum       d1, d2;
    int         len;
    char       *s;
    int         i, size1, size2;
    /* extern   void  print(Datum); */

    DEBUGPRINT("VMOPER|DATSTK concat\n", 0);

    d2 = pop();
    d1 = pop();

    /* printf("d2 "); print(d2); printf("d1 "); print(d1); */

    if (isundef(d1) || isundef(d2))
	pushUNDEF();
    else if (is_string(d1) && is_string(d2)) {
	d1 = ctos(d1);
	d2 = ctos(d2);
	len = strlen(d1.u.s) + strlen(d2.u.s);
	s = (char *) getheap(len + 1);
	d1.u.s = (char *) strcat(strcpy(s, d1.u.s), d2.u.s);
	push(d1);
    } else if (is_list(d1) && is_list(d2)) {
	size1 = d1.u.a->u.i;
	size2 = d2.u.a->u.i;
	/* This can easily blow the stack if we are concatenating long
           lists [Ash] */
	for (i = 1; i <= size1; i++)
	    push(d1.u.a[i]);
	for (i = 1; i <= size2; i++)
	    push(d2.u.a[i]);
	makearr(size1 + size2);
    } else
      errorf("type clash on concatenation: expecting strings or lists (got %s and %s)",
	     typename(d1.type), typename(d2.type));
}

/* concat uses the stack to form the result of a list concatenation,
   which limits the size of the result.  This is a problem when people
   use the common form 'l = l // stuff' and long lists.  The opcode below
   is an optimised concatenation combined with assignment to work
   around this particular problem [Ash] */
void concatopt(void)
{
  Datum       toAppend, currValue, addr, d;
  Datum      *a, *dp;
  int         len;
  char       *s;
  int         i, nAppend;

  DEBUGPRINT("VMOPER|DATSTK concatopt\n", 0);

  /* Language:
     l = l // expr (or l might be a string: s = s // expr)
     
     VM code:
     addr l               | lvalue | asgn
     addr l     | primary | expr   |
     getvalue   | primary |        |
     expr stuff           |        |
     concatopt                     |
     
     Stack:
     expr result (to be appended)
     current value of l (mainly to be ignored except for type checking)
     address of l (where to append)
  */
  
  toAppend = pop();
  currValue = pop();
  addr = pop();
  
  mustaddr(addr, "concatopt");
  
  if (isundef(toAppend) || isundef(currValue))
    pushUNDEF();
  else if (is_string(toAppend) && is_string(currValue)) {
    /* form the concatenation of the two strings */

    /* !@!@ Need to do a DISTRIB handle_check here really [Ash] */

    /* if they are a char (in the Datum), convert them to a string (in
       the heap) */
    currValue = ctos(currValue);
    toAppend = ctos(toAppend);

    /*
    fprintf(stdout, "concatopt: strings: %s // %s\n",
	    currValue.u.s, toAppend.u.s);
    */

    len = strlen(currValue.u.s) + strlen(toAppend.u.s);

    /* Create a new Datum with malloc'd string value */
    d.type = STRING;
    d.u.s = (char *)emalloc(len + 1);
    d.u.s = strcat(strcpy(d.u.s, currValue.u.s), toAppend.u.s);

    /* This new Datum replaces the old one at addr */
    freedatum(*dptr(addr));
    *dptr(addr) = d;

    push(d);
    ChangeSym(addr); /* or symbol_of(currValue), but addr should be ==
                        to this */

  } else if (is_list(toAppend) && is_list(currValue)) {
    /*
    fprintf(stdout, "concatopt: lists\n");
    */

    mustlist(*(dp = dptr(addr)), "concatopt");

    /* !@!@ Need to do a DISTRIB handle_check here really [Ash] */

    nAppend = toAppend.u.a->u.i;

    /* Lists are an array of Datums starting at dp->u.a.  The first
       Datum is an INTEGER, giving the length of the list.  Therefore
       dp->u.a must always be malloc'd at least to the size of one
       Datum.  [Ash] */
    a = (Datum *) erealloc(dp->u.a,
			   (dp->u.a->u.i + nAppend + 1) * sizeof(Datum));

    for (i = 1; i <= nAppend; i++) {
      /* a->u.i is no of items in the list [Ash] */
      a[++a->u.i] = newdatum(toAppend.u.a[i]);
    }

    dp->u.a = a;
    ChangeSym(addr);

    /* Need to return the result of this operation.  The resultant
       list might be large in this case, so can't really push back the
       result as we might blow the stack.  address(symbol_of(addr));
       would push &l, but that isn't right either.  Follow what append
       does instead and give @ as a result.  [Ash] */
    pushUNDEF();

  } else
    errorf("type clash on concatenation: expecting strings or lists (got %s and %s)",
	   typename(currValue.type), typename(toAppend.type));
}

void jmp(void)
{				/* unconditional jump */
    Int         i = (Int) (*pc++);

    DEBUGPRINT("VMOPER|DATSTK jmp\n", 0);

    pc += i;
}

void jpt(void)
{				/* jump on true */
    Int         i = (Int) (*pc++);
    Datum       d;

    DEBUGPRINT("VMOPER|DATSTK jpt\n", 0);

    d = pop();
    if (isundef(d)) {
	return;
    } else {
	mustint(d, "jpt (loop construct)");
	if (d.u.i)
	    pc += i;
    }
}

void jpf(void)
{				/* jump on false */
    Int         i = (Int) (*pc++);
    Datum       d;

    DEBUGPRINT("VMOPER|DATSTK jpf\n", 0);

    d = pop();
    if (isundef(d)) {
	return;
    } else {
	mustint(d, "jpf (loop construct)");
	if (!d.u.i)
	    pc += i;
    }
}

void jpnt(void)
{				/* jump on not true, i.e. on false or @  */
    Int         i = (Int) (*pc++);
    Datum       d;

    DEBUGPRINT("VMOPER|DATSTK jpnt\n", 0);

    d = pop();
    if (isundef(d)) {
	pc += i;
    } else {
	mustint(d, "jpnt (loop construct)");
	if (!d.u.i)
	    pc += i;
    }
}

void jpnf(void)
{				/* jump on not false, i.e. on true or @ */
    Int         i = (Int) (*pc++);
    Datum       d;

    DEBUGPRINT("VMOPER|DATSTK jpnf\n", 0);

    d = pop();
    if (isundef(d)) {
	pc += i;
    } else {
	mustint(d, "jpnf (loop construct)");
	if (d.u.i)
	    pc += i;
    }
}

void and(void)
{				/* eager, logical and */
    Datum       d1, d2;

    DEBUGPRINT("VMOPER|DATSTK and\n", 0);

    switch (get2num(&d1, &d2)) {
    case INTEGER:
	d1.u.i = d1.u.i && d2.u.i;
	break;
    case REAL:
        errorf("type clash: eager logical and: expecting %s (got %s and %s)",
	       typename(INTEGER), typename(d1.type), typename(d2.type));
    default:
	pushUNDEF();
	return;
    }
    push(d1);
}

void or(void)
{				/* eager, logical or */
    Datum       d1, d2;

    DEBUGPRINT("VMOPER|DATSTK or\n", 0);

    switch (get2num(&d1, &d2)) {
    case INTEGER:
	d1.u.i = d1.u.i || d2.u.i;
	break;
    case REAL:
        errorf("type clash: eager logical or: expecting %s (got %s and %s)",
	       typename(INTEGER), typename(d1.type), typename(d2.type));
    default:
	pushUNDEF();
	return;
    }
    push(d1);
}

/* bitwise and [Ash] */
void bitand(void) {
  Datum d1, d2;

  DEBUGPRINT("VMOPER|DATSTK bitand\n", 0);

  switch (get2num(&d1, &d2)) {
  case INTEGER:
    d1.u.i = d1.u.i & d2.u.i;
    break;
  case REAL:
    errorf("type clash: bitwise and: expecting %s (got %s and %s)",
	   typename(INTEGER), typename(d1.type), typename(d2.type));
  default:
    pushUNDEF();
    return;
  }
  push(d1);
}

/* bitwise or [Ash] */
void bitor(void) {
  Datum d1, d2;

  DEBUGPRINT("VMOPER|DATSTK bitor\n", 0);

  switch (get2num(&d1, &d2)) {
  case INTEGER:
    d1.u.i = d1.u.i | d2.u.i;
    break;
  case REAL:
    errorf("type clash: bitwise or: expecting %s (got %s and %s)",
	   typename(INTEGER), typename(d1.type), typename(d2.type));
  default:
    pushUNDEF();
    return;
  }
  push(d1);
}

void ddup(void)
{				/* duplicate datum */
    Datum       d;

    DEBUGPRINT("VMOPER|DATSTK ddup\n", 0);

    d = top_of_stack;
    push(d);			/* since "push: is a macro, it is saver to
				   copy top-of-stack in variables first. */
}

void popd(void)
{				/* pop a datum off stack */
    Datum       dummy;

    DEBUGPRINT("VMOPER|DATSTK popd\n", 0);

    dummy = pop();		/* the compiler don't like we just do the
				   macro pop(), so we assign it to a dummy
				   variable. */
}

void pushUNDEF(void)
{				/* push @ on stack */
    DEBUGPRINT("VMOPER|DATSTK pushUNDEF\n", 0);

    push(UndefDatum);
}

void pushint(void)
{				/* interpret next instruction as an
				   integer and push it onto stack */
    Datum       d;

    DEBUGPRINT("VMOPER|DATSTK pushint\n", 0);

    d.type = INTEGER;
    d.u.i = (Int) * pc++;
    push(d);
}

void constpush(void)
{				/* push a constant */
    DEBUGPRINT("VMOPER|DATSTK constpush\n", 0);

    /* the constant is pointed to by the rest of the code*/
    push(*((Datum *) (*pc)));
    /* Note that using push with an expression with side-effect is
       dangerous if DEBUG [Ash] */
    pc++;
}

/***** datacmp() is called by all comparison operators *****/
int datacmp(d1, d2)			/* internal function */
    Datum       d1, d2;		/* compare two data */
{				/* return 0 = equal, +ve = greater than */
    int         i;		/* -ve = less than */
    double      r;

    if (sametype(d1, d2)) {
	if (d1.type != REAL && d2.type == REAL) {
	    /* convert d1 to real */
	    d1.type = REAL;
	    d1.u.r = d1.u.i;
	} else if (d1.type == REAL && d2.type != REAL) {
	    /* convert d2 to real */
	    d2.type = REAL;
	    d2.u.r = d2.u.i;
	}
	switch (d1.type) {
	case UNDEF:
	    return 0;
	case REAL:
	    return (r = d1.u.r - d2.u.r) ? (r > 0.0 ? 1 : -1) : 0;
	case STRING:
	    return strcmp(d1.u.s, d2.u.s);
	case LIST:
	    if (d1.u.a->u.i != d2.u.a->u.i)
		return 1;	/* not equal */
	    for (i = 1; i <= d1.u.a->u.i; i++)
		if (datacmp(d1.u.a[i], d2.u.a[i]))
		    return 1;	/* not equal */
	    return 0;		/* equal */
	default:
	    return (d1.u.i - d2.u.i);
	}
    } else {
	return 1;		/* not equal */
    }
}

void
cnv_2_bool(void)
{				/* convert an integer to boolean value */
    DEBUGPRINT("VMOPER|DATSTK cnv_2_bool\n", 0);

    switch (top_of_stack.type) {
	case INTEGER:
	case MYCHAR:
	top_of_stack.type = INTEGER;
	top_of_stack.u.i = top_of_stack.u.i != 0;
	break;
    case UNDEF:
	break;
    default:
        errorf("type clash: convert integer to bool: expecting %s (got %s)",
	       typename(INTEGER), typename(top_of_stack.type));
	break;
    }
}

void gt(void)
{				/* > */
    Datum       d1, d2, d;

    DEBUGPRINT("VMOPER|DATSTK gt\n", 0);

    d2 = pop();
    d1 = pop();
    if (sametype(d1, d2)) {
	if (d1.type != UNDEF && d1.type != LIST) {
	    dpush(d, INTEGER, datacmp(d1, d2) > 0);
	    return;
	}
    }
    pushUNDEF();
}

void lt(void)
{				/* < */
    Datum       d1, d2, d;

    DEBUGPRINT("VMOPER|DATSTK lt\n", 0);

    d2 = pop();
    d1 = pop();
    if (sametype(d1, d2)) {
	if (d1.type != UNDEF && d1.type != LIST) {
	    dpush(d, INTEGER, datacmp(d1, d2) < 0);
	    return;
	}
    }
    pushUNDEF();
}

void ge(void)
{				/* >= */
    Datum       d1, d2, d;

    DEBUGPRINT("VMOPER|DATSTK ge\n", 0);

    d2 = pop();
    d1 = pop();
    if (sametype(d1, d2)) {
	if (d1.type != UNDEF && d1.type != LIST) {
	    dpush(d, INTEGER, datacmp(d1, d2) >= 0);
	    return;
	}
    }
    pushUNDEF();
}

void le(void)
{				/* <= */
    Datum       d1, d2, d;

    DEBUGPRINT("VMOPER|DATSTK le\n", 0);

    d2 = pop();
    d1 = pop();
    if (sametype(d1, d2)) {
	if (d1.type != UNDEF && d1.type != LIST) {
	    dpush(d, INTEGER, datacmp(d1, d2) <= 0);
	    return;
	}
    }
    pushUNDEF();
}

void eq(void)
{				/* == */
    Datum       d1, d2;

    DEBUGPRINT("VMOPER|DATSTK eq\n", 0);

    d2 = pop();
    d1 = pop();
    d1.u.i = datacmp(d1, d2) == 0;
    d1.type = INTEGER;
    push(d1);
}

void ne(void)
{				/* != */
    Datum       d1, d2;

    DEBUGPRINT("VMOPER|DATSTK ne\n", 0);

    d2 = pop();
    d1 = pop();
    d1.u.i = datacmp(d1, d2) != 0;
    d1.type = INTEGER;
    push(d1);
}

/*-
*   Switch is implement by a jump table. Table entries are of the form:
*		<const-ptr> <rel-addr>
*   where
*	<const-ptr> is a pointer pointing to the actual constant datum, and
*	<rel-addr> is the relative offset to code entry.
*   If <const-ptr> is a null ptr, then it is the default case.
*   If the switch statement has no default case, the <rel-addr> must
*   point to the next statement (i.e. <rel-addr> = 2).
*   See codeswitch() in file code.c for encoding the switch statement.
-*/

void switchcode(void)
{				/* switch statement code */
    Datum       d;

    DEBUGPRINT("VMOPER|DATSTK|VMREAD|VMEXEC switchcode\n", 0);

    d = pop();			/* value */
    for (; *pc && datacmp(d, *(Datum *) (*pc)); pc += 2);
    pc += (Int) pc[1];		/* do a relative jump */
}

void definition(void)
{				/* put defn/func/action in symbol table */
    symptr      sp;
    symptr_QUEUE *splist;
    Inst       *prog_begin, *prog_end;
    Int         type;
    Int         nauto;
    char       *text;
    int         checkok(symptr, symptr_QUEUE *);
    void        change_sources(symptr, symptr_QUEUE *);

#ifdef NO_CHECK_CIRCULAR
    extern int  NCC;
#endif				/* NO_CHECK_CIRCULAR */

    type = (Int) (*pc++);
    sp = (symptr) (*pc++);
    splist = (symptr_QUEUE *) (*pc++);
    prog_begin = (Inst *) (*pc++);
    prog_end = (Inst *) (*pc++);
    nauto = (Int) (*pc++);
    text = (char *) (*pc++);

#ifdef NO_CHECK_CIRCULAR
    if (!NCC)
#endif				/* NO_CHECK_CIRCULAR */
	if (!checkok(sp, splist)) {
	    error2(sp->name, ": CYCLIC DEF : ABORTED");
	}

    /* If the symbol is already installed and has not changed, don't
       install it again (I think) [Ash] */
    if (prog_begin != sp->inst) {
	/*-
	if (sp->inst && !indef) {
	    free(sp->inst);
	    DEBUGPRINT("** definition: free %s %d\n", sp->name);
	}
	-*/
	if (type > 265)	/* [Nick] Fix for losing doste dependency */
		sp->nauto = nauto;
	sp->inst = prog_begin;
	sp->stype = type;
	sp->d.type = type;
	sp->d.u.sym = sp;
	sp->text = text;
	sp->entry = topEntryStack();
	sp->master = topMasterStack();
	change_sources(sp, splist);
    }
    DEBUGPRINT("VMOPER|VMREAD|DEFNET definition %s\n", sp->name);
}

/* definition_runtimelhs appears to have been added by Patrick to be
   used in the "`expr` is expr;" case.  It used to (less meaningfully)
   be called definition1.  [Ash] */
void definition_runtimelhs(void)
{				/* put defn/func/action in symbol table */
    symptr      sp;
    symptr_QUEUE *splist;
    Inst       *prog_begin, *prog_end;
    Int         type;
    Int         nauto;
    char       *text;
    int         checkok(symptr, symptr_QUEUE *);
    void        change_sources(symptr, symptr_QUEUE *);
    Datum       addr;

#ifdef NO_CHECK_CIRCULAR
    extern int  NCC;

#endif				/* NO_CHECK_CIRCULAR */

    /* For "`expr1` is expr2;", the parser codes opcodes which create
       the result of expr1 on the stack, followed by lookup_address.
       This pops the string off the stack, looks up that name as a
       reference into the symbol table, then pops back the address of
       that variable (creating it if it doesn't exist).  Now the
       following finds the symbol to be redefined.  [Ash] */
    pop_address(addr, "definition_runtimelhs (adding definition)");
    sp = symbol_of(addr);

    type = (Int) (*pc++);
    splist = (symptr_QUEUE *) (*pc++);
    prog_begin = (Inst *) (*pc++);
    prog_end = (Inst *) (*pc++);
    nauto = (Int) (*pc++);
    text = (char *) (*pc++);

#ifdef NO_CHECK_CIRCULAR
    if (!NCC)
#endif				/* NO_CHECK_CIRCULAR */
	if (!checkok(sp, splist)) {
	    error2(sp->name, ": CYCLIC DEF : ABORTED");
	}

    /* If the symbol is already installed and has not changed, don't
       install it again (I think) [Ash] */
    if (prog_begin != sp->inst) {
	/*-
	if (sp->inst && !indef) {
	    free(sp->inst);
	    DEBUGPRINT("** free %s %d\n", sp->name);
	}
	-*/

	if (type > 265)	/* [Nick] Fix for losing doste dependency */
		sp->nauto = nauto;
	sp->inst = prog_begin;
	sp->stype = type;
	sp->d.type = type;
	sp->d.u.sym = sp;
	sp->text = text;
	sp->entry = topEntryStack();
	sp->master = topMasterStack();
	change_sources(sp, splist);
    }
    DEBUGPRINT("VMOPER|VMREAD|DEFNET definition_runtimelhs %s\n", sp->name);
}


/*******************************************************************
 *	RUN-TIME CODES OF ASSIGNMENT AND INCREMENT OPERATORS       *
 *******************************************************************/

static void need_rwv(Datum addr, char *from)
{				/* check if lvalue a RWV ? */
  if (is_local(addr)) return;

  if (is_symbol(addr)) {
    if (symbol_of(addr)->stype != VAR) {
      errorf("%s %s is %s, not a read/write variable",
	     from, symbol_of(addr)->name,
	     typename(symbol_of(addr)->stype));
    }
  }
}

/* This only seems to be called by assign() [Ash] */
static
void cnv_formula_to_rwv(Datum addr)
{				/* if sym is a formula, convert to rwv */
    extern void change_sources(symptr, symptr_QUEUE *);
    symptr      sp;
    static symptr_QUEUE NullList = EMPTYQUEUE(NullList);

    if (is_symbol(addr) && (sp = symbol_of(addr))->stype == FORMULA) {
	/*-
	  * free(sp->inst);
	  -*//* remove the code */

	sp->inst = (Inst *) 0;
	sp->nauto = 0;
	sp->stype = VAR;

	/* free(sp->text); */

	sp->text = (char *) 0;
	change_sources(sp, &NullList);
    }
}

void assign(void)
{				/* lvalue = expression */
    Datum       d, addr, tmp;
#ifdef DISTRIB
    symptr  sp;
#endif /* DISTRIB */

    DEBUGPRINT("VMOPER|DATSTK|SYMTBL assign\n", 0);

    d = pop();			/* value */
    addr = pop();		/* address */

    mustaddr(addr, "=");

#ifdef DISTRIB
    if (is_symbol(addr)) sp = symbol_of(addr);      /* for agency --sun */
    else sp=0;

    if (handle_check(sp)) {
#endif
      /* !@!@ perhaps in the future here, if addr is a symbol which is
         currently a proc, func or procmacro, 'forget' it first so
         that funcs etc can be overwritten by assignment.  Give a
         warning also. [Ash, October 2002] */
      cnv_formula_to_rwv(addr);
      need_rwv(addr, "'=':");
      switch (address_type(addr)) {
      case DPTR:
	tmp = newdatum(d);
	freedatum(*dptr(addr));
	*dptr(addr) = tmp;
	break;
      case CPTR:
	mustchar(d, "=");
	*cptr(addr) = d.u.i;
	break;
      default:
	address_error("=");
	break;
      }
      push(d);
      ChangeSym(addr);
#ifdef DISTRIB
    }
    else {
      pushUNDEF();
    }
#endif
}

void inc_asgn(void)
{				/* lvalue += expression */
    Datum       d, addr;
#ifdef DISTRIB
    symptr      sp;
#endif /* DISTRIB */

    DEBUGPRINT("VMOPER|DATSTK|SYMTBL inc_asgn\n", 0);

    mustint(d = pop(), "+=");		/* value */
    addr = pop();		/* address */

    mustaddr(addr, "+=");
#ifdef DISTRIB
    if (is_symbol(addr)) sp = symbol_of(addr);      /* for agency --sun */
    else sp=0;
#endif /* DISTRIB */

#ifdef DISTRIB
    if (handle_check(sp)) {
#endif
      need_rwv(addr, "'+=':");   /* original code  --sun */

      switch (address_type(addr)) {
      case DPTR:
	mustint(*dptr(addr), "+=");	/* += only works on integers - could
					   be fixed [Ash] */
	dptr(addr)->u.i += d.u.i;
	break;

      case CPTR:
	mustchar(d, "+=");
	*cptr(addr) += d.u.i;
	break;

      default:
	address_error("+=");
	break;
      }
      push(d);
      ChangeSym(addr);
#ifdef DISTRIB
    }
    else {
      pushUNDEF();
    }
#endif
}

void dec_asgn(void)
{				/* lvalue -= expression */
    Datum       d, addr;
#ifdef DISTRIB
    symptr      sp;
#endif /* DISTRIB */

    DEBUGPRINT("VMOPER|DATSTK|SYMTBL dec_asgn\n", 0);

    mustint(d = pop(), "-=");		/* value */
    addr = pop();		/* address */

    mustaddr(addr, "-=");
#ifdef DISTRIB
    if (is_symbol(addr)) sp = symbol_of(addr);      /* for agency --sun */
    else sp=0;
#endif /* DISTRIB */

#ifdef DISTRIB
    if (handle_check(sp)) {
#endif
      need_rwv(addr, "'-=':");   /* original code  --sun */

      switch (address_type(addr)) {
      case DPTR:
	mustint(*dptr(addr), "-=");	/* -= only works on integers - could
					   be fixed [Ash] */
	dptr(addr)->u.i -= d.u.i;
	break;

      case CPTR:
	mustchar(d, "-=");
	*cptr(addr) -= d.u.i;
	break;

      default:
	address_error("-=");
	break;
      }
      push(d);
      ChangeSym(addr);
#ifdef DISTRIB
    }
    else {
      pushUNDEF();
    }
#endif
}

void pre_inc(void)
{				/* ++lvalue */
    Datum       d, addr;
#ifdef DISTRIB
    symptr      sp;
#endif /* DISTRIB */

    DEBUGPRINT("VMOPER|DATSTK|SYMTBL pre_inc\n", 0);

    addr = pop();		/* address */

    mustaddr(addr, "++ (pre-inc)");
#ifdef DISTRIB
    if (is_symbol(addr)) sp = symbol_of(addr);      /* for agency --sun */
    else sp=0;
#endif /* DISTRIB */

#ifdef DISTRIB
    if (handle_check(sp)) {
#endif
      need_rwv(addr, "'++':");   /* original code  --sun */

      switch (address_type(addr)) {
      case DPTR:
	mustint(*dptr(addr), "++ (pre-inc)");
	(dptr(addr)->u.i)++;
	push(*dptr(addr));
	break;

      case CPTR:
	(*cptr(addr))++;
	dpush(d, MYCHAR, *cptr(addr));
	break;

      default:
	address_error("++ (pre-inc)");
	break;
      }
      ChangeSym(addr);
#ifdef DISTRIB
    }
    else {
      pushUNDEF();
    }
#endif
}

void post_inc(void)
{				/* lvalue++ */
    Datum       d, addr;
#ifdef DISTRIB
    symptr      sp;
#endif /* DISTRIB */

    DEBUGPRINT("VMOPER|DATSTK|SYMTBL post_inc\n", 0);

    addr = pop();		/* address */

    mustaddr(addr, "++ (post-inc)");
#ifdef DISTRIB
    if (is_symbol(addr)) sp = symbol_of(addr);      /* for agency --sun */
    else sp=0;
#endif /* DISTRIB */

#ifdef DISTRIB
    if (handle_check(sp)) {
#endif
      need_rwv(addr, "'++':");   /* original code  --sun */

      switch (address_type(addr)) {
      case DPTR:
	mustint(*dptr(addr), "++ (post-inc)");
	push(*dptr(addr));
	(dptr(addr)->u.i)++;
	break;

      case CPTR:
	dpush(d, MYCHAR, *cptr(addr));
	(*cptr(addr))++;
	break;

    default:
	address_error("++ (post-inc)");
	break;
      }
      ChangeSym(addr);
#ifdef DISTRIB
    }
    else {
      pushUNDEF();
    }
#endif
}

void pre_dec(void)
{				/* --lvalue */
    Datum       d, addr;
#ifdef DISTRIB
    symptr      sp;
#endif /* DISTRIB */

    DEBUGPRINT("VMOPER|DATSTK|SYMTBL pre_dec\n", 0);

    addr = pop();		/* address */

    mustaddr(addr, "-- (pre-dec)");
#ifdef DISTRIB
    if (is_symbol(addr)) sp = symbol_of(addr);      /* for agency --sun */
    else sp=0;
#endif /* DISTRIB */

#ifdef DISTRIB
    if (handle_check(sp)) {
#endif
      need_rwv(addr, "'--':");   /* original code  --sun */

      switch (address_type(addr)) {
      case DPTR:
	mustint(*dptr(addr), "-- (pre-dec)");
	--(dptr(addr)->u.i);
	push(*dptr(addr));
	break;

      case CPTR:
	--(*cptr(addr));
	dpush(d, MYCHAR, *cptr(addr));
	break;

      default:
	address_error("-- (pre-dec)");
	break;
      }
      ChangeSym(addr);
#ifdef DISTRIB
    }
    else {
      pushUNDEF();
    }
#endif
}


void post_dec(void)
{				/* lvalue-- */
    Datum       d, addr;
#ifdef DISTRIB
    symptr      sp;
#endif /* DISTRIB */

    DEBUGPRINT("VMOPER|DATSTK|SYMTBL post_dec\n", 0);

    addr = pop();		/* address */

    mustaddr(addr, "-- (post-dec)");
#ifdef DISTRIB
    if (is_symbol(addr)) sp = symbol_of(addr);      /* for agency --sun */
    else sp=0;
#endif /* DISTRIB */

#ifdef DISTRIB
    if (handle_check(sp)) {
#endif
      need_rwv(addr, "'--':");   /* original code  --sun */

      switch (address_type(addr)) {
      case DPTR:
	mustint(*dptr(addr), "-- (post-dec)");
	push(*dptr(addr));
	--(dptr(addr)->u.i);
	break;

      case CPTR:
	dpush(d, MYCHAR, *cptr(addr));
	--(*cptr(addr));
	break;

      default:
	address_error("-- (post-dec)");
	break;
      }
      ChangeSym(addr);
#ifdef DISTRIB
    }
    else {
      pushUNDEF();
    }
#endif
}

/* UPDATE seems used by the parser to implement the ? query operator,
   presumably to stop this operator causing evaluation as its argument
   is parsed.  [Ash] */
static int  UPDATE = 1;

void noupdate(void)
{
    DEBUGPRINT("VMOPER|MCSTAT noupdate\n", 0);

    UPDATE = 0;
}

void resetupdate(void)
{
    DEBUGPRINT("VMOPER|MCSTAT resetupdate\n", 0);

    UPDATE = 1;
}

void update(symptr sp)
{				/* force the evaluation/execution of
				   defn/action */
    extern void execute(Inst *);

    DEBUGPRINT("MCSTAT|VMEXEC|SYMTBL update\n", 0);

    if (UPDATE) {
	execute(sp->inst);	/* eval/exec defn/action */
	freedatum(sp->d);	/* free the old value */
	sp->d = newdatum(pop());/* store the new value in data register */
	change(sp, FALSE);	/* update the flag */
    }
}

void addr(void)
{				/* push addr of data */
    symptr      sp;

    sp = (symptr) (*pc++);
    DEBUGPRINT("VMOPER|VMREAD|DATSTK addr: %s\n", sp->name);
    if (sp->stype == FORMULA && sp->changed) {
	update(sp);
    }
    address(sp);		/* address() will do the push */
}

/*-
   Take a variable name from stack, and look up from symbol table.
   Put the address onto stack.
   If symbol not found, create it.
-*/
void
lookup_address(void)
{
    Datum       d;
    symptr      sp;
    extern  char agentName[128];  /* for distributed Tkeden --sun */
    extern  int  appAgentName, inEVAL, inPrefix;
    char   newStr[128], *tempStr;
    extern  void  addID(symptr);

    d = pop();
    /* Cope with `@`, for example [Ash] */
    if (d.type == UNDEF) {
      pushUNDEF();
      return;
    }

    muststr(d, "lookup_address"); 		/* variable id name */
    /* printf("oldString = %s %s %i\n", d.u.s, agentName, appAgentName); */
    newStr[0] = '\0';
    if (*agentName !=0 && appAgentName >0 && strncmp(d.u.s, "~", 1)
	&& !inEVAL) {  /* for distributed --sun  */
       tempStr = d.u.s;
       if (!strncmp(d.u.s, "_", 1)) {
           strcat(newStr, "_");
           tempStr++;
       }
       if (inPrefix) strcpy(newStr, agentName);
          else strcpy(newStr, tempStr);
       strcat(newStr, "_");
       if (inPrefix) strcat(newStr, tempStr);
          else strcat(newStr, agentName);
    } else {
      if (!strncmp(d.u.s, "~", 1)) d.u.s++;
         strcpy(newStr, d.u.s);
    }                   /* original sy code --use d.u.s but not newStr  */
    /* printf("newString = %s", newStr); */
    sp = lookup(newStr, basecontext);		/* look up for name */

    /* install it in symbol table if not there */
    if (sp == NULL) {
      sp = install(newStr, basecontext, VAR, UNDEF, 0);
    }

    DEBUGPRINT("VMOPER|DATSTK|SYMTBL lookup_address: %s\n", sp->name);

    if (sp->stype == FORMULA && sp->changed) {
	update(sp);
    }
    address(sp);		/* put the address of sp onto stack */

    if (informula && !inEVAL) addID(sp);   /* put sp into IDlist for
                                              dependency: bugfix! --sun */

}

void localaddr(void)
{				/* push the address of a local variable */
    Datum       d;
    Int         n;

    n = (Int) * pc++;
    DEBUGPRINT("VMOPER|VMREAD|DATSTK localaddr: %d\n", n);
    d.type = LOCALV | DPTR;
    d.u.v.x = (Int) & fp->stackp[n];
    d.u.v.y = n;
    push(d);			/* push the address on stack */
}

void indexcalc(void)
{				/* compute the address of the i-th
				   item/char of a list/string */
    Datum       addr, *dp, index;
    Int         i;
    extern Int *eden_error_index_range;

    DEBUGPRINT("VMOPER|DATSTK indexcalc\n", 0);
    mustint(index = pop(), "indexing into list/string");	/* index */
    i = index.u.i;
    pop_address(addr, "indexing into list/string");

    if (address_type(addr) == DPTR) {
	dp = dptr(addr);

	switch (dp->type) {
	case LIST:
	    /*CheckRange(i, 1, dp->u.a->u.i);*/
	    if (i < 1 || i > dp->u.a->u.i) {
	      if (*eden_error_index_range)
		out_of_range_error(i, 1, dp->u.a->u.i);
	      else {
		pushUNDEF();
		return;
	      }
	    }
	    addr.type = (addr.type & ADDRESSMASK) | DPTR;
	    addr.u.v.x = (Int) & (dp->u.a[i]);
	    break;
	case STRING:
	    /*CheckRange(i, 1, (int) strlen(dp->u.s));*/
	    if (i < 1 || i > strlen(dp->u.s)) {
	      if (*eden_error_index_range)
		out_of_range_error(i, 1, strlen(dp->u.s));
	      else {
		pushUNDEF();
		return;
	      }
	    }
	    --i;
	    addr.type = (addr.type & ADDRESSMASK) | CPTR;
	    addr.u.v.x = (Int) & (dp->u.s[i]);
	    break;

	    /* This doesn't work as we can't generate an address of a
               char, unless we malloc'd some space here with the char
               followed by \000... [Ash] */
	    /*
	case MYCHAR:
	  if (i != 1) {
	    errorf("index error: trying to find the %dth item of a %s",
		   i, typename(MYCHAR));
	  }
	  addr.type = (addr.type & ADDRESSMASK) | CPTR;
	  addr.u.v.x = (Int) & (dp->u.s[0]);
	  break;
	    */

	default:
	    errorf("index error: list or string required (got %s, when trying to find item with index %d)",
		   typename(dp->type), i); /* it isn't a list/string */
	    break;
	}
    } else {
      errorf("index error: data isn't a list or string (when trying to find item with index %d)", i);
    }
    push(addr);
}

void makelist(void)
{				/* create a list */
    DEBUGPRINT("VMOPER|VMREAD makelist\n", 0);

    makearr((Int) * pc++);	/* the rest of instruction indicates the
				   no. of items (on the stack) */
}

/* makearr moves n items off the stack. It copies them to the heap,
 * and links them together to form a list. Item number 0 in the list
 * is an INTEGER, containing the number of items in the list. [Ash] */
void
makearr(Int n)
{				/* create a list */
    /* n - no. of items (on the stack) */
    Datum       arr, d;

    DEBUGPRINT("HEAPAL|DATSTK makearr(%d)\n", n);

    arr.type = LIST;
    /* allocate memory on the heap */
    arr.u.a = (Datum *) getheap((n + 1) * sizeof(Datum));
    arr.u.a->type = INTEGER;	/* the 0-th item stores the no. of items */
    arr.u.a->u.i = n;
    while (n > 0) {
      /* printf("ASH: makearr: n=%d\n", n); */
      d = pop();
      /* printf("ASH: makearr: done pop, &arr.u.a[n]=0x%x &d=0x%x\n",
	 &arr.u.a[n], &d); */
      /* I've replaced this code: arr.u.a[n--] = d; with the memcpy
         version below as I've had problems with it and "Advanced C
         programming by example" (John Perry) suggests this on page 25
         [Ash] */
      memcpy(&arr.u.a[n], &d, sizeof(d));
      n--;
    }
    push(arr);			/* return the list */
}

void getvalue(void)
{
    Datum       d, addr;
    symptr      sp;
    extern Int *autocalc;
    extern int  ready();
    extern Int *eden_notice_undef_reference;
#ifdef DISTRIB
    extern int  oracle_check(symptr);
#endif /* DISTRIB */

    DEBUGPRINT("VMOPER|DATSTK getvalue\n", 0);

    /* Originally pop_address(addr);... I'm attempting to make `@`
       work...  [Ash] */
    addr=pop();
    if (addr.type == UNDEF) {
      pushUNDEF();
      return;
    }

    mustaddr(addr, "getvalue");

    sp = symbol_of(addr);
#ifdef DISTRIB
    if (is_symbol(addr)) {   /* for agency   --sun */
       if (!oracle_check(sp)) { pushUNDEF(); return; }
    }
#endif /* DISTRIB */

    if (*autocalc && is_symbol(addr)
	&& sp->stype == FORMULA && sp->changed && ready(sp)) {
	update(sp);
    }

    if (is_symbol(addr) && sp->d.type == UNDEF &&
	*eden_notice_undef_reference) {
      noticef("reference to undefined variable %s (turn these notices off with eden_notice_undef_reference=0;)", sp->name);
    }

    switch (address_type(addr)) {
    case DPTR:
      push(*dptr(addr));
      break;
      
    case CPTR:
      dpush(d, MYCHAR, *cptr(addr) & 0xFF);
      break;
      
    default:
      address_error("getvalue");
      break;
    }
}

void sel(void)
{				/* return an item of a list or string */
    Datum       index, dat;
    Int         i;

    DEBUGPRINT("VMOPER|DATSTK sel\n", 0);
    mustint(index = pop(), "indexing into list/string");
    i = index.u.i;

    dat = pop();
    switch (dat.type) {
    case LIST:
	CheckRange(i, 1, dat.u.a->u.i);
	push(dat.u.a[i]);
	break;

    case STRING:
	--i;
	CheckRange(i, 0, (int) strlen(dat.u.s));
	dpush(dat, MYCHAR, dat.u.s[i] & 0xFF);
	break;

    default:
      errorf("index error: list or string required (got %s, when trying to find item with index %d)",
	     typename(dat.type), i);
    }
}

void listsize(void)
{				/* return the current size of list */
    Datum       d;

    DEBUGPRINT("VMOPER|DATSTK listsize\n", 0);
    d = pop();
    switch (d.type) {
    case MYCHAR:
	dpush(d, INTEGER, 1);
	break;

    case STRING:
	dpush(d, INTEGER, strlen(d.u.s));
	break;

    case LIST:
	push(d.u.a[0]);
	break;

    default:
	pushUNDEF();
	break;
    }
}

void shift(void)
{
    Datum       addr, *dp;
    Int         i;

    DEBUGPRINT("VMOPER|DATSTK|SYMTBL shift\n", 0);
    pop_address(addr, "shift");
    mustlist(*(dp = dptr(addr)), "shift");

    if (dp->u.a->u.i > 0) {
	freedatum(dp->u.a[1]);
	for (i = 1; i < dp->u.a->u.i; i++)
	    dp->u.a[i] = dp->u.a[i + 1];
	--(dp->u.a->u.i);	/* a->u.i is no of items in the list [Ash] */
	ChangeSym(addr);
    } else
	error("zero sized list found in 'shift'");
}

void append(void)
{
    Datum       addr, *dp;
    Datum       d, *a;

    DEBUGPRINT("VMOPER|DATSTK|SYMTBL append\n", 0);
    d = pop();			/* value */

    pop_address(addr, "append");
    mustlist(*(dp = dptr(addr)), "append");

    a = (Datum *) erealloc(dp->u.a, (dp->u.a->u.i + 2) * sizeof(Datum));
    a[++a->u.i] = newdatum(d);	/* a->u.i is no of items in the list [Ash] */
    dp->u.a = a;
    ChangeSym(addr);
}

void insert(void)
{
    Datum       addr, *dp;
    Datum       d, p, *a;
    Int         i, pos;

    DEBUGPRINT("VMOPER|DATSTK|SYMTBL insert\n", 0);
    d = pop();			/* value */
    mustint(p = pop(), "insert");		/* position */
    pos = p.u.i;

    pop_address(addr, "insert");
    mustlist(*(dp = dptr(addr)), "insert");

    CheckRange(pos, 1, dp->u.a->u.i + 1);
    a = (Datum *) erealloc(dp->u.a, (dp->u.a->u.i + 2) * sizeof(Datum));
    for (i = a->u.i; i >= pos; --i)	/* move backward */
	a[i + 1] = a[i];
    a[pos] = newdatum(d);	/* insert */
    ++a->u.i;	/* a->u.i is no of items in the list [Ash] */
    dp->u.a = a;
    ChangeSym(addr);
}

void delete(void)
{
    Datum       addr, *dp;
    Datum       p, *a;
    Int         i, pos;

    DEBUGPRINT("VMOPER|DATSTK|SYMTBL delete\n", 0);
    mustint(p = pop(), "delete");		/* position */
    pos = p.u.i;

    pop_address(addr, "delete");
    mustlist(*(dp = dptr(addr)), "delete");

    CheckRange(pos, 1, dp->u.a->u.i);
    a = dp->u.a;
    freedatum(a[pos]);		/* delete */
    for (i = pos + 1; i <= a->u.i; i++)	/* move forward */
	a[i - 1] = a[i];
    --a->u.i;	/* a->u.i is no of items in the list [Ash] */
    dp->u.a = a;
    ChangeSym(addr);
}

#ifdef WEDEN_ENABLED

void query(void)
{

	Datum addr;
	symptr sp;
	symptr_QUEUE *P, *Q;
	extern void print(Datum, FILE *, int);

	pop_address(addr, "query (?)");
	sp = symbol_of(addr);

	startWedenMessage();
	appendWedenMessage("<b><item type=\"stdoutput\"><![CDATA[");

	if (sp == (symptr)0) {
		/* addr.u.v.y did not point to the containing symbol of the
		 Datum, so just print the Datum.  [Ash] */
		print(addr, stdout, 1);
		appendWedenMessage("\n");

	} else {
		switch (sp->stype) {
			case FORMULA:
				appendFormattedWedenMessage("%s is%s ", sp->name, sp->text);
				appendFormattedWedenMessage("/* current value of %s is ", sp->name);
				print(sp->d, stdout, 1);
				appendWedenMessage(" */\n");
			break;

			case FUNCTION:
			case PROCMACRO:
			case PROCEDURE:
				appendFormattedWedenMessage("%s %s",
						sp->stype == FUNCTION ? "func" :
						(sp->stype == PROCMACRO ? "procmacro" : "proc"),
						sp->name);
				Q = &sp->sources;
				for (P = FRONT(Q); P; P = NEXT(Q, P)) {
					appendFormattedWedenMessage("%c %s", P == Q->next ? ':' : ',', P->obj->name);
				}
				appendWedenMessage(" ");
				if (sp->text)
					appendFormattedWedenMessage("%s\n", sp->text);
				break;

			default:
				appendFormattedWedenMessage("%s=", sp->name);
				print(sp->d, stdout, 1); /* print datum: in builtin.c */
				appendWedenMessage(";\n");
				break;
		}

		appendFormattedWedenMessage("%s ~> [", sp->name);
		Q = &sp->targets;
		for (P = FRONT(Q); P; P = NEXT(Q, P)) {
			appendFormattedWedenMessage(P == Q->next ? "%s" : ", %s", P->obj->name);
		}
		appendFormattedWedenMessage("]; /* %s last changed by %s */\n", sp->name, sp->master);

	}

	appendWedenMessage("]]></item></b>");
	endWedenMessage();

	pushUNDEF();
}

#else
void query(void)
{
	
    Datum       addr;
    symptr      sp;
    symptr_QUEUE *P, *Q;
    extern void print(Datum, FILE *, int);

    DEBUGPRINT("VMOPER|DATSTK query\n", 0);

    pop_address(addr, "query (?)");
    sp = symbol_of(addr);

    if (sp == (symptr)0) {
      /* addr.u.v.y did not point to the containing symbol of the
         Datum, so just print the Datum.  [Ash] */
      print(addr, stdout, 1);
      outputMessage("\n");

    } else {
      switch (sp->stype) {
      case FORMULA:
	outputFormattedMessage("%s is%s ", sp->name, sp->text);
	outputFormattedMessage("/* current value of %s is ", sp->name);
	print(sp->d, stdout, 1);
	outputMessage(" */\n");
	break;

      case FUNCTION:
      case PROCMACRO:
      case PROCEDURE:
	outputFormattedMessage("%s %s",
	       sp->stype == FUNCTION ? "func" :
	       (sp->stype == PROCMACRO ? "procmacro" : "proc"),
	       sp->name);
	Q = &sp->sources;
	for (P = FRONT(Q); P; P = NEXT(Q, P)) {
	  outputFormattedMessage("%c %s", P == Q->next ? ':' : ',', P->obj->name);
	}
	outputMessage(" ");
	if (sp->text)
	  outputFormattedMessage("%s\n", sp->text);
	break;

      default:
	outputFormattedMessage("%s=", sp->name);
	print(sp->d, stdout, 1); /* print datum: in builtin.c */
	outputMessage(";\n");
	break;
      }

      outputFormattedMessage("%s ~> [", sp->name);
      Q = &sp->targets;
      for (P = FRONT(Q); P; P = NEXT(Q, P)) {
	outputFormattedMessage(P == Q->next ? "%s" : ", %s", P->obj->name);
      }
      outputFormattedMessage("]; /* %s last changed by %s */\n", sp->name, sp->master);

    }
    pushUNDEF();
}
#endif


