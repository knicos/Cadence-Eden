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

extern void dd_parse(int);
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

static char *dd_text = 0;
static bufsize = 256;
static dd_leng = 0;
static state = _S_;	/* start state */
static gtarget;		/* which graph token seeking? */
static int isOpenshape=0;  /* for distributed --sun */
static int curlyCount=0;
int    dd_appAgentName=1;
static int DonaldAppAgentName=1;

/* function prototypes */
int map_name_to_token(char *);
void dd_restart(void);
char * map_token_to_name(int);
void dd_lex(int);
void init_donald(void);
static void save(int);
static void action_A(void);
static int tokenize(int);

#define clearbuf() dd_text[dd_leng=0] = '\0'

void
dd_restart(void)
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
    if (dd_leng == bufsize - 1) {
	bufsize *= 2;
	dd_text = (char *) erealloc(dd_text, bufsize * sizeof(int));
    }
    dd_text[dd_leng++] = c;
    dd_text[dd_leng] = '\0';
}

static void
action_A(void)
{
    int token;
    char new_dd_text[256];   /* for distributed tkEden --sun */
    extern int  inPrefix;

    if ((token = map_name_to_token(dd_text)) == 0) {
      /* Virtual agents */
    	if (*agentName != 0 && dd_appAgentName >0
    	   && appAgentName > 0 && append_NoAgentName >0
    	   && DonaldAppAgentName >0 ) {
	   new_dd_text[0] = '\0';
	   if (inPrefix) strcpy(new_dd_text, agentName);
	      else  strcpy(new_dd_text, dd_text);
	   strcat(new_dd_text, "_");
	   if (inPrefix) strcat(new_dd_text, dd_text);
	      else  strcat(new_dd_text, agentName);
       	   strcpy(dd_text, new_dd_text);
        /*   printf("new_dd_text = %s\n", new_dd_text);  */
	}
	dd_lval.s = strdup(dd_text);
	dd_parse(ID);
    } else {	/* it is a token */
	if (token == BOOL)
	    dd_lval.i = (strcmp(dd_text, "true") == 0);
	if (token == WITHIN && *agentName != 0 && append_NoAgentName > 0) {
	  /* printf("within \n"); */
	   isOpenshape = 1;
	   curlyCount = 0;
	}
	dd_parse(token);
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
dd_lex(int c)
{
    switch (state) {
    case _S_:
	switch (c) {
	case ' ':
	case '\t':
	    state = _S_;
	    break;
	case '\n':
	    dd_parse(NEWLINE);
	    state = _S_;
	    break;
	case '\r':
	    dd_parse(NEWLINE);
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
		dd_parse(tokenize(c));
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
	    dd_lex(c);
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
	    dd_lval.i = atoi(dd_text);
	    dd_parse(INUMBER);
	    clearbuf();
	    state = _S_;
	    dd_lex(c);
	}
	break;
    case _DDOT_:
	if (isdigit(c)) {
	    save(c);
	    state = _DDOT_;
	} else if (c == '.') {
	    dd_lval.r = atof(dd_text);
	    dd_parse(RNUMBER);
	    clearbuf();
	    state = _S_;
	    dd_lex(c);
	} else if (c == 'E' || c == 'e') {
	    save(c);
	    state = _DE_;
	} else {
	    dd_lval.r = atof(dd_text);
	    dd_parse(RNUMBER);
	    clearbuf();
	    state = _S_;
	    dd_lex(c);
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
	    dd_lval.r = atof(dd_text);
	    dd_parse(RNUMBER);
	    clearbuf();
	    state = _S_;
	    dd_lex(c);
	}
	break;
    case _DED_:
    case _DESD_:
	if (isdigit(c)) {
	    save(c);
	    state = _DED_;
	} else {
	    dd_lval.r = atof(dd_text);
	    dd_parse(RNUMBER);
	    clearbuf();
	    state = _S_;
	    dd_lex(c);
	}
	break;
    case _DES_:
	if (isdigit(c)) {
	    save(c);
	    state = _DESD_;
	} else {
	    int E, S;
	    dd_lval.r = atof(dd_text);
	    dd_parse(RNUMBER);
	    E = dd_text[dd_leng-2];
	    S = dd_text[dd_leng-1];
	    clearbuf();
	    state = _S_;
	    dd_lex(E);
	    dd_lex(S);
	    dd_lex(c);
	}
	break;
    case _DOT_:
	if (c == 'x') {
	    dd_parse(DOTX);
	    clearbuf();
	    state = _S_;
	} else if (c == 'y') {
	    dd_parse(DOTY);
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
	    dd_parse('.');
	    clearbuf();
	    state = _S_;
	    dd_lex(c);
	}
	break;
    case _DOT1_:
	if (isdigit(c)) {
	    state = _DDOT_;
	    dd_lex(c);
	} else {
	    dd_parse(DOT1);
	    clearbuf();
	    state = _S_;
	    dd_lex(c);
	}
	break;
    case _DOT2_:
	if (isdigit(c)) {
	    state = _DDOT_;
	    dd_lex(c);
	} else {
	    dd_parse(DOT2);
	    clearbuf();
	    state = _S_;
	    dd_lex(c);
	}
	break;
    case _DA_:
	if (c == 'r') {
	    save(c);
	    state = _DAR_;
	} else {
	    dd_parse('.');
	    clearbuf();
	    state = _S_;
	    dd_lex('a');
	    dd_lex(c);
	}
	break;
    case _DAR_:
	if (c == 'g') {
	    dd_parse(DOTARG);
	    clearbuf();
	    state = _S_;
	} else {
	    dd_parse('.');
	    clearbuf();
	    state = _S_;
	    dd_lex('a');
	    dd_lex('r');
	    dd_lex(c);
	}
	break;
    case _DR_:
	if (c == 'a') {
	    save(c);
	    state = _DRA_;
	} else {
	    dd_parse('.');
	    clearbuf();
	    state = _S_;
	    dd_lex('r');
	    dd_lex(c);
	}
	break;
    case _DRA_:
	if (c == 'd') {
	    dd_parse(DOTRAD);
	    clearbuf();
	    state = _S_;
	} else {
	    dd_parse('.');
	    clearbuf();
	    state = _S_;
	    dd_lex('r');
	    dd_lex('a');
	    dd_lex(c);
	}
	break;
    case _QUOTE_:
	if (c == '"') {
	    save(c);
	    dd_lval.s = strdup(dd_text);
	    dd_parse(CSTRING);
	    clearbuf();
	    state = _S_;
	} else if (c == '\n' || c == '\r') {
	    save('"');
	    dd_lval.s = strdup(dd_text);
	    dd_parse(CSTRING);
	    clearbuf();
	    state = _S_;
	} else {
	    save(c);
	    state = _QUOTE_;
	}
	break;
    case _QUERY_:
	if (c == '\n' || c == '\r') {
	    dd_lval.s = strdup(dd_text);
	    dd_parse(QUERY);
	    dd_parse(NEWLINE);
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
	    dd_parse('\\');
	    state = _S_;
	    dd_lex(c);
	}
	break;
    case _HASH_:
	if (c == '\n' || c == '\r') {
	    dd_parse(NEWLINE);
	    state = _S_;
	} else {
	    state = _HASH_;
	}
	break;
    case _AND_:
	if (c == '&') {
	    dd_parse(AND);
	    state = _S_;
	} else {
	    dd_parse(AMPERSAND);
	    state = _S_;
	    dd_lex(c);
	}
	break;
    case _OR_:
	if (c == '|') {
	    dd_parse(OR);
	    state = _S_;
	} else {
	    dd_parse(VERTBAR);
	    state = _S_;
	    dd_lex(c);
	}
	break;
    case _EQ_:
	if (c == '=') {
	    dd_parse(EQ_EQ);
	    state = _S_;
	} else {
	    dd_parse(EQUALS);
	    state = _S_;
	    dd_lex(c);
	}
	break;
    case _NOT_:
	if (c == '=') {
	    dd_parse(NOT_EQ);
	    state = _S_;
	} else {
	    dd_parse(NOT);
	    state = _S_;
	    dd_lex(c);
	}
	break;
    case _LT_:
	if (c == '=') {
	    dd_parse(LT_EQ);
	    state = _S_;
	} else {
	    dd_parse(LT);
	    state = _S_;
	    dd_lex(c);
	}
	break;
    case _GT_:
	if (c == '=') {
	    dd_parse(GT_EQ);
	    state = _S_;
	} else {
	    dd_parse(GT);
	    state = _S_;
	    dd_lex(c);
	}
	break;
    case _SLASH_:
	if (c == '/') {
	    dd_parse(SLASH_SLASH);
	    state = _S_;
	} else {
	    dd_parse(SLASH);
	    state = _S_;
	    DonaldAppAgentName = 0;
	    dd_lex(c);
	}
	break;
    case _TILDE_:
	if (c == '/') {
	    DonaldAppAgentName = 1;
	    dd_appAgentName++;
	    dd_parse(TILDE_SLASH);
	    state = _S_;
	} else {
	    DonaldAppAgentName = 0;
	    state = _S_;
	    dd_lex(c);
	}
	break;
    case _F_:
	if (c == ' ' || c == '\t') {
	    state = _F_;
	} else if (c == '<') {
	    state = _B_;
	} else {
	    state = _A_;
	    dd_lex(c);
	}
	break;
    case _X_:
	if (c == ' ' || c == '\t') {
	    state = _X_;
	} else if (c == '<') {
	    state = _B_;
	} else {
	    state = _A_;
	    dd_lex(c);
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
		dd_lex('<');
		dd_lex(c);
		break;
	    default:
		state = _LT_;
		dd_lex(c);
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
		dd_parse(FI);
		break;
	    case _X_:
		dd_parse(XI);
		break;
	    case _I_:
		dd_parse(I);
		break;
	    }
	    clearbuf();
	    state = _S_;
	} else {
	    switch (gtarget) {
	    case _F_:
	    case _X_:
		state = _A_;
		dd_lex('<');
		dd_lex('i');
		if (state == _BISP_) dd_lex(' ');
		dd_lex(c);
		break;
	    default:
		state = _LT_;
		dd_lex('i');
		if (state == _BISP_) dd_lex(' ');
		dd_lex(c);
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
		dd_lex('<');
		dd_lex('i');
		dd_lex('-');
		dd_lex(c);
		break;
	    default:
		state = _LT_;
		dd_lex('i');
		dd_lex('-');
		dd_lex(c);
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
		dd_parse(FI_1);
		break;
	    case _X_:
		dd_parse(XI_1);
		break;
	    case _I_:
		dd_parse(I_1);
		break;
	    }
	    clearbuf();
	    state = _S_;
	} else {
	    switch (gtarget) {
	    case _F_:
	    case _X_:
		state = _A_;
		dd_lex('<');
		dd_lex('i');
		dd_lex('-');
		dd_lex('1');
		if (state == _BI_1SP_) dd_lex(' ');
		dd_lex(c);
		break;
	    default:
		state = _LT_;
		dd_lex('i');
		dd_lex('-');
		dd_lex('1');
		if (state == _BI_1SP_) dd_lex(' ');
		dd_lex(c);
		break;
	    }
	}
	break;
    case _II_:
	if (c == '!') {
	    state = _IN_;
	} else {
	    state = _A_;
	    dd_lex('I');
	    dd_lex(c);
	}
	break;
    case _IN_:
	if (isalpha(c)) {
	   save(c);
	   state = _INA_;
	} else {
	    state = _S_;
	    dd_lex('I');
	    dd_lex('!');
	    dd_lex(c);
	}
	break;
    case _INA_:
	if (isalpha(c)) {
	   save(c);
	   state = _INA_;
	} else {
	    dd_lval.s = strdup(dd_text);
	    dd_parse(IMGFUNC);
	    clearbuf();
	    state = _S_;
	    dd_lex(c);
	}
	break;
    case _IGNORENL_:
      if (c == '\n') {
	state = _S_;		/* discard \n after \r */
      } else {
	state = _S_;
	dd_lex(c);
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

    if (dd_text == 0) {
	dd_text = (char *) emalloc(bufsize * sizeof(int));
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
