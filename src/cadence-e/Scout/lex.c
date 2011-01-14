/*
 * $Id: lex.c,v 1.17 2002/02/27 16:55:56 cssbz Exp $
 *
 * This file is part of Eden.
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

static char rcsid[] = "$Id: lex.c,v 1.17 2002/02/27 16:55:56 cssbz Exp $";

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../../../config.h"
#include "symbol.h"
#include "tree.h"
#include "parser.h"
#include "../EX/script.h"

#include "../Eden/emalloc.h"
#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

extern void yyparse(int);
extern char agentName[128];  /* for distributed Tkeden --sun */
extern int appAgentName;
extern int append_NoAgentName; /* [Patrick] we are within a window
                                  definition, so don't prepend the
                                  agent name.  [Ash] now thinks that
                                  this is 0 when a reference to
                                  variable explicitly in the root
                                  context is made (eg ~ON), which then
                                  stops the prepending of the agent
                                  name. */
extern char *libLocation;

/* function prototypes */
static int  search_constant(char *);
static int  search_token(char *);
static int  search_field(char *);
void        yyrestart(void);
static void save(int);
static void action_A(void);
void        yylex(int);
void        init_scout(void);
static int  ScoutAppAgentName; /* for agency -- sun */

Script     *st_script;
char       *st_prompt;
#define NCONSTANTS 16
static struct {
        char   *name;
	int	val;
        int     token;
} constants[NCONSTANTS] = {
        /* This table must be in alphabetical order.  Note you need to
           adjust the pre-processor size if you add or delete items
           and possibly change stuff in print.c and store.c... [Ash] */
	{ "ARCA"	,	2,	CONTENT		},
	{ "CENTRE"	,	3,	JUST		},
	{ "DONALD"	,	1,	CONTENT		},
	{ "ENTER" 	,  	4, 	INTEGERHONEST},
	{ "EXPAND"	,	4,	JUST		},
	{ "IMAGE"	,	3,	CONTENT		},
	{ "LEAVE"  	,	8,  INTEGERHONEST},
	{ "LEFT"	,	1,	JUST		},
	{ "MOTION"  ,   2,  INTEGERHONEST},
	{ "NOADJ"	,	0,	JUST		},
	{ "OFF"		,	0,	INTEGERHONEST},
	{ "ON"		,	1,	INTEGERHONEST},
	{ "RIGHT"	,	2,	JUST		},
	{ "TEXT"	,	0,	CONTENT		},
	{ "TEXTBOX"	,	4,	CONTENT		},
	{ 0, 0 }
};

#define NTOKENS 47
static struct {
	char   *name;
	int	token;
} tokens[NTOKENS] = {
	{ "ImageFile"	,	ImageFile	},
	{ "ImageScale"	,	ImageScale	},
	{ "alignment"	,	ALIGN		},
	{ "all"			,	ALL			},
	{ "append"		,	APPEND		},
	{ "bdcolor"		,	BDCOLOR		},
	{ "bdcolour"	,	BDCOLOR		},
	{ "bgcolor"		,	BG			},
	{ "bgcolour"	,	BG			},
	{ "border"		,	BORDER		},
	{ "bordercolor"	,	BDCOLOR		},
	{ "bordercolour",	BDCOLOR		},
	{ "box"			,	BOX			},
	{ "centre"		,	BOXCENTRE	},
	{ "delete"		,	DELETE		},
	{ "display"		,	DISPLAY		},
	{ "else"		,	ELSE		},
	{ "enclose"		,	BOXENCLOSING},
	{ "endif"		,	ENDIF		},
	{ "fgcolor"		,	FG			},
	{ "fgcolour"	,	FG			},
	{ "font"		,	FONT		},
	{ "frame"		,	FRAME		},
	{ "if"			,	IF			},
	{ "image"		,	IMAGE		},
	{ "integer"		,	INTEGER		},
	{ "intersect"	,	BOXINTERSECT},
	{ "itos"		,	TOSTRING	},
	{ "pict"		,	PICT		},
	{ "point"		,	POINT		},
	{ "real"		,	INTEGER		},
	{ "reduce"		,	BOXREDUCE	},
	{ "relief"		,	BDTYPE		},
	{ "sensitive"	,	SENSITIVE	},
	{ "shift"		,	BOXSHIFT	},
	{ "strcat"		,	STRCAT		},
	{ "string"		,	STRING		},
	{ "strlen"		,	STRLEN		},
	{ "substr"		,	SUBSTR		},
	{ "then"		,	THEN		},
	{ "type"		,	TYPE		},
	{ "window"		,	WINDOW		},
	{ "xmax"		,	XMAX		},
	{ "xmin"		,	XMIN		},
	{ "ymax"		,	YMAX		},
	{ "ymin"		,	YMIN		},
	{ 0				, 	0 			}
};

#define NFIELDS 33
static struct {
	char   *name;
	int	token;
} fields[NFIELDS] = {
	{ "alignment"	,	DOTALIGN	},
	{ "bdcolor"		,	DOTBDCOLOR	},
	{ "bdcolour"	,	DOTBDCOLOR	},
	{ "bgcolor"		,	DOTBG		},
	{ "bgcolour"	,	DOTBG		},
	{ "border"		,	DOTBORDER	},
	{ "bordercolor"	,	DOTBDCOLOR	},
	{ "bordercolour",	DOTBDCOLOR	},
	{ "box"			,	DOTBOX		},
	{ "c"			,	DOTC		},
	{ "e"			,	DOTE		},
	{ "fgcolor"		,	DOTFG		},
	{ "fgcolour"	,	DOTFG		},
	{ "font"		,	DOTFONT		},
	{ "frame"		,	DOTFRAME	},
	{ "n"			,	DOTN		},
	{ "ne"			,	DOTNE		},
	{ "nw"			,	DOTNW		},
	{ "pict"		,	DOTPICT		},
	{ "r"			,	DOTR		},
	{ "relief"		,	DOTBDTYPE	},
	{ "s"			,	DOTS		},
	{ "se"			,	DOTSE		},
	{ "sensitive"	,	DOTSENSITIVE},
	{ "string"		,	DOTSTR		},
	{ "sw"			,	DOTSW		},
	{ "type"		,	DOTTYPE		},
	{ "w"			,	DOTW		},
	{ "xmax"		,	DOTXMAX		},
	{ "xmin"		,	DOTXMIN		},
	{ "ymax"		,	DOTYMAX		},
	{ "ymin"		,	DOTYMIN		},
	{ 0				, 	0 			}
};

static int search_constant(char *name)
{
    int         i, start, end, cmp;

    start = 0;
    end = NCONSTANTS - 1;
    for (i = (NCONSTANTS - 1) >> 1;
	 start <= i && i <= end;
	 i = (start + end) >> 1) {

	if (constants[i].name == 0)
	    return -1;
	cmp = strcmp(name, constants[i].name);
	if (cmp == 0)
	    return i;
	else if (cmp > 0)
	    start = i + 1;
	else			/* cmp < 0 */
	    end = i - 1;
    }
    return -1;
}

static int search_token(char *name)
{
    int         i, start, end, cmp;

    start = 0;
    end = NTOKENS - 1;
    for (i = (NTOKENS - 1) >> 1;
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

static int search_field(char *name)
{
    int         i, start, end, cmp;

    start = 0;
    end = NFIELDS - 1;
    for (i = (NFIELDS - 1) >> 1;
	 start <= i && i <= end;
	 i = (start + end) >> 1) {

	if (fields[i].name == 0)
	    return -1;
	cmp = strcmp(name, fields[i].name);
	if (cmp == 0)
	    return i;
	else if (cmp > 0)
	    start = i + 1;
	else			/* cmp < 0 */
	    end = i - 1;
    }
    return -1;
}

enum {
    _S_, _A_, _D_, _DOT_, _DDOT_, _DDOTD_, _DOTA_,
    _QUOTE_, _QBSLASH_, _HASH_,
    _AND_, _OR_, _EQ_, _NOT_, _LT_, _GT_, _SLASH_
};

static char *yytext = 0;
static int  bufsize = 256;
static int  yyleng = 0;
static int  state = _S_;	/* start state */

#define clearbuf() (yytext[yyleng=0] = '\0')

void yyrestart(void)
{
    clearbuf();
    state = _S_;
}

static void save(int c)
{
    if (yyleng == bufsize - 1) {
	bufsize *= 2;
	yytext = (char *) erealloc(yytext, bufsize * sizeof(int));
    }
    yytext[yyleng++] = c;
    yytext[yyleng] = '\0';
}

static void action_A(void) {
	int pos;
	char new_yytext[256];
	extern int inPrefix; /* which mode of VA are we in: SUN_A or A_SUN... */

	if ((pos = search_constant(yytext)) != -1) {
		yylval.i = constants[pos].val;
		yyparse(constants[pos].token);
	} else if ((pos = search_token(yytext)) != -1) {
		yyparse(tokens[pos].token);
	} else { /* it is a variable */
		/* printf("yytext= %s %s %i %i %i\n", yytext, agentName, appAgentName,
		 append_NoAgentName, ScoutAppAgentName); */
		if (*agentName != 0 && appAgentName > 0 && append_NoAgentName > 0
				&& ScoutAppAgentName > 0) { /* for distributed tkEden -- sun */

			new_yytext[0] = '\0';
			if (yytext[0] == '_' && inPrefix) {
				strcat(new_yytext, "_");
				yytext++;
			}
			if (inPrefix)
				strcat(new_yytext, agentName);
			else
				strcat(new_yytext, yytext);
			strcat(new_yytext, "_");
			if (inPrefix)
				strcat(new_yytext, yytext);

			else
				strcat(new_yytext, agentName);
			strcpy(yytext, new_yytext);
		}
		yylval.v = lookUp(yytext);
		yyparse((yylval.v)->type);
	}
}


void yylex(int c)
{
    switch (state) {
	case _S_:
	if (isalpha(c) || c == '_') {
	    save(c);
	    state = _A_;
	    ScoutAppAgentName = 1;
	} else if (isdigit(c)) {
	    save(c);
	    state = _D_;
	} else if (c == ' ' || c == '\t') {
	    state = _S_;
	} else if (c == '\n') {
	    state = _S_;
	} else if (c == '\r') {
	    state = _S_;
	} else if (c == '.') {
	    state = _DOT_;
	} else if (c == '"') {
	    state = _QUOTE_;
	} else if (c == '#') {
	    state = _HASH_;
	} else if (c == '&') {
	    state = _AND_;
	} else if (c == '|') {
	    state = _OR_;
	} else if (c == '=') {
	    state = _EQ_;
	} else if (c == '!') {
	    state = _NOT_;
	} else if (c == '<') {
	    state = _LT_;
	} else if (c == '>') {
	    state = _GT_;
	} else if (c == '/') {
	    state = _SLASH_;
	} else if (c == '~') {   /*  for agency -- sun */
	    state = _A_;
	    ScoutAppAgentName = 0;
	} else {
	    yyparse(c);
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
	    yylex(c);
	}
	break;
    case _D_:
	if (isdigit(c)) {
	    save(c);
	    state = _D_;
	} else if (c == '.') {
	    save(c);
	    state = _DDOT_;
	} else {
	    yylval.d = atof(yytext);
	    yyparse(NUMBER);
	    clearbuf();
	    state = _S_;
	    yylex(c);
	}
	break;
    case _DDOT_:
	if (isdigit(c)) {
	    save(c);
	    state = _DDOTD_;
	} else if (c == '.') {
	    yylval.d = atof(yytext);
	    yyparse(NUMBER);
	    clearbuf();
	    state = _DOT_;
	} else {
	    yylval.d = atof(yytext);
	    yyparse(NUMBER);
	    clearbuf();
	    state = _DOT_;
	    yylex(c);
	}
	break;
    case _DDOTD_:
	if (isdigit(c)) {
	    save(c);
	    state = _DDOTD_;
	} else {
	    yylval.d = atof(yytext);
	    yyparse(NUMBER);
	    clearbuf();
	    state = _S_;
	    yylex(c);
	}
	break;
    case _DOT_:
	if (c == ' ' || c == '\t') {
	    state = _DOT_;
	} else if (isalpha(c)) {
	    save(c);
	    state = _DOTA_;
	} else {
	    yyparse('.');
	    clearbuf();
	    state = _S_;
	    yylex(c);
	}
	break;
    case _DOTA_:
	if (isalpha(c)) {
	    save(c);
	    state = _DOTA_;
	} else {
	    int         pos = search_field(yytext);

	    clearbuf();
	    yyparse(pos == -1 ? ERROR : fields[pos].token);
	    state = _S_;
	    yylex(c);
	}
	break;
    case _QUOTE_:
	if (c == '"') {
	    yylval.s = strdup(yytext);
	    yyparse(STR);
	    clearbuf();
	    state = _S_;
	} else if (c == '\\') {
	    state = _QBSLASH_;
	} else {
	    save(c);
	    state = _QUOTE_;
	}
	break;
    case _QBSLASH_:
	switch (c) {
	case 'n':
	    save('\n');
	    break;
	case '\n':		/* ignore \newline */
	    break;
	default:
	    save(c);
	    break;
	}
	state = _QUOTE_;
	break;
    case _HASH_:
	state = c == '\n' ? _S_ : _HASH_;
	break;
    case _AND_:
	if (c == '&') {
	    yyparse(AND);
	    state = _S_;
	} else {
	    yyparse('&');
	    state = _S_;
	    yylex(c);
	}
	break;
    case _OR_:
	if (c == '|') {
	    yyparse(OR);
	    state = _S_;
	} else {
	    yyparse('|');
	    state = _S_;
	    yylex(c);
	}
	break;
    case _EQ_:
	if (c == '=') {
	    yyparse(EQ);
	    state = _S_;
	} else {
	    yyparse('=');
	    state = _S_;
	    yylex(c);
	}
	break;
    case _NOT_:
	if (c == '=') {
	    yyparse(NE);
	    state = _S_;
	} else {
	    yyparse('!');
	    state = _S_;
	    yylex(c);
	}
	break;
    case _LT_:
	if (c == '=') {
	    yyparse(LE);
	    state = _S_;
	} else {
	    yyparse(LT);
	    state = _S_;
	    yylex(c);
	}
	break;
    case _GT_:
	if (c == '=') {
	    yyparse(GE);
	    state = _S_;
	} else {
	    yyparse(GT);
	    state = _S_;
	    yylex(c);
	}
	break;
    case _SLASH_:
	if (c == '/') {
	    yyparse(CONCAT);
	    state = _S_;
	} else {
	    yyparse('/');
	    state = _S_;
	    yylex(c);
	}
	break;
    }
}

#define FILE_DEV 1

void init_scout(void)
{
    char       *name = "/scout.eden";
    char        fullname[255];
    FILE       *initFile;
    extern char *progname;
    extern int run(short, void *, char *);
    char *envcontents;

    if (yytext == 0)
    	yytext = (char *) emalloc(bufsize * sizeof(int));

    strcpy(fullname, libLocation);
    strcat(fullname, name);

    if ((initFile = fopen(fullname, "r")) == 0) {
    	fprintf(stderr, "%s: can't open %s\n", progname, fullname);
    	exit(1);
    }
    lookUp("screen")->type = DISPVAR;	/* screen is of type display */
    run(FILE_DEV, initFile, name);

    /* pre-define these: if you change these, change also the
       corresponding values in the Eden namespace in scout.init.e
       [Ash] */
    /* These need to be in the constants table if VA is not to mess
       them up, so removed again.  [Ash] */
    /*
    installIntVar("OFF",    0);
    installIntVar("ON",     1);
    installIntVar("MOTION", 2);
    installIntVar("ENTER",  4);
    installIntVar("LEAVE",  8);
    */

}
