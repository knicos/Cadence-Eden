/*
 * $Id: heap.c,v 1.10 2001/07/27 17:32:20 cssbz Exp $
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

static char rcsid[] = "$Id: heap.c,v 1.10 2001/07/27 17:32:20 cssbz Exp $";

/*********************************
 *	HEAP MEMORY MANAGER      *
 *********************************/

#include "../../../config.h"

#include "eden.h"

#include <stdio.h>
#ifdef DEBUG
#define DEBUGPRINT(s,t)		{if(Debug&1)fprintf(stderr,s,t);}
#else
#define DEBUGPRINT(s,t)
#endif

char        heap[HEAPSIZE];
char       *hptr;

/* function prototypes */
char       *getheap(int);
void        freeheap(void);

#define heap_overflow(p)	((p)>=&heap[HEAPSIZE])

/* Note that freeheap must be called before getheap is ever called, or
   else hptr will be uninitialised. [Ash] */

char *
getheap(int size)
{				/* allocate memory from heap */
    char       *p;

    DEBUGPRINT("HEAPAL getheap(%d) ", size);
    DEBUGPRINT("start hptr=0x%x ", hptr);
    /* printf("ASH: getheap(%d). hptr currently=0x%x\n", size, hptr); */

    if (heap_overflow(hptr + size))
    	error("heap overflow");
    p = hptr;
    hptr = hptr + size;

    /* printf("ASH: hptr now=0x%x\n", hptr); */

    /* Ensure the pointer is correctly aligned, so that the next
       allocated block does not cause an unaligned access when the
       data is accessed [Ash] */
    if (((Int) hptr & (DOUBLE_ALIGNMENT-1)) != 0) {
      hptr += DOUBLE_ALIGNMENT - ((Int) hptr & (DOUBLE_ALIGNMENT-1));
      /* printf("ASH: Corrected hptr=0x%x\n", hptr); */
    }

    /*
     * sy's old version:
     *
     * #ifdef __GNUC__
     * GCC requires double values on 8-byte boundaries on a Sparc
     * hptr += ((((int) hptr & 7) == 0) ? 0 : 8 - ((int) hptr & 7));
     * #else
     * 4-byte boundaries alignment is enough for CC
     * hptr += ((((int) hptr & 3) == 0) ? 0 : 4 - ((int) hptr & 3));
     * #endif
     */

    DEBUGPRINT("end hptr=0x%x\n", hptr);
    return p;
}

void
freeheap(void)
{				/* free heap by resetting the heap ptr */
    DEBUGPRINT("VMOPER|HEAPAL freeheap ", 0);
    DEBUGPRINT("start hptr=0x%x ", hptr);

    if (fp == frame)
	hptr = heap;
    else
	hptr = fp->hptr;

    if (((Int) hptr & (DOUBLE_ALIGNMENT-1)) != 0) {
      hptr += DOUBLE_ALIGNMENT - ((Int) hptr & (DOUBLE_ALIGNMENT-1));
    }

    DEBUGPRINT("end hptr=0x%x\n", hptr);
}
