/*
 * $Id: entry.q.h,v 1.6 2001/07/27 17:29:07 cssbz Exp $
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

#ifndef _ENTRY_Q_H

struct entry_queue {
    struct entry_queue *prev;
    struct entry_queue *next;
    entry       obj;
};

typedef struct entry_queue entry_QUEUE;
typedef entry_QUEUE *entry_ATOM;
extern entry_QUEUE *SEARCH_entry(entry_QUEUE *, entry, int);
extern void DELETE_entry_ATOM(entry_QUEUE *, entry_ATOM);
extern void MOVE_entry_Q(entry_QUEUE *, entry_QUEUE *);

#ifndef NEW_entry
#define NEW_entry(OBJ) OBJ
#endif

#ifndef FREE_entry
#define FREE_entry(OBJ)		/* do nothing */
#endif

#ifndef EQUAL_entry
#define EQUAL_entry(A,B) ((A)==(B))
#endif

#define DESTROY_entry_ATOM(A) \
	{ \
	FREE_entry((A)->obj); \
	free(A); \
	}

#define DELETE_NTH_entry(Q,OBJ,n) DELETE_entry_ATOM(Q,SEARCH_entry(Q,OBJ,n))
#define DELETE_FIRST_entry(Q,OBJ) DELETE_entry_ATOM(Q,SEARCH_entry(Q,OBJ,1))
#define DELETE_FRONT_entry(Q) DELETE_entry_ATOM(Q,FRONT(Q))
#define DELETE_LAST_entry(Q) DELETE_entry_ATOM(Q,LAST(Q))

#define CLEAR_entry_Q(Q) \
	while (!Q_EMPTY(Q)) DELETE_entry_ATOM(Q,(Q)->next)

#define APPEND_entry_Q(Q,OBJ) \
	{ \
	entry_ATOM A; \
	ALLOC_ATOM(A); \
	A->obj = NEW_entry(OBJ); \
	APPEND_Q(Q,A); \
	}

#define INSERT_entry_Q(Q,OBJ) \
	{ \
	entry_ATOM A; \
	ALLOC_ATOM(A); \
	A->obj = NEW_entry(OBJ); \
	INSERT_Q(Q,A); \
	}

#define CONCAT_entry_Q(Qdst, Qsrc) \
	{ \
	entry_QUEUE *P; \
	FOREACH (P, Qsrc) APPEND_entry_Q(Qdst, P->obj); \
	}

#define IN_entry_Q(Q,OBJ) SEARCH_entry(Q,OBJ,1)

#define _ENTRY_Q_H
#endif
