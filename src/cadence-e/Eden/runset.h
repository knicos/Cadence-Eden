/*
 * $Id: runset.h,v 1.4 1999/11/16 21:20:40 ashley Rel1.10 $
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

#ifndef TTYEDEN
#include <tcl.h> /* for Tcl_TimerToken */
#endif

typedef struct Action {
    char       *s;
    char       *master;
#ifndef TTYEDEN
    Tcl_TimerToken tok;
#endif
    struct Action *next;
}           Action;

typedef struct RunSet {
    int         nitems;
    struct Action *head;
    struct Action *tail;
}           RunSet;

extern void initRunSet(RunSet *);
extern void clearRunSet(RunSet *);
extern Action getAction(RunSet *);
extern void rmAction(RunSet *);
#ifndef TTYEDEN
/* {d}tkeden */
extern void addAction(RunSet *, char *, char *, Tcl_TimerToken);
extern void queue(char *, char *, Tcl_TimerToken);
extern void deleteActionWithToken(Tcl_TimerToken, RunSet *);
extern void deleteAllTokenActions(RunSet *);
#else
/* ttyeden */
extern void addAction(RunSet *, char *, char *);
extern void queue(char *, char *);
#endif /* TTYEDEN */

#ifdef DEBUG
extern void printRunSet(RunSet *);
#endif /* DEBUG */
