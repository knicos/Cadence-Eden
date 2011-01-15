/*
 * $Id: utility.c,v 1.8 2001/07/27 16:34:39 cssbz Exp $
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

static char rcsid[] = "$Id: utility.c,v 1.8 2001/07/27 16:34:39 cssbz Exp $";

#include "../../../config.h"
#include "error.h"

#include "../Eden/emalloc.h"
#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

/*------------------------------------------------------------+--------+
                                                              | getmem |
                                                              +--------*/
void   *
getmem(int size)
{				/* get memory of size */
    void   *ptr;

    if (!(ptr = emalloc(size)))
	don_err(OutOfMemory, 0);
    return ptr;
}
