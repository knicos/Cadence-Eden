/*
 * $Id: agency.q.c,v 1.6 2001/07/27 17:09:09 cssbz Exp $
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

static char rcsid5[] = "$Id: agency.q.c,v 1.6 2001/07/27 17:09:09 cssbz Exp $";

#include "../../../../../config.h"

#define EQUAL_agent(A,B)	streq(A.name,B.name)
#define FREE_agent(A)		if(A.name)free(A.name)

/* function prototypes */
agent_QUEUE *SEARCH_agent(agent_QUEUE *, agent, int);
void        DELETE_agent_ATOM(agent_QUEUE *, agent_ATOM);
void        MOVE_agent_Q(agent_QUEUE *, agent_QUEUE *);

 /* return the position of the n-th object in a agent queue */
agent_QUEUE *
SEARCH_agent(agent_QUEUE * Q, agent obj, int n)
{
    agent_QUEUE *P;

    FOREACH(P, Q) {
	if (EQUAL_agent(obj, P->obj) && !--n)
	    return P;
    }
    return (agent_QUEUE *) 0;	/* NOT FOUND */
}

void
DELETE_agent_ATOM(agent_QUEUE * Q, agent_ATOM A)
{
    if (A && A != Q) {
	A->prev->next = A->next;
	A->next->prev = A->prev;
	DESTROY_agent_ATOM(A);
    }
}

void
MOVE_agent_Q(agent_QUEUE * Qsrc, agent_QUEUE * Qdst)
{
    if (!Q_EMPTY(Qsrc)) {
	(Qdst->prev->next = Qsrc->next)->prev = Qdst->prev;
	(Qsrc->prev->next = Qdst)->prev = Qsrc->prev;
	CLEAN_Q(Qsrc);
    }
}
