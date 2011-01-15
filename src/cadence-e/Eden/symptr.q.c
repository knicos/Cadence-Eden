/*
 * $Id: symptr.q.c,v 1.8 2001/07/27 17:57:16 cssbz Exp $
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

static char rcsid3[] = "$Id: symptr.q.c,v 1.8 2001/07/27 17:57:16 cssbz Exp $";

#include "../../../config.h"
#include "symptr.q.h"

/* This function has highly concealed semantics.  Note particularly
   the &&: the expression with side-effect on the RHS will only be
   executed if the LHS is true.  The function appears to iterate
   through the given queue until the nth occurrance of obj, then
   returns a new queue pointer to that point.  [Ash] */
/* return the position of the n-th object in a symptr queue [sy or edward] */
symptr_QUEUE *
SEARCH_symptr(symptr_QUEUE * Q, symptr obj, int n)
{
    symptr_QUEUE *P;

    FOREACH(P, Q) {
	if (EQUAL_symptr(obj, P->obj) && !--n)
	    return P;
    }
    return (symptr_QUEUE *) 0;	/* NOT FOUND */
}

void
DELETE_symptr_ATOM(symptr_QUEUE * Q, symptr_ATOM A)
{
    if (A && A != Q) {
	A->prev->next = A->next;
	A->next->prev = A->prev;
	DESTROY_symptr_ATOM(A);
    }
}

void
MOVE_symptr_Q(symptr_QUEUE * Qsrc, symptr_QUEUE * Qdst)
{
    if (!Q_EMPTY(Qsrc)) {
	(Qdst->prev->next = Qsrc->next)->prev = Qdst->prev;
	(Qsrc->prev->next = Qdst)->prev = Qsrc->prev;
	CLEAN_Q(Qsrc);
    }
}
