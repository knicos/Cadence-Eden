/*
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

// --------------------------------------------------------------------------
// utils.c - This contains various Sasami utility functions
// --------------------------------------------------------------------------

#include "../../../../../config.h"

#ifdef WANT_SASAMI

#include "debug.h"



/*
// --------------------------------------------------------------------------
// This copies the first word of a string into the buffer given
// --------------------------------------------------------------------------

void sa_firstWord(char *dest,char *src);
{
	int i = 0;
	while (src[i]!='\0') and (src[i]!=' ') and (src[i]!='\9')
	{
		dest[i]=src[i];
		i++;
	}
	dest[i]='\0'; // Add terminator
}

// --------------------------------------------------------------------------
// This copies the last (ie not first) word of a string into the buffer given
// --------------------------------------------------------------------------

void sa_lastWords(char *dest,char *src);
{
	int i = 0;
	bool found = false;

	while (src[i]!='\0') and (src[i]!=' ') and (src[i]!='\9')
	{
		if (found)
		{
			dest[i]=src[i];
		}
		if (src[i]==' ') or (src[i]=='\9')
		{
			found=true;
		}
		i++;
	}
	dest[i]='\0'; // Add terminator
}
*/

#endif /* WANT_SASAMI */
