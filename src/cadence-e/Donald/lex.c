/*
 * $Id: lex.c,v 1.21 2002/03/01 23:37:56 cssbz Exp $
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

static char rcsid[] = "$Id: lex.c,v 1.21 2002/03/01 23:37:56 cssbz Exp $";

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../config.h"
#include "tree.h"
#include "parser.h"
#include "../Eden/eden.h"

#include "../Eden/emalloc.h"
#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

extern void yyparse(int);
extern char agentName[128]; /* for distributed tkEden --sun */
extern int  appAgentName;
extern int append_NoAgentName;
extern char *libLocation;

char *dd_prompt;

enum {
    _S_, _A_, _D_, _DDOT_, _DE_, _DED_, _DES_, _DESD_,
    _DOT_, _DOT1_, _DOT2_,
    _DA_, _DAR_, _DR_, _DRA_,
    _QUOTE_, _QUERY_, _BSLASH_, _HASH_,
    _AND_, _OR_, _EQ_, _NOT_, _LT_, _GT_, _SLASH_,
    _B_, _BI_, _BISP_, _BI__, _BI_1_, _BI_1SP_, _I_, _X_, _F_,
    _II_, _IN_, _INA_, _TILDE_, _IGNORENL_
};

static char *yytext = 0;
static bufsize = 256;
static yyleng = 0;
static state = _S_;	/* start state */
static gtarget;		/* which graph token seeking? */
static int isOpenshape=0;  /* for distributed --sun */
static int curlyCount=0;
int    dd_appAgentName=1;
static int DonaldAppAgentName=1;

/* function prototypes */
int map_name_to_token(char *);
void yyrestart(void);
char * map_token_to_name(int);
void yylex(int);
void init_donald(void);
static void save(int);
static void action_A(void);
static int tokenize(int);

#define clearbuf() yytext[yyleng=0] = '\0'

void
yyrestart(void)
{
    clearbuf();
    state = _S_;
}

#define NUMTOKEN 63
struct {
        char   *name;
        int     token;
} translation[NUMTOKEN] = {
	{ "acos"	,	ACOS		},
/*	{ "alias"	,	ALIAS		},	*/
	{ "and"		,	AND		},
	{ "arc"		,	ARC		},
/*	{ "as"		,	AS		},	*/
	{ "asin"	,	ASIN		},
	{ "atan"	,	ATAN		},
	{ "boolean"	,	BOOLEAN		},
	{ "char"	,	MYCHAR		},
	{ "circle"	,	CIRCLE		},
	{ "colinear"	,	COLINEAR	},
	{ "cos"		,	COS		},
	{ "delete"	,	DELETE		},
	{ "dist"	,	DISTANCE	},
	{ "distlarger"	,	DISTLARGER	},
	{ "distsmaller"	,	DISTSMALLER	},
	{ "div"		,	DIV		},
/*	{ "draw"	,	DRAW		},	*/
	{ "ellipse"	,	ELLIPSE		},
	{ "else"	,	ELSE		},
	{ "exp"		,	EXP		},
	{ "false"	,	BOOL		},
	{ "float"	,	FLOAT		},
/*	{ "for"		,	FOR		},	*/
	{ "graph"	,	GRAPH		},
	{ "if"		,	IF		},
	{ "image"	,	IMAGE		},
/*	{ "impose"	,	IMPOSE		},	*/
/*	{ "in"		,	IN		},	*/
	{ "incident"	,	INCIDENT	},
	{ "includes"	,	INCLUDES	},
	{ "inf"		,	INF		},
	{ "int"		,	INT		},
	{ "intersect"	,	INTERSECT	},
	{ "intersects"	,	INTERSECTS	},
	{ "itos"	,	ITOS		},
	{ "label"	,	LABEL		},
	{ "line"	,	LINE		},
	{ "log"		,	LOG		},
	{ "midpoint"	,	MIDPOINT	},
	{ "mod"		,	MOD		},
/*	{ "monitor"	,	MONITOR		},	*/
	{ "not"		,	NOT		},
	{ "of"		,	OF		},
	{ "openshape"	,	OPENSHAPE	},
	{ "or"		,	OR		},
	{ "parallel"	,	PARALLEL	},
	{ "perpend"	,	PERPEND		},
	{ "pi"		,	PI		},
	{ "point"	,	POINT		},
	{ "pt_betwn_pts",	PT_BETWN_PTS	},
	{ "rand"	,	RANDOM		},
/*	{ "range"	,	RANGE		},	*/
	{ "real"	,	REAL		},
        { "rectangle"   ,       RECTANGLE       },
	{ "reflect"     ,       REFLECT         }, /* Ash November 2001 */
	{ "rot"		,	ROT		},
	{ "rtos"	,	RTOS		},
	{ "scale"	,	SCALE		},
	{ "scalexy"	,	SCALEXY		}, /* Ant 19/10/2005 */
	{ "separates"	,	SEPARATES	},
	{ "shape"	,	SHAPE		},
	{ "sin"		,	SIN		},
	{ "sqrt"	,	SQRT		},
	{ "tan"		,	TAN		},
	{ "then"	,	THEN		},
	{ "trans"	,	TRANS		},
	{ "true"	,	BOOL		},
	{ "trunc"	,	TRUNC		},
	{ "viewport"	,	VIEWPORT	},
	{ "within"	,	WITHIN		}/*,
	{ 0, 0 }*/
};

int
map_name_to_token(char *name)
{
    int         i, start, end, cmp;

    start = 0;
    end = NUMTOKEN - 1;
    for (i = (NUMTOKEN - 1) >> 1;
	 start <= i && i <= end;
	 i = (start + end) >> 1) {

	if (translation[i].name == 0)
	    return 0;
	cmp = strcmp(name, translation[i].name);
	if (cmp == 0)
	    return translation[i].token;
	else if (cmp > 0)
	    start = i + 1;
	else			/* cmp < 0 */
	    end = i - 1;
    }
    return 0;
}

char *
map_token_to_name(int token)
{
    int         i;

    for (i = 0; i < NUMTOKEN; i++)
	if (translation[i].token == token)
	    return translation[i].name;
    return 0;
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
    int token;
    char new_yytext[256];   /* for distributed tkEden --sun */
    extern int  inPrefix;

    if ((token = map_name_to_token(yytext)) == 0) {
      /* Virtual agents */
    	if (*agentName != 0 && dd_appAgentName >0
    	   && appAgentName > 0 && append_NoAgentName >0
    	   && DonaldAppAgentName >0 ) {
	   new_yytext[0] = '\0';
	   if (inPrefix) strcpy(new_yytext, agentName);
	      else  strcpy(new_yytext, yytext);
	   strcat(new_yytext, "_");
	   if (inPrefix) strcat(new_yytext, yytext);
	      else  strcat(new_yytext, agentName);
       	   strcpy(yytext, new_yytext);
        /*   printf("new_yytext = %s\n", new_yytext);  */
	}
	yylval.s = strdup(yytext);
	yyparse(ID);
    } else {	/* it is a token */
	if (token == BOOL)
	    yylval.i = (strcmp(yytext, "true") == 0);
	if (token == WITHIN && *agentName != 0 && append_NoAgentName > 0) {
	  /* printf("within \n"); */
	   isOpenshape = 1;
	   curlyCount = 0;
	}
	yyparse(token);
    }
    DonaldAppAgentName = 1;
}

static int
tokenize(int c)
{
    switch (c) {
    case '!':	return NOT;
    case ',':	return COMMA;
    case '+':	return PLUS;
    case '-':	return MINUS;
    case '*':	return STAR;
    case '/':	return SLASH;
    case '%':	return PERCENT;
    case '=':	return EQUALS;
    case '&':	return AMPERSAND;
    case '|':	return VERTBAR;
    case ':':	return COLON;
    case ';':	return SEMICOLON;
    case '~':	DonaldAppAgentName = 0; return TILDE;
    case '@':	return ATSIGN;
    case '(':	return LPAREN;
    case ')':	return RPAREN;
    case '<':	return LT;
    case '>':	return GT;
    case '[':	return LBRACK;
    case ']':	return RBRACK;
    case '{':	return LCURLY;
    case '}':	return RCURLY;
    default:	return c;
    }
}

void
yylex(int c)
{
    switch (state) {
    case _S_:
	switch (c) {
	case ' ':
	case '\t':
	    state = _S_;
	    break;
	case '\n':
	    yyparse(NEWLINE);
	    state = _S_;
	    break;
	case '\r':
	    yyparse(NEWLINE);
	    state = _IGNORENL_;
	    break;
	case '.':
	    save(c);
	    state = _DOT_;
	    break;
	case '?':
	    state = _QUERY_;
	    break;
	case '"':
	    save(c);
	    state = _QUOTE_;
	    break;
	case '\\':
	    state = _BSLASH_;
	    break;
	case '#':
	    state = _HASH_;
	    break;
	case '&':
	    state = _AND_;
	    break;
	case '|':
	    state = _OR_;
	    break;
	case '=':
	    state = _EQ_;
	    break;
	case '!':
	    state = _NOT_;
	    break;
	case '<':
	    gtarget = _I_;
	    state = _B_;
	    break;
	case '>':
	    state = _GT_;
	    break;
	case '/':
	    state = _SLASH_;
	    break;
	case 'f':
	    gtarget = _F_;
	    state = _F_;
	    save(c);
	    break;
	case 'x':
	    gtarget = _X_;
	    state = _X_;
	    save(c);
	    break;
	case 'I':
	    state = _II_;
	    break;
	case '~':
	    state = _TILDE_;
	    break;
	default:
	    if (isalpha(c) || c == '_') {
		save(c);
		state = _A_;
	    } else if (isdigit(c)) {
		save(c);
		state = _D_;
	    } else {
		yyparse(tokenize(c));
		state = _S_;
	    }
	    break;
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
	} else if (c == 'E' || c == 'e') {
	    save(c);
	    state = _DE_;
	} else {
	    yylval.i = atoi(yytext);
	    yyparse(INUMBER);
	    clearbuf();
	    state = _S_;
	    yylex(c);
	}
	break;
    case _DDOT_:
	if (isdigit(c)) {
	    save(c);
	    state = _DDOT_;
	} else if (c == '.') {
	    yylval.r = atof(yytext);
	    yyparse(RNUMBER);
	    clearbuf();
	    state = _S_;
	    yylex(c);
	} else if (c == 'E' || c == 'e') {
	    save(c);
	    state = _DE_;
	} else {
	    yylval.r = atof(yytext);
	    yyparse(RNUMBER);
	    clearbuf();
	    state = _S_;
	    yylex(c);
	}
	break;
    case _DE_:
	if (isdigit(c)) {
	    save(c);
	    state = _DED_;
	} else if (c == '+' || c == '-') {
	    save(c);
	    state = _DES_;
	} else {
	    yylval.r = atof(yytext);
	    yyparse(RNUMBER);
	    clearbuf();
	    state = _S_;
	    yylex(c);
	}
	break;
    case _DED_:
    case _DESD_:
	if (isdigit(c)) {
	    save(c);
	    state = _DED_;
	} else {
	    yylval.r = atof(yytext);
	    yyparse(RNUMBER);
	    clearbuf();
	    state = _S_;
	    yylex(c);
	}
	break;
    case _DES_:
	if (isdigit(c)) {
	    save(c);
	    state = _DESD_;
	} else {
	    int E, S;
	    yylval.r = atof(yytext);
	    yyparse(RNUMBER);
	    E = yytext[yyleng-2];
	    S = yytext[yyleng-1];
	    clearbuf();
	    state = _S_;
	    yylex(E);
	    yylex(S);
	    yylex(c);
	}
	break;
    case _DOT_:
	if (c == 'x') {
	    yyparse(DOTX);
	    clearbuf();
	    state = _S_;
	} else if (c == 'y') {
	    yyparse(DOTY);
	    clearbuf();
	    state = _S_;
	} else if (c == '1') {
	    save(c);
	    state = _DOT1_;
	} else if (c == '2') {
	    save(c);
	    state = _DOT2_;
	} else if (c == 'a') {
	    save(c);
	    state = _DA_;
	} else if (c == 'r') {
	    save(c);
	    state = _DR_;
	} else if (isdigit(c)) {
	    save(c);
	    state = _DDOT_;
	} else {
	    yyparse('.');
	    clearbuf();
	    state = _S_;
	    yylex(c);
	}
	break;
    case _DOT1_:
	if (isdigit(c)) {
	    state = _DDOT_;
	    yylex(c);
	} else {
	    yyparse(DOT1);
	    clearbuf();
	    state = _S_;
	    yylex(c);
	}
	break;
    case _DOT2_:
	if (isdigit(c)) {
	    state = _DDOT_;
	    yylex(c);
	} else {
	    yyparse(DOT2);
	    clearbuf();
	    state = _S_;
	    yylex(c);
	}
	break;
    case _DA_:
	if (c == 'r') {
	    save(c);
	    state = _DAR_;
	} else {
	    yyparse('.');
	    clearbuf();
	    state = _S_;
	    yylex('a');
	    yylex(c);
	}
	break;
    case _DAR_:
	if (c == 'g') {
	    yyparse(DOTARG);
	    clearbuf();
	    state = _S_;
	} else {
	    yyparse('.');
	    clearbuf();
	    state = _S_;
	    yylex('a');
	    yylex('r');
	    yylex(c);
	}
	break;
    case _DR_:
	if (c == 'a') {
	    save(c);
	    state = _DRA_;
	} else {
	    yyparse('.');
	    clearbuf();
	    state = _S_;
	    yylex('r');
	    yylex(c);
	}
	break;
    case _DRA_:
	if (c == 'd') {
	    yyparse(DOTRAD);
	    clearbuf();
	    state = _S_;
	} else {
	    yyparse('.');
	    clearbuf();
	    state = _S_;
	    yylex('r');
	    yylex('a');
	    yylex(c);
	}
	break;
    case _QUOTE_:
	if (c == '"') {
	    save(c);
	    yylval.s = strdup(yytext);
	    yyparse(CSTRING);
	    clearbuf();
	    state = _S_;
	} else if (c == '\n' || c == '\r') {
	    save('"');
	    yylval.s = strdup(yytext);
	    yyparse(CSTRING);
	    clearbuf();
	    state = _S_;
	} else {
	    save(c);
	    state = _QUOTE_;
	}
	break;
    case _QUERY_:
	if (c == '\n' || c == '\r') {
	    yylval.s = strdup(yytext);
	    yyparse(QUERY);
	    yyparse(NEWLINE);
	    clearbuf();
	    state = _S_;
	} else {
	    save(c);
	    state = _QUERY_;
	}
	break;
    case _BSLASH_:
	if (c == '\n' || c == '\r') {
	    state = _S_;
	} else {
	    yyparse('\\');
	    state = _S_;
	    yylex(c);
	}
	break;
    case _HASH_:
	if (c == '\n' || c == '\r') {
	    yyparse(NEWLINE);
	    state = _S_;
	} else {
	    state = _HASH_;
	}
	break;
    case _AND_:
	if (c == '&') {
	    yyparse(AND);
	    state = _S_;
	} else {
	    yyparse(AMPERSAND);
	    state = _S_;
	    yylex(c);
	}
	break;
    case _OR_:
	if (c == '|') {
	    yyparse(OR);
	    state = _S_;
	} else {
	    yyparse(VERTBAR);
	    state = _S_;
	    yylex(c);
	}
	break;
    case _EQ_:
	if (c == '=') {
	    yyparse(EQ_EQ);
	    state = _S_;
	} else {
	    yyparse(EQUALS);
	    state = _S_;
	    yylex(c);
	}
	break;
    case _NOT_:
	if (c == '=') {
	    yyparse(NOT_EQ);
	    state = _S_;
	} else {
	    yyparse(NOT);
	    state = _S_;
	    yylex(c);
	}
	break;
    case _LT_:
	if (c == '=') {
	    yyparse(LT_EQ);
	    state = _S_;
	} else {
	    yyparse(LT);
	    state = _S_;
	    yylex(c);
	}
	break;
    case _GT_:
	if (c == '=') {
	    yyparse(GT_EQ);
	    state = _S_;
	} else {
	    yyparse(GT);
	    state = _S_;
	    yylex(c);
	}
	break;
    case _SLASH_:
	if (c == '/') {
	    yyparse(SLASH_SLASH);
	    state = _S_;
	} else {
	    yyparse(SLASH);
	    state = _S_;
	    DonaldAppAgentName = 0;
	    yylex(c);
	}
	break;
    case _TILDE_:
	if (c == '/') {
	    DonaldAppAgentName = 1;
	    dd_appAgentName++;
	    yyparse(TILDE_SLASH);
	    state = _S_;
	} else {
	    DonaldAppAgentName = 0;
	    state = _S_;
	    yylex(c);
	}
	break;
    case _F_:
	if (c == ' ' || c == '\t') {
	    state = _F_;
	} else if (c == '<') {
	    state = _B_;
	} else {
	    state = _A_;
	    yylex(c);
	}
	break;
    case _X_:
	if (c == ' ' || c == '\t') {
	    state = _X_;
	} else if (c == '<') {
	    state = _B_;
	} else {
	    state = _A_;
	    yylex(c);
	}
	break;
    case _B_:
	if (c == ' ' || c == '\t') {
	    state = _B_;
	} else if (c == 'i') {
	    state = _BI_;
	} else {
	    switch (gtarget) {
	    case _F_:
	    case _X_:
		state = _A_;
		yylex('<');
		yylex(c);
		break;
	    default:
		state = _LT_;
		yylex(c);
		break;
	    }
	}
	break;
    case _BI_:
    case _BISP_:
	if (c == ' ' || c == '\t') {
            state = _BISP_;
        } else if (c == '-') {
	    state = _BI__;
	} else if (c == '>') {
	    switch (gtarget) {
	    case _F_:
		yyparse(FI);
		break;
	    case _X_:
		yyparse(XI);
		break;
	    case _I_:
		yyparse(I);
		break;
	    }
	    clearbuf();
	    state = _S_;
	} else {
	    switch (gtarget) {
	    case _F_:
	    case _X_:
		state = _A_;
		yylex('<');
		yylex('i');
		if (state == _BISP_) yylex(' ');
		yylex(c);
		break;
	    default:
		state = _LT_;
		yylex('i');
		if (state == _BISP_) yylex(' ');
		yylex(c);
		break;
	    }
	}
	break;
    case _BI__:
	if (c == ' ' || c == '\t') {
            state = _BI__;
        } else if (c == '1') {
            state = _BI_1_;
	} else {
	    switch (gtarget) {
	    case _F_:
	    case _X_:
		state = _A_;
		yylex('<');
		yylex('i');
		yylex('-');
		yylex(c);
		break;
	    default:
		state = _LT_;
		yylex('i');
		yylex('-');
		yylex(c);
		break;
	    }
	}
	break;
    case _BI_1_:
    case _BI_1SP_:
	if (c == ' ' || c == '\t') {
            state = _BI_1SP_;
        } else if (c == '>') {
	    switch (gtarget) {
	    case _F_:
		yyparse(FI_1);
		break;
	    case _X_:
		yyparse(XI_1);
		break;
	    case _I_:
		yyparse(I_1);
		break;
	    }
	    clearbuf();
	    state = _S_;
	} else {
	    switch (gtarget) {
	    case _F_:
	    case _X_:
		state = _A_;
		yylex('<');
		yylex('i');
		yylex('-');
		yylex('1');
		if (state == _BI_1SP_) yylex(' ');
		yylex(c);
		break;
	    default:
		state = _LT_;
		yylex('i');
		yylex('-');
		yylex('1');
		if (state == _BI_1SP_) yylex(' ');
		yylex(c);
		break;
	    }
	}
	break;
    case _II_:
	if (c == '!') {
	    state = _IN_;
	} else {
	    state = _A_;
	    yylex('I');
	    yylex(c);
	}
	break;
    case _IN_:
	if (isalpha(c)) {
	   save(c);
	   state = _INA_;
	} else {
	    state = _S_;
	    yylex('I');
	    yylex('!');
	    yylex(c);
	}
	break;
    case _INA_:
	if (isalpha(c)) {
	   save(c);
	   state = _INA_;
	} else {
	    yylval.s = strdup(yytext);
	    yyparse(IMGFUNC);
	    clearbuf();
	    state = _S_;
	    yylex(c);
	}
	break;
    case _IGNORENL_:
      if (c == '\n') {
	state = _S_;		/* discard \n after \r */
      } else {
	state = _S_;
	yylex(c);
      }
      break;
    }
}

#define FILE_DEV 1

void
init_donald(void)
{
    char       *name = "/donald.eden";
    char        fullname[255];
    FILE       *initFile;
    extern char *progname;
    extern char viewport_name[];
    extern int run(short, void *, char *);
    char *envcontents;

    if (yytext == 0) {
	yytext = (char *) emalloc(bufsize * sizeof(int));
    };
    *viewport_name = '\0';
    dd_prompt = (char *)malloc(4);
    strcpy(dd_prompt, "[/]");

    strcpy(fullname, libLocation);
    strcat(fullname, name);

    if ((initFile = fopen(fullname, "r")) == 0) {
	fprintf(stderr, "%s: can't open %s\n", progname, fullname);
	exit(1);
    }
    run(FILE_DEV, initFile, name);
}
