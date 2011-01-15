/*
 * $Id: lib.c,v 1.7 2002/02/18 19:29:51 cssbz Exp $
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

static char rcsid[] = "$Id: lib.c,v 1.7 2002/02/18 19:29:51 cssbz Exp $";

#include "../../../config.h"
#include "eden.h"
#include "builtin.h"
#include "yacc.h"

/*
 * call_lib --- C library interface
 *
 * It is a useful interface to include the C library routines into the
 * language.
 *
 * Weak points:
 *
 * (1)  Only the first 10 arguments can be passed.
 *
 * (2)  The interface cannot deal with list (cause the different internal
 * representation of data structures).
 *
 * (3)  All the values returned by library routines are integers.
 * Programmers must explicitly write the type convertion codes. See
 * type.c for the type-convertion functions.
 *
 * (4)  The interface can handle ``true'' functions, NOT macros.
 *
 */

#define PARSIZE 10
#define PushInteger(data) {Datum d;d.type=INTEGER;d.u.i=(Int)data;push(d);}
#define PushPointer(data) {Datum d;d.type=POINTER;d.u.i=(Int)data;push(d);}
#define PushReal(data) {Datum d;d.type=REAL;d.u.r=(double)data;push(d);}

/**************************************/

void
call_lib(Inst inst)
{
    Int         i, j;
    Int         A[PARSIZE];
    Int         result;

    for (i = 1, j = 0; i <= paracount && j < PARSIZE; i++) {
	if (is_address(para(i)))
	    A[j++] = (Int) & (((Datum *) para(i).u.v.x)->u.i);
	else
	    switch (para(i).type) {
	    case INTEGER:
	    case MYCHAR:
	    case STRING:
		A[j++] = para(i).u.i;
		break;

	    case REAL:
		A[j++] = para(i).u.v.x;
		A[j++] = para(i).u.v.y;
		break;

	    default:
		errorf("parameter with illegal type passed to C-lib function (got %s for parameter no %d)",
		       typename(para(i).type), i);
		break;
	    }
    }

    result = ((int (*) ()) inst) (A[0], A[1], A[2], A[3], A[4],
				  A[5], A[6], A[7], A[8], A[9]);
    PushInteger(result);
}

void
call_lib64(Inst inst)
{
    Int         i, j;
    Int         A[PARSIZE];
    Int         result;

    for (i = 1, j = 0; i <= paracount && j < PARSIZE; i++) {
	if (is_address(para(i)))
	    A[j++] = (Int) & (((Datum *) para(i).u.v.x)->u.i);
	else
	    switch (para(i).type) {
	    case INTEGER:
	    case MYCHAR:
	    case STRING:
		A[j++] = para(i).u.i;
		break;

	    case REAL:
		A[j++] = para(i).u.v.x;
		A[j++] = para(i).u.v.y;
		break;

	    default:
		errorf("parameter with illegal type passed to C-lib function (got %s for parameter no %d)",
		       typename(para(i).type), i);
		break;
	    }
    }

    result = ((Int (*) ()) inst) (A[0], A[1], A[2], A[3], A[4],
				  A[5], A[6], A[7], A[8], A[9]);
    PushInteger(result);
}

void
call_float(Inst inst)
{
  /* All the functions that return real values seem to require real
     values for their parameters, so I'm casting any numbers passed in
     to reals.  It isn't now possible to pass integers to a function
     declared as SameReal.  [Ash] */
  double A[PARSIZE];
  double result;
  int i;
  
  for (i = 1; i <= paracount; i++) {
    switch(para(i).type) {
    case INTEGER:
      A[i-1] = (double)(fp->stackp->u.a[i].u.i);
      break;

    case REAL:
      A[i-1] = (double)(fp->stackp->u.a[i].u.r);
      break;

    default:
      errorf("parameter with illegal type passed to C-lib function: (got %s for parameter no %d)",
	     typename(para(i).type), i);
      break;
    }
  }

  result = ((double (*) ()) inst) (A[0], A[1], A[2], A[3], A[4],
				   A[5], A[6], A[7], A[8], A[9]);
  PushReal(result);
}
