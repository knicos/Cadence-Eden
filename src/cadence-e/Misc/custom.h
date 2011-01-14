/*
 * $Id: custom.h,v 1.5 1999/11/16 21:20:40 ashley Rel1.10 $
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

struct ILIBTBL {		/* C LIB FUNCTIONS */
    char       *name;
    int         (*func) ();
};

struct RLIBTBL {		/* FLOATING POINT C LIB FUNCTIONS */
    char       *name;
    double      (*func) ();
};

extern struct ILIBTBL ilibtbl[];
extern struct RLIBTBL rlibtbl[];

void install_custom_variables(void);
