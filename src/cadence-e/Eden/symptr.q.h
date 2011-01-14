/*
 * $Id: symptr.q.h,v 1.5 2001/07/27 17:57:42 cssbz Exp $
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

#include "global.q.h"

#ifndef _SYMPTR_Q_H

struct symptr_queue {
    struct symptr_queue *prev;
    struct symptr_queue *next;
    symptr      obj;
};

typedef struct symptr_queue symptr_QUEUE;
typedef symptr_QUEUE *symptr_ATOM;
extern symptr_QUEUE *SEARCH_symptr(symptr_QUEUE *, symptr, int);
extern void DELETE_symptr_ATOM(symptr_QUEUE *, symptr_ATOM);

#ifndef NEW_symptr
#define NEW_symptr(OBJ) OBJ
#endif

#ifndef FREE_symptr
#define FREE_symptr(OBJ)	/* do nothing */
#endif

#ifndef EQUAL_symptr
#define EQUAL_symptr(A,B) ((A)==(B))
#endif

#define DESTROY_symptr_ATOM(A) \
	{ \
	FREE_symptr((A)->obj); \
	free(A); \
	}

#define DELETE_NTH_symptr(Q,OBJ,n) DELETE_symptr_ATOM(Q,SEARCH_symptr(Q,OBJ,n))
#define DELETE_FIRST_symptr(Q,OBJ) DELETE_symptr_ATOM(Q,SEARCH_symptr(Q,OBJ,1))
#define DELETE_FRONT_symptr(Q) DELETE_symptr_ATOM(Q,FRONT(Q))
#define DELETE_LAST_symptr(Q) DELETE_symptr_ATOM(Q,LAST(Q))

#define CLEAR_symptr_Q(Q) \
	while (!Q_EMPTY(Q)) DELETE_symptr_ATOM(Q,(Q)->next)

#define APPEND_symptr_Q(Q,OBJ) \
	{ \
	symptr_ATOM A; \
	ALLOC_ATOM(A); \
	A->obj = NEW_symptr(OBJ); \
	APPEND_Q(Q,A); \
	}

#define CONCAT_symptr_Q(Qdst, Qsrc) \
	{ \
	symptr_QUEUE *P; \
	FOREACH (P, Qsrc) APPEND_symptr_Q(Qdst, P->obj); \
	}

/* This finds the first occurrance of OBJ in Q [Ash] */
#define IN_symptr_Q(Q,OBJ) SEARCH_symptr(Q,OBJ,1)

#define _SYMPTR_Q_H
#endif
