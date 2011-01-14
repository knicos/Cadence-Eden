/*
 * $Id: tree.h,v 1.5 2001/07/27 17:04:34 cssbz Exp $
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

/************************************************************************
 *									*
 *   int_tree(int i) - build a tree which denotes an integer		*
 *									*
 *   str_tree(char *s) - build a tree which denotes a string constant	*
 *									*
 *   def_tree(symbol *s, tree *def) - build a parse tree of definition	*
 *				      variable s is defined to be `def'	*
 *									*
 *   sym_tree(symbol* sym, int type) - build a tree which denotes a	*
 *				      variable of type `type'		*
 *									*
 *   tree2(tree *l, int opcode) - build a tree with only its left son	*
 *			(= `l') and opcode(= `opcode') are defined	*
 *									*
 *   tree3(tree *l, int opcode, tree*r) - build a tree with left son	*
 *			= `l', opcode = `opcode' and right son = `r'	*
 *									*
 *   freetree(tree *t) - free the tree `t'				*
 *									*
 ************************************************************************/

typedef struct {
	int type;
	struct tree *frame;
	struct tree *string;
	struct tree *box;
	struct tree *pict;
	struct tree *xmin;
	struct tree *ymin;
	struct tree *xmax;
	struct tree *ymax;
	struct tree *font;
	struct tree *bgcolor;
	struct tree *fgcolor;
	struct tree *bdcolor;
	struct tree *border;
	struct tree *bdtype;
	int align;
	//    int         sensitive;
	struct tree *sensitive;
} WinStruct;

typedef struct {
	int change;
	union {
		int type;
		struct tree *frame;
		struct tree *string;
		struct tree *box;
		struct tree *pict;
		struct tree *xmin;
		struct tree *ymin;
		struct tree *xmax;
		struct tree *ymax;
		struct tree *font;
		struct tree *bgcolor;
		struct tree *fgcolor;
		struct tree *bdcolor;
		struct tree *border;
		struct tree *bdtype;
		int align;
		//	int         sensitive;
		struct tree *sensitive;
	} f; /* window attributes */
} WinField;

struct tree {
	int op;
	union {
		char *s;
		char c;
		double i;
		struct symbol *v;
		struct tree *t;
		WinStruct w;
	} l, r;
};
typedef struct tree tree;

extern tree *int_tree();
extern tree *inthonest_tree();
extern tree *str_tree();
extern tree *win_tree();
extern tree *def_tree();
extern tree *sym_tree();
extern tree *tree2();
extern tree *tree3();
extern void freetree();
