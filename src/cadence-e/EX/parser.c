/*
 * $Id: parser.c,v 1.14 2001/09/27 14:07:23 cssbz Exp $
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

static char rcsid[] = "$Id: parser.c,v 1.14 2001/09/27 14:07:23 cssbz Exp $";

#include "../../../../../config.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <tk.h>

extern Tcl_Interp *interp;
extern void change_prompt(void);
extern char agentName[128];  /*  sun */

#include "script.h"
#include "../Eden/notation.h"
#include "../Eden/eden.h"
#include "../Eden/runset.h"

int         lineno = 1;		/* starting line number = 1	 */
int         prompt_on = 1;

#define STRING_DEV      0

extern notationType currentNotation;
extern char *notationGet();

char       currentAgentName[128] = " ";
char       currentAgentType[3] = ">>";
/* EXPERIENCE: connecting to a TCL variable, you have to use a pointer
   rather than an array. --sun */
char       *currentAgentNamePtr = 0;
char       *currentAgentTypePtr = 0;
char       *current_Notation = "%eden";  /* sun - different from
                                            currentNotation (mind the
                                            underbar!) */
char       *otherNotation = 0;	/* added by Chris Brown, 2000 */

Script     *curScript;

/* function prototypes */
void        changeNotation(notationType);
void        setprompt(void);
static void distribute(char *, Script *);
void        evaluate(char *, char *);
#ifdef DISTRIB
int         getNotation(ClientData, Tcl_Interp *, int, char *[]);  /* sun */
static void getClientName(char *, Script *);
static int  readIn;
static char clientName[128];
#endif /* DISTRIB */

#ifdef DISTRIB
/* To enable us to pack data in its own notation to enable it to be
   sent between machines (or clients... :) [Ash] */
int             /* for distributed tkeden --sun */
getNotation(ClientData clientData, Tcl_Interp * interp, int argc,
	    char *argv[])
{
    extern char agentType[3];

    if (currentNotation == EDEN)
	current_Notation = "%eden";
    if (currentNotation == DONALD)
	current_Notation = "%donald";
    if (currentNotation == SCOUT)
	current_Notation = "%scout";
    if (currentNotation == LSD)
        current_Notation = "%lsd";

    /* added by Chris Brown, 2000 */        
    if (currentNotation == NEWOTHER) {
        if (otherNotation != (char *) 0) {
            free (otherNotation);
            otherNotation = (char *) 0;
        }

	otherNotation = notationGet();
	/* getnotcomment (&otherNotation); */

        if (otherNotation != (char *) 0)
            current_Notation = otherNotation;
    }

#ifdef WANT_SASAMI
    if (currentNotation == SASAMI)
      current_Notation = "%sasami";
#endif

    /* currentAgentName = agentName;
    currentAgentType = agentType; */
    return TCL_OK;
}
#endif /* DISTRIB */

void
setprompt(void)
{
    char        label[128];
    char       *stmtType; /* the contents of the label above the Input
                             window [Ash] */
    extern char *dd_prompt;
    extern char agentType[3];
#ifdef DISTRIB
    extern char *LSDagentName;
#endif /* DISTRIB */

    if (currentNotation == EDEN) {
	current_Notation = "%eden";
	stmtType = "Enter EDEN statements:";
    }

    if (currentNotation == NEWOTHER) {
      if (otherNotation != (char *) 0) {
        free(otherNotation);
        otherNotation = (char *) 0;
      }

      otherNotation = notationGet();
      if (otherNotation != (char *) 0) {
	current_Notation = otherNotation;
      
	/* Put the name of the notation into the prompt [Ash] */
	strcpy(label, "Enter ");
	strcat(label, otherNotation);
	strcat(label, " statements:");
	stmtType = label;
      } else {
	/* This probably shouldn't ever be reached, but leave it here
           for safety... [Ash] */
	stmtType = "Enter statements:";
      }

    }

#ifdef WANT_SASAMI
    if (currentNotation == SASAMI) {
	current_Notation = "%sasami";
	strcpy(label,"Enter Sasami statements:");
	stmtType = label;
    }
#endif

    if (currentNotation == DONALD) {
	current_Notation = "%donald";
	change_prompt();	/* update string for DoNaLD context */
	strcpy(label, "Enter DoNaLD statements (in context ");
	strcat(label, dd_prompt);
	strcat(label, "):");
	stmtType = label;
    }

    if (currentNotation == SCOUT) {
	current_Notation = "%scout";
	stmtType = "Enter SCOUT statements:";
    }

#ifdef DISTRIB
    if (currentNotation == LSD) {
        current_Notation = "%lsd";
        if (LSDagentName == (char *) 0)
	   sprintf(label, "Enter LSD description:");
        else
	   sprintf(label, "Enter LSD description for AGENT %s: ",
	                  LSDagentName);
	stmtType = label;
    }
#endif /* DISTRIB */

    Tcl_VarEval(interp, ".prompt config -text {", stmtType, "}", 0);
    Tcl_VarEval(interp, "set notation {", current_Notation, "}", 0);

    /* Notation seems able to be changed here. mainly it seems from eden to donald and back again. So we need to send these updates about notation changes
     * on top of those being sent from the eden.eden files notationSwitch action */
#ifdef WEDEN_ENABLED
    sendCompleteFormattedWedenMessage("<b><item type=\"notation\" operation=\"switch\"><notationname>%s</notationname></item></b>", (current_Notation+1));
#endif
    /* Display the current VA above the Input window [Ash] */
    if (*agentName) strcpy(currentAgentName, agentName);
       else strcpy(currentAgentName, "");
    if (*agentType) strcpy(currentAgentType, agentType);
       else strcpy(currentAgentType, ">>");
}

#ifdef DISTRIB
/* This implements some wierd functionality for Meurig, sending
   definitions to clients using a >>$ type notation.  It probably
   isn't needed any more, but a little tricky to delete it.  [Ash,
   with Patrick] */
static void
getClientName(char *str, Script *script)
{
   extern symptr lookup(char *);
   symptr sp;
   char     *s1= str;
   char     *s2 =0;
   char       *sptr;
   int         i;
   char     clientVarName[128];

   if (strncmp(s1, ">$", 2) == 0 || strncmp(s1, "<$", 2) == 0) {
     s2=s1;
     s1++;
     s1++;
     for (sptr = s1, i = 0; *s1 != '\0' && *s1 != '\n'; s1++, i++);
     if (strncmp(s2, ">$", 2) == 0) {
       strncat(clientName, sptr, i);
     } else {
       *clientVarName=0;
       strncpy(clientVarName, sptr, i);
       if ((sp=lookup(clientVarName)) == 0 || i==0) {
	 *clientName=0;
       } else {
	 strcpy(clientName, sp->d.u.s);
       }
     }
     readIn=0;
   }
   if (strncmp(s1, "$$", 2) == 0) {
     *clientName=0;
     readIn=1;
   }
   if (strncmp(s1, ">>$", 3) == 0 || strncmp(s1, "><$", 3) == 0) {
     s2=s1;
     s1 = s1+3;
     for (sptr = s1, i = 0; *s1 != '\0' && *s1 != '\n'; s1++, i++);
     if (strncmp(s2, ">>$", 3) == 0) {
       strncpy(clientName, sptr, i);
     } else {
       *clientVarName=0;
       strncpy(clientVarName, sptr, i);
       if ((sp=lookup(clientVarName)) == 0 || i==0) {
	 *clientName=0;
       } else {
	 strcpy(clientName, sp->d.u.s);
       }
     }
     appendnEden(">>", script, 2);
     appendEden(clientName, script);
     appendnEden("\n", script, 1);
     readIn=1;
   }
}
#endif /* DISTRIB */

static void
distribute(char *str, Script * script)
{
    char       *s = str;
    char       *sptr;
    char       *notname;
    int         i;
#ifdef DISTRIB
    char       sentData[2048];
#endif /* DISTRIB */

    /* Ensure the script is in the correct notation [Ash] */
    switch (currentNotation) {
    case DONALD:
	appendEden("%donald\n", script);
	break;
    case SCOUT:
	appendEden("%scout\n", script);
	break;

    case NEWOTHER:
      notname = notationGet();
      appendEden(notname, script);
      appendEden("\n", script);
      free(notname);
      break;

#ifdef WANT_SASAMI
    case SASAMI:
	appendEden("%sasami\n", script);
	break;
#endif /* WANT_SASAMI */
#ifdef DISTRIB
    case LSD:
	appendEden("%lsd\n", script);
#endif /* DISTRIB */
    default:
      break;
    }

    while (*s) {
        /* If this line is a %notation line, update our idea of which
           notation we are currently using.  A %notation line is
           appended for the current notation for every line above
           [Ash] */
	if (strncmp(s, "%donald", 7) == 0) {	/* is DoNaLD */
	    changeNotation(DONALD);
	} else if (strncmp(s, "%scout", 6) == 0) {	/* is SCOUT */
	    changeNotation(SCOUT);
	} else if (notationSwitch(s)) {
	    changeNotation(NEWOTHER);
#ifdef DISTRIB
	} else if (strncmp(s, "%lsd", 4) == 0) {	/* is LSD */
	    changeNotation(LSD);
#endif /* DISTRIB */
	} else if (strncmp(s, "%eden", 5) == 0) {	/* is EDEN */
	    changeNotation(EDEN);
#ifdef WANT_SASAMI
	} else if (strncmp(s, "%sasami", 7) == 0) {	/* is Sasami */
	    changeNotation(SASAMI);
#endif /* WANT_SASAMI */
	}

	/* Count the number of chars in s (terminated by \0 or \n) [Ash] */
	for (sptr = s, i = 0; *s != '\0' && *s != '\n'; s++, i++);
#ifdef DISTRIB
        if (strncmp(sptr, ">>$", 3)==0 || strncmp(sptr, "><$", 3)==0 ||
            strncmp(sptr, ">$", 2) == 0 || strncmp(sptr, "<$", 2) == 0 ||
            strncmp(sptr, "$$", 2) == 0)  {
	  /* Meurig's funny extra notation - probably not used any
             more [Ash, with Patrick <--blame Patrick for this! :] */
	  getClientName(sptr, script);
        } else {
          if (readIn == 1) {
	     appendnEden(sptr, script, i);
	     appendnEden("\n", script, 1);
	  }
	  if (*clientName !=0) {
	     *sentData=0;
	     strncat(sentData, sptr, i);
	     appendEden("sendClient(\"", script);
	     appendEden(clientName, script);
	     appendEden("\", \"", script);
	     appendEden(sentData, script);
	     appendEden("\");\n", script);
	  }
	}
#else
	/* Append one line of s to script [Ash] */
	appendnEden(sptr, script, i);
	appendnEden("\n", script, 1);
#endif /* DISTRIB */
	if (*s != '\0') {	/* *s must be '\n' */
	  s++;
	}
	lineno++;
    }
}

/* evaluate() = translate followed by queue().  Appears to only be
   called from evaluateCmd (implementation of the Tcl 'evaluate'
   command).  [Ash] */
void
evaluate(char *s, char *master)
{
    Script     *script;
    extern Script *dd_script, *st_script;
#ifdef WANT_SASAMI
    extern Script *sa_script;
#endif

    script = newScript();
    dd_script = st_script = script;
#ifdef WANT_SASAMI
    sa_script = script;
#endif

#ifdef DISTRIB
    *clientName=0;
    readIn=1;
#endif /* DISTRIB */

    distribute(s, script);
#ifndef TTYEDEN
    queue(script->text, master, NULL);
#else
    queue(script->text, master);
#endif
    deleteScript(script);
}
