/*
 * $Id: tree.h,v 1.4 1999/11/16 21:20:40 ashley Rel1.10 $
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

struct node4 {
        int     type;
        int     op;
        struct node4 *left;
        struct node4 *right;
        struct node4 *mid;
        struct node4 *extra;
};

struct node3 {
        int     type;
        int     op;
        struct node4 *left;
        struct node4 *right;
        struct node4 *mid;
};

struct node2 {
        int     type;
        int     op;
        struct node4 *left;
        struct node4 *right;
};

struct node1 {
        int     type;
        int     op;
        struct node4 *left;
};

struct node0 {
        int     type;
        int     op;
        union {
                int     i;
                double  r;
                char    *s;
        } d;
};

typedef struct node4 node4;
typedef struct node3 node3;
typedef struct node2 node2;
typedef struct node1 node1;
typedef struct node0 node0;
typedef node4 *tree;

extern tree dtree0(), dtree1(), dtree2(), dtree3(), dtree4();

#define EMPTYTREE       (tree)0
#define Ivalue(expr)    ((node0*)expr)->d.i
#define Rvalue(expr)    ((node0*)expr)->d.r
#define Cvalue(expr)    ((node0*)expr)->d.s
#define Bvalue(expr)    ((node0*)expr)->d.i
#define Lexpr           expr->left
#define Mexpr           expr->mid
#define Rexpr           expr->right
#define Xexpr           expr->extra
#define Ltype           Lexpr->type
#define Mtype           Mexpr->type
#define Rtype           Rexpr->type
#define Xtype           Xexpr->type
#define Etype           expr->type
#define check_left()    count_expr(Lexpr)
#define check_right()   count_expr(Rexpr)
#define check_mid()     count_expr(Mexpr)
#define check_extra()   count_expr(Xexpr)
