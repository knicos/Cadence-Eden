/*
 * $Id: emalloc.h,v 1.3 2001/07/27 17:28:17 cssbz Exp $
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

/* Error checking malloc functions - rewritten as macros so that we
   can tell where the memory leaks are using dmalloc [Ash] */

#include <stdlib.h>

#include "error.h"

#include "../../../../../config.h"

#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

#if defined(DEBUG) && !defined(HAVE_DMALLOC)
#include <stdio.h>
void *emalloc_p;
extern int Debug;
/* This will make a mess if used with expressions with side-effect.  The info
 provided is also too simplistic to figure out where a memory leak might
 be occurring: for that, use dmalloc... [Ash] */
#define emalloc(n) ((emalloc_p = malloc(n)) ? \
                    (Debug&64 ? \
		     fprintf(stderr, \
                             "MALLOC emalloc(%d) 0x%x\n", n, emalloc_p) \
                     : 0),\
                    emalloc_p : \
		    (error("out of memory"), (void *)NULL) \
		   )
#define free(x)  ((Debug&64 ? fprintf(stderr, "MALLOC free 0x%x\n", x) \
		                : 0), free(x))

#else /* not debugging */

void *emalloc_p;
#define emalloc(n) ((emalloc_p = malloc(n)) ? \
		emalloc_p \
		: \
		(error("out of memory"), (void *)NULL) \
		)
#endif

void *erealloc_p;
#define erealloc(ptr, size) ((erealloc_p = realloc(ptr, size)) ? \
		erealloc_p \
		: \
		(error("system error: erealloc"), (void *)NULL) \
		)
