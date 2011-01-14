/*
 * $Id: screen.q.c,v 1.6 2001/07/27 17:54:03 cssbz Exp $
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

static char rcsid2[] = "$Id: screen.q.c,v 1.6 2001/07/27 17:54:03 cssbz Exp $";

void
init_ScoutScreenList(void)
{
    CLEAN_Q(&ScoutScreenList);
}

ScoutScreen_ATOM
search_ScoutScreenQ(char *name)
{
    ScoutScreen_ATOM  A;

    FOREACH(A, &ScoutScreenList) {
	if (A->obj.name == (char *) 0)
	    break;
	if (streq(A->obj.name, name))	/* match */
	    return A;
    }
    return (ScoutScreen_ATOM) 0;
}

ScoutScreen_ATOM
add_ScoutScreen(char *name)
{
    ScoutScreen       E;
    ScoutScreen_ATOM  F;
    makearr(0);    /* used to make a null datum  --sun */
    F = FRONT(&ScoutScreenList);
    E.name = emalloc(strlen(name) + 1);
    strcpy(E.name, name);
    E.oScreen = newdatum(pop());
    E.oWinInfo = 0;
    E.MaxRef = 1;
    E.Refer = (intQ *) emalloc(sizeof(intQ));
    CLEAN_Q(E.Refer);
    INSERT_ScoutScreen_Q(&ScoutScreenList, E);
    return FRONT(&ScoutScreenList);
}


