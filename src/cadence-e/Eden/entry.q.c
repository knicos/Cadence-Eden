/*
 * $Id: entry.q.c,v 1.10 2001/07/27 17:28:43 cssbz Exp $
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

static char rcsid4[] = "$Id: entry.q.c,v 1.10 2001/07/27 17:28:43 cssbz Exp $";

#include "../../../../../config.h"
#include "entry.q.h"

/* function prototypes */
entry_QUEUE *SEARCH_entry(entry_QUEUE *, entry, int);
void        DELETE_entry_ATOM(entry_QUEUE *, entry_ATOM);
void        MOVE_entry_Q(entry_QUEUE *, entry_QUEUE *);

 /* return the position of the n-th object in a entry queue */
entry_QUEUE *
SEARCH_entry(entry_QUEUE * Q, entry obj, int n)
{
    entry_QUEUE *P;

    FOREACH(P, Q) {
	if (EQUAL_entry(obj, P->obj) && !--n)
	    return P;
    }
    return (entry_QUEUE *) 0;	/* NOT FOUND */
}

void
DELETE_entry_ATOM(entry_QUEUE * Q, entry_ATOM A)
{
    if (A && A != Q) {
	A->prev->next = A->next;
	A->next->prev = A->prev;
	DESTROY_entry_ATOM(A);
    }
}

void
MOVE_entry_Q(entry_QUEUE * Qsrc, entry_QUEUE * Qdst)
{
    if (!Q_EMPTY(Qsrc)) {
	(Qdst->prev->next = Qsrc->next)->prev = Qdst->prev;
	(Qsrc->prev->next = Qdst)->prev = Qsrc->prev;
	CLEAN_Q(Qsrc);
    }
}
