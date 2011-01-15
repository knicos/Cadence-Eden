/*
 * $Id: runset.c,v 1.9 2001/12/06 22:48:41 cssbz Exp $
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

static char rcsid[] = "$Id: runset.c,v 1.9 2001/12/06 22:48:41 cssbz Exp $";

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../../../config.h"
#include "runset.h"
#include "eden.h"

#include "emalloc.h"
#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

/* A RunSet is a FIFO queue of Actions, each of which contains a 's'
   string and a 'master' string.  [comment by Ash] */

void
initRunSet(RunSet * rs)
{
    rs->nitems = 0;
    /* The Action at rs->head does not contain a valid s and master: it
       is there just to hold a value for next.  [comment by Ash] */
    rs->head = rs->tail = (Action *) emalloc(sizeof(Action));
    rs->tail->s = 0;
    rs->tail->master = 0;
#ifndef TTYEDEN
    rs->tail->tok = NULL;
#endif
    rs->tail->next = 0;
}

void
clearRunSet(RunSet * rs)
{
    while (rs->nitems)
	rmAction(rs);
}

/* Remove the Action from the head of the RunSet and return its values
   for a and master.  (Actually: remove the Action one removed from the
   the head, since the Action at rs->head does not contain valid s and
   master values.)  [comment by Ash]
*/
Action
getAction(RunSet * rs)
{
    Action     *ptr = rs->head->next;
    Action      a;

    a.s = ptr->s;
    a.master = ptr->master;
#ifndef TTYEDEN
    a.tok = ptr->tok;
#endif
    rs->head->next = rs->head->next->next;
    free(ptr);
    rs->nitems--;
    if (rs->nitems == 0)
	rs->tail = rs->head;
    return a;
}

/* Remove the Action from the head of the RunSet.  Free the storage for the
   's' value and the storage for the Action, but not the 'master' value.
   (See also caveat above: actually the Action one removed from the head
   is removed.)  [comment by Ash] */
void
rmAction(RunSet * rs)
{
    Action     *ptr = rs->head->next;

    free(ptr->s);
    rs->head->next = rs->head->next->next;
    free(ptr);
    rs->nitems--;
    if (rs->nitems == 0)
	rs->tail = rs->head;
}

/* Add an Action to the tail of the RunSet [comment by Ash] */
#ifndef TTYEDEN
void addAction(RunSet * rs, char *s, char *master, Tcl_TimerToken tok)
#else
void addAction(RunSet * rs, char *s, char *master)
#endif
{
    rs->tail->next = (Action *) malloc(sizeof(Action));
    rs->tail = rs->tail->next;
    rs->tail->s = strdup(s);
    rs->tail->master = master;
#ifndef TTYEDEN
    rs->tail->tok = tok;
#endif
    rs->tail->next = 0;
    rs->nitems++;
}


#ifndef TTYEDEN

void deleteActionWithToken(Tcl_TimerToken tok, RunSet *rs) {
  Action *current = rs->head->next;
  Action *prev = rs->head;
  int found = 0;
  
  while (current != 0) {
    if (current->tok == tok) {
      assert(found == 0); /* shouldn't ever be two Actions with the same tok */
      found = 1;

#ifdef DEBUG
      if (Debug & 32768)
        debugMessage("deleteActionWithToken: removing master <%s> "
          "timertoken <%d> action <%s>\n", current->master, current->tok,
          current->s);
#endif
      
      free(current->s);
      prev->next = current->next;
      free(current);
      rs->nitems--;
      if (rs->nitems == 0)
	rs->tail = rs->head;

      current = prev;
    }

    prev = current;
    current = current->next;
  }

#ifdef DEBUG
  if (Debug & 32768) {
    if (found == 0) {
      debugMessage("deleteActionWithToken: failed to find Action for"
              "token <%d> in RunSet <%d>\n", tok, rs);
    }
  }
#endif
}

void deleteAllTokenActions(RunSet *rs) {
  Action *current = rs->head->next;
  Action *prev = rs->head;
  int found = 0;
  
  while (current != 0) {
    if (current->tok != NULL) {
      found = 1;

#ifdef DEBUG
      if (Debug & 32768)
        debugMessage("deleteAllTokenActions: removing master <%s> "
          "timertoken <%d> action <%s>\n", current->master, current->tok,
          current->s);
#endif

      free(current->s);
      prev->next = current->next;
      free(current);
      rs->nitems--;
      if (rs->nitems == 0)
        rs->tail = rs->head;
        
      current = prev;
    }
    
    prev = current;
    current = current->next;  
  }

#ifdef DEBUG
  if (Debug & 32768) {
    if (found == 0) {
      debugMessage("deleteAllTokenActions: failed to find any Actions with"
        " timer tokens to remove from RunSet <%d>\n", rs);
    }
  }
#endif /* DEBUG */
}

#endif /* not TTYEDEN */


#if defined(DEBUG)
/* for debugging purpose */

#include <stdio.h>

void
printRunSet(RunSet * rs)
{
    Action     *ptr;
    int n = 0;

    if (rs->nitems == 0) {
      fprintf(stderr, "** printRunSet: no items in RS\n");
    } else {
      for (ptr = rs->head->next; ptr; ptr = ptr->next) {
	fprintf(stderr, "** printRunSet: %d: %s\n", ++n, ptr->s);
      }
    }
}

#endif /* DEBUG */
