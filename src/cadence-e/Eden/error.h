/*
 * $Id: error.h,v 1.1 2001/07/27 17:08:43 cssbz Exp $
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

/* error.h */

extern void errorf(char *, ...);
#define error(s)  errorf("%s", s)
#define error2(s, t) errorf("%s%s", s, t)

// It seems these are not used anywhere [Richard]

//extern void notice(char *, char *);
//extern void msgContent(char *, char *, int);
