/*
 * $Id: check.c,v 1.11 2002/07/10 19:20:26 cssbz Exp $
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

static char rcsid[] = "$Id: check.c,v 1.11 2002/07/10 19:20:26 cssbz Exp $";

#include "../../../config.h"
#include "error.h"
#include "oper.h"
#include "tree.h"
#include "symbol.h"
#include "parser.h"

#include <inttypes.h>

#define ERROR -1

extern void change_context(symbol *);
extern void resume_context(void);
extern int  reset_context(void);
extern void backup_context(void);
extern void restore_context(void);
extern int  nary(int);

/*function prototypes */
int         count_id(tree);
int         count_expr(tree);
static void TypeClash(void);
static void check_id(tree);
static void check_children_type(tree);
static int  semantic_1(tree, int, int);
static int  semantic_2(tree, int, int, int);
static int  semantic_3(tree, int, int, int, int);
static int  semantic_4(tree, int, int, int, int, int);


static void
TypeClash(void)
{
    don_err(TypeMismatch, 0);
}

/*--------------------------------------------------+----------+
                                                    | check_id |
                                                    +----------*/
static void
check_id(tree expr)
{				/* Is variable declared ? */
    /* id-expr is assumed */
    char       *id;
    symbol     *sym;
    symbol     *look_up(char *);
    char       *donald_full_name(char *);

    switch (expr->op) {
    case OP_ID:
	id = (char *) Lexpr;
	if (id == 0)
	    don_err(Unclassified, "illegal use of `~'");
	if ((sym = look_up(id)) == SymEnd)
	    don_err(UndeclareID, donald_full_name(id));
	Etype = sym->type;
	break;

    case OP_SLASH:
	id = (char *) Lexpr->left;
	if (id) {
	    if ((sym = look_up(id)) == SymEnd)
		don_err(UndeclareID, donald_full_name(id));
	    if (sym->type != OPENSHAPE && sym->type != GRAPH)
		don_err(NotOpenshapeOrGraph, donald_full_name(id));
	    change_context(sym);
	} else {		/* id == ~ */
	    change_context(SymEnd);
	}
	check_id(Rexpr);
	Etype = Rtype;
	resume_context();
	break;

    case OP_GLOBAL:
	backup_context();
	reset_context();
	check_id(Lexpr);
	Etype = Ltype;
	restore_context();
	break;

    case OP_EDEN:
	Etype = ANY;
	break;

    default:
	don_err(Impossible, "check.c");
	break;
    }
}

/*--------------------------------------------------+----------+
                                                    | count_id |
                                                    +----------*/
int
count_id(tree expr)
{				/* return no. of id in an id-list */
    int         lcount, rcount;

    if (expr == EMPTYTREE)
	return 0;

    switch (expr->op) {
    case OP_ID:
    case OP_SLASH:
    case OP_GLOBAL:
	check_id(expr);
    case OP_EDEN:
	return 1;

    case OP_COMMA:
	lcount = count_id(Lexpr);
	rcount = count_id(Rexpr);
	return (lcount == ERROR) || (rcount == ERROR)
	    ? ERROR : lcount + rcount;

    default:
	return ERROR;
    }
}

/*------------+------------------------------------------------+
              | semantic_1, semantic_2, semantic_3, semantic_4 |
              +------------------------------------------------*/
/* check expression types:
**      E = result data type (if ok)
**      L = left expr data type expected
**      M = middle expr data type expected
**      R = right expr data type expected
**      X = extra expr data type expected
**      return 0 if ok
**             1 if not match
**
**      and also set expr->type [Ash] to E
**
*/

static int
semantic_1(tree expr, int E, int L)
{
    return (Ltype != L && Ltype != ANY) ||
    ((Etype = E), 0);
}

static int
semantic_2(tree expr, int E, int L, int R)
{
    return (Ltype != L && Ltype != ANY) ||
    (Rtype != R && Rtype != ANY) ||
    ((Etype = E), 0);
}

static int
semantic_3(tree expr, int E, int L, int M, int R)
{
    return (Ltype != L && Ltype != ANY) ||
    (Mtype != M && Mtype != ANY) ||
    (Rtype != R && Rtype != ANY) ||
    ((Etype = E), 0);
}

static int
semantic_4(tree expr, int E, int L, int M, int R, int X)
{
    return (Ltype != L && Ltype != ANY) ||
    (Mtype != M && Mtype != ANY) ||
    (Rtype != R && Rtype != ANY) ||
    (Xtype != X && Xtype != ANY) ||
    ((Etype = E), 0);
}

/*------------------------------------------------+------------+
                                                  | count_expr |
                                                  +------------*/
int
count_expr(tree expr)
{				/* return no. of expr in an expr-list */
    if (expr == EMPTYTREE)
	return 0;

    check_children_type(expr);
    switch (expr->op) {
    case OP_COMMA:
	return count_expr(Lexpr) + count_expr(Rexpr);

    case OP_INF:
	Etype = INT;
	break;

    case OP_PLUS:
    case OP_MINUS:
	if (semantic_2(expr, Ltype, Ltype, Ltype) &&
	    semantic_2(expr, REAL, REAL, INT) &&
	    semantic_2(expr, REAL, INT, REAL))
	    TypeClash();
	else if (Etype == POINT) {
	    switch (expr->op) {
	    case OP_PLUS:
		expr->op = OP_VECT_ADD;
		break;
	    case OP_MINUS:
		expr->op = OP_VECT_SUB;
		break;
	    }
	}
	break;

    case OP_MULT:
    case OP_DIV:
	if (semantic_2(expr, INT, INT, INT) &&
	    semantic_2(expr, REAL, INT, REAL) &&
	    semantic_2(expr, REAL, REAL, INT) &&
	    semantic_2(expr, REAL, REAL, REAL) &&
	    semantic_2(expr, POINT, POINT, INT) &&
	    semantic_2(expr, POINT, POINT, REAL))
	    TypeClash();
	else {
	    if (Ltype == POINT) {
		Etype = POINT;
	    } else if (Ltype == REAL || Rtype == REAL) {
		Etype = REAL;
	    }
	    if (Etype == POINT) {
		switch (expr->op) {
		case OP_MULT:
		    expr->op = OP_SCALAR_MULT;
		    break;
		case OP_DIV:
		    expr->op = OP_SCALAR_DIV;
		    break;
		}
	    }
	}
	break;

    case OP_MOD:
	if (semantic_2(expr, Ltype, Ltype, INT))
	    TypeClash();
	else if (Etype == POINT)
	    expr->op = OP_SCALAR_MOD;
	else if (Etype != INT)
	    TypeClash();
	break;

    case OP_UMINUS:
    case OP_RANDOM:
	if (semantic_1(expr, INT, INT) &&
	    semantic_1(expr, REAL, REAL))
	    TypeClash();
	break;

    case OP_TRUNC:
	if (semantic_1(expr, INT, REAL))
	    TypeClash();
	break;

    case OP_FLOAT:
	if (semantic_1(expr, REAL, INT))
	    TypeClash();
	break;

    case OP_SQRT:
    case OP_SIN:
    case OP_COS:
    case OP_TAN:
    case OP_ASIN:
    case OP_ACOS:
    case OP_ATAN:
    case OP_LOG:
    case OP_EXP:
	if (semantic_1(expr, REAL, INT)) {
	    if (semantic_1(expr, REAL, REAL))
		TypeClash();
	} else			/* cast the argument to floating point */
	    expr->left = dtree1(OP_FLOAT, expr->left);
	break;

    case OP_DOTX:
    case OP_DOTY:
	if (semantic_1(expr, POINT, POINT))
	    TypeClash();
	break;

    case OP_DOTRAD:
    case OP_DOTARG:
	if (semantic_1(expr, REAL, POINT) &&	/* can be INT */
	    semantic_1(expr, REAL, ARC))
	    TypeClash();
	break;

    case OP_DOT1:
    case OP_DOT2:
	if (semantic_1(expr, REAL, POINT) &&	/* can be INT */
	    semantic_1(expr, POINT, LINE) &&
	    semantic_1(expr, POINT, ARC))
	    TypeClash();
	break;

    case OP_CART:
    case OP_POLAR:
	if (semantic_2(expr, POINT, INT, INT) &&
	    semantic_2(expr, POINT, REAL, REAL) &&
	    semantic_2(expr, POINT, INT, REAL) &&
	    semantic_2(expr, POINT, REAL, INT))
	    TypeClash();
	break;

    case OP_LINE:
	if (semantic_2(expr, LINE, POINT, POINT))
	    TypeClash();
	break;

    case OP_ARC:
	if (semantic_3(expr, ARC, POINT, POINT, REAL) &&
	    semantic_3(expr, ARC, POINT, POINT, INT))
	    TypeClash();
	break;

    case OP_CIRCLE:
	if (semantic_2(expr, CIRCLE, POINT, REAL) &&
	    semantic_2(expr, CIRCLE, POINT, INT))
	    TypeClash();
	break;

    case OP_ELLIPSE:
	if (semantic_3(expr, ELLIPSE, POINT, POINT, POINT) &&
	    semantic_3(expr, ELLIPSE, POINT, POINT, POINT))
	    TypeClash();
	break;

    case OP_RECTANGLE:
	if (semantic_2(expr, RECTANGLE, POINT, POINT))
	    TypeClash();
	break;

    case OP_LABEL:
	if (semantic_2(expr, LABEL, MYCHAR, POINT) &&
	    semantic_2(expr, LABEL, IMAGE, POINT))
	    TypeClash();
	break;

    case OP_IF:
	if (semantic_3(expr, Ltype, Ltype, BOOLEAN, Ltype))
	    TypeClash();
	break;

    case OP_SLASH_SLASH:
	if (semantic_2(expr, MYCHAR, MYCHAR, MYCHAR))
	    TypeClash();
	break;

    case OP_ITOS:
	if (semantic_1(expr, MYCHAR, INT))
	    TypeClash();
	break;

    case OP_RTOS:
	if (semantic_2(expr, MYCHAR, REAL, MYCHAR))
	    TypeClash();
	break;

/*
    case OP_MONITOR:
	if (semantic_3(expr, MONITOR, MYCHAR, BOOLEAN, MYCHAR))
	    TypeClash();
	break;

    case OP_IMPOSE:
	if (semantic_1(expr, IMPOSE, BOOLEAN))
	    TypeClash();
	break;
*/

    case OP_INTERSECT:
	if (semantic_2(expr, POINT, LINE, LINE))
	    TypeClash();
	break;

    case OP_PARALLEL:
	if (semantic_4(expr, LINE, LINE, POINT, INT, INT) &&
	    semantic_4(expr, LINE, LINE, POINT, REAL, REAL) &&
	    semantic_4(expr, LINE, LINE, POINT, INT, REAL) &&
	    semantic_4(expr, LINE, LINE, POINT, REAL, INT))
	    TypeClash();
	break;

    case OP_PERPEND:
	if (semantic_2(expr, LINE, POINT, LINE))
	    TypeClash();
	break;

    case OP_DISTANCE:
	if (semantic_2(expr, REAL, POINT, POINT) &&
	    semantic_2(expr, REAL, LINE, LINE))
	    TypeClash();
	break;

    case OP_MIDPOINT:
	if (semantic_1(expr, POINT, LINE))
	    TypeClash();
	break;

    case OP_TRANS:
	switch (Ltype) {
	case SHAPE:
	case OPENSHAPE:
	    if (semantic_3(expr, SHAPE, Ltype, INT, INT) &&
		semantic_3(expr, SHAPE, Ltype, INT, REAL) &&
		semantic_3(expr, SHAPE, Ltype, REAL, INT) &&
		semantic_3(expr, SHAPE, Ltype, REAL, REAL))
		TypeClash();
	    break;
	case INT:
	case REAL:
	    TypeClash();
	    break;
	default:
	    if (semantic_3(expr, Ltype, Ltype, INT, INT) &&
		semantic_3(expr, Ltype, Ltype, INT, REAL) &&
		semantic_3(expr, Ltype, Ltype, REAL, INT) &&
		semantic_3(expr, Ltype, Ltype, REAL, REAL))
		TypeClash();
	    break;
	}
	break;

    case OP_ROT:
	switch (Ltype) {
	case SHAPE:
	case OPENSHAPE:
	    if (semantic_3(expr, SHAPE, Ltype, POINT, INT) &&
		semantic_3(expr, SHAPE, Ltype, POINT, REAL))
		TypeClash();
	    break;
	case INT:
	case REAL:
	    TypeClash();
	    break;
	default:
	    if (semantic_3(expr, Ltype, Ltype, POINT, INT) &&
		semantic_3(expr, Ltype, Ltype, POINT, REAL))
		TypeClash();
	    break;
	}
	break;

    case OP_SCALE:
	switch (Ltype) {
	case SHAPE:
	case OPENSHAPE:
	    if (semantic_2(expr, SHAPE, Ltype, INT) &&
		semantic_2(expr, SHAPE, Ltype, REAL))
		TypeClash();
	    break;
	default:
	    if (semantic_2(expr, Ltype, Ltype, INT) &&
		semantic_2(expr, Ltype, Ltype, REAL))
		TypeClash();
	    break;
	}
	break;

    case OP_SCALEXY:
	switch (Ltype) {
	case SHAPE:
	case OPENSHAPE:
	    if (semantic_3(expr, SHAPE, Ltype, INT, INT) &&
		semantic_3(expr, SHAPE, Ltype, INT, REAL) &&
		semantic_3(expr, SHAPE, Ltype, REAL, INT) &&
		semantic_3(expr, SHAPE, Ltype, REAL, REAL))
		TypeClash();
	    break;
	case INT:
	case REAL:
	    TypeClash();
	    break;
	default:
	    if (semantic_3(expr, Ltype, Ltype, INT, INT) &&
		semantic_3(expr, Ltype, Ltype, INT, REAL) &&
		semantic_3(expr, Ltype, Ltype, REAL, INT) &&
		semantic_3(expr, Ltype, Ltype, REAL, REAL))
		TypeClash();
	    break;
	}
	break;

    /* Reflect takes an entity and a line and returns an entity.
       Parameters to semantic_2 are expr, the resulting type in this
       case, the first and second parameters.  [Ash] */
    case OP_REFLECT:
      switch (Ltype) {
      case SHAPE:
      case OPENSHAPE:
	if (semantic_2(expr, SHAPE, Ltype, LINE))
	  TypeClash();
	break;
      case INT:
      case REAL:
	TypeClash();
	break;
      default:
	if (semantic_2(expr, Ltype, Ltype, LINE))
	  TypeClash();
	break;
      }
      break;

    case OP_PAREN:
	Etype = Ltype;
	break;

    case OP_ID:
    case OP_SLASH:
    case OP_GLOBAL:
	check_id(expr);
	break;

    case OP_AND:
    case OP_OR:
	if (semantic_2(expr, BOOLEAN, BOOLEAN, BOOLEAN))
	    TypeClash();
	break;

    case OP_NOT:
	if (semantic_1(expr, BOOLEAN, BOOLEAN))
	    TypeClash();
	break;

    case OP_EQ_EQ:
    case OP_NOT_EQ:
    case OP_LT:
    case OP_LT_EQ:
    case OP_GT:
    case OP_GT_EQ:
	if (semantic_2(expr, BOOLEAN, INT, INT) &&
	    semantic_2(expr, BOOLEAN, REAL, REAL) &&
	    semantic_2(expr, BOOLEAN, INT, REAL) &&
	    semantic_2(expr, BOOLEAN, REAL, INT))
	    TypeClash();
	break;

    case OP_COLINEAR:
    case OP_PT_BETWN_PTS:
	if (semantic_3(expr, BOOLEAN, POINT, POINT, POINT))
	    TypeClash();
	break;

    case OP_INTERSECTS:
	if (semantic_2(expr, BOOLEAN, LINE, LINE))
	    TypeClash();
	break;

    case OP_SEPARATES:
	if (semantic_3(expr, BOOLEAN, LINE, POINT, POINT))
	    TypeClash();
	break;

    case OP_INCLUDES:
	if (semantic_2(expr, BOOLEAN, CIRCLE, POINT))
	    TypeClash();
	break;

    case OP_INCIDENT:
	if (semantic_2(expr, BOOLEAN, LINE, POINT) &&
	    semantic_2(expr, BOOLEAN, CIRCLE, POINT))
	    TypeClash();
	break;

    case OP_DISTLARGER:
    case OP_DISTSMALLER:
	if (semantic_3(expr, BOOLEAN, POINT, POINT, INT) &&
	    semantic_3(expr, BOOLEAN, POINT, POINT, REAL) &&
	    semantic_3(expr, BOOLEAN, LINE, POINT, INT) &&
	    semantic_3(expr, BOOLEAN, LINE, POINT, REAL))
	    TypeClash();
	break;

    case OP_I:
	Etype = INT;
	break;

    case OP_XI:
	Etype = REAL;
	break;

    case OP_FI:
	Etype = REAL;
	break;

    case OP_I_1:
	Etype = INT;
	break;

    case OP_XI_1:
	Etype = REAL;
	break;

    case OP_FI_1:
	Etype = REAL;
	break;

    case OP_GSPECLIST:
	if (Lexpr == 0)
	    Etype = GSPEC;
	else if (semantic_2(expr, GSPEC, GSPEC, GSPEC))
	    TypeClash();
	break;

    case OP_GSPEC:
	if (Lexpr == 0)
	    Etype = GSPEC;
	else if ((Ltype != (intptr_t) Rexpr) || ((Etype = GSPEC), 0))
	    TypeClash();
	break;

    case OP_IMGFUNC:
	count_expr(Rexpr);
	Etype = IMAGE;
	break;

    case OP_FUNC:
	count_expr(Rexpr);
    case OP_EDEN:
	Etype = ANY;
	break;
    }
    return 1;			/* default */
}

static void
check_children_type(tree expr)
{
    if (expr->type != UNDEFINED)/* already checked */
	return;

    switch (nary(expr->op)) {
    case 4:
	count_expr(expr->extra);
    case 3:
	count_expr(expr->mid);
    case 2:
	count_expr(expr->right);
    case 1:
	count_expr(expr->left);
    }
}
