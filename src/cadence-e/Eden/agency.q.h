/*
 * $Id: agency.q.h,v 1.2 1999/11/16 21:20:40 ashley Rel1.10 $
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

#ifndef STAMP_agent_QUEUE
struct agent {
        char  *name;
};
typedef struct agent agent;
struct agent_queue {
    struct agent_queue *prev;
    struct agent_queue *next;
    agent       obj;
};

typedef struct agent_queue agent_QUEUE;
typedef agent_QUEUE *agent_ATOM;
#ifndef streq
#define	streq(X,Y)		(strcmp(X,Y)==0)
#endif
#define EQUAL_agent(A,B)	streq(A.name,B.name)
#define FREE_agent(A)		if(A.name)free(A.name)

extern agent_QUEUE *SEARCH_agent(agent_QUEUE *, agent, int);
extern void        DELETE_agent_ATOM(agent_QUEUE *, agent_ATOM);
extern void        MOVE_agent_Q(agent_QUEUE *, agent_QUEUE *);

#ifndef NEW_agent
#define NEW_agent(OBJ) OBJ
#endif

#define DESTROY_agent_ATOM(A) \
	{ \
	FREE_agent((A)->obj); \
	free(A); \
	}

#define DELETE_NTH_agent(Q,OBJ,n) DELETE_agent_ATOM(Q,SEARCH_agent(Q,OBJ,n))
#define DELETE_FIRST_agent(Q,OBJ) DELETE_agent_ATOM(Q,SEARCH_agent(Q,OBJ,1))
#define DELETE_FRONT_agent(Q) DELETE_agent_ATOM(Q,FRONT(Q))
#define DELETE_LAST_agent(Q) DELETE_agent_ATOM(Q,LAST(Q))

#define CLEAR_agent_Q(Q) \
	while (!Q_EMPTY(Q)) DELETE_agent_ATOM(Q,(Q)->next)

#define APPEND_agent_Q(Q,OBJ) \
	{ \
	agent_ATOM A; \
	ALLOC_ATOM(A); \
	A->obj = NEW_agent(OBJ); \
	APPEND_Q(Q,A); \
	}

#define INSERT_agent_Q(Q,OBJ) \
	{ \
	agent_ATOM A; \
	ALLOC_ATOM(A); \
	A->obj = NEW_agent(OBJ); \
	INSERT_Q(Q,A); \
	}

#define CONCAT_agent_Q(Qdst, Qsrc) \
	{ \
	agent_QUEUE *P; \
	FOREACH (P, Qsrc) APPEND_agent_Q(Qdst, P->obj); \
	}

#define IN_agent_Q(Q,OBJ) SEARCH_agent(Q,OBJ,1)

#define STAMP_agent_QUEUE
#endif
