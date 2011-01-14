%{
/*
 * $Id: parser.y,v 1.6 2001/07/27 16:41:17 cssbz Exp $
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

/* This is from the Autoconf manual. Note the pragma directive is
   indented so that pre-ANSI C compilers will ignore it. [Ash] */
/* AIX requires this to be the first thing in the file */
#ifdef __GNUC__
# define alloca __builtin_alloca
#else
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
 #pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#endif

static char rcsid[] = "$Id: parser.y,v 1.6 2001/07/27 16:41:17 cssbz Exp $";

#include "../config.h"
#include <stdio.h>
#include <string.h>

#include "Obs.q.h"
#include "LSDagent.q.h"
#include "../Eden/eden.h"

#include "../Eden/emalloc.h"
#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

#define YYERROR_VERBOSE 1

/* function prototypes */
void        LSD_err(char *);
void        yyerror(char *);

char        LSDErrorStr[80];
extern void   declare_LSDagent(char *);
extern void   declare_ObsQUEUE(int);
extern void   add_ObsQUEUE(char *);
extern void   remove_ObsQUEUE(char *);


%}
%union {
	int		 i;     /* number of arguments */
	char            *s;     /* string */
	Obs_QUEUE       *O;     
}

%token <i> HANDLE ORACLE STATE   
%token <s> ID
%token     AGENT REMOVE

%type <s> id agent_id 
%type <O> obs_list
%left ','

%%
program : 
           statement 
         | program statement 
         ; 
     
statement : 
        | AGENT  agent_id              { declare_LSDagent($2);}
        | typename obs_list          
        | REMOVE typename rmobs_list      
        ;

typename : 
        ORACLE     { declare_ObsQUEUE(ORACLE);}
      | HANDLE     { declare_ObsQUEUE(HANDLE);}
      | STATE      { declare_ObsQUEUE(STATE);}
      ;

obs_list :
        id                 { add_ObsQUEUE($1); }
      | obs_list ',' id     { add_ObsQUEUE($3); }
      ;
      
rmobs_list :
        id                 { remove_ObsQUEUE($1); }
      | rmobs_list ',' id     { remove_ObsQUEUE($3); }
      ;
      
agent_id :
        id
        ;
        
id : 
     ID               {/* printf("ID %s", $1); */}
   ;
%%


#include <tk.h>
extern Tcl_Interp *interp;


void
LSD_err(char *s)
{
    extern void yyrestart(void);
    extern int  yy_parse_init;
    Tcl_DString err, message;

    yyrestart();		/* reset lexical analyzer */
    yy_parse_init = 1;		/* reset bison */

    /* deleteScript(st_script); */

    Tcl_EvalEC(interp, "appendHist {/*** }");
    Tcl_DStringInit(&err);
    Tcl_DStringInit(&message);
    Tcl_DStringAppend(&message, "LSD: ", -1);
    Tcl_DStringAppend(&message, s, -1);
    Tcl_DStringAppendElement(&err, "appendHist");
    Tcl_DStringAppendElement(&err, message.string);
    Tcl_EvalEC(interp, err.string);
    Tcl_DStringFree(&err);
    Tcl_DStringFree(&message);
    Tcl_EvalEC(interp, "appendHist { ***/\n}");

    error2("LSD translator error", "");
}

void
yyerror(char *s)
{
    LSD_err("parse error");
}
