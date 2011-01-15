/*
 * $Id: tree.c,v 1.8 2001/07/27 17:03:59 cssbz Exp $
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

static char rcsid[] = "$Id: tree.c,v 1.8 2001/07/27 17:03:59 cssbz Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "../../../config.h"
#include "tree.h"
#include "symbol.h"
#include "parser.h"

#include "../Eden/emalloc.h"
#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

/* function prototypes */
tree *int_tree(double);
tree *str_tree(char *);
tree *win_tree(WinStruct);
tree *sym_tree(symbol *, int);
tree *def_tree(symbol *, tree *);
tree *tree2(tree *, int);
tree *tree3(tree *, int, tree *);
void freetree(tree *);

/*
 treealloc - allocate memory of a new tree node
 */
#define treealloc() (tree *)emalloc(sizeof(tree))

/*
 int_tree - build a tree denoting an integer
 */
tree * int_tree(double i) {
	tree *t;

	/* [Ash] According to the Scout documentation, Scout uses only
	 integers, and provides only integer arithmetic.  Previously
	 Scout stored integers as a double type and passed them to Eden
	 as floating point numbers.  I've changed this, but I'm leaving
	 this assertion here to check my assumption that only integers
	 are dealt with in Scout. */
	/* Commented out after discovering that Scout actually has a real
	 datatype, which is treated internally as "integer" (hurrumph) */
	/*
	 assert(i == (int)i);
	 */

	t = treealloc();
	t->op = NUMBER;
	t->l.i = i;
	return t;
}

/* inthonest_tree - build a tree denoting an integer, which is
 actually an integer, honestly, not a double like 'integer' [Ash] */
tree * inthonest_tree(int i) {
	tree *t;

	t = treealloc();
	t->op = NUMBER;
	t->l.i = (double)i;
	return t;
}

/*
 str_tree - build a tree denoting a string
 */
tree * str_tree(char *s) {
	tree *t;

	t = treealloc();
	t->op = STR;
	t->l.s = s;
	return t;
}

/*
 win_tree - build a tree denoting a string
 */
tree * win_tree(WinStruct w) {
	tree *t;

	t = treealloc();
	t->op = FORMWIN;
	t->l.w = w;
	return t;
}

/*
 sym_tree - build a tree denoting a variable
 */
tree * sym_tree(symbol * sym, int type) {
	tree *t;

	t = treealloc();
	t->op = type;
	t->l.v = sym;
	return t;
}

/*
 def_tree - build a parse tree of a definition
 */
tree * def_tree(symbol * v, tree * def) {
	tree *t;

	t = treealloc();
	t->op = '=';
	t->l.v = v;
	t->r.t = def;
	return t;
}

/*
 tree2 - build a tree with only the left subtree and
 the opcode fields defined
 */
tree * tree2(tree * t1, int opcode) {
	tree *t;

	t = treealloc();
	t->op = opcode;
	t->l.t = t1;
	t->r.t = 0;
	return t;
}

/*
 tree3 - build a tree with all 3 fields defined
 */
tree * tree3(tree * t1, int opcode, tree * t2) {
	tree *t;

	t = treealloc();
	t->op = opcode;
	t->l.t = t1;
	t->r.t = t2;
	return t;
}

/*
 freetree - free the memory occupied by the tree t
 */
void freetree(tree * t) {
	if (t == 0)
		return;
	switch (t->op) {
	case NUMBER:
	case ROW:
	case COLUMN:
	case UNKNOWN:
	case INTVAR:
	case STRVAR:
	case PTVAR:
	case BOXVAR:
	case FRAMEVAR:
	case WINVAR:
	case DISPVAR:
		free(t);
		break;
	case STR:
		free(t->l.s);
		free(t);
		break;
	case '=':
		freetree(t->r.t);
		free(t);
		break;
	case FORMWIN:
		freetree(t->l.w.frame);
		freetree(t->l.w.string);
		freetree(t->l.w.box);
		freetree(t->l.w.pict);
		freetree(t->l.w.xmin);
		freetree(t->l.w.ymin);
		freetree(t->l.w.xmax);
		freetree(t->l.w.ymax);
		freetree(t->l.w.font);
		freetree(t->l.w.bgcolor);
		freetree(t->l.w.fgcolor);
		freetree(t->l.w.bdcolor);
		freetree(t->l.w.bdtype);
		free(t);
		break;
	default:
		freetree(t->l.t);
		freetree(t->r.t);
		free(t);
		break;
	}
}
