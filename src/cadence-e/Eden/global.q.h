/*
 * $Id: global.q.h,v 1.8 2002/03/01 23:45:34 cssbz Exp $
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

#ifndef _GLOBAL_Q_H

#include "emalloc.h"

struct eden_queue {
    struct eden_queue *prev;
    struct eden_queue *next;
    void       *obj;		/* dummy */
};

typedef struct eden_queue QUEUE;

/* This is how this seems to be organised:
     - items are added to the head of the queue
     - the head of the queue can be found at *(Q->next)
     - the end of the queue is detected by (A->next) == Q
   [Ash] */

#define EMPTYQUEUE(Q) {&Q, &Q}
#define CLEAN_Q(Q) ((Q)->prev=(Q)->next=(Q))
#define Q_EMPTY(Q) ((Q)->next==(Q))
/* Remove atom A */
#define DELETE_ATOM(Q,A) \
	(((A)->prev->next=(A)->next)->prev=(A)->prev,(A)->prev=(A)->next=0)
/* Insert A at Q: where Q was, A is, and Q==A->next */
#define INSERT_ATOM(Q,A) \
	((((A)->prev=((A)->next=(Q))->prev)->next=(A)),(Q)->prev=(A))
/* Insert A just after Q: Q->next==A */
#define APPEND_ATOM(Q,A) \
	((((A)->next=((A)->prev=(Q))->next)->prev=(A)),(Q)->next=(A))
#define APPEND_Q(Q,A) INSERT_ATOM(Q,A)
#define INSERT_Q(Q,A) APPEND_ATOM(Q,A)
#define NEXT(Q,A) ((Q)==(A)->next?0:(A)->next)
#define PREV(Q,A) ((Q)==(A)->prev?0:(A)->prev)
#define FRONT(Q) NEXT(Q,Q)
#define LAST(Q) PREV(Q,Q)

#define ALLOC_ATOM(A) (A=emalloc(sizeof(*A)))

#define FOREACH(A,Q) for(A=FRONT(Q);A;A=NEXT(Q,A))

extern int  Q_LEN(QUEUE *);
extern QUEUE *WHERE_IS(QUEUE *, int);

#define _GLOBAL_Q_H
#endif
