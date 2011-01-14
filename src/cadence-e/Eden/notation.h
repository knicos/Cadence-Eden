/*
 * $Id: notation.h,v 1.8 2001/08/03 15:49:03 cssbz Exp $
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

/* OTHER changed to NEWOTHER during rework on translator framework */
typedef enum {
  INTERNAL, EDEN, SCOUT, DONALD, ARCA, NEWOTHER
#ifdef WANT_SASAMI
, SASAMI
#endif /* WANT_SASAMI */
#ifdef DISTRIB
,LSD
#endif
} notationType;

extern notationType currentNotation;

void        changeNotation(notationType);
void        setprompt(void);
void        evaluate(char *, char *);

void pushEntryStack(notationType);
void popEntryStack(void);
int topEntryStack(void);
