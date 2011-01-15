/*
 * $Id: lex.c,v 1.30 2002/02/18 19:28:14 cssbz Exp $
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

static char rcsid[] = "$Id: lex.c,v 1.30 2002/02/18 19:28:14 cssbz Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <ctype.h>
#include <assert.h>

#include "../../../config.h"
#include "eden.h"
#include "yacc.h"
#include "builtin.h"

#if defined(TTYEDEN)
#if defined(HAVE_CURSES) || defined(HAVE_NCURSES)
#include <curses.h>
#ifdef HAVE_READLINE
#include <readline.h>
#include <history.h>
#endif /* HAVE_READLINE */
#endif /* HAVE_CURSES or HAVE_NCURSES */
#else /* not TTYEDEN */
#include <tk.h>
#include "../EX/script.h"
#endif

#include "notation.h"
#include "input_device.h"

#include "emalloc.h"
#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

#define MAXTEXTLEN 4096  /* change from 1024 --sun */

static int  bof;
int         nextc = ' ';
char        yytext[MAXTEXTLEN];
int         yyleng = 0;
extern YYSTYPE yylval;

#ifdef TTYEDEN
extern char *prompt, *prompt1;
extern char *promptsemi, *promptchar, *promptcomment;
#else /* not TTYEDEN */
extern Tcl_Interp *interp;
#endif

extern void push_text(char *, int);
int inEVAL = 0;

#ifdef TTYEDEN
extern int preprinted;
extern void print_prompt_if_necessary(int);
#endif

/* DOSTE functions */
extern int doste_context(const char *);
extern int basecontext; /* DOSTE context [Nick] */


/* For VAs [Ash] */
char   agentName[128];
char   agentType[3];
int    appAgentName = 1;
int append_agentName = 1;
int append_NoAgentName = 1;
int inPrefix = 1;

/* function prototypes */
Datum      *makedatum(int, uDatum);
void        init_lex(void);
int         yylex(void);
int         peek(void);
int         keyin(void);
static void append_char(int);
static int  buffer_overflow(void);
static int  keyword_token(char *);
static int  number_token(void);
static int  id_token(void);
static int  multi_symbol_token(void);
static void skip_comment(void);
static void skip_one_line_comment(void);
static void skip_percent_comment(void);
static void backslash(char *);
/* These for VA [Ash] */
int         builtin_ft_check(char *);
static char new_yytext[MAXTEXTLEN]; /* for distributed tkeden -- sun */
static void getAgentName(void);
#ifdef DISTRIB
static int  setHighPrior = 0; /* Setting a higher priority (which
                                 doesn't work very well) - see the
                                 Tcl/Eden variable higherPriority in
                                 main.c [Ash] */
#endif /* DISTRIB */

/* app_char(c): put c into the yytext buffer and complain if it
   overflows [Ash] */
#define app_char(c)	(yyleng>=MAXTEXTLEN?buffer_overflow():(yytext[yyleng++]=c))

/* input(): update nlstts from Inp_Dev, then basically set nextc=getc()
   [Ash] */
#ifdef TTYEDEN
extern void nlstatus(void);
#define input()		(nlstatus(),app_char(nextc),nextc=keyin())
#else
#define input()		(app_char(nextc),nextc=keyin())
#endif /* TTYEDEN */

#define safe_input()	(append_char(nextc),nextc=keyin())
#define unput(c)	(pop_char(),ungetkey(c))
#define	pop_char()	--yyleng
#define last_char	yytext[yyleng-1]

#define end_text1()     yytext[yyleng]='\0'

#define end_text()	{accept_text(yytext,yyleng);yytext[yyleng]='\0';}
#define clear_text()	yyleng=0

#define accept_text(Text,Leng) if((indef||informula) && !inEVAL) push_text(Text,Leng)

#define RETURN_TOKEN(T)	{end_text(); return T;}

/* changeNotation moved out of EX into Eden/lex.c [Ash] */
notationType currentNotation = EDEN;

void
changeNotation(notationType notation)
{
  /*  fprintf(stderr, "changeNotation %d\n", notation); */
    currentNotation = notation;
}

Datum      *
makedatum(int type, uDatum value)
{				/* MAKE A NEW DATUM FOR CONSTANTS */
    Datum      *p;

    p = (Datum *) emalloc(sizeof(Datum));
    p->type = type;
    p->u = value;
    return p;
}

static void
append_char(int c)
{
    if (yyleng >= MAXTEXTLEN - 1) {
	end_text();
	clear_text();
    }
    yytext[yyleng++] = c;
}

static int
buffer_overflow(void)
{
    error("input buffer overflow");
    return 1;	/* would not reach */
}

/* init_lex: read in one character [Ash] */
void
init_lex(void)
{
    bof = 1;
    input();
}

static int
keyword_token(char *name)
{				/* Is name a keyword ? 0 = no; lexeme =
				   yes) */
    static struct keyword_table {	/* Keywords */
	char       *name;
	int         kval;
    }           keywords[] = {
#include "keyword.h"
	{ 0, 0 }
    };

    int         i;

    for (i = 0; keywords[i].name; i++) {
	if (strcmp(name, keywords[i].name) == 0)
	    return keywords[i].kval;
    }
    return 0;
}

/* When prefixing names with the agent name, we need to check whether
   a function to be prefixed is a built-in function or not.  Built-in
   functions like writeln, propagate etc should not be prefixed with
   an agent name eg SUN, but user-defined functions should be
   prefixed. [Ash, with Patrick] */
int
builtin_ft_check(char *name)     /*  --sun */
{				/* Is name a builtin function ? 0 = no; 1 =
				   yes) */
    static struct builtinF_table {	/* builtin or predefined functions */
	char       *name;
    }           builtinFs[] = {
#include "builtinf.h"
	{ 0 }
    };

    int         i;

    for (i = 0; builtinFs[i].name; i++) {
	if (strcmp(name, builtinFs[i].name) == 0)
	    return 1;
    }
    return 0;
}

static int
number_token(void)
{
    int         i;
    double      r;
    int         m, n, e, is_float;
    char       *format;
    int         is_hex;
    uDatum	u;

    is_float = m = n = e = FALSE;
    while (isdigit(nextc)) {
	input();
	m = TRUE;
    }
    if (nextc == '.') {
	input();
	is_float = TRUE;
    }
    while (isdigit(nextc)) {
	input();
	n = TRUE;
    }

    if (is_float && !m && !n)	/* a single dot */
	return '.';

    if (nextc == 'E' || nextc == 'e') {
	input();
	is_float = TRUE;
	if (nextc == '+' || nextc == '-')
	    input();
	while (isdigit(nextc)) {
	    input();
	    e = TRUE;
	}
	if (!e)
	    error("Floating-point format error");
    }
    if (is_float) {		/* floating point */
	end_text();
	sscanf(yytext, "%lf", &r);
	u.r = r;
	yylval.dp = makedatum(REAL, u);
	return CONSTANT;
    } else if (*yytext == '0') {/* octal or hexdecimal */
	is_hex = nextc == 'x' || nextc == 'X';
	format = is_hex ? "%x" : "%o";
	if (is_hex) {
	    do {
		input();	/* read the 'x' */
	    } while (isxdigit(nextc));
	}
	end_text();
	sscanf((is_hex ? yytext + 2 : yytext), format, &i);
	u.i = i;
	yylval.dp = makedatum(INTEGER, u);
	return CONSTANT;
    } else {			/* decimal */
	end_text();
	u.i = atoi(yytext);
	yylval.dp = makedatum(INTEGER, u);
	return CONSTANT;
    }
}

static int
id_token(void)
{
    extern symptr lookup(char *, int);
    extern symptr install(char *, int, int, int, Int);
    extern int  local_declare(char *);
    extern int  lookup_local(char *);
    int         i;
    symptr      sp;
    char        *tempyytext;
    char	*tempagentname;
    char	aname[80];
	int context;
	void *tempoid;

	context = basecontext;
    tempagentname = agentName;

    while (isalnum(nextc) || nextc == '_') {
      input();
    }

	end_text1();

    /* printf("yytext %s", yytext); */
    if ((i = keyword_token(yytext))) {	/* keyword ? */
      if (i==EVAL) inEVAL = 1;
      end_text();
      return i;		/* return the lexeme */
    }

	if (nextc == ':') {
		if (peek() == ':') {
			strcpy(aname, yytext);
			tempagentname = aname;
			input();
			input();
			end_text();
			clear_text();
	
			while (isalnum(nextc) || nextc == '_') {
				input();
			}
	
			end_text1();
		}
	
	}


    if (inauto || inpara) {	/* declaring local variables ? */
        end_text();
	yylval.narg = local_declare(yytext);
	return LOCAL;
    } else {			/* in statements */
	if ((i = lookup_local(yytext))) {	/* local variable ? */
            end_text();
	    if (i < 0) {	/* it is an argument */
		yylval.narg = -i;
		return ARG;
	    } else {		/* it is a local variable */
		yylval.narg = i;
		return LOCAL;
	    }
	} else {
	    if (builtin_ft_check(yytext) == 0) {
	      /* Don't append (pre or post) an agent name if: 1) if a
                 built-in function, 2) if we're in the general context
                 (no >> syntax etc used), 3) the definitions are from
                 a Scout window definition, 4) similarly, if the
                 definitions are from a Donald 'within' definition, 5)
                 if the name already has a ~ prefix [Ash, with
                 Patrick] */
	      /* printf("yytext1 = %s %s %i %i %i\n", yytext, agentName,
		 appAgentName, append_agentName, append_NoAgentName); */
	       if (*tempagentname != 0 && appAgentName > 0 && append_agentName > 0
	           && append_NoAgentName > 0) {
		   /*tempyytext = yytext;
		   new_yytext[0] = '\0';
                   if (yytext[0] == '_' && inPrefix) {
                     strcat(new_yytext, "_");
                     tempyytext++;
                   }
		   if (inPrefix) strcat(new_yytext, agentName);
		      else  strcat(new_yytext, tempyytext);
		   strcat(new_yytext, "_"); *//* New double underbar for DOSTE context [Nick] */
		   /*if (inPrefix) strcat(new_yytext, tempyytext);
		      else  strcat(new_yytext, agentName);
          	   strcpy(yytext, new_yytext);
          	   yyleng = strlen(yytext);*/
 		   /*   printf("agentName = %s \n", agentName);  */
		   /* printf("yytext = %s \n", yytext); */

			/* Make a context from agent name [Nick] */
			context = doste_context(tempagentname);
	       }

		
	    }
	    end_text();

	    /* must be global [Ash] */
 	    if ((sp = lookup(yytext, context)) == 0)  {
	      /* doesn't already exist [Ash] */
	      sp = install(yytext,context, VAR, UNDEF, 0);
	      /* printf("new_yytext = %s", yytext);  */
            }
#ifdef DISTRIB
            if (!strcmp(sp->name, "higherPriority")) setHighPrior=1;
#endif /* DISTRIB */
	    yylval.sym = sp;
	    return sp->stype;
	}
    }
}

static int
multi_symbol_token(void)
{
    static struct {
	char        c1, c2;
	int         token;
    }          *tab, table[] = {
	{ '+', '=', PLUS_EQ },
	{ '+', '+', PLUS_PLUS },
	{ '-', '=', MINUS_EQ },
	{ '-', '-', MINUS_MINUS },
	{ '/', '/', SLASH_SLASH },
	{ '=', '=', EQ_EQ },
	{ '!', '=', NOT_EQ },
	{ '~', '>', TILDE_GT },
	{ '>', '=', GT_EQ },
	{ '<', '=', LT_EQ },
	{ '&', '&', LAZY_AND },
	{ '|', '|', LAZY_OR },
	/*{ ':', ':', CONTEXT },*/
	{ 0, 0, 0 }
    };

    for (tab = table; tab->c1; tab++) {
	if (*yytext == tab->c1 && nextc == tab->c2) {
	    input();
	    RETURN_TOKEN(tab->token);
	}
    }
    RETURN_TOKEN(*yytext);
}

static void
skip_comment(void)
{
    safe_input();		/* '/' has already skipped, skip '*' */
    for (;;) {
	switch (nextc) {
	case '*':
	    safe_input();	/* '*' */
	    if (nextc == '/') {
		safe_input();	/* skip '/' */
		return;
	    }
	    break;
	case '/':
	    safe_input();	/* skip '/' */
	    if (nextc == '*') {
		skip_comment();
	    }
	    break;
	case 0:		/* EOF */
	  error("unexpected end-of-file in /* */ comment");
	    return;
	default:
	    safe_input();
	}
    }
}

static void skip_one_line_comment(void) {
  /* We have '##' in the input, and the first # has already been
     skipped - skip the second #... */
  safe_input();

  /* Now nextc == character after the second #.  Swallow characters
     until the beginning of the next line.  Note that 0 (EOF) doesn't
     cause an error with a ## comment. [Ash] */
  while (nextc != '\n' && nextc != '\r' && nextc != 0)
    safe_input();
}


// 																	<<<<<<<<<------------- TODO: Need to handel this ???
static int scoutScreenInitOpened = 0;
/* Open the scout screen, but only if we haven't done it before */
void scoutScreenInitOpen(void) {
  symptr scoutScreenInitOpenSym;
  extern char *progname;

  /* Stop a second %scout reopening screen if the user closed it, and
     also stop re-entry into this function [Ash] */
  if (scoutScreenInitOpened) return;
  scoutScreenInitOpened = 1;

  makearr(0);
  scoutScreenInitOpenSym = lookup("scoutScreenInitOpen", basecontext);
  if (scoutScreenInitOpenSym) {
    call(scoutScreenInitOpenSym, pop(), 0);
  } else {
    fprintf(stderr, "%s: Can't find scoutScreenInitOpen: do you have the correct -l setting?\n", progname);
    exit(-1);
  }
}

/* notationGet: Get the name of the notation currently in effect,
   including the preceeding '%' character.  Caller has the
   responsibility of freeing the result.  [Ash] */
char * notationGet(void) {
  Datum ret;

  if (currentNotation != NEWOTHER)
    errorf("notationGet: called when currentNotation != NEWOTHER");

  makearr(0);
  pushMasterStack("notationGet");
  call(lookup("notationGet", basecontext), pop(), 0);
  popMasterStack();
  ret = pop();
  muststr(ret);

  return strdup(ret.u.s);
}

/* notationSwitch: Pass this a string which might be a notation switch
   statement (eg %edensl).  This returns non-zero if the switch to
   this new notation was successful.  Replaces Chris Brown's
   setcurrentnot function. [Ash] */
int notationSwitch(char * s) {
  Datum d, ret;
  char *t;
  extern char *libLocation;
  extern int TopEntryStack;

  if ((s[0] != '%') || !libLocation) return 0;

  /* This code leads to the Eden notationSwitch failing to compare
     7-character-long strings and find them equal, for some strange
     reason, so notations with names of length 6 don't work.

    t = getheap(strlen(s));
    strcpy(t, s);
    dpush(d, STRING, t);
  */

  dpush(d, STRING, s);
  makearr(1);
  pushMasterStack("notationSwitch");
  call(lookup("notationSwitch", basecontext), pop(), 0);
  popMasterStack();
  ret = pop();
  mustint(ret);
  /*  fprintf(stderr, "RET %d\n", ret.u.i); */

  return ret.u.i;
}

void notationPushPop(int direction) {
  Datum d;
  char *t;
  symptr nppsym;

  /* !@!@ should probably pass direction as an integer, not a string */
  t = getheap(8);
  snprintf(t, 8, "%d", direction);
  dpush(d, STRING, t);
  makearr(1);
  pushMasterStack("notationPushPop");

  nppsym = lookup("notationPushPop", basecontext);
  if (nppsym == 0)
    errorf("Eden proc 'notationPushPop' not found: was eden.eden loaded correctly?");

  call(nppsym, pop(), 0);
  popMasterStack();
  d = pop(); /* discard result */
  return;
}


// 																		<<<<<<<<<<-------------- TODO: Check this out...dose some intresting things
static void
skip_percent_comment(void)
{

#ifndef TTYEDEN
    Script     *script;
    extern Script *st_script, *dd_script;
    extern int  run(short, void *, char *);
    extern void dd_lex(int), st_lex(int);
#ifdef WANT_SASAMI
    extern Script *sa_script;
    extern void sa_input(char);
#endif /* WANT_SASAMI */
#ifdef DISTRIB
    extern void lsd_lex(int);
#endif /* DISTRIB */
#endif /* not TTYEDEN */

    /* skip_percent_comment is called when we are starting a new line
       and nextc=='%' [Ash] */

    char       *s;
    int         start = yyleng;

    safe_input();		/* read in the initial '%' */

    for (;;) {

	s = yytext + start;

	/* safe_input until we have an end-of-line or EOF */
	if ((nextc != '\n') && (nextc != '\r') && (nextc != 0)) {
	  safe_input();
	  continue;
	} else {
	  /* now we've found the end of the line, terminate it correctly */
	  end_text1();
	}

	if ((nextc == '\n') || (nextc == '\r')) {
	  if (notationSwitch(s)) {
	    char execstr[100];
	    int c;
	    
	    changeNotation(NEWOTHER);

	    /* Notation now changed to a non-default one.  Slurp in
	       the rest of the text until (just before) the next
	       change of notation and parse it.  [Ash] */
	    for (nextc = peek();
		 (nextc != '\0') && (nextc != EOF);
		 nextc = peek()) {
	      
	      if (Inp_Dev->newline && nextc == '%') {
		/* Trying to fix sqleddi "prepare to eat and then eat"
		   bug [Ash, 23rd October 2002] */
		/*
		  strcpy(execstr, "notationChar('\\\n');");
		  run(STRING_DEV, execstr, 0);
		*/
		
		nextc = '\n';
		return;
	      } else if (Inp_Dev->newline && nextc == '#') {
		c = keyin();
		if (nextc == '#') {
		  /* "##" comment input: ignore to end of line */
		  do {c = keyin();} while (!Inp_Dev->newline);
		  
		} else {
		  /* input line starts with #, but second character is
		     not #.  Pass the first # to the Eden parser, and
		     parse the second character the next time round
		     the loop. */
		  strcpy(execstr, "notationChar('#');");
		  run(STRING_DEV, execstr, 0);
		}
	      } else {
		/* Something in the sqleddi model causes C's idea
                   about the status of the current notation to get
                   confused.  As this piece of code is executing, we
                   must be in a NEWOTHER notation, so set it
                   explicitly.  [Ash, 24th October 2002] */
#ifdef TTYEDEN
		changeNotation(NEWOTHER);
#endif
		c = keyin();
		/* fprintf(stderr, "KEYIN %d\n", c); */
		
		if (c == '\'')
		  strcpy(execstr, "notationChar('\\\'');");
		else if (c == '\\')
		  strcpy(execstr, "notationChar('\\\\');");
		else
		  sprintf(execstr, "notationChar('%c');", c);
		
		run(STRING_DEV, execstr, 0);
	      }
	      
	    } /* end for */
	    
#ifdef TTYEDEN
	    /* ttyeden does not currently keep track of the current
	       notation properly.  It should still be %eddi after an
	       included file has changed to that notation and not back
	       again, but this would require some work on the lexer.
	       At least we can ensure that the prompt is correct here.
	       [Ash] */
	    /*
	      if ((nextc == EOF) || (nextc == '\0')) {
	      fprintf(stderr, "EOF FUDGE\n");
	      notationSwitch("%eden");
	      currentNotation = EDEN;
	      }
	    */

	    currentNotation = EDEN;
	    /* peek() does not seem to manage to successfully push
	       back the EOF character in TTYEDEN and so keyin() would
	       cause another character to be read.  Stop immediately
	       instead.  [Ash] */
	    return;
#else
	    keyin();	/* read the null character */
	    /* treat end-of-file as end-of-line */
	    /* the code feeding the parser with \n below is a
	       reinterpretation of something Chris Brown put in, but
	       it doesn't seem to be required any more */
	    /*
	     * strcpy(execstr, "notationChar('\\n');");
	     * run(STRING_DEV, execstr, 0);
	     */
	    setprompt();
#endif
	    
	  } /* end if notationSwitch(s) */


#ifndef TTYEDEN
	  /* DoNaLD -------------------------------------------- */
	  /* %donald0 is an alternative alias for %donald, so that we
	     can replace %donald with a new translator but still
	     access this builtin as %donald0 */
	  else if ((strncmp(s, "%donald0", 8) == 0) ||
		   (strncmp(s, "%donald", 7) == 0)) {
	    changeNotation(DONALD);
	    dd_script = script = newScript();
	    for (nextc = peek(); nextc != '\0'; nextc = peek()) {
	      if (Inp_Dev->newline && (nextc == '%' || nextc == '>'
				       || nextc == '<')) {
		if (nextc == '>' || nextc == '<') {
		  input();
		  getAgentName();
		}
		else {
		  nextc = '\n';
		  if (script->text[0] != '\0') {
		    appAgentName--;
		    /* printf("from donald 1"); */
		    pushEntryStack(DONALD);
		    run(STRING_DEV, script->text, 0);
		    popEntryStack();
		    appAgentName++;
		  }
		  deleteScript(script);
		  return;
		} /* end if (nextc == '>' || nextc == '<') */
	      } /* end if (Inp_Dev->newline...) */
	      else {
		dd_lex(keyin());
		if (script->ready) {
		  if (script->text[0] != '\0') {
		    appAgentName--;
		    /* printf("from donald 2"); */
		    pushEntryStack(DONALD);
		    run(STRING_DEV, script->text, 0);
		    dd_script = script;
		    popEntryStack();
		    /* printf("from donald 3"); */
		    appAgentName++;
		  }
		  resetScript(script);
		} /* end if (script->ready) */
	      } /* end if (Inp_Dev->newline...) (two versions) */
	    } /* end for (nextc = peek()...) */


	    keyin();	/* read the null character */
	    dd_lex('\n');	/* treat end-of-file as end-of-line */

	    if (script->text[0] != '\0') {
	      appAgentName--;
	      pushEntryStack(DONALD);
	      run(STRING_DEV, script->text, 0);
	      popEntryStack();
	      /* printf("from donald 4"); */
	      appAgentName++;
	    }
	    deleteScript(script);
	  } /* end if (strncmp(s, "%donald", 7) == 0) */


	  /* SCOUT -------------------------------------------- */
	  else if (strncmp(s, "%scout", 6) == 0) {	/* is SCOUT */
	    scoutScreenInitOpen();
	    changeNotation(SCOUT);
	    st_script = script = newScript();
	    for (nextc = peek(); nextc != '\0'; nextc = peek()) {
	      if (Inp_Dev->newline && (nextc == '%' || nextc == '>'
				       || nextc == '<')) {
		if (nextc == '>' || nextc == '<') {
		  input();
		  getAgentName();
		} else {
		  nextc = '\n';
		  if (script->text[0] != '\0') {
		    appAgentName--;
		    /* printf("from scout 1"); */
		    pushEntryStack(SCOUT);
		    run(STRING_DEV, script->text, 0);
		    popEntryStack();
		    appAgentName++;
		  }
		  deleteScript(script);
		  return;
		}
	      } /* end if (Inp_Dev->newline...) */
	      else {
		st_lex(keyin());
		if (script->ready) {
		  if (script->text[0] != '\0') {
		    appAgentName--;
		    /* printf("from scout 2 %s\n", script->text); */
		    pushEntryStack(SCOUT);
		    run(STRING_DEV, script->text, 0);
		    st_script = script;
		    popEntryStack();
		    appAgentName++;
		  }
		  resetScript(script);
		}
	      }
	    } /* end for (nextc = peek();... */

	    keyin();	/* read the null character */
	    st_lex('\n');	/* treat end-of-file as end-of-line */
	    if (script->text[0] != '\0') {
	      appAgentName--;
	      pushEntryStack(SCOUT);
	      run(STRING_DEV, script->text, 0);
	      popEntryStack();
	      appAgentName++;
	    }
	    deleteScript(script);
	  } /* end if (strncmp(s, "%scout", 6)... */

#ifdef WANT_SASAMI

	  /* Sasami -------------------------------------------- */

	  else if (strncmp(s, "%sasami", 7) == 0) {
	    changeNotation(SASAMI);
	    sa_script = script = newScript();
	    for (nextc = peek(); nextc != '\0'; nextc = peek())	{
	      if (Inp_Dev->newline && (nextc == '%' || nextc == '>' ||
				       nextc == '<')) {
		/* This is a %, > or < statement and shouldn't be
                   passed to Sasami */
		if (nextc == '>' || nextc == '<') {
		  input();
		  getAgentName();
		} else {
		  nextc = '\n';
		  if (script->text[0] != '\0') {
		    appAgentName--;
		    /* printf("from sasami 1"); */
		    pushEntryStack(SASAMI);
		    run(STRING_DEV, script->text, 0);
		    popEntryStack();
		    appAgentName++;
		  }
		  deleteScript(script);
		  return;
		}
	      } else {
		/* This must be something worth sending to Sasami */
		sa_input(keyin());
		/* Now run the EDEN script that Sasami's produced (if
                   there is one) */
		if (script->ready) {
		  if (script->text[0] != '\0') {
		    appAgentName--;
		    /* printf("from sasami 2"); */
		    pushEntryStack(SASAMI);
		    run(STRING_DEV, script->text, 0);
		    sa_script = script;
		    popEntryStack();
		    /* printf("from sasami 3"); */
		    appAgentName++;
		  }
		  resetScript(script);
		}
	      }
	    }

	    /* OK - finished with the input, so send an EOL to make
	       sure that the parser runs the last input line, and
	       exit. */

	    keyin();	/* read the null character */

	    sa_input('\n');	/* treat end-of-file as end-of-line */

	    if (script->text[0] != '\0') {
	      appAgentName--;
	      pushEntryStack(SASAMI);
	      run(STRING_DEV, script->text, 0);
	      popEntryStack();
	      /* printf("from sasami 4"); */
	      appAgentName++;
	    }
	    deleteScript(script);
	  } /* end if (strncmp(s, "%sasami", 7) == 0) */
#endif /* WANT_SASAMI */

#ifdef DISTRIB
	  /* LSD -------------------------------------------- */
	  else if (strncmp(s, "%lsd", 4) == 0) {
	    changeNotation(LSD);
	    for (nextc = peek(); nextc != '\0'; nextc = peek()) {
	      if (Inp_Dev->newline && (nextc == '%' || nextc == '>'
				       || nextc == '<')) {
		if (nextc == '>' || nextc == '<') {
		  input();
		  getAgentName();
		} else {
		  nextc = '\n';
		  return;
		}
	      } else {
		lsd_lex(keyin());
	      }
	    }
	    keyin();	/* read the null character */
	    lsd_lex('\n');	/* treat end-of-file as end-of-line */
	  } /* end if (strncmp(s, "%lsd", 4)... */
#endif /* DISTRIB */

	  /* This push and pop facility ("%+eden") appears be used
             only in Donald [Ash] */

	  /* Push entry -------------------------------------------- */
	  else if (s[1] == '+') {
	    /*  appAgentName=0; */
	    if ((strncmp(s + 2, "donald0", 7) == 0) ||
		(strncmp(s + 2, "donald", 6) == 0)) {	/* is DoNaLD */
	      pushEntryStack(DONALD);
	    } else if (strncmp(s + 2, "scout", 5) == 0) {	/* is SCOUT */
	      pushEntryStack(SCOUT);
	    } else if (strncmp(s + 2, "eden", 4) == 0) {	/* is EDEN */
	      pushEntryStack(EDEN);
#ifdef WANT_SASAMI
	    } else if (strncmp(s + 2, "sasami", 6) == 0) {	/* is Sasami */
	      pushEntryStack(SASAMI);
#endif /* WANT_SASAMI */
	    }
	  } /* end if (s[1] == '+') */


	  /* Pop entry -------------------------------------------- */
	  else if (s[1] == '-') {
	    if ((strncmp(s + 2, "donald0", 7) == 0)
		|| strncmp(s + 2, "donald", 6) == 0
		|| strncmp(s + 2, "scout", 5) == 0	/* is SCOUT */
#ifdef WANT_SASAMI
		|| strncmp(s + 2, "sasami", 6) == 0	/* is Sasami */
#endif
		|| strncmp(s + 2, "eden", 4) == 0) {	/* is EDEN */
	      popEntryStack();
	      /*  appAgentName=1; */
	    }
	  } /* end if (s[1] == '-') */

#endif /* not TTYEDEN */

	  /* Eden -------------------------------------------- */
	  else {
	    changeNotation(EDEN);
	  }

	  return;

	} else if (nextc == 0) {
	  /* EOF */
#ifdef TTYEDEN
	  /* Dummy if start to make the else's below work... [Ash] */
	  if (0) {}
#else
	  if ((strncmp(s, "%donald0", 8) == 0) ||
	      (strncmp(s, "%donald", 7) == 0)) {	/* is DoNaLD */
	    changeNotation(DONALD);
	  }
	  else if (strncmp(s, "%scout", 6) == 0) {	/* is SCOUT */
	    changeNotation(SCOUT);
	  }
#ifdef WANT_SASAMI
	  else if (strncmp(s, "%sasami", 7) == 0) {	/* is Sasami */
	    changeNotation(SASAMI);
	  }
#endif /* WANT_SASAMI */
#endif /* not TTYEDEN */
          else if (notationSwitch(s)) {
	    changeNotation(NEWOTHER);
	  } else {		/* is EDEN */
	    changeNotation(EDEN);
	  }
	  return;
	} /* end if nextc==0 */
    } /* end for ;; */
} /* end skip_percent_comment */

static void
backslash(char *text)
{				/* interpret backslashs in a string */
    int         i, n;
    char       *s = text;
    static char transtab[] = "b\bf\fn\nr\rt\t";

    while (*s) {
	if (*s == '\\') {
	    s++;
	    if (isdigit(*s)) {
		for (n = 3, i = 0; n && isdigit(*s); --n, s++)
		    i = (i << 3) | (*s - '0');
		*text++ = i;
	    } else if (islower(*s) && strchr(transtab, *s))
		*text++ = strchr(transtab, *s++)[1];
	    else
		*text++ = *s++;
	} else {
	    *text++ = *s++;
	}
    }
    *text = '\0';
}

int
yylex(void)
{
    char       *s;
    uDatum	u;
#ifdef DISTRIB
    extern Int *higherPriority;
#endif /* DISTRIB */

    /* initial for a token */
    clear_text();

    if (bof) {
	bof = 0;
	if (nextc == '%') {	/* comment line */
	    skip_percent_comment();/* return next token */
	    end_text();
	    clear_text();
	}
	/* printf("bof \n"); */
	if (nextc == '>' || nextc == '<') getAgentName();
    }

restart:

    while (isspace(nextc) && (nextc != '\n') && (nextc != '\r')) {
      /* skip blanks */
      input();
    }

    if ((nextc == '\n') || (nextc == '\r')) {
      input();
      if (nextc == '%') {	/* comment line */
	skip_percent_comment();/* return next token */
	end_text();
	clear_text();
      }
      /* printf("restart\n"); */
      if (nextc == '>' || nextc == '<') getAgentName();
      
      goto restart;
    }
    end_text();
    clear_text();

    if (nextc == 0) {		/* EOF */
#ifdef DISTRIB
        if (setHighPrior) { *higherPriority=0; setHighPrior = 0; }
#endif
	return 0;		/* don't do input() so next token is EOF */
    }

#ifdef TTYEDEN
    prompt = promptsemi;
#endif

    if (isdigit(nextc) || nextc == '.')	/* a number or a dot */
	return number_token();

    if (isalpha(nextc) || nextc == '_')	/* a keyword or an identifier */
      {
        append_agentName = 1;
	return id_token();
      }

    /*if (nextc == '^') {
	input();
	append_agentName = 1;
	return context_token();
	}*/

    if (nextc == '\'') {
	while (input(), nextc != '\'') {
	    if (nextc == '\\')
		input();
	    else if (nextc == 0)
		error("unexpected end-of-file in character constant");
	}
	input();		/* read the quote */

	/* mustn't alter the following two lines */
	end_text();		/* accept the text */
	last_char = '\0';	/* delete the last quote */

	backslash(yytext + 1);	/* resolve the backslashs */
	if (yytext[2])		/* more than one char */
	    error("single char expected");
	u.i = yytext[1];
	yylval.dp = makedatum(MYCHAR, u);
	return CONSTANT;
    }
    if (nextc == '\"') {
#ifdef TTYEDEN
        prompt = promptchar;
#endif
	while (input(), nextc != '\"') {
	    if (nextc == '\\')
		input();
	    else if (nextc == 0)
		error("unexpected end-of-file in string constant");
	}
	input();		/* read the quote */

	/* mustn't alter the following two lines */
	end_text();		/* accept the text */
	last_char = '\0';	/* delete the last quote */

	backslash(yytext + 1);	/* resolve the backslashs */
	s = (char *) emalloc(strlen(yytext));
	strcpy(s, yytext + 1);
	u.s = s;
	yylval.dp = makedatum(STRING, u);
	return CONSTANT;
    }
    input();			/* accept the char, must be the 1st char */

    switch (*yytext) {		/* the 1st char */
    case '@':
      yylval.dp = makedatum(UNDEF, u);  /* bug fix Ash + Carters */
      RETURN_TOKEN(CONSTANT);
      break;

    case '~':                   /* for agency  --sun */
         if (isalpha(nextc) || nextc == '_') {
            clear_text();
            append_agentName = 0;
            return id_token();
         }
         break;

    case '$':
	while (isdigit(nextc))
	    input();
	if (yyleng == 1)
	    RETURN_TOKEN('$');
	yylval.narg = atoi(yytext + 1);
	RETURN_TOKEN(ARG);

    case '/':
#ifdef TTYEDEN
        prompt = promptcomment;
#endif
	if (nextc == '*') {	/* nested comment */
	    skip_comment();	/* return next token */
	    end_text();
	    clear_text();
#ifdef TTYEDEN
	    prompt = prompt1;
#endif
	    goto restart;
	}
	break;

    case '#':
      if (nextc == '#') {
	skip_one_line_comment();
	end_text();
	clear_text();
#ifdef TTYEDEN
	prompt = prompt1;
#endif
	goto restart;
      }
      break;

    }

    return (multi_symbol_token());
}

/* Figure out the prompt to be used at this moment for ttyeden.
   Caller has the responsibility of freeing the result.  [Ash] */
#ifdef TTYEDEN
char *fullPrompt() {
  char *toReturn, *notationName=0;

  toReturn = emalloc(30);

  /*
  notationName = notationGet();
  sprintf(toReturn, "%s%s%d%s",
	  notationName,
	  ((strlen(notationName) > 0) ? " " : ""),
	  Inp_Dev->lineno + 1, prompt);
  free(notationName);
  */

  if (currentNotation == NEWOTHER) {
    notationName = notationGet();
    sprintf(toReturn, "%s %d%s", notationName, Inp_Dev->lineno + 1, prompt);
    free(notationName);

  } else {
    /* notation is Eden, so need not be included in the prompt */
    sprintf(toReturn, "%d%s", Inp_Dev->lineno + 1, prompt);

  }

  return toReturn;
}
#endif /* TTYEDEN */

#if defined(TTYEDEN) && defined(HAVE_READLINE)
/* rl_gets is taken straight from the readline info manual [Ash] */

static char *line_read = (char *)NULL;
/* Read a string, and return a pointer to it.  Returns NULL on EOF. */
char *
rl_gets ()
{
  char *fullPromptText;
  extern symptr eden_prompt_sym;

  /* If the buffer has already been allocated, return the memory
     to the free pool. */
  if (line_read)
    {
      free (line_read);
      line_read = (char *)NULL;
    }

  /* Get a line from the user. */
  if (eden_prompt_sym->d.u.i) {
    fullPromptText = fullPrompt();
    line_read = readline(fullPromptText);
    free(fullPromptText);
  } else {
    //rl_expand_prompt("");	/* work around bug in readline4-4.2
    //                               reported to Debian as bug #105231 */
    line_read = readline("");
  }

  /* If the line has any text in it, save it on the history. */
  if (line_read && *line_read)
    add_history (line_read);

  return (line_read);
}

/* Simulate a call to getc(stdin), but actually call rl_gets whenever
   necessary to use readline instead.  Like getc(stdin), EOF is
   returned if Ctrl-D is pressed, and \n is returned at the end of
   each line. [Ash] */
static int getc_require_new_readline = 1;
static char *getc_ptr = (char *)NULL;
int
rl_getc_wrapper() {
  if (getc_require_new_readline)
    getc_ptr = rl_gets();

  if (getc_ptr == (char *)NULL)
    return EOF;

  if (*getc_ptr == '\0') {
    getc_require_new_readline = 1;
    *getc_ptr = '\n';
  } else
    getc_require_new_readline = 0;

  return *getc_ptr++;
}

/* Simulate a call to ungetc(getc(stdin)), but actually use rl_gets to
   use readline instead.  This is used in peek().  [Ash] */
int rl_getungetc() {
  int c;

  if (getc_require_new_readline) {
    getc_ptr = rl_gets();
    getc_require_new_readline = 0;
  }

  if (getc_ptr == (char *)NULL)
    return EOF;

  if (*getc_ptr == '\0')
    return '\n';

  return *getc_ptr;
}

int
rl_getc_wouldBlock() {
  if (getc_ptr == (char *)NULL) return 0;
  if (*getc_ptr == '\0') return 1;
  else return 0;
}
#endif /* TTYEDEN && HAVE_READLINE */

/* Append c to Inp_Dev->linebuf, expanding the allocation if
   necessary, incrementing the Inp_Dev->linebufend pointer and keep
   linebuf terminated with a '\0'.  [Ash] */
void append_linebuf(char c) {
  if ((Inp_Dev->linebufend+2) >= Inp_Dev->linebufsize) {
    Inp_Dev->linebufsize *= 2;
    Inp_Dev->linebuf = erealloc(Inp_Dev->linebuf, Inp_Dev->linebufsize);
  }
  Inp_Dev->linebuf[++Inp_Dev->linebufend] = c;
  Inp_Dev->linebuf[Inp_Dev->linebufend+1] = '\0'; /* keep the string
                                                     terminated */
}

int
keyin(void)
{				/* READ A CHAR FROM INPUT DEVICE */
    int         c;
    FILE       *filein;

    switch (Inp_Dev->type) {
    case FILE_DEV:
      filein = (FILE *) Inp_Dev->ptr;
#ifdef TTYEDEN
      print_prompt_if_necessary(2);
      preprinted = FALSE;

#ifdef HAVE_READLINE
      /* TTYEDEN and HAVE_READLINE */
      if (filein == stdin && Inp_Dev->usereadline)
	c = rl_getc_wrapper();
      else
#endif
        /* TTYEDEN but don't HAVE_READLINE or not usereadline */
        c = getc(filein);
#else
      /* not TTYEDEN */
      c = getc(filein);
#endif
      if (c == EOF)
	c = 0;
      break;

    case STRING_DEV:
      c = *(Inp_Dev->ptr)++;
      break;
    }

    if (Inp_Dev->newline) {
      /* this is a /character after/ newline */
      Inp_Dev->lineno++;
      Inp_Dev->linebufend = -1;
      Inp_Dev->charno = -1;
    }
    append_linebuf(c);
    Inp_Dev->newline = ((c == '\n') || (c == '\r'));

    Inp_Dev->charno++;		/* keep this in sync with linebufend
                                   as long as we are not doing
                                   flushRestOfLine() */
    return c;
}

/* Read the rest of the line into linebuf so we can print it all in an
   error condition [Ash] */
void flushRestOfLine(void) {
    FILE       *filein;
    int c = 'a';
    extern int wouldBlock(void);

    if (Inp_Dev->newline) {
      Inp_Dev->linebuf[Inp_Dev->linebufend] = '\0'; /* clear the
						       unwanted \n */
      return;	   /* already have the complete line */
    }

    /* This case can occur if tkeden is interrupted with ctrl-c */
    if (!(Inp_Dev->ptr)) return;

    /* This wouldBlock stuff might be invalidated by the above newline
       logic, I'm not sure.  I'll leave it in anyway.  [Ash] */
    if (Inp_Dev->type == FILE_DEV) {
      filein = (FILE *) Inp_Dev->ptr;
      /* if the whole line has already been read in, then doing a
	 getc() would cause a block */
      if (filein == stdin) {
#if defined(TTYEDEN) && defined(HAVE_READLINE)
	if (rl_getc_wouldBlock()) return;
#else
	if (wouldBlock()) return;
#endif
      }
    }

    while (isprint(c)) {
      switch (Inp_Dev->type) {
      case FILE_DEV:
#ifdef TTYEDEN
#ifdef HAVE_READLINE
	if (filein == stdin && Inp_Dev->usereadline)
	  c = rl_getc_wrapper();
	else
#endif /* HAVE_READLINE */
	  c = getc(filein);
#else
	c = getc(filein);
#endif
	break;

      case STRING_DEV:
	c = *(Inp_Dev->ptr)++;
	break;
      }

      append_linebuf(c);
    }

    /* Last character read was invalid */
    Inp_Dev->linebuf[Inp_Dev->linebufend] = '\0';
}

/* Examine the next char to come from the input device, but leave it
   in the input buffer to actually be processed by keyin().  Note this
   can cause the process to become blocked.  [Ash] */
int
peek(void)
{
    int         c;
    FILE       *filein;

    switch (Inp_Dev->type) {
    case FILE_DEV:
      filein = (FILE *) Inp_Dev->ptr;
#ifdef TTYEDEN
      print_prompt_if_necessary(3);
#endif
#if defined(TTYEDEN) && defined(HAVE_READLINE)
      if (filein == stdin && Inp_Dev->usereadline)
	c = rl_getungetc();
      else
	/* not stdin or not usereadline */
        ungetc(c = getc(filein), filein);
#else
      /* not TTYEDEN or don't HAVE_READLINE */
      ungetc(c = getc(filein), filein);
#endif
      if (c == EOF)
	c = 0;
      break;

    case STRING_DEV:
      c = *(Inp_Dev->ptr);
      break;
    }
    return c;
}

/* For VA [Ash, with Patrick] */
static void
getAgentName(void) {
   extern symptr lookup(char *, int);
   symptr sp;
   char secondC, firstC;

      *agentType = '\0';
      if ((firstC = nextc) == '>') inPrefix = 1;
         else  inPrefix = 0;
      agentType[0] = firstC;
      input();     /* skip second '>' */
      secondC = nextc;
      agentType[1] = secondC;
      agentType[2] = '\0';
      input();
      clear_text();
      while (isalnum(nextc) || nextc == '_')
        input();
      end_text1();
      *agentName = '\0';
      /* printf("secondc = %c \n", secondC); */
      if (yytext[0] == '\0') {
#ifndef TTYEDEN
	/* Show the agent name on the interface [Ash] */
          Tcl_VarEval(interp, ".agentName config -text {}", 0);
#endif
          clear_text();
          return;
      }
      if (secondC=='>' || secondC=='~') {
           /* printf("getagentName = %s \n", yytext); */
           strcpy(agentName, yytext);
#ifndef TTYEDEN
           Tcl_VarEval(interp, ".agentName config -text {current agent: ", agentName, "}", 0);
#endif
           /*strcat(agentName, "_");*/
           clear_text();
           if (secondC=='~') append_NoAgentName = 0;
           else append_NoAgentName = 1;
    	   return;
      }
      if (secondC=='<') {
        append_NoAgentName = 1;
        if ((sp = lookup(yytext, basecontext)) == 0) {
#ifndef TTYEDEN
           Tcl_VarEval(interp, ".agentName config -text {}", 0);
#endif
           clear_text();
           return;
        } else {
           switch(sp->d.type) {
             case  INTEGER:
                  sprintf(agentName, "%d", sp->d.u.i); break;
             case  MYCHAR:
                  sprintf(agentName, "%c", sp->d.u.i); break;
             case  STRING:
                  sprintf(agentName, "%s", sp->d.u.s); break;
             default:
               error2("expecting string, char or integer for virtual agent", sp->name);
           }
           /* printf("getagentName = %s \n", sp->d.u.s); */

           /* strcpy(agentName, sp->d.u.s); */
#ifndef TTYEDEN
           Tcl_VarEval(interp, ".agentName config -text {current agent: ", agentName, "}", 0);
#endif
           /*strcat(agentName, "_");*/
           clear_text();
    	   return;
    	}
      }
}
