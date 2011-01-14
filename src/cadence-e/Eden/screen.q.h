/*
 * $Id: screen.q.h,v 1.3 2001/07/27 17:54:29 cssbz Exp $
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

typedef struct intQ {
    struct intQ *prev;
    struct intQ *next;
    int         obj;
}           intQ;

typedef struct {
    int         ref;
    int         nbox;
}           winfo;


typedef struct ScoutScreen {
    char *name;
    Datum oScreen;
    winfo *oWinInfo;
    int   MaxRef;
    intQ  *Refer;
}   ScoutScreen;

typedef struct ScoutScreenQ {
    struct ScoutScreenQ *prev;
    struct ScoutScreenQ *next;
    ScoutScreen      obj;
}  ScoutScreenQ;

typedef ScoutScreenQ *ScoutScreen_ATOM;
#ifndef NEW_ScoutScreen
#define NEW_ScoutScreen(OBJ) OBJ
#endif


#define INSERT_ScoutScreen_Q(Q,OBJ) \
	{ \
	ScoutScreen_ATOM A; \
	ALLOC_ATOM(A); \
	A->obj = NEW_ScoutScreen(OBJ); \
	INSERT_Q(Q,A); \
	}
