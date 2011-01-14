/*
 * $Id: tree.c,v 1.12 2001/08/02 16:26:11 cssbz Exp $
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

static char rcsid[] = "$Id: tree.c,v 1.12 2001/08/02 16:26:11 cssbz Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tk.h>

#include "../../../../../config.h"
#include "tree.h"
#include "symbol.h"
#include "error.h"
#include "oper.h"
#include "parser.h"
#include "../EX/script.h"

#include "../Eden/emalloc.h"
#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

#include <inttypes.h>

extern Tcl_Interp *interp;
extern void *getmem(int);
extern char *map_token_to_name(intptr_t);
extern char *expr_to_eden_name(tree);
extern symbol *look_up_tree(tree);
extern symbol *look_up(char *);
extern char *eden_full_name(char *);

extern Script *dd_script;
static char temp[256];
static int  stage = 1;		/* controlling the translation process */
static char *defname;		/* name of the variable to be defined */
static tree *TreeList;		/* list of pointers to the image
				   expressions */
static int  treeListSize = 0;
static int  TopTreeList = 0;
static char tempname[80];	/* name of a temp. image variable */
static int  inPrintTree = 0;

#define lastchar(s)     s[strlen(s)-1]

/* function prototypes */
void        initbuf(int);
void        extendbuf(int);
int         is_shape(tree);
int         is_openshape(tree);
int         is_graph(tree);
static void appendTreeList(tree);
tree        dtree4(int, tree, tree, tree, tree);
tree        dtree3(int, tree, tree, tree);
tree        dtree2(int, tree, tree);
tree        dtree1(int, tree);
tree        dtree0(int);
int         nary(int);
void        freeTree(tree);
tree        copyTree(tree);
static void dump_str(char *);
void        dumpdtree(tree);
void        print_tree(tree expr);
void        Define(tree expr1, tree expr2);
void        DefineFunc(int FX, tree expr);
#ifdef DISTRIB
static int DonaldExprOracleCheck(tree);
#endif /* DISTRIB */

static int  dumpbufsize = 0;
char       *dumpbuf = 0;

void
initbuf(int size)
{
    if (dumpbuf == 0) {
	dumpbuf = (char *) emalloc(size);
	dumpbufsize = size;
    } else if (size > dumpbufsize) {
	dumpbuf = (char *) erealloc(dumpbuf, size);
	dumpbufsize = size;
    }
    *dumpbuf = '\0';
}

void
extendbuf(int size)
{
    if (dumpbuf == 0) {
	dumpbuf = (char *) emalloc(size);
	dumpbufsize = size;
    } else if (size > dumpbufsize) {
	dumpbuf = (char *) erealloc(dumpbuf, size);
	dumpbufsize = size;
    }
}

int
is_shape(tree expr)
{
    return expr->type == SHAPE;
}

int
is_openshape(tree expr)
{
    return expr->type == OPENSHAPE;
}

int
is_graph(tree expr)
{
    return expr->type == GRAPH;
}

static void
appendTreeList(tree t)
{
    if (treeListSize == 0) {
	treeListSize = 4;
	TreeList = (tree *) emalloc(sizeof(tree) * treeListSize);
    } else if (TopTreeList == treeListSize) {
	treeListSize += 4;
	TreeList = (tree *) erealloc(TreeList, sizeof(tree) * treeListSize);
    }
    TreeList[TopTreeList++] = t;
}

#define clearTreeList() (TopTreeList = 0)

/*-------------------------+----------------------------------------+
                           | dtree0, dtree1, dtree2, dtree3, dtree4 |
                           +----------------------------------------*/
 /*----- creat a new tree -----*/

tree
dtree4(int op, tree l, tree m, tree r, tree x)
 /* parallel, left, mid, right and extra children */
{
    tree        ptr;

    ptr = (tree) getmem(sizeof(node4));
    ptr->type = UNDEFINED;
    ptr->op = op;
    ptr->left = l;
    ptr->right = r;
    ptr->mid = m;
    ptr->extra = x;
    return ptr;
}

tree
dtree3(int op, tree l, tree m, tree r)
 /* if...then...else/rot(,,)/etc. */
 /* left, mid and right children */
{
    tree        ptr;

    ptr = (tree) getmem(sizeof(node3));
    ptr->type = UNDEFINED;
    ptr->op = op;
    ptr->left = l;
    ptr->right = r;
    ptr->mid = m;
    return ptr;
}

tree
dtree2(int op, tree l, tree r)
 /* operator: + - * / etc. */
 /* left and right children */
{
    tree        ptr;

    ptr = (tree) getmem(sizeof(node2));
    ptr->type = UNDEFINED;
    ptr->op = op;
    ptr->left = l;
    ptr->right = r;
    return ptr;
}

tree
dtree1(int op, tree l)
 /* id, inumber, etc. */
 /* left child */
{
    tree        ptr;

    ptr = (tree) getmem(sizeof(node1));
    ptr->type = UNDEFINED;
    ptr->op = op;
    ptr->left = l;
    return ptr;
}

tree
dtree0(int op)
 /* inf */
{
    tree        ptr;

    ptr = (tree) getmem(sizeof(node0));
    ptr->type = UNDEFINED;
    ptr->op = op;
    return ptr;
}

/*---------------------------------------------------+---------+
                                                     | trans[] |
                                                     +---------*/
static struct trans trans[] = TRANSLAT;

/*--------------------------------------------+----------------+
                                              | match_operator |
                                              +----------------*/
#define match_operator(op, s) \
        (s = trans[(int)op].op_str, trans[(int)op].order)

/*----------------------------------------------------+--------+
                                                      | orig[] |
                                                      +--------*/
static struct trans orig[] = ORIGINAL;

/*---------------------------------------+---------------------+
                                         | match_orig_operator |
                                         +---------------------*/
#define match_orig_operator(op, s) \
        (s = orig[(int)op].op_str, orig[(int)op].order)

int
nary(int opcode)
{
    switch (trans[opcode].order) {
	case OP_IMPOSE:
	case OP_GSPEC:
	case UNARY:
	return 1;
    case BINARY:
    case INFIX:
    case OP_GSPECLIST:
	return 2;
    case OP_IF:
    case OP_MONITOR:
    case TRIARY:
	return 3;
    case QUADARY:
	return 4;
    default:
	return 0;
    }
}

/*--------------------------------------------------+----------+
                                                    | freeTree |
                                                    +----------*/
/*----- free the memory of a tree -----*/
void
freeTree(tree expr)
{
    char       *op_string;	/* dummy */

    switch (match_operator(expr->op, op_string)) {
    case OP_ID:
    case OP_EDEN:
	if (Lexpr)
	    free(Lexpr);
    case OP_BOOL:
    case OP_CSTRING:
    case OP_RNUMBER:
    case OP_INUMBER:
    case CONSTANT:
	break;
    case QUADARY:
	freeTree(Xexpr);	/* quadary */
    case OP_IF:
    case OP_MONITOR:
    case TRIARY:
	freeTree(Mexpr);	/* tri-ary */
    case OP_SLASH:
    case INFIX:
    case BINARY:
	freeTree(Rexpr);	/* binary */
    case OP_IMPOSE:
    case UNARY:
    case OP_GLOBAL:
	freeTree(Lexpr);	/* unary */
	break;
    case OP_GSPECLIST:
	if (Lexpr) {
	    freeTree(Lexpr);
	    freeTree(Rexpr);
	}
	break;
    case OP_GSPEC:
	if (Lexpr)
	    freeTree(Lexpr);
	break;
    case OP_IMGFUNC:
    case OP_FUNC:
	if (Lexpr)
	    free(Lexpr);
	if (Rexpr)
	    freeTree(Rexpr);
	break;
    default:
	don_err(Impossible, "free_tree");
	break;
    }
    free(expr);			/* free itself */
}

#define Value(x) ((node0*)x)->d

/*--------------------------------------------------+----------+
                                                    | copyTree |
                                                    +----------*/
/*----- copy a tree -----*/
tree
copyTree(tree expr)
{
    tree        t;
    char       *op_string;	/* dummy */

    if (expr == 0)
	return 0;
    switch (match_operator(expr->op, op_string)) {
    case OP_ID:
    case OP_EDEN:
	t = dtree1(expr->op, (tree) (Lexpr ? strdup((char *) Lexpr) : 0));
	t->type = expr->type;
	break;
    case OP_BOOL:
    case OP_CSTRING:
    case OP_RNUMBER:
    case OP_INUMBER:
    case CONSTANT:
	t = dtree0(expr->op);
	t->type = expr->type;
	Value(t) = Value(expr);
	break;
    case QUADARY:		/* quadary */
	t = dtree4(expr->op, copyTree(Lexpr), copyTree(Mexpr),
		   copyTree(Rexpr), copyTree(expr->extra));
	t->type = expr->type;
	break;
    case OP_IF:
    case OP_MONITOR:
    case TRIARY:		/* tri-ary */
	t = dtree3(expr->op, copyTree(Lexpr), copyTree(Mexpr),
		   copyTree(Rexpr));
	t->type = expr->type;
	break;
    case INFIX:
    case BINARY:		/* binary */
    case OP_SLASH:
    case OP_GSPECLIST:
	t = dtree2(expr->op, copyTree(Lexpr), copyTree(Rexpr));
	t->type = expr->type;
	break;
    case OP_IMPOSE:
    case OP_GLOBAL:
    case UNARY:		/* unary */
	t = dtree1(expr->op, copyTree(Lexpr));
	t->type = expr->type;
	break;
    case OP_GSPEC:
	t = dtree2(expr->op, copyTree(Lexpr), Rexpr);
	t->type = expr->type;
	break;
    case OP_IMGFUNC:
    case OP_FUNC:
	t = dtree2(expr->op, (tree) strdup((char *) Lexpr), copyTree(Rexpr));
	t->type = expr->type;
	break;
    default:
	don_err(Impossible, "copyTree");
	break;
    }
    return t;
}

/*-------------------------------------------------+-----------+
                                                   | dumpdtree |
                                                   +-----------*/
static char Comma[] = ", ";
static char Rparen[] = ")";

static void
dump_str(char *s)
{
    if ((int) strlen(dumpbuf) + (int) strlen(s) >= dumpbufsize)
	extendbuf(dumpbufsize + 2048);
    strcat(dumpbuf, s);
}

void
dumpdtree(tree expr)
{
    int         order;
    char       *op_string;
    char       *s, *t;
    static      glevel = 0;

    if (expr == EMPTYTREE)
	return;

    switch (expr->op) {
    case OP_EDEN:
	dump_str((char *) Lexpr);
	dump_str("!");
	return;

    case OP_ID:
	dump_str(Lexpr ? (char *) Lexpr : "~");
	return;

    case OP_SLASH:
	dumpdtree(Lexpr);
	dump_str("/");
	dumpdtree(Rexpr);
	return;

    case OP_GLOBAL:
	dump_str("/");
	dumpdtree(Lexpr);
	return;

    case OP_INUMBER:		/* integer number */
	sprintf(temp, "%d", Ivalue(expr));
	dump_str(temp);
	return;

    case OP_RNUMBER:		/* real number */
	sprintf(temp, "%f", Rvalue(expr));
	dump_str(temp);
	return;

    case OP_CSTRING:		/* character string */
	for (s = Cvalue(expr); *s != '\0'; s++) {
	    t = dumpbuf + strlen(dumpbuf);
	    switch (*s) {
	    case '\\':
		*t++ = '\\';
		*t++ = '\\';
		*t = '\0';
		break;
	    case '"':
		*t++ = '\\';
		*t++ = '"';
		*t = '\0';
		break;
	    default:
		*t++ = *s;
		*t = '\0';
		break;
	    }
	}
	return;

    case OP_BOOL:		/* boolean value */
	sprintf(temp, Bvalue(expr) ? "true" : "false");
	dump_str(temp);
	return;

    case OP_IMPOSE:		/* impose variable */
	dump_str("impose ");
	dumpdtree(Lexpr);
	return;

    case OP_IF:		/* if .. then .. else .. */
	dump_str("if ");
	dumpdtree(Mexpr);
	dump_str(" then ");
	dumpdtree(Lexpr);
	dump_str(" else ");
	dumpdtree(Rexpr);
	return;
    case OP_MONITOR:
	dump_str("(");
	dumpdtree(Mexpr);
	dump_str(") ? ");
	dumpdtree(Lexpr);
	dump_str(" : ");
	dumpdtree(Rexpr);
	return;
    case OP_CART:
	dump_str("{");
	dumpdtree(Lexpr);
	dump_str(Comma);
	dumpdtree(Rexpr);
	dump_str("}");
	return;
    case OP_POLAR:
	dump_str("{");
	dumpdtree(Lexpr);
	dump_str(" @ ");
	dumpdtree(Rexpr);
	dump_str("}");
	return;
    case OP_LINE:
	if (inPrintTree)
	    dump_str("[");
	else
	    dump_str("\\[");
	dumpdtree(Lexpr);
	dump_str(Comma);
	dumpdtree(Rexpr);
	dump_str("]");
	return;
    case OP_ARC:
	if (inPrintTree)
	    dump_str("[");
	else
	    dump_str("\\[");
	dumpdtree(Lexpr);
	dump_str(Comma);
	dumpdtree(Mexpr);
	dump_str(Comma);
	dumpdtree(Rexpr);
	dump_str("]");
	return;
    case OP_GSPECLIST:
	if (Lexpr == 0) {
	    if (inPrintTree)
		dump_str("[]");
	    else
		dump_str("\\[]");
	    return;
	}
	if (glevel == 0) {
	    if (inPrintTree)
		dump_str("[");
	    else
		dump_str("\\[");
	    glevel++;
	    dumpdtree(Lexpr);
	    dump_str(";\n\t");
	    dumpdtree(Rexpr);
	    glevel--;
	    dump_str("]");
	} else {
	    dumpdtree(Lexpr);
	    dump_str(";\n\t");
	    dumpdtree(Rexpr);
	}
	return;
    case OP_GSPEC:
	if (glevel == 0) {
	    if (inPrintTree)
		dump_str("[");
	    else
		dump_str("\\[");
	    glevel++;
	    dump_str(map_token_to_name((intptr_t) expr->right));
	    dump_str(":");
	    if (Lexpr)
		dumpdtree(Lexpr);
	    glevel--;
	    dump_str("]");
	} else {
	    dump_str(map_token_to_name((intptr_t) expr->right));
	    dump_str(":");
	    if (Lexpr)
		dumpdtree(Lexpr);
	}
	return;
    case OP_I:
    case OP_XI:
    case OP_FI:
    case OP_I_1:
    case OP_XI_1:
    case OP_FI_1:
	if (inPrintTree)
	    order = match_operator(expr->op, op_string);
	else
	    order = match_orig_operator(expr->op, op_string);
	/* order is not used */
	dump_str(op_string);
	return;
    case OP_IMGFUNC:
	dump_str("I!");
	dump_str((char *) Lexpr);
	dump_str("(");
	dumpdtree(Rexpr);
	dump_str(")");
	return;
    case OP_FUNC:
	dump_str((char *) Lexpr);
	dump_str("!(");
	dumpdtree(Rexpr);
	dump_str(")");
	return;
    }

    /* otherwise, node is operator */

    switch (order = match_orig_operator(expr->op, op_string)) {
    case CONSTANT:
	dump_str(op_string);
	return;

    case UNARY:		/* unary operator */
	dump_str(op_string);
	dumpdtree(Lexpr);
	if (lastchar(op_string) == '(')
	    dump_str(Rparen);
	return;

    case POSTFIX:		/* unary operator */
	dumpdtree(Lexpr);
	dump_str(op_string);
	return;

    case BINARY:		/* binary operator */
	dump_str(op_string);
	dumpdtree(Lexpr);
	dump_str(Comma);
	dumpdtree(Rexpr);
	dump_str(Rparen);
	return;

    case INFIX:
	dumpdtree(Lexpr);
	dump_str(op_string);
	dumpdtree(Rexpr);
	return;

    case TRIARY:		/* 3-ary */
	dump_str(op_string);
	dumpdtree(Lexpr);
	dump_str(Comma);
	dumpdtree(Mexpr);
	dump_str(Comma);
	dumpdtree(Rexpr);
	dump_str(Rparen);
	return;

    case QUADARY:		/* 4-ary */
	dump_str(op_string);
	dumpdtree(Lexpr);
	dump_str(Comma);
	dumpdtree(Mexpr);
	dump_str(Comma);
	dumpdtree(Rexpr);
	dump_str(Comma);
	dumpdtree(Xexpr);
	dump_str(Rparen);
	return;

    default:
	fprintf(stderr, "%d", order);
	don_err(Impossible, "dumpdtree");
    }
}

/*------------------------------------------------+------------+
                                                  | print_tree |
                                                  +------------*/
#define print_comma() appendEden(", ", dd_script)
#define print_rparen() appendEden(")", dd_script)

void
print_tree(tree expr)
{
    int         order;
    char       *op_string;
    static      glevel = 0;

    if (expr == EMPTYTREE)
	return;

    switch (expr->op) {
    case OP_ID:
    case OP_SLASH:
    case OP_GLOBAL:
	appendEden(expr_to_eden_name(expr), dd_script);
	return;

    case OP_INUMBER:		/* integer number */
	sprintf(temp, "%d", Ivalue(expr));
	appendEden(temp, dd_script);
	return;

    case OP_RNUMBER:		/* real number */
	sprintf(temp, "%f", Rvalue(expr));
	appendEden(temp, dd_script);
	return;

    case OP_EDEN:
    case OP_CSTRING:		/* character string */
	appendEden(Cvalue(expr), dd_script);
	return;

    case OP_BOOL:		/* boolean value */
	sprintf(temp, Bvalue(expr) ? "TRUE" : "FALSE");
	appendEden(temp, dd_script);
	return;

    case OP_IMPOSE:		/* impose variable */
	appendEden(" impose ", dd_script);
	print_tree(Lexpr);
	return;

    case OP_IF:		/* if .. then .. else .. */
    case OP_MONITOR:
	appendEden("(", dd_script);
	print_tree(Mexpr);
	appendEden(") ? ", dd_script);
	print_tree(Lexpr);
	appendEden(" : ", dd_script);
	print_tree(Rexpr);
	return;

    case OP_GSPECLIST:
	if (Lexpr == 0) {
	    appendEden("[]", dd_script);
	    return;
	}
	if (glevel == 0) {
	    appendEden("[", dd_script);
	    glevel++;
	    print_tree(Lexpr);
	    appendEden(", ", dd_script);
	    print_tree(Rexpr);
	    --glevel;
	    appendEden("]", dd_script);
	} else {
	    print_tree(Lexpr);
	    appendEden(", ", dd_script);
	    print_tree(Rexpr);
	}
	return;

    case OP_GSPEC:
	if (glevel == 0) {
	    appendEden("[", dd_script);
	    glevel++;
	    appendEden("\"", dd_script);
	    appendEden(map_token_to_name((intptr_t) expr->right), dd_script);
	    appendEden("\", \"", dd_script);
	    if (Lexpr) {
		initbuf(2048);
		inPrintTree++;
		dumpdtree(Lexpr);
		inPrintTree--;
		appendEden(dumpbuf, dd_script);
	    }
	    appendEden("\"", dd_script);
	    --glevel;
	    appendEden("]", dd_script);
	} else {
	    appendEden("\"", dd_script);
	    appendEden(map_token_to_name((intptr_t) expr->right), dd_script);
	    appendEden("\", \"", dd_script);
	    if (Lexpr) {
		initbuf(2048);
		inPrintTree++;
		dumpdtree(Lexpr);
		inPrintTree--;
		appendEden(dumpbuf, dd_script);
	    }
	    appendEden("\"", dd_script);
	}
	return;

    case OP_IMGFUNC:
	if (stage == 1) {	/* replace the function with a variable */
	    appendTreeList(expr);
	    sprintf(tempname, "%s_temp%d", defname, TopTreeList);
	    appendEden(tempname, dd_script);
	    return;
	}
	/* stage 2 - actually translate the image function */
	stage = 1;		/* suppress expanding image
				   sub-expressions */
	appendEden((char *) Lexpr, dd_script);
	appendEden("(\"", dd_script);
	appendEden(tempname, dd_script);
	appendEden("\", ", dd_script);
	print_tree(Rexpr);
	appendEden(")", dd_script);
	return;

    case OP_FUNC:
	appendEden((char *) Lexpr, dd_script);
	appendEden("(", dd_script);
	print_tree(Rexpr);
	appendEden(")", dd_script);
	return;

    case OP_LABEL:
	appendEden(Ltype == IMAGE ? "image(" : "label(", dd_script);
	print_tree(Lexpr);
	print_comma();
	print_tree(Rexpr);
	print_rparen();
	return;
    }

    /* otherwise, node is operator */

    switch (order = match_operator(expr->op, op_string)) {
    case CONSTANT:
	appendEden(op_string, dd_script);
	return;

    case UNARY:		/* unary operator */
	appendEden(op_string, dd_script);
	print_tree(Lexpr);
	if (lastchar(op_string) == '(')
	    print_rparen();
	return;

    case BINARY:		/* binary operator */
	appendEden(op_string, dd_script);
	print_tree(Lexpr);
	print_comma();
	print_tree(Rexpr);
	print_rparen();
	return;

    case INFIX:
	print_tree(Lexpr);
	appendEden(op_string, dd_script);
	print_tree(Rexpr);
	return;

    case TRIARY:		/* 3-ary */
	appendEden(op_string, dd_script);
	print_tree(Lexpr);
	print_comma();
	print_tree(Mexpr);
	print_comma();
	print_tree(Rexpr);
	print_rparen();
	return;

    case QUADARY:		/* 4-ary */
	appendEden(op_string, dd_script);
	print_tree(Lexpr);
	print_comma();
	print_tree(Mexpr);
	print_comma();
	print_tree(Rexpr);
	print_comma();
	print_tree(Xexpr);
	print_rparen();
	return;

    default:
	fprintf(stderr, "%d", order);
	don_err(Impossible, "print_tree");
    }
}


/*-----------------------------------------------------+--------+
                                                       | Define |
                                                       +--------*/
void
Define(tree expr1, tree expr2)
{
    symbol     *sym;
    int         start, top, i;
#ifdef DISTRIB
    extern int handle_check1(char *);
    extern void propagateDonaldDef(char *, char *);
    extern int withinHandle;
#endif /* DISTRIB */

    if (expr1 != EMPTYTREE) {
	switch (expr1->op) {
	case OP_ID:
	case OP_SLASH:
	case OP_GLOBAL:
	  /* DISTRIB: need to check expr1 for handle
	     & expr2 for oracle, and then propagate to
	     others who are allowed to know. */
	    /* translate */
	    defname = strdup(expr_to_eden_name(expr1));

#ifdef DISTRIB
           if (!handle_check1(defname))
	     return; /* not be allowed to change --sun */
           if (!DonaldExprOracleCheck(expr2))
	     return; /* some variables are not allowed to be observed --sun */
#endif /* DISTRIB */

	    stage = 1;		/* to control the translation process
				   translate image function as a temporary
				   variable */
	    if (is_shape(expr1) && is_openshape(expr2)) {
		appendEden(defname, dd_script);
		appendEden(" is open2shape(", dd_script);
		print_tree(expr2);
		appendEden(");\n", dd_script);
	    } else if (expr1->type == expr2->type) {
		appendEden(defname, dd_script);
		appendEden(" is ", dd_script);
		print_tree(expr2);
		appendEden(";\n", dd_script);
	    } else
		don_err(IdExprUnmatch, 0);
	    top = 0;
	    do {
		start = top + 1;
		top = TopTreeList;
		for (i = TopTreeList; i >= start; --i) {
		    stage = 2;	/* to control the translation process;
				   truly transate the image functions;
				   stage may reset to 1 if the expr
				   contains image subexpressions */
		    sprintf(tempname, "%s_temp%d", defname, i);
		    appendEden(tempname, dd_script);
		    appendEden(" is ", dd_script);
		    print_tree(TreeList[i - 1]);
		    appendEden(";\n", dd_script);
		}
	    } while (top != TopTreeList);	/* if there are image
						   subexpr */
	    clearTreeList();

	    /* update symbol table entry */
	    sym = look_up_tree(expr1);
	    sym->defn.t = copyTree(expr2);
	    sym->master = topMasterStack();

#ifdef DISTRIB
	    initbuf(2048);    /* send DonaldDefinition to others  --sun */
	    dump_str("%donald\n");
	    sprintf(temp, "%s = ", sym->name);
	    dump_str(temp);
	    inPrintTree++;
	    dumpdtree(sym->defn.t);
	    inPrintTree--;
	    dump_str("\n%eden\n");
/*	    propagateAgency1(sym->name, dumpbuf); */
            if (withinHandle)
               propagateDonaldDef(eden_full_name(sym->name), dumpbuf);
/*            appendEden("propagate(\"", dd_script);
            appendEden(eden_full_name(sym->name), dd_script);
            appendEden("\", \"", dd_script);
            appendEden(dumpbuf, dd_script);
            appendEden("\");\n", dd_script); */
#endif /* DISTRIB */

	    free(defname);
	    break;

	case OP_COMMA:
	    Define(expr1->left, expr2->left);
	    Define(expr1->right, expr2->right);
	    break;

	default:
	    don_err(Impossible, "define");
	    break;
	}
    }
}


/*-------------------------------------------------+------------+
                                                   | DefineFunc |
                                                   +------------*/
void
DefineFunc(int FX, tree expr)
 /* FX = FI or XI */
{
    symbol     *sym;

    if (Etype != REAL && Etype != ANY)
	don_err(IdExprUnmatch, 0);
    sym = look_up(FX == FI ? "f<i>" : "x<i>");
    sym->defn.t = copyTree(expr);
    sym->master = topMasterStack();
    appendEden(eden_full_name(FX == FI ? "_fi_" : "_xi_"), dd_script);
    appendEden(" = \"", dd_script);
    initbuf(2048);
    inPrintTree++;
    dumpdtree(expr);
    inPrintTree--;
    appendEden(dumpbuf, dd_script);
    appendEden("\";\n", dd_script);
}

#ifdef DISTRIB
static int DonaldExprOracle;      /* for agency --sun  */

void
checkDonaldExprOracle(tree expr)
{
    extern int oracle_check1(char *);
    int         order;
    char       *op_string;
    static      glevel = 0;
    char       *exprName;

    if (expr == EMPTYTREE)
	return;

    switch (expr->op) {
    case OP_ID:
    case OP_SLASH:
    case OP_GLOBAL:
	    exprName = strdup(expr_to_eden_name(expr));
	    if (!oracle_check1(exprName)) DonaldExprOracle = 0;
	    return;

    case OP_INUMBER:		/* integer number */
    case OP_RNUMBER:		/* real number */
    case OP_EDEN:
    case OP_CSTRING:		/* character string */
    case OP_BOOL:		/* boolean value */
        return;

    case OP_IMPOSE:		/* impose variable */
	checkDonaldExprOracle(Lexpr);
	return;

    case OP_IF:		/* if .. then .. else .. */
    case OP_MONITOR:
	checkDonaldExprOracle(Mexpr);
	checkDonaldExprOracle(Lexpr);
	checkDonaldExprOracle(Rexpr);
	return;

    case OP_GSPECLIST:
	if (Lexpr == 0) {
	    return;
	}
	if (glevel == 0) {
	    glevel++;
	    checkDonaldExprOracle(Lexpr);
	    checkDonaldExprOracle(Rexpr);
	    --glevel;
	} else {
	    checkDonaldExprOracle(Lexpr);
	    checkDonaldExprOracle(Rexpr);
	}
	return;

    case OP_GSPEC:
	if (glevel == 0) {
	    glevel++;
	    if (Lexpr) {
               checkDonaldExprOracle(Lexpr);
	    }
	    --glevel;
	} else {
	    if (Lexpr) {
               checkDonaldExprOracle(Lexpr);
	    }
	}
	return;

    case OP_IMGFUNC:
	if (stage == 1) {	/* replace the function with a variable */
	    return;
	}
	/* stage 2 - actually translate the image function */
	stage = 1;		/* suppress expanding image
				   sub-expressions */
	checkDonaldExprOracle(Rexpr);
	return;

    case OP_FUNC:
	checkDonaldExprOracle(Rexpr);
	return;

    case OP_CART:
    case OP_POLAR:
    case OP_LINE:
    case OP_LABEL:
	checkDonaldExprOracle(Lexpr);
	checkDonaldExprOracle(Rexpr);
	return;
    case OP_ARC:
	checkDonaldExprOracle(Lexpr);
	checkDonaldExprOracle(Mexpr);
	checkDonaldExprOracle(Rexpr);
	return;

    }

    /* otherwise, node is operator */

    switch (order = match_operator(expr->op, op_string)) {
    case CONSTANT:
	return;

    case UNARY:		/* unary operator */
	checkDonaldExprOracle(Lexpr);
	return;

    case BINARY:		/* binary operator */
	checkDonaldExprOracle(Lexpr);
	checkDonaldExprOracle(Rexpr);
	return;

    case INFIX:
	checkDonaldExprOracle(Lexpr);
	checkDonaldExprOracle(Rexpr);
	return;

    case TRIARY:		/* 3-ary */
	checkDonaldExprOracle(Lexpr);
	checkDonaldExprOracle(Mexpr);
	checkDonaldExprOracle(Rexpr);
	return;

    case QUADARY:		/* 4-ary */
	checkDonaldExprOracle(Lexpr);
	checkDonaldExprOracle(Mexpr);
	checkDonaldExprOracle(Rexpr);
	checkDonaldExprOracle(Xexpr);
	return;

    default:
	fprintf(stderr, "%d", order);
	don_err(Impossible, "print_tree");
    }
}

static int
DonaldExprOracleCheck(tree expr)
{
   DonaldExprOracle = 1;
   checkDonaldExprOracle(expr);
   return DonaldExprOracle;
}
#endif /* DISTRIB */
