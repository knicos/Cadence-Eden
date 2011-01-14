/*
 * $Id: lex.c,v 1.7 2001/10/15 16:10:29 cssbz Exp $
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

static char rcsid[] = "$Id: lex.c,v 1.7 2001/10/15 16:10:29 cssbz Exp $";

#include "../config.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../Eden/eden.h"
#include "Obs.q.h"
#include "LSDagent.q.h"
#include "parser.h"

#include "../Eden/emalloc.h"
#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

extern void lsd_parse(int);

/* function prototypes */
static int  search_token(char *);
void        yyrestart(void);
static void save(int);
static void action_A(void);
/* char   yytext[256]; */
void        lsd_lex(int);

enum {
    _S_, _A_, _COMMENT_
};

static char *yytext = 0;
static int  bufsize = 256;
/* static char yytext = (char *) erealloc(yytext, bufsize * sizeof(int)); */
static int  yyleng = 0;
static int  state = _S_;	/* start state */
static int  notAgent = 1;


#define clearbuf() (yytext[yyleng=0] = '\0')

#define LSDTOKENS 6
static struct {
	char   *name;
	int	token;
} tokens[LSDTOKENS] = {
        { "agent"       ,       AGENT   },
	{ "handle"	,	HANDLE	},
	{ "oracle"	,	ORACLE	},
	{ "state"	,	STATE	},
	{ "remove"      ,       REMOVE  },
	{ 0, 0 }
};

static int
search_token(char *name)
{
    int         i, start, end, cmp;
    start = 0;
    end = LSDTOKENS - 1;
    for (i = (LSDTOKENS - 1) >> 1;
	 start <= i && i <= end;
	 i = (start + end) >> 1) {

	if (tokens[i].name == 0)
	    return -1;
	cmp = strcmp(name, tokens[i].name);
	if (cmp == 0)
	    return i;
	else if (cmp > 0)
	    start = i + 1;
	else			/* cmp < 0 */
	    end = i - 1;
    }
    return -1;
}


void
yyrestart(void)
{

    if (yytext) clearbuf();
    state = _S_;
}

static void
save(int c)
{
    if (yyleng == bufsize - 1) {
	bufsize *= 2;
	yytext = (char *) erealloc(yytext, bufsize * sizeof(int));
    }
    yytext[yyleng++] = c;
    yytext[yyleng] = '\0';
}

static void
action_A(void)
{
    int         pos;

     /*  check if yytext is a token? If yes, return token,
           otherwise return yytext. --sun */

    if ((pos = search_token(yytext)) != -1) {
	lsd_parse(tokens[pos].token);
    } else {    /* it is an observable or agentname*/
      lsd_lval.s = strdup(yytext);
      lsd_parse(ID);
    }
}

void
lsd_lex(int c)
{
    /* printf("=%c", c);*/
    if (yytext == 0)
	yytext = (char *) emalloc(bufsize * sizeof(int));
    switch (state) {
      case _S_:
	if (isalpha(c) || c == '_') {
	    save(c);
	    state = _A_;
	} else if (c == ' ' || c == '\t') {
	    state = _S_;
	} else if (c == '\n') {
	    state = _S_;
	} else if (c == '\r') {
	    state = _S_;
	} else if (c == '#') {
	    state = _COMMENT_;
	} else {
	    lsd_parse(c);
	    state = _S_;
	}
	break;
      case _A_:
	if (isalnum(c) || c == '_') {
	    save(c);
	    state = _A_;
	} else {
	    action_A();
	    clearbuf();
	    state = _S_;
	    lsd_lex(c);
	}
	break;
      case _COMMENT_:
	if (c == '\n' || c == '\r') {
	  state = _S_;
	}
	break;
    }
}

