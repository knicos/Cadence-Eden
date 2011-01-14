/*
 * $Id: Obs.q.h,v 1.3 2001/07/27 16:35:39 cssbz Exp $
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

#include "../Eden/global.q.h"

#ifndef STAMP_Obs_QUEUE
struct Obs {
        char  *name;
};
typedef struct Obs Obs;
struct Obs_queue {
    struct Obs_queue *prev;
    struct Obs_queue *next;
    Obs       obj;
};

typedef struct Obs_queue Obs_QUEUE;
typedef Obs_QUEUE *Obs_ATOM;

#define STAMP_Obs_QUEUE
#endif

