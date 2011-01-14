/*
 * $Id: script.h,v 1.6 1999/11/16 21:20:40 ashley Rel1.10 $
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

typedef struct Script {
    int         maxScript;	/* maximum size of script */
    char       *text;		/* actual translated script */
    int         ready;		/* if a complete statement is translated */
}           Script;

extern Script *newScript(void);
extern void resetScript(Script *);
extern void deleteScript(Script *);
extern void appendEden(char *, Script *);
extern void appendnEden(char *, Script *, int);
