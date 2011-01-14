/*
 * $Id: script.c,v 1.15 2002/03/01 23:39:40 cssbz Exp $
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

static char rcsid[] = "$Id: script.c,v 1.15 2002/03/01 23:39:40 cssbz Exp $";

#include <stdlib.h>
#include <string.h>

#include "../../../../../config.h"
#include "script.h"

#include "../Eden/emalloc.h"
#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

/* function prototypes */
Script     *newScript(void);
void        resetScript(Script *);
void        deleteScript(Script *);
void        appendEden(char *, Script *);
void        appendnEden(char *, Script *, int);

#define TEXTLEN 1024

static Script	**ScriptBuffer;
static int	nScriptBuffer = 0;
static int	scriptLevel = 0;

/* A script appears to be the area in which Eden code is gradually
   built up from Eden code (with no translation) and Donald and Scout
   code (via translation).  This code implements a stack of
   scripts. [Ash] */

Script     *
newScript(void)
{
    Script     *script;

    if (nScriptBuffer > scriptLevel) {
	/* there are spare script buffers */
	script = ScriptBuffer[scriptLevel];
    } else {
	/* need a new script buffer */
	script = (Script *) emalloc(sizeof(Script));
	script->text = (char *) emalloc(TEXTLEN);
	script->maxScript = TEXTLEN;
	if (nScriptBuffer == 0) {
	    ScriptBuffer = (Script **) emalloc(sizeof(Script *));
	} else {
	    ScriptBuffer = (Script **) erealloc(ScriptBuffer,
		sizeof(Script *) * (nScriptBuffer + 1));
	}
	ScriptBuffer[nScriptBuffer++] = script;
    }
    script->text[0] = '\0';
    script->ready = 0;
    scriptLevel++;
    return script;
}

void
resetScript(Script * script)
{
    script->text[0] = '\0';
    script->ready = 0;
}

void
deleteScript(Script * script)
{
    scriptLevel--;
    script->ready = 0;
}

/* Append s to the end of script, expanding script if necessary [Ash] */
void
appendEden(char *s, Script * script)
{
    if (script->maxScript > (int) strlen(script->text) + (int) strlen(s)) {
	strcat(script->text, s);
    } else {
	script->text = (char *) erealloc(script->text,
					 script->maxScript * 2);
	script->maxScript *= 2;
	strcat(script->text, s);
    }
}

/* Append at most n characters of s to the end of script, expanding script
   if necessary [Ash] */
void
appendnEden(char *s, Script * script, int n)
{
  if (script->maxScript > (int) strlen(script->text) + n) {
    strncat(script->text, s, n);
  } else {
    while (script->maxScript < strlen(script->text) + n) {
      script->maxScript *= 2;
    }
    script->text = (char *) erealloc(script->text, script->maxScript);
    strncat(script->text, s, n);
  }
}
