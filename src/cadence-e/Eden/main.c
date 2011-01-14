/*
 * $Id: main.c,v 1.53 2002/03/01 23:47:43 cssbz Exp $
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

static char rcsid[] = "$Id: main.c,v 1.53 2002/03/01 23:47:43 cssbz Exp $";

#ifndef __WIN32__
#   if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__)
#	define __WIN32__
#   endif
#endif

#include "../../../../../config.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <setjmp.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>		/* for S_ISREG */
#include <unistd.h>			/* for getcwd() and gethostname() for
                                   DISTRIB [Ash] */
#include <limits.h>			/* for PATH_MAX */
#include <math.h>			/* for M_PI */
#include <assert.h>

#ifdef DISTRIB
#include <netdb.h>			/* for MAXHOSTNAMELEN on Solaris [Ash] */
#include <sys/param.h>		/* for MAXHOSTNAMELEN on Linux [Ash] */
#endif

#include "../version.h"

#include "emalloc.h"
#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

/* From Autoconf manual [Ash] */
# if TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
# else
#  if HAVE_SYS_TIME_H
#   include <sys/time.h>
#  else
#   include <time.h>
#  endif
# endif

#include "eden.h"
#include "yacc.h"
#include "builtin.h"
#include "runset.h"
#include "notation.h"

#if defined(TTYEDEN) && defined(HAVE_READLINE)
/* this must be included after yacc.h as it potentially defines RETURN */
#include <readline.h>
#endif

#include "../Misc/custom.h"

#ifndef TTYEDEN
#include <tk.h>
/* Tcl caches environment variables: so we must use Tcl's versions of
 the getenv and putenv functions [Ash] */
#ifdef __APPLE__
/* Tcl/Tk on MacOS X seems to have a non-functioning Tcl_PutEnv */
#define putenv putenv
#else
#define putenv Tcl_PutEnv
#endif
/* Tcl 8.2.0 Makefile implies don't use the Tcl getenv [Ash] */
#define setenv TclSetEnv
#define unsetenv TclUnsetEnv

#include <tcl.h>
extern Tcl_Interp *interp;
extern void EXinitTcl(int, char **);
#endif /* not TTYEDEN */

#ifndef TTYEDEN
#ifdef __APPLE__
/*#include <Carbon/Carbon.h>
 #include "tkMacOSX.h"
 #include "tkMacOSXEvent.h"*/
#endif
#endif

extern notationType currentNotation;

char *libLocation = NULL; /* Location of Eden and Tcl library files -
 this replaces the use of the PUBLIC
 environment variable [Ash] */

char *progname;

jmp_buf start;
int startValid = 0;

char **gargv; /* global argument list */
int gargc;
Int *autocalc;
Int *eden_error_index_range; /* enable / disable errors on
 encountering list or string
 references that are out of
 range [Ash] */
Int *eden_notice_undef_reference; /* enable / disable warning
 notices when making
 references to variables
 that are undefined [Ash] */
Int *eden_backticks_dependency_hack; /* enable / disable hack
 that gives back-ticks
 on the RHS
 "dependency", but on a
 one-off basis [Ash] */
int interrupted = 0;

int startupargc;
char **startupargv;
char *startupcwd;

int         processingRunSet = 0;

#ifdef INSTRUMENT
/* instrumenting the code to find how much time is taken by various
 operations [Ash] */
#include <sys/time.h>
hrtime_t insSchedule = 0;
hrtime_t insCycle = 0;
hrtime_t insStart = 0;
#endif /* INSTRUMENT */

#ifdef DISTRIB
int socketPort = 9000; /* for distributed Tkeden  --sun */
char hostName[MAXHOSTNAMELEN];
char *hostNamePtr = 0;
char *everyone = "EVERYONE";
Int *synchronize; /* connecting with Tcl/Tk to handle
 synchronized communication */
Int *EveryOneAllowed; /* to handle agency of an observable with
 a positive way, i.e. allowing every
 agent has agency when no listed name,
 or a negative way, i.e. allowing every
 agent has no agency when no listed
 name. */
Int *higherPriority; /* to handle execusively running a runset
 without any new running runset,
 i.e. all actions in queue will not be
 put into runset before current runset
 is finished --sun.  Patrick confesses
 to this not working very well, which is
 why it is left undocumented ;) [Ash,
 with Patrick] */
Int *propagateType; /* to handle scripts' distributed propagation */
int isServer = 1;
char loginname[255] = "";
char *loginnamePtr = loginname;
#endif /* DISTRIB */

#include "input_device.h"
struct input_device *Inp_Dev = Input_Devices;
struct input_device *Inp_Dev_Save = 0; /* work around bug in gcc - see
 below [Ash] */

#ifdef DEBUG
#define DEBUGPRINT(s,i) if (Debug & 1) fprintf(stderr,s,i);
#define DEBUGPRINT2(s,i,j) if (Debug & 1) fprintf(stderr,s,i,j);
#else
#define DEBUGPRINT(s,i)
#define DEBUGPRINT2(s,i,j)
#endif

RunSet RS1, RS2;
static RunSet *RS;

#ifdef TTYEDEN
void checkRunSet(void);
#else
void swapAndRunRunSet(ClientData);
#endif

extern void incGarbageLevel(void), decGarbageLevel(void);
extern void clearGarbage(void);

extern int doste_context(char*);
int basecontext = 0;

#ifdef TTYEDEN
char *prompt1 = "|> "; /* Usual prompt [Ash] */
char *promptsemi = ";> "; /* Prompt when waiting for a semi-colon */
char *promptchar = "\"> "; /* Prompt when waiting for a close-quote */
char *promptcomment = "*> "; /* Prompt when waiting for a close-comment */

char *prompt;
symptr eden_prompt_sym; /* ttyeden: print prompts or not [Ash] */
int preprinted = FALSE; /* if TRUE, this makes printing the
 prompt a no-op.  It is set to TRUE
 whenever the prompt is printed and
 FALSE whenever input is read.
 [Ash] */
int nlstts = TRUE; /* newline status? [Ash] */

/* nlstatus: update nlstts from Inp_Dev [Ash] */
void nlstatus(void)
{
	nlstts = Inp_Dev->newline;
}

void print_prompt(void)
{
	char *fullPromptText;
	extern char *fullPrompt();

	if (!Inp_Dev->usereadline) {
		if (eden_prompt_sym->d.u.i) {
			fullPromptText = fullPrompt();
			fprintf(stderr, "%s", fullPromptText);
			free(fullPromptText);
		}
	}

	preprinted = TRUE;
}

/* Also does a checkRunSet as this is ommitted otherwise when using
 the Eden parser.  This is the easiest place to find where to do
 the checkRunSet.  [Ash] */
void print_prompt_if_necessary(int source) {
	int doCheckRunSet;

	if (Inp_Dev->type == FILE_DEV && (FILE *) Inp_Dev->ptr == stdin) {

#ifdef DEBUG
		if (Debug & 2) {
			debugMessage("p(%d,%d,%d)", source, Inp_Dev->newline, preprinted);
		}
#endif

		if (Inp_Dev->newline) {
			if (!preprinted) print_prompt();
		}

		switch (source) {
			/* main.c: initial machine startup */
			case 0: doCheckRunSet = 0; break;

			/* main.c: after yyparse */
			case 1: doCheckRunSet = 1; break;

			/* lex.c: keyin() */
			case 2: doCheckRunSet = 0; break;

			/* lex.c: peek() */
			/*    case 3: doCheckRunSet = Inp_Dev->newline; break; */
			/* or possibly wouldBlock ? */
#ifdef HAVE_READLINE
			case 3: doCheckRunSet = rl_getc_wouldBlock(); break;
#else
			case 3: doCheckRunSet = 1; break;
#endif /* HAVE_READLINE */

		}

		if (doCheckRunSet) {
			do {
				checkRunSet();
				clearGarbage();
				incGarbageLevel();
			}while (RS->nitems > 0 && wouldBlock());
		}
	}
}
#else /* not TTYEDEN */


#ifdef USE_TCL_CONST84_OPTION
int eden_interrupt_handler(ClientData, Tcl_Interp *, int, CONST84 char *[]);
#else
int eden_interrupt_handler(ClientData, Tcl_Interp *, int, char *[]);
#endif

#endif

int wouldBlock(void) {
	fd_set fdset;
	int fd = fileno((FILE *) (Inp_Dev->ptr));
	struct timeval timeout;

	timeout.tv_sec = timeout.tv_usec = 0;
	FD_ZERO(&fdset);
	FD_SET(fd, &fdset);
	return select(FD_SETSIZE, &fdset, 0, 0, &timeout) <= 0;
}

int run(short, void *, char *);
void clearEntryStack(void);
void pushMasterStack(char *);
void popMasterStack(void);
void clearMasterStack(void);
char *topMasterStack(void);
void warning(char *, char *);
void yyerror(char *);
void init(void);
void terminate(int);
void run_init(Inst *);
void user_trace(void);

//Formally the main function
int cadence_e_initialise(int, char *[]); /* change type from void to int due to gcc 2.8.1 */

char * notationTypeToString(notationType n) {
	switch (n) {
	case INTERNAL:
		return "INTERNAL";
	case EDEN:
		return "%eden";
	case SCOUT:
		return "%scout";
	case DONALD:
		return "%donald";
	case ARCA:
		return "%arca";
	case NEWOTHER:
		return "NEWOTHER";
	default:
		return "UNKNOWNOHDEAR";
	}
}

/* Not sure what EntryStack really does.  I previously thought this,
 but it seems that I am wrong, as more INTERNAL items get popped
 than this would explain.  <The EntryStack is a stack of notation
 invocations used in parsing.  For example, an execute(); causes
 another Eden to be pushed on the top of this stack, which is popped
 off, reverting the notation to the previously active one, when the
 execute finishes.> [Ash] */
static int entryStackSize = 0;
int TopEntryStack = 0;
static int *EntryStack;

int topEntryStack(void) {
	if (TopEntryStack <= 0)
		errorf("topEntryStack failed: TopEntryStack <= 0");
	else
		return EntryStack[TopEntryStack - 1];
}

void pushEntryStack(notationType m) {
#ifdef DEBUG
	if (Debug & (16384 | 1)) {
		debugMessage("ENTSTK pushEntryStack(%s)\n", notationTypeToString(m));
	}
#endif
	if (entryStackSize == 0) {
		entryStackSize = 16;
		EntryStack = (int *) emalloc(sizeof(int) * entryStackSize);
	} else if (TopEntryStack == entryStackSize) {
		entryStackSize += 16;
		EntryStack = (int *) erealloc(EntryStack, sizeof(int) * entryStackSize);
	}
	EntryStack[TopEntryStack++] = m;
}

void popEntryStack(void) {
#ifdef DEBUG
	if (Debug & (16384 | 1)) {
		debugMessage("ENTSTK popEntryStack\n", 0);
	}
#endif
	if (--TopEntryStack < 0)
		TopEntryStack = 0;
}

void clearEntryStack(void) {
#ifdef DEBUG
	if (Debug & (16384 | 1)) {
		debugMessage("ENTSTK clearEntryStack\n", 0);
	}
#endif
	TopEntryStack = 0;
	pushEntryStack(0);
}

static int masterStackSize = 0;
static int TopMasterStack = 0;
static char **MasterStack;

void pushMasterStack(char *m) {
	if (masterStackSize == 0) {
		/* agent names can be maximum 16 chars in length [Ash] */
		masterStackSize = 16;
		MasterStack = (char **) emalloc(sizeof(char *) * masterStackSize);
	} else if (TopMasterStack == masterStackSize) {
		masterStackSize += 16;
		MasterStack = (char **) erealloc(MasterStack, sizeof(char *)
				* masterStackSize);
	}
	MasterStack[TopMasterStack++] = m;
}

/* !@!@ I think popMasterStack and clearMasterStack should free the
 associated memory [Ash] */
void popMasterStack(void) {
	if (--TopMasterStack < 0)
		TopMasterStack = 0;
}

void clearMasterStack(void) {
	TopMasterStack = 0;
}

char * topMasterStack(void) {
	return MasterStack[TopMasterStack - 1];
}

static time_t lastBeepTime = 0;

static void setTimer(time_t * t) {
	time(t);
}

static char timedEventsEnabled(time_t * t) {
	return (time(NULL) > (*t + 3));
}

#ifdef TTYEDEN

void errorComplete(void) {
	/* do nothing */
}

void errorContentf(char *fmt, ...) {
	va_list argp;

	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
}

void appendHist(char *toAppend) {
	/* do nothing */
}

// 																					<<<------- TODO: This will likly be removed when we are done! (well at least set to return nothing)
#elif defined(WEDEN_ENABLED)

// Just overiding these to output a 'not yet modified message to help detect anything I miss.
// Overided functions will miss this stuff out!

void errorComplete(void) {
	fprintf(stderr, "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\nWARNING : MISSED OUT A FUNCTION CALLING main.c - errorComplete()\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
}

void errorContentf(char *fmt, ...) {
	fprintf(stderr, "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\nWARNING : MISSED OUT A FUNCTION CALLING main.c - errorContentf()\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
}

// Weden to handel its own history ???
void appendHist(char *toAppend) {
	/* do nothing */
}


#else /* not TTYEDEN OR Web EDEN */

void errorComplete(void) {
	char tclCommand[30];

	strcpy(tclCommand, "errorComplete");

	/* Stop rapid beeping: only beep if we haven't done so for a while [Ash] */
	if (timedEventsEnabled(&lastBeepTime)) {
		setTimer(&lastBeepTime);
		strcat(tclCommand, " 1\n");
	} else {
		strcat(tclCommand, " 0\n");
	}

	if (Tcl_Eval(interp, tclCommand) != TCL_OK) {
		fprintf(stderr, "Tcl error (errorComplete): %s\n",
				Tcl_GetStringResult(interp));
		exit(-1);
	}
}

void errorContentf(char *fmt, ...) {
	va_list argp;
	char *errorMessage;
	Tcl_DString tclCommand; /* need to use Tcl_DStrings as want to
	 ensure spaces and other characters
	 are properly dealt with */

	errorMessage = emalloc(Inp_Dev->linebufsize + 100);

	Tcl_DStringInit(&tclCommand);
	Tcl_DStringAppendElement(&tclCommand, "appendErr");

	va_start(argp, fmt);
	vsnprintf(errorMessage, Inp_Dev->linebufsize + 100, fmt, argp);
	va_end(argp);

	Tcl_DStringAppendElement(&tclCommand, errorMessage);
	if (Tcl_Eval(interp, tclCommand.string) != TCL_OK) {
		fprintf(stderr, "Tcl error (errorContentf): %s\n",
				Tcl_GetStringResult(interp));
		exit(-1);
	}

	Tcl_DStringFree(&tclCommand);
	free(errorMessage);
}

void appendHist(char *toAppend) {
	Tcl_DString tclCommand;

	Tcl_DStringInit(&tclCommand);
	Tcl_DStringAppendElement(&tclCommand, "appendHist");
	Tcl_DStringAppendElement(&tclCommand, toAppend);

	if (Tcl_Eval(interp, tclCommand.string) != TCL_OK) {
		fprintf(stderr, "Tcl error (appendHist): %s\n",
				Tcl_GetStringResult(interp));
		exit(-1);
	}

	Tcl_DStringFree(&tclCommand);
}
#endif // End not Web EDEN or TTY Eden


/* So that noticef() knows we are in the middle of reporting an error
 so that it can format the output correctly */
static int inerrorfretcall = 0;

// 																					<<<------- TO STILL DO (Customise this) 
void noticef(char *fmt, ...) {
	va_list argp;
	char errorsbuf[256];

#ifdef TTYEDEN
	if (inerrorfretcall)
	fprintf(stderr, "\n");

	fprintf(stderr, "%s: notice: ", progname);
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);

	if (fp != Inp_Dev->frame) {
		fprintf(stderr, " in %s%s", fp->master ? "action " : "", fp->master ? fp->master : fp->sp->name);
	}
	fprintf(stderr, "\n");

#elif defined(WEDEN_ENABLED)

	// Generate the notification message
	va_start(argp, fmt);
	vsnprintf(errorsbuf, sizeof(errorsbuf), fmt, argp);
	va_end(argp);

	// See if we are handeling an error currently?
	if (inerrorfretcall) {

		// We are outputting an error so just include the notice in with the error. Do not create new xml element
		appendFormattedWedenMessage("\nNotice: %s", errorsbuf);
		if (fp != Inp_Dev->frame) {
			appendFormattedWedenMessage(" in %s%s", fp->master ? "action " : "", fp->master ? fp->master : fp->sp->name);
		}
		appendWedenMessage("\n");

	} else {
		
		// This is simply a unique notice and not part of an error so place in its own xml tags
		startWedenMessage();
		appendFormattedWedenMessage("<b><item type=\"notice\"><![CDATA[%s", errorsbuf);
		if (fp != Inp_Dev->frame) {
			appendFormattedWedenMessage(" in %s%s", fp->master ? "action " : "", fp->master ? fp->master : fp->sp->name);
		}
		appendWedenMessage("\n]]></item></b>");
		endWedenMessage();
	}

#else
	va_start(argp, fmt);
	vsnprintf(errorsbuf, sizeof(errorsbuf), fmt, argp);
	va_end(argp);
	if (inerrorfretcall)
		errorContentf("\n");
	errorContentf("notice: %s", errorsbuf);
	if (fp != Inp_Dev->frame) {
		errorContentf(" in %s%s", fp->master ? "action " : "",
				fp->master ? fp->master : fp->sp->name);
	}
	errorContentf("\n");
	errorComplete();
	printf("%s: notice: %s\n", progname, errorsbuf);
#endif
}


void warningf(char *fmt, ...) {
	char errorsbuf[256]; /* to hold the individual line causing the error */
	va_list argp;

	va_start(argp, fmt);
	vsnprintf(errorsbuf, sizeof(errorsbuf), fmt, argp);
	va_end(argp);

#ifdef TTYEDEN
	fprintf(stderr, "%s: warning: %s\n", progname, errorsbuf);
#elif defined(WEDEN_ENABLED)

	// Send a warning to Weden Interface
	sendCompleteFormattedWedenMessage("<b><item type=\"warning\">%s</item></b>\n", errorsbuf);

#else 

	errorContentf("warning: %s\n", errorsbuf);
	errorComplete();
	fprintf(stderr, "%s: warning: %s\n", progname, errorsbuf);
#endif
}

void warning(char *s, char *t) { /* print warning message */
	warningf("%s%s", s, t);
}

#ifdef WEDEN_ENABLED

/* Print error message and skip the rest of the input.
 Call this with printf style arguments */
void errorf(char *fmt, ...) {
	extern void ret_call(void);
	extern void reset_pseudo_machine_status(void);
	extern void reset_compiler_status(void);
	extern void freeheap(void);
	extern void flushRestOfLine(void);
	extern int nextc;
	int ntmp, i;
	char *stmp, *stmpout;
	char *errorsbuf; /* to hold the individual line causing the error */
	va_list argp;

	// Make room for the error
	errorsbuf = emalloc(Inp_Dev->linebufsize + 100);

	/* Must take a copy of the error */
	va_start(argp, fmt);
	vsnprintf(errorsbuf, Inp_Dev->linebufsize + 100, fmt, argp);
	va_end(argp);

	// Start sending the error information
	startWedenMessage();
	appendFormattedWedenMessage("<b><item type=\"error\"><info><![CDATA[%s]]>", errorsbuf);

#ifdef DEBUG
	if (Debug & 4096)
	debugMessage("%s: error: %s\n", progname, errorsbuf);
#endif /* DEBUG */

	// Free up the memory used to hold the error message
	free(errorsbuf);

	// Get the rest of the error data
	flushRestOfLine();

	if (Inp_Dev != Input_Devices) {

		// Show the error messages
		i = 0;
		while (fp > Inp_Dev->frame) {
			/* free the memory for frame */
			if (fp->master) {
				/* fp->sp->name == fp->master */
				appendFormattedWedenMessage(i++ ? " called by action %s" : " in action %s", fp->sp->name);
			} else {
				if (fp->sp) {
					appendFormattedWedenMessage(i++ ? " called by %s" : " in %s", fp->sp->name);
				}
			}
			inerrorfretcall++;
			ret_call();
			inerrorfretcall--;
		}

		if (Inp_Dev->type == FILE_DEV) {
			if (Inp_Dev->name)
				appendFormattedWedenMessage(" while executing file %s", Inp_Dev->name);
			else
				appendFormattedWedenMessage(" while executing stdin");
		} else {
			appendFormattedWedenMessage(" while executing string");
		}

		int errorLineNo = Inp_Dev->lineno;
		int errorCharNo = Inp_Dev->charno;
		char *errorLineBuf = Inp_Dev->linebuf;

		appendFormattedWedenMessage(" near line %d, char %d:\n</info>", errorLineNo, errorCharNo);

		// The error line
		appendFormattedWedenMessage("<statement>%s\n</statement>\n<location line=\"%d\" char=\"%d\">", errorLineBuf, errorLineNo, errorCharNo);

		// Places a ^ under the error char indicated above
		for (i = 0; i < errorCharNo-1; i++) {
			if (errorLineBuf[i] == '\t')
				appendFormattedWedenMessage("\t");
			else
				appendFormattedWedenMessage(" ");
		}
		appendFormattedWedenMessage("^\n</location></item></b>");
		endWedenMessage();
		
		switch (Inp_Dev->type) {
			case FILE_DEV:
			/* flush rest of file */
			fseek((FILE *) Inp_Dev->ptr, 0L, 2);
			break;
			case STRING_DEV:
			/* empty the string */
			*Inp_Dev->ptr = '\0';
			break;
		}
		nextc = 0; /* EOF */

		Inp_Dev->charno = 0;
		Inp_Dev->linebuf[0] = '\0';
	}

	/* errorComplete() may cause Tcl to do edenCmd (via the <Configure>
	 binding on window changes.  The Eden invoked will use the stack,
	 so don't reset everything until /after/ errorComplete() has
	 finished.  (fix for "bug42") [Ash] */
	freeheap();
	reset_stack();
	reset_eval();
	reset_compiler_status();

	if (startValid) {
		DEBUGPRINT("errorf: longjmp\n", 0);
		/* Work around bug in gcc - see this being used in run [Ash] */
		Inp_Dev_Save = Inp_Dev;
		/* Jump to resume.  The execution will jump to the last matching
		 call to setjmp (either to the last setjmp(start) or
		 setjmp(Inp_Dev->begin), depending on the value of Inp_Dev), as
		 if it is a return from that call.  setjmp returns the last
		 parameter given here, 1.  [Ash] */
		longjmp((Inp_Dev == Input_Devices ? start : Inp_Dev->begin), 1);
	} else {
		/* Can't restart */
		fprintf(stderr, "%s: error: ", progname);
		va_start(argp, fmt);
		vfprintf(stderr, fmt, argp);
		va_end(argp);
		fprintf(stderr, "\n");
		exit(-1);
	}
}

#else // Not WebEDEN


/* Print error message and skip the rest of the input.
 Call this with printf style arguments */
void errorf(char *fmt, ...) {
	extern void ret_call(void);
	extern void reset_pseudo_machine_status(void);
	extern void reset_compiler_status(void);
	extern void freeheap(void);
	extern void flushRestOfLine(void);
	extern int nextc;
	int ntmp, i;
	char *stmp, *stmpout;
	char *errorsbuf; /* to hold the individual line causing the error */
	va_list argp;

	errorsbuf = emalloc(Inp_Dev->linebufsize + 100);

#ifdef TTYEDEN
	fprintf(stderr, "%s: error: ", progname);
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);

#else // Likly Tk/Dtk eden
	/* must make our own copy of the error first (especially in the case
	 of errors from Tcl_EvalEC, where a call to Tcl_Eval here would
	 overwrite the error information) [Ash] */
	va_start(argp, fmt);
	vsnprintf(errorsbuf, Inp_Dev->linebufsize + 100, fmt, argp);
	va_end(argp);

	/* careful: we can't use Tcl_EvalEC as if the error was generated
	 there, it would cause re-entrancy [Ash] */
	errorContentf("%s", errorsbuf);

#ifdef DEBUG
	if (Debug & 4096)
		debugMessage("%s: error: %s\n", progname, errorsbuf);
#endif /* DEBUG */

#endif /* not TTYEDEN */

	free(errorsbuf);

	/* slurp the rest of the line into Inp_Dev->linebuf so it shows in
	 the error message */
	flushRestOfLine();

	if (Inp_Dev != Input_Devices) {
		/* show error messages */

		i = 0;
		while (fp > Inp_Dev->frame) {
			/* free the memory for frame */
			if (fp->master) {
				/* fp->sp->name == fp->master */
				errorContentf(i++ ? " called by action %s" : " in action %s",
						fp->sp->name);
			} else {
				if (fp->sp) {
					errorContentf(i++ ? " called by %s" : " in %s",
							fp->sp->name);
				}
			}
			inerrorfretcall++;
			ret_call();
			inerrorfretcall--;
		}

		if (Inp_Dev->type == FILE_DEV) {
			if (Inp_Dev->name)
				errorContentf(" while executing file %s", Inp_Dev->name);
			else
				errorContentf(" while executing stdin");
		} else {
			errorContentf(" while executing string");
		}

		errorContentf(" near line %d, char %d:\n", Inp_Dev->lineno,
				Inp_Dev->charno);

		errorContentf("%s\n", Inp_Dev->linebuf);
		for (i = 0; i < Inp_Dev->charno-1; i++) {
			if (Inp_Dev->linebuf[i] == '\t')
				errorContentf("\t");
			else
				errorContentf(" ");
		}
		errorContentf("^\n");

		switch (Inp_Dev->type) {
		case FILE_DEV:
			/* flush rest of file */
			fseek((FILE *) Inp_Dev->ptr, 0L, 2);
			break;
		case STRING_DEV:
			/* empty the string */
			*Inp_Dev->ptr = '\0';
			break;
		}
		nextc = 0; /* EOF */

		Inp_Dev->charno = 0;
		Inp_Dev->linebuf[0] = '\0';
	}


#ifdef TTYEDEN
	prompt = prompt1;
#else
	errorComplete();
#endif

	/* errorComplete() may cause Tcl to do edenCmd (via the <Configure>
	 binding on window changes.  The Eden invoked will use the stack,
	 so don't reset everything until /after/ errorComplete() has
	 finished.  (fix for "bug42") [Ash] */
	freeheap();
	reset_stack();
	reset_eval();
	reset_compiler_status();

	if (startValid) {
		DEBUGPRINT("errorf: longjmp\n", 0);
		/* Work around bug in gcc - see this being used in run [Ash] */
		Inp_Dev_Save = Inp_Dev;
		/* Jump to resume.  The execution will jump to the last matching
		 call to setjmp (either to the last setjmp(start) or
		 setjmp(Inp_Dev->begin), depending on the value of Inp_Dev), as
		 if it is a return from that call.  setjmp returns the last
		 parameter given here, 1.  [Ash] */
		longjmp((Inp_Dev == Input_Devices ? start : Inp_Dev->begin), 1);
	} else {
		/* Can't restart */
		fprintf(stderr, "%s: error: ", progname);
		va_start(argp, fmt);
		vfprintf(stderr, fmt, argp);
		va_end(argp);
		fprintf(stderr, "\n");
		exit(-1);
	}
}

#endif // Not WebEDEN



void yyerror(char *s) { /* report compile-time error */
	error(s);
}

// 																					<<<------- TODO: Could we possibly allow Web EDEN to trigger an interupt ? 

#ifndef TTYEDEN
/* This is the implementation for the new Tcl "interrupt" command,
 which is called when the Interrupt button is pressed... [Ash] */
#ifdef USE_TCL_CONST84_OPTION
int eden_interrupt_handler(ClientData clientData, Tcl_Interp * interp, int argc, CONST84 char *argv[]) 
#else
int eden_interrupt_handler(ClientData clientData, Tcl_Interp * interp, int argc, char *argv[]) 
#endif
{
	/* Given that Tcl has called this handler, the EDEN VM machine is not
	 running (this can be verified by seeing with gdb that the ultimate
	 parent of this proc in the callstack is Tk_MainLoop()), so setting
	 'interrupted' will not stop it.  However setting it will cause
	 swapAndRunRunSet to clear the RunSets. */
	interrupted = 1;

	/* swapAndRunRunSet will not be called unless Tcl has it on the
	 DoWhenIdle list. */
	Tcl_DoWhenIdle(swapAndRunRunSet, 0);

	fprintf(stderr, "%s: interrupt button pressed\n", progname);
	return TCL_OK;
}
#endif

static time_t lastControlCTime = 0;

/*RETSIGTYPE Control_C(int sig) {
	if (timedEventsEnabled(&lastControlCTime)) {*/
		/* First press of ctrl-c within three seconds.  [Ash] */
		/*signal(SIGINT, Control_C);
		setTimer(&lastControlCTime);

	} else {*/
		/* Second press of ctrl-c within three seconds.  [Ash] */
		/* Linux doesn't seem to reset to SIG_DFL (default behaviour) 
		 automatically (although I think K+R says it should) [Ash] */
	/*	signal(SIGINT, SIG_DFL);*/

		/* Two interrupts within three seconds cause the normal behaviour (which
		 probably kills the EDEN process).  [Ash] */
	/*	raise(SIGINT);
	}

#ifndef TTYEDEN*/
	/* The EDEN VM may be running at the moment.  Setting 'interrupted' will
	 cause it to stop after processing the current VM opcode.
	 swapAndRunRunSet will then clear the RunSets.  We need to clear the
	 RunSets indirectly by activating swapAndRunRunSet because we shouldn't
	 clear them whilst the EDEN VM is running.  [Ash] */
	/*interrupted = 1;*/

	/* swapAndRunRunSet will not be called unless Tcl has it on the DoWhenIdle
	 list.  There is a remaining problem here: if EDEN is currently running (eg
	 type 'while (1) {};' in the input window, accept it, then press ctrl-c),
	 then swapAndRunRunSet is called straight away and things are as I expect.
	 But on Mac OS X it seems that if EDEN is currently quiescent,
	 swapAndRunRunSet does not seem to be called until tkeden is given the
	 input focus after ctrl-c has been pressed.  I suspect this is a Tcl bug.
	 It is a little difficult to debug: eg you must use 'signal(SIGINT)' in
	 gdb to generate the signal, since gdb catches ctrl-c itself.  [Ash] */
	/*Tcl_DoWhenIdle(swapAndRunRunSet, 0);
#endif*/

	/* don't use Tcl to give the error message as it might be in a
	 non-re-entrable state (seem to have this problem particularly in
	 Sasami) [Ash] */
	/*fprintf(stderr, "%s: interrupted using control-c: press again within"
		" three seconds to quit\n", progname);
}*/

/* {d}tkeden is interleaved with Tcl (and hence user input) in the following
 * way.
 *
 * (1) Tcl active or inactive  <------------------------------------\
 *     (EDEN "quiescent")                                           |
 *           |                                                      |
 *         queue()                                                  |
 *           |                                                      |
 *           |  NB: active RS is empty                              |
 *           |      inactive RS is non-empty                        |
 *           |                                                      |
 *          \|/                                                     |
 * (2) Tcl active                                                   |
 *     DoWhenIdle=swapAndRunRunSet                                  |
 *           |                                                      |
 *         (Tcl becomes idle)                                       |
 *           |                                                      |
 *          \|/                                                     |
 * (3) swapAndRunRunSet()                                           |
 *           |                                                      |
 *           |  NB: active RS is non-empty                          |
 *           |      inactive RS is empty                            |
 *           |                                                      |
 *          (swaps RunSets)                                         |
 *          (runs RunSet)                                           |
 *           |                                                      |
 *           |  NB: if Eden todo() was not called during execution  |
 *           |      of RunSet:                                      |
 *           |        active RS is empty                            |
 *           |        inactive RS is empty                          |
 *           |        DoWhenIdle=nothing                            |
 *           |      if Eden todo() (and hence, queue()) was called: |
 *           |        active RS is empty                            |
 *           |        inactive RS is non-empty                      |
 *           |        DoWhenIdle=swapAndRunRunSet                   |
 *           |                                                      |
 *           \------------------------------------------------------/
 *
 * Notes:
 * - The above diagram and following notes do not describe handling of 
 *   'interrupted'.
 * - A RunSet is a FIFO queue of Actions.  There are two RunSets, one
 *   active and one inactive (the "todo queue").
 * - The only manipulations on the RunSets are as follows:
 *   - Actions are added to the inactive RunSet by queue()
 *   - Actions are removed from the active RunSet by the second, running
 *     portion of swapAndRunRunSet()
 *   - The inactive and active RunSets are swapped by initial, swapping
 *     portion of swapAndRunRunSet()
 * - queue() is called by the Eden todo() builtin, our Tcl 'todo' command
 *     (C todoCmd()), our Tcl 'evaluate' command (C evaluateCmd(), which
 *     calls C evaluate()) and our Tcl 'interface' and 'interfaceTEXT'
 *     commands (written in Tcl in edenio.tcl, which call Tcl 'todo').
 * - The Eden todo() builtin sets the master to topMasterStack (probably
 *   the currently executing procedure)
 * - Tcl 'todo' sets master to "interface"
 * - Tcl 'evaluate' sets master to "input"
 * - Tcl 'update' is (only?) called in C, therefore the intermediate states
 *   produced whilst a RunSet is executing will not be visible -- the state
 *   will only become visible when a RunSet has finished executing.
 *   (Although the Eden eager() builtin circumvents this.)
 *
 * NB There is a diagram in my (Ashley's) thesis (Figure 4.38, p.273) which was
 * correct for earlier versions of EDEN, but I have simplified the code since,
 * on realising that checkRunSet could be split into two independent cases
 * (which is possible since the edge from "Tcl interpreter until idle" to
 * "checkRunSet" can in fact go directly to "Tcl_Update" despite the
 * appearance of the code at the time to the contrary -- I tested this
 * assumption by introducing some extra state and an assert check and then
 * running cruisecontrolBridge1991 and planimeterCare2005: see the code in
 * an earlier revision of this file in svn.)  [Ash]
 */

#ifndef TTYEDEN

void swapAndRunRunSet(ClientData clientData) {
	Action a;
	extern void setprompt(void);

#ifdef DEBUG
	if (Debug & 2)
		debugMessage("swapAndRunRunSet start\n");
#endif /* DEBUG */

/*
#ifdef WEDEN_ENABLED
	if (Tcl_Eval(interp, "weden_StartEdenRunSetProcessing") != TCL_OK) {
		fprintf(stderr, "Tcl error (edenStartRunSetProcessing): %s\n", Tcl_GetStringResult(interp));
	}
#endif
*/

	/* We must prevent this proc from being invoked whilst it is already
	 running.  This is possible in the 'interrupted' case and also if we
	 have a sensitive:MOTION Scout window. Then, scout.eden sets up a Tk
	 binding to mouse <MOTION>, which invokes our Tcl 'interface'
	 command in order to make a redefinition to the appropriate mousePos
	 variable.  That Tcl command invocation can happen during the Tcl
	 "update" command below, causing queue() to be called, which seems
	 to cause swapAndRunRunSet() to be called -- but swapAndRunRunSet()
	 is already running.  If left unchecked and if this happens enough,
	 eventually we blow a Tcl stack.  (Charlie's planimeters are
	 susceptible to this problem.)  The large call stacks that this
	 problem creates are visible in Apple's Shark profiling utility (use
	 the Chart tab). So we protect against this problem: if we are
	 already in an invocation of this proc (and hence processingRunSet
	 is 1), don't invoke it again. [Ash] */

	 if (processingRunSet) {
	 #ifdef DEBUG
	 if (Debug & 2) {
	 debugMessage("swapAndRunRunSet: ignoring invocation since already"
	 " processingRunSet\n");
	 }
	 #endif
	 return;
	 }
	 processingRunSet = 1;

	Tcl_CancelIdleCall(swapAndRunRunSet, 0);

	/* If interrupted is already set, don't do any work: skip to the end to
	 deal with it */
	if (!interrupted) {
		Tcl_EvalEC(interp, "update");

		/* sanity check: old RunSet should be empty at this point [Ash] */
		/* assert(RS->nitems == 0); [Ant][25/01/2007] Assertion no longer relevant */

		/*
		 * SWAP RunSets
		 */
		RS = (RS == &RS1) ? &RS2 : &RS1;

		/* sanity check: new RunSet should be non-empty at this point [Ash] */
		/* assert(RS->nitems > 0); [Ant][25/01/2007] Assertion no longer relevant */

#ifdef DEBUG
		if (Debug & 2) {
			debugMessage("swapAndRunRunSet: newly active RunSet RS->nitems=%d\n",
					RS->nitems);
			printRunSet(RS);
		}
#endif /* DEBUG */

		/*
		 * RUN RunSet
		 */
		while (RS->nitems > 0) {
			a = getAction(RS); /* getAction removes and returns the value from the
			 head of the RunSet [Ash] */
			pushMasterStack(a.master);
#ifdef DEBUG
			if (Debug & 2)
			debugMessage("swapAndRunRunSet: run with master <%s> action <%s>\n",
					a.master, a.s);
#endif /* DEBUG */
			run(STRING_DEV, a.s, 0);
			popMasterStack();
			free(a.s);
		}

		setprompt();
#ifdef DEBUG
		if (Debug & 2)
		debugMessage("swapAndRunRunSet exiting: EDEN becoming quiescent"
				" unless queue() was called recently\n");
#endif /* DEBUG */

	}

	/* 'interrupted' might have been set at any point during swapAndRunRunSet */
	if (interrupted) {
		warning("interrupted: clearing both RunSets (current queue and todo)",
				"");
		clearRunSet(&RS1);
		clearRunSet(&RS2);
		interrupted = 0;
		queue("edeninterrupted();", "edeninterrupted", NULL);
	}

/*
#ifdef WEDEN_ENABLED
	if (Tcl_Eval(interp, "weden_FinishEdenRunSetProcessing") != TCL_OK) {
		fprintf(stderr, "Tcl error (edenEndRunSetProcessing): %s\n", Tcl_GetStringResult(interp));
	}
#endif
*/
	 processingRunSet = 0;

}

/* Add a line of script (cmd) to the tail of the non-active RunSet.
 master is the name of the agent responsible (eg "input",
 "interface"...). [Ash] */
void queue(char *cmd, char *master, Tcl_TimerToken tok) {
#ifdef DEBUG
	if (Debug & 2)
		debugMessage("queue: adding master <%s> timertoken <%" PRIdPTR "> action <%s>\n",
			master, (Int)tok, cmd);
#endif /* DEBUG */
	addAction((RS == &RS1) ? &RS2 : &RS1, cmd, master, tok);
	Tcl_DoWhenIdle(swapAndRunRunSet, 0);
}

#endif /* not TTYEDEN */

/* ttyeden's run-time machine is not interleaved with Tcl, since ttyeden
 does not include Tcl.  Note that ttyeden's checkRunSet is called by
 print_prompt_if_necessary.  The strategy is probably buggy at present
 -- but this is only a problem if people use todo() in a ttyeden model,
 which is rare.  [Ash] */
#ifdef TTYEDEN

void queue(char *cmd, char *master) {
#ifdef DEBUG
	if (Debug & 2)
		debugMessage("queue: adding master <%s> action <%s>\n", master, cmd);
#endif /* DEBUG */
	addAction((RS == &RS1) ? &RS2 : &RS1, cmd, master);
}

void checkRunSet(void)
{
	Action a;

	if (interrupted) {
		warning("interrupted: clearing both RunSets (current queue and todo)",
				"");
		clearRunSet(&RS1);
		clearRunSet(&RS2);
		interrupted = 0;
	}

#ifdef DEBUG
	if (Debug & 2) {
		debugMessage("checkRunSet start: RS->nitems=%d\n", RS->nitems);
		printRunSet(RS);															//      <<<<<<<<<<<---------- TODO: Need to see about getting runset sent as a complete debug output!
	}
#endif /* DEBUG */

	if (RS->nitems > 0) {
		/* There are some items in RS [Ash] */

		while (RS->nitems > 0) {
			a = getAction(RS);
			pushMasterStack(a.master);
#ifdef DEBUG
			if (Debug & 2)
				debugMessage("checkRunSet: RS->nitems=%d: run action %s\n",
					RS->nitems, a.s);
#endif /* DEBUG */
			run(STRING_DEV, a.s, 0);
			popMasterStack();
			free(a.s);
		}

	} else {
		/* The RS is empty: swap RSs [Ash] */

		/* Swap RunSets */
#ifdef DEBUG
		if (Debug & 2) debugMessage("checkRunSet: swapping RunSets\n");
#endif
		RS = (RS == &RS1) ? &RS2 : &RS1;

#ifdef DEBUG
		if (Debug & 2) {
			debugMessage("checkRunSet just swopped RS: RS->nitems=%d\n",
					RS->nitems);
			printRunSet(RS);
		}
#endif /* DEBUG */

	}
}

#endif /* TTYEDEN */

void eden_init(void) { /* INITIALIZATION */
	/* init builtin function, variables, etc */
	void install_custom_variables(void);
	register int i;
	register symptr s;

	/* install builtin C library functions */
	for (i = 0; blibtbl[i].name; i++) {
		s = install(blibtbl[i].name, basecontext, BLTIN, BLTIN, 0);
		s->inst = (Inst *) blibtbl[i].func;
		s->d.type = BLTIN;
		s->d.u.sym = s;
	}

	/* install integer C library functions */
	for (i = 0; ilibtbl[i].name; i++) {
		s = install(ilibtbl[i].name, basecontext, LIB, LIB, 0);
		s->inst = (Inst *) ilibtbl[i].func;
		s->d.type = LIB;
		s->d.u.sym = s;
	}

	/* install real (floating-point) C library functions */
	for (i = 0; rlibtbl[i].name; i++) {
		s = install(rlibtbl[i].name, basecontext, RLIB, RLIB, 0);
		s->inst = (Inst *) rlibtbl[i].func;
		s->d.type = RLIB;
		s->d.u.sym = s;
	}

	install_custom_variables(); /* custom-built */
}

// 																					<<<------- TO STILL DO (What values can this take ?) 
void terminate(int code) {
	exit(code);
}

void run_init(Inst * pc) { /* init before running */
	progp = pc;
	reset_entry_tbl();
	reset_compiler_flags();

#ifdef TTYEDEN
	prompt = prompt1;
#endif
}

int run(short type, void *ptr, char *name) { /* RUN A PROGRAM */
	/* type - SOURCE TYPE: FILE_DEV/STRING_DEV (note stdin is a file [Ash])*/
	/* ptr - FILE POINTER / STRING_DEV */
	/* name - FILE NAME (IF ANY) */
	/* EXECUTE UNTIL EOF OR ERROR OCCURRED */
	char errorflag = FALSE;
	Inst *savepc = progp;
	int token;
	extern int nextc;
	extern void init_lex(void), execute(Inst *);
#ifdef DEBUG
#define INITPTRSTUFFLEN 30
	/* for printing out the initial part of ptr in debugging output [Ash] */
	char initPtrStuff[INITPTRSTUFFLEN+1];
#endif

	/* It is good to use our custom bison.simple if we can.  Eden doesn't
	 apparently seem to need to token-at-a-time parsing (indeed, I've
	 had to modify things to make Eden work with this style of parser),
	 but I'm hoping that this might solve the problems we've had
	 recently with user input being locked out on occasion.  We also
	 benefit from my modification to bison.simple which prints out
	 information after a parse error, even if there are many
	 possibilities to show.  I've made my modifications here a
	 conditional compile as I'm not sure I've got it right!  [Ash] */
#define USE_CUSTOM_BISON_SIMPLE 1
#ifdef USE_CUSTOM_BISON_SIMPLE
	extern int yyparse(int);
	extern int yylex(void);
	extern void setyyparseinit(int);
	int parseret;
#else
	extern int yyparse(void);
#endif

	Inp_Dev++;
	Inp_Dev->frame = fp;
	Inp_Dev->name = name;

#define LINEBUFINITIALSIZE 256
	if (!Inp_Dev->linebuf) {
		Inp_Dev->linebuf = emalloc(LINEBUFINITIALSIZE);
		Inp_Dev->linebufsize = LINEBUFINITIALSIZE;
	}
	Inp_Dev->linebuf[0] = '\0';
	Inp_Dev->linebufend = -1;
	Inp_Dev->charno = -1;

	switch (Inp_Dev->type = type) {
	case FILE_DEV:
		Inp_Dev->ptr = ptr;
		break;

	case STRING_DEV:
		Inp_Dev->sptr = (char *) emalloc(strlen(ptr) + 1);
		strcpy(Inp_Dev->sptr, ptr);
		Inp_Dev->ptr = Inp_Dev->sptr;
		break;
	}
	Inp_Dev->newline = TRUE;
#ifdef TTYEDEN
	nlstatus();
#endif
	Inp_Dev->lineno = 0;
	Inp_Dev->lastc = nextc;

#ifdef DEBUG
	if (Debug&1) {
		if (type == STRING_DEV) {
			char *p; int n;
			for (p = ptr, n = 0; isprint(*p) && n<(INITPTRSTUFFLEN-3); p++, n++)
			initPtrStuff[n] = *p;
			initPtrStuff[n] = '\0';
			if ((n == INITPTRSTUFFLEN-3) || (*p != '\0'))
			strcat(initPtrStuff, "...");
		}
		debugMessage("MCSTAT run start: %s %s%s%s Inp_Dev level: %d\n",
				type == FILE_DEV ? "FILE_DEV" : "STRING_DEV",
				name ? name : "",
				name ? " " : "",
				type == STRING_DEV ? initPtrStuff : "",
				Inp_Dev - Input_Devices);
	}
#endif

	/* A matching call of longjmp(Inp_Dev->begin, val) will cause
	 execution to jump to here.  errorflag will be set to the val
	 passed above.  (K+R C, second edition, page 254).  [Ash] */
	DEBUGPRINT("run: setjmp\n", 0);
	errorflag = setjmp(Inp_Dev->begin);

#ifdef TTYEDEN
#ifdef HAVE_READLINE
	Inp_Dev->usereadline=1;
#else
	Inp_Dev->usereadline=0;
#endif
#endif

#if defined(TTYEDEN) && defined(HAVE_READLINE)
	/* don't use readline for input which is coming via a pipe */
	if (type == FILE_DEV) {
		struct stat stats;

		fstat(fileno((FILE *)Inp_Dev->ptr), &stats);

		/*  debugMessage("st_mode = %o\n", stats.st_mode); */

		if (S_ISFIFO(stats.st_mode)) {
			Inp_Dev->usereadline=0;
		}
	}
#endif

	init_lex(); /* reads in one character.  However I
	 think UNIX blocks until a complete
	 line has been read in the case of
	 ttyeden [Ash] */
	run_init(savepc);
	incGarbageLevel();
#ifdef TTYEDEN
	/* There seems to be a strange bug in gcc.  Under some (quite
	 normal) circumstances, when compiled with -O2, gcc seems to
	 optimise away Inp_Dev, leaving it = 0 at this point after the
	 longjmp to the setjmp above.  Unfortunately gdb seems to lie
	 about this - the value of 0 can only be seen by observing the
	 Seg fault that results and by manually inserting printfs below.
	 Working around it by setting it back to what it was before the
	 longjmp here.  [Ash] */
	//debugMessage("Inp_Dev = %d\n", Inp_Dev);
	if (errorflag) Inp_Dev = Inp_Dev_Save; /* work around bug in gcc [Ash] */
	//debugMessage("Inp_Dev = %d\n", Inp_Dev);

	print_prompt_if_necessary(0);
#endif

#ifdef USE_CUSTOM_BISON_SIMPLE
	while (!interrupted) {
		setyyparseinit(1); /* re-initialise the parser */
		while (!interrupted) {
			token = yylex();DEBUGPRINT("PARSER yylex() returned %d\n", token);
			if (!token)
				goto postexec;
			/* same effect as interrupted,
			 but no other effects */
			parseret = yyparse(token);DEBUGPRINT("PARSER yyparse() returned %d\n", parseret);
			if (parseret >= 0)
				break;
		}
		if (interrupted)
			break;
#else
		while (!interrupted && yyparse()) {
#endif

		execute(savepc);
		run_init(savepc);
		clearGarbage();
		incGarbageLevel();

#ifdef TTYEDEN
		print_prompt_if_necessary(1);
#endif
	}
	postexec: decGarbageLevel();

	/* finished, remove the device */
	nextc = Inp_Dev->lastc;
	switch (Inp_Dev->type) {
	case FILE_DEV:
		/* close the file, but not stdin */
		if ((FILE *) Inp_Dev->ptr != stdin)
			fclose((FILE *) Inp_Dev->ptr);
		break;

	case STRING_DEV:
		free(Inp_Dev->sptr);
		break;
	}
	--Inp_Dev; /* remove device */

	progp = savepc;
	freeheap();

	DEBUGPRINT2("MCSTAT run end: Inp_Dev level: %d, errorflag: %d\n",
			Inp_Dev - Input_Devices, errorflag);

	return errorflag; /* return error flag */
}

void user_trace(void) { /* EDEN function */
	Frame *p;

	fprintf(stderr, "\n");
	for (p = &frame[1]; p <= fp; p++)
		fprintf(stderr, p == fp ? "%s\n" : "%s->", p->sp->name);
}

void setLibLocation(char *location) {
	char *toPutEnv;
	char *cwd;

	if (libLocation)
		free(libLocation);

	libLocation = emalloc(255);

#ifndef __WIN32__
	if (location[0] != '/') {
		/* Location specified as a relative path, so make it absolute by
		 prepending the current working directory */
		if ((cwd = getcwd(NULL, PATH_MAX)) == NULL) {
			perror("Failed to get current working directory");
			exit(2);
		}
		strcpy(libLocation, cwd);
		free(cwd);
		strcat(libLocation, "/");
		strcat(libLocation, location);
	} else {
		strcpy(libLocation, location);
	}
#else
	strcpy(libLocation, location);
#endif

	/* Assign the result to an environment variable as well (so it can
	 be used by Tcl) */
	toPutEnv = emalloc(255);
	*toPutEnv = '\0';
	strcat(toPutEnv, "TKEDEN_LIB=");
	strcat(toPutEnv, libLocation);

	if (putenv(toPutEnv) == -1) {
		perror("Failed to putenv TKEDEN_LIB");
	}

	/* It appears we must not now free(toPutEnv) [Ash] */
}

#ifndef streq
#define streq(s1,s2) (strcmp(s1,s2)==0)
#endif

#ifdef DISTRIB

void printUsage()
{
	/* DISTRIB */
	fprintf(stderr, "Usage: %s [-s|-a] -c<channel number> -h<hostname> {-w<client login name>} {-l<lib directory>} {-v|-u} {-e <code>}|{<filename>} {-e <code>}|{<filename>}...\n",
			progname);
	fprintf(stderr,
			"       -s invoke as the server (\"superagent\")\n       -a invoke as a client (\"agent\")\n       -c channel number: 0,1,2,3, ..., 99\n       -h hostname of the machine running the dtkeden server (only required\n          if running as a client [-a])\n       -w dtkeden client login name\n       -l name of the directory containing the library files\n       -v output version information and exit\n       -u output this usage information and exit\n       -e execute 'code'\n       if file is '-', standard input will be read\nSee %s for more information\n", TKEDEN_WEB_SITE);
}
#elif defined(TTYEDEN)

void printUsage()
{
	/* TTYEDEN */
	fprintf(stderr,
			"Usage: %s [-l<lib directory>] {-v|-u} {-n|-i} {-e <code>|<filename>} {{-n|-i} {-e <code>|<filename>}}...\n",
			progname);
	fprintf(stderr,
			"       -l name of the directory containing the library files (optional,\n          but required for some functionality eg the definitive parser)\n       -n non-interactive mode (don't print prompts)\n       -i interactive mode (do print prompts)\n       -v output version information and exit\n       -u output this usage information and exit\n       -e execute 'code'\n       if file is '-', standard input will be read\nSee %s for more information\n", TKEDEN_WEB_SITE);
}

#elif defined(WEDEN_ENABLED) /* weden */

void printUsage()
{
	/* TKEDEN */
	fprintf(stderr,
			"Usage: %s {-l<lib directory>} {-v|-u} -y <relayerServerAddress> -z <relayServerPort> {-e <code>}|{<filename>} {-e <code>}|{<filename>}...\n",
			progname);
	fprintf(stderr,
			"       -l name of the directory containing the library files\n       -v output version information and exit\n       -u output this usage information and exit\n       -y connect to this 'relayServerAddress'\n       -z using this 'relayServerPort'\n       -e execute 'code'\n       if file is '-', standard input will be read\nSee %s for more information\n", TKEDEN_WEB_SITE);
}
#else /* probably TKEDEN */

void printUsage() {
	/* TKEDEN */
	fprintf(
			stderr,
			"Usage: %s {-l<lib directory>} {-v|-u} {-e <code>}|{<filename>} {-e <code>}|{<filename>}...\n",
			progname);
	fprintf(
			stderr,
			"       -l name of the directory containing the library files\n       -v output version information and exit\n       -u output this usage information and exit\n       -e execute 'code'\n       if file is '-', standard input will be read\nSee %s for more information\n",
			TKEDEN_WEB_SITE);
}
#endif /* probably TKEDEN */

/* Print the version number and exit (probably requested to do so via
 a command line option) [Ash] */
void printVersion(void) {
	fprintf(stderr, "%s: %s version %s (%s)\n", progname, TKEDEN_VARIANT,
			TKEDEN_VERSION, EDEN_SVNVERSION);
	fprintf(stderr, "Lib directory is %s\n", libLocation ? libLocation
			: "unknown");
#ifdef DISTRIB
	fprintf(stderr, "hostName is %s\n",
			hostName ? hostName : "unknown");
#endif
	fprintf(stderr,
			"Use the -u option for information on usage of command options\n");
	fprintf(stderr, "See %s for more information\n", TKEDEN_WEB_SITE);
	exit(0);
}

#ifdef DISTRIB
/* Figure out the name of this host.  This is used particularly by a
 dtkeden client to figure out which host to contact as the server.
 Note this setting may well be overridden by the user specifying a
 -h option.  [Ash] */
void setHostName(void)
{
	if (gethostname(hostName, MAXHOSTNAMELEN)) {
		fprintf(stderr, "%s: gethostname failed\n", progname);
		exit(1);
	}
	hostNamePtr = hostName;
}
#endif

#if defined(__WIN32__) && !defined(TTYEDEN)
RETSIGTYPE
confirmSig(int sig)
{
	signal(sig, SIG_DFL);
	fprintf(stderr, "Signal %d received - press return to continue\n", sig);
	getchar();
	raise(sig);
}
#endif

/* Create system Eden func and procs.  [Ash] */
void init_eden(void) {
	FILE *initFile;
	char *name = "/eden.eden";
	char fullname[255];
	char initStr[512];

	/* This hack comes from C FAQ Q 11.7 (eg
	 http://www.faqs.org/faqs/C-faq/faq).  It seems to be the only way
	 to get the value of a cpp constant into a C string constant [Ash] */
#define Str(x) #x
#define Xstr(x) Str(x)
	/* Want these to be accessible from ttyeden when libLocation is not
	 set, so can't read these from an init file.  [Ash] */
	run(
			STRING_DEV,
			"func cwd {\n  return _eden_internal_cwd();\n}\nproc cd {\n  _eden_internal_cd($1);\n  touch(&cwd);\n}\ndirname is _eden_internal_dirname;\nbasename is _eden_internal_basename;\nPI = " Xstr(M_PI) ";", 0);

#if !defined(__CYGWIN__) && !defined(HAVE_LIBGEN_H)
	/* system doesn't have dirname and basename (eg on Mac OS X), and so
	 _eden_internal_dirname and _eden_internal_basename won't have
	 been defined in builtin.c: resort to versions written in Eden
	 instead.  If we have system versions, we want to use those to
	 capture any platform specific stuff (eg cygwin). */
	run(
			STRING_DEV,
			"proc _eden_internal_dirname {\n  auto s;\n  ## if there are no slashes, the result is .\n  if (regmatch(\"/\", $[1]) == []) return \".\";\n  s = $[1];\n  ## remove trailing slashes and white space\n  s = regreplace(\"^(.*)\\s*/+\\s*$\", \"$1\", s);\n  ## find left portion\n  s = regreplace(\"^(.*)/[^/]*$\", \"$1\", s);\n  ## if we are left with the empty string, we want /\n  s = regreplace(\"^\\s*$\", \"/\", s);\n  return s;\n}\n\nproc _eden_internal_basename {\n  auto s;\n  s = $[1];\n  ## remove trailing slashes and white space\n  s = regreplace(\"^(.*)\\s*/+\\s*$\", \"$1\", s);\n  ## find right portion\n  s = regreplace(\"^.*/([^/]*)$\", \"$1\", s);\n  ## if we are left with the empty string, we want /\n  s = regreplace(\"^\\s*$\", \"/\", s);\n  return s;\n}\n",
			0);

#endif

// Indicates to EDEN if we are running web eden
#ifdef WEDEN_ENABLED
	run(STRING_DEV, "weden_UsingWebEden = 1;\n", 0);
#else
	run(STRING_DEV, "weden_UsingWebEden = 0;\n", 0);
#endif

	/* If it is possible, execute the initialisation file */

	if (!libLocation)
		return;

	strcpy(fullname, libLocation);
	strcat(fullname, name);

	if ((initFile = fopen(fullname, "r")) == 0)
		errorf("couldn't open initialisation file %s: %s", fullname,
				strerror(errno));

	run(FILE_DEV, initFile, name);
}

#if defined(TTYEDEN) && defined(HAVE_READLINE)
void initreadline(void) {
	/* Allow conditional parsing of the ~/.inputrc file */
	rl_readline_name = "ttyeden";

	/* Stop tab key attempting to expand into filenames */
	rl_bind_key('\t', rl_insert);
}
#endif

//Formally MAIN
int cadence_e_initialise(int argc, char *argv[]) { /* MAIN PROGRAM */

	/* Options processing [Ash] */
	char op; /* stores one option returned at a
	 time from getopt [Ash] */
	int argerr = 0; /* whether there has been an error
	 during option processing [Ash] */
	int usedArg = 1; /* the arguments processed so far - so
	 we can figure out later on how many
	 filenames are on the command line
	 [Ash] */
	extern char *optarg; /* getopt sets this when it comes
	 across an option with an argument [Ash] */
	extern int optind; /* this is an index into argv set by
	 getopt [Ash] */

	extern void initcode(void), init_LocalVarList(void);
	FILE *filein;
	char *name;
	symptr s;

#ifdef DISTRIB
	extern void init_LSDagentList(void);
#endif /* DISTRIB */

#ifndef TTYEDEN
	extern void EXinit(int, char **);
	extern void init_donald(void), init_scout(void);
#endif

#ifdef __WIN32__
	extern cygwin_conv_to_full_win32_path(const char *path, char *win32_path);
	char *cwd;
	char *cwdwin32style;
	char *toPutEnv;
	char *passToSetLibLocation;
#endif

#ifdef WANT_SASAMI
	extern void sa_init_sasami();
#endif /* WANT_SASAMI */

	char edenCmd[PATH_MAX + 100];

	basecontext = doste_context("base");

	/* save the arguments for use in the tcl "restart" command [Ash] */
	startupargc = argc;
	startupargv = argv;
	if ((startupcwd = getcwd(NULL, PATH_MAX)) == NULL) {
		perror("Failed to get current working directory -- we will not be able to restart.  Continuing anyway.");
	}

#ifdef INSTRUMENT
	insStart = gethrtime();
#endif /* INSTRUMENT */

#if defined(__APPLE__) && !defined(TTYEDEN)
	Tk_MacOSXSetupTkNotifier();
#endif

#if defined(__WIN32__) && !defined(TTYEDEN)
	/* Try and give the user a chance to see any fatal error messages
	 before tkeden quits and the DOS stdout/stderr window disappears
	 [Ash] */
	signal(SIGHUP, confirmSig);
	signal(SIGINT, confirmSig);
	signal(SIGQUIT, confirmSig);
	signal(SIGILL, confirmSig);
	signal(SIGKILL, confirmSig);
	signal(SIGBUS, confirmSig);
	signal(SIGSEGV, confirmSig);
	signal(SIGTERM, confirmSig);
#endif

	progname = argv[0];
#ifdef __WIN32__
	/* Let's try and derive the info required from the current working
	 directory, on Windows only [Ash] */
	if ((cwd = getcwd(NULL, PATH_MAX)) == NULL) {
		perror("Failed to get current working directory");
		exit(2);
	}
	cwdwin32style = emalloc(PATH_MAX);
	cygwin_conv_to_full_win32_path(cwd, cwdwin32style);
	free(cwd);

	// if we don't do this then Tcl_PutEnv falls over
#ifndef TTYEDEN
	TclInitEncodingSubsystem();
#endif

	toPutEnv = emalloc(255);
	*toPutEnv = '\0';
	/* This (did - a while ago) work manually...
	 strcat(toPutEnv, "TCL_LIBRARY=c:\\cygwin\\usr\\share\\tcl8.0");
	 */
	strcat(toPutEnv, "TCL_LIBRARY=");
	strcat(toPutEnv, cwdwin32style);
	strcat(toPutEnv, "\\share\\tcl8.4");
	putenv(toPutEnv);

	/* I think we can't free(toPutEnv) now */

	passToSetLibLocation = emalloc(255);
	*passToSetLibLocation = '\0';
	/* This works manually...
	 strcat(passToSetLibLocation, "c:\\cygwin\\home\\default\\tkeden1.15\\lib-tkeden");
	 */
	strcat(passToSetLibLocation, cwdwin32style);
	strcat(passToSetLibLocation, "\\lib-tkeden");
	setLibLocation(passToSetLibLocation);
	free(passToSetLibLocation);
#endif /* __WIN32__ */

#if defined(__APPLE__) && !defined(TTYEDEN)
	setMacOSXLibLoc();
#endif /* __APPLE__ and not TTYEDEN */

#ifndef TTYEDEN
	/* If we don't do this, on cygwin we get an error from Tcl_Init
	 "couldn't stat "": no such file or directory" [Ash] */
	/*  Tcl_FindExecutable(progname); */

	/* This is required by Tcl 8.2.0, otherwise the Tcl_PutEnv in
	 setLibLocation falls over due to an uninitialised Tcl encoding
	 system.  The initialisation of Tcl probably calls
	 TclInitEncodingSubsystem();, which is required. [Ash] */
	EXinitTcl(argc, argv);
	/* TclInitEncodingSubsystem(); */
#endif /* TTYEDEN */

#ifdef DISTRIB
	setHostName(); // RICHARD +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#endif /* DISTRIB */

#ifdef DISTRIB
	/* ':' in the getopt opt-string means that the preceding option has
	 an argument.  A '+' character starting the options string
	 requests getopt to stop options processing as soon as a
	 non-option argument is encountered (see getopt(3) on
	 Linux).  [Ash] */
	while ((op = getopt(argc, argv, "+sac:h:vul:ew:")) != EOF) {
#elif defined(TTYEDEN)
		while ((op = getopt(argc, argv, "+vunil:e")) != EOF) {
#elif defined(WEDEN_ENABLED)
			while ((op = getopt(argc, argv, "+y:z:vul:e")) != EOF) { // RICHARD: In weden we get the relayer server ip and port to use (odd arg id's because others taken)
#else
	/* probably tkeden */
#ifdef __APPLE__
	/* For some reason, -p (perhaps -psn actually) is passed on Mac OS
	 X: ignore it */
	while ((op = getopt(argc, argv, "+vul:ep:")) != EOF) {
#else
	while ((op = getopt(argc, argv, "+vul:e")) != EOF) {
#endif /* __APPLE__ */
#endif

		/* -e, -n, -i must come after the other options which are
		 processed by getopt (ie they represent the start of the
		 filename list...) [Ash] */
		if ((op == 'e') || (op == 'n') || (op == 'i')) {
			optind--; /* we want the filename reading while
			 loop below to process this argument */
			break;
		}

		char tclCommand[60];
		switch (op) {
#ifdef DISTRIB																
		case 's':
		isServer = 1; usedArg++; break;
		case 'a':
		isServer = 0; usedArg++; break;
		case 'c':
		socketPort = socketPort + atoi(optarg); usedArg += 2; break;
		case 'h':
		strcpy(hostName, optarg); usedArg += 2; break;
		case 'w':
		/* dtkeden client login name: used 'w' for 'who' as there aren't any
		 sensible single letters left! */
		strcpy(loginname, optarg); usedArg += 2; break;
#endif

#ifdef WEDEN_ENABLED														// RICHARD: Note. Weden can only run on linux based servers as tkeden
		case 'y':

		strcpy(tclCommand, "set WedenRelayServerHostName ");
		strcat(tclCommand, optarg);

		if (Tcl_Eval(interp, tclCommand) != TCL_OK) {
			fprintf(stderr, "Tcl error (%s): %s\n", tclCommand, Tcl_GetStringResult(interp));
			exit(-1);
		}
		usedArg += 2;
		break;
		case 'z':
		strcpy(tclCommand, "set WedenRelayServerHostPort ");
		strcat(tclCommand, optarg);

		if (Tcl_Eval(interp, tclCommand) != TCL_OK) {
			fprintf(stderr, "Tcl error (%s): %s\n", tclCommand, Tcl_GetStringResult(interp));
			exit(-1);
		}
		usedArg += 2;
		break;
#endif
		case 'u':
			printUsage();
			exit(0);
		case 'l':
			setLibLocation(optarg);
			usedArg += 2;
			break;
		case 'v':
			printVersion(); /* does not return */
		case 'p':
			/* For some reason, -p (perhaps -psn actually) is passed on Mac
			 OS X: ignore it */
			fprintf(stderr, "argument p %s\n", optarg);
			break;

		case '?': /* unknown option character */
		case ':': /* missing parameter for one of the options */
			argerr++;
			break;
		}

		if (argerr) {
			printUsage();
			exit(-1);
		}
	}
	


	/* TTYEDEN still works if the -l argument is not given, but some
	 facilities (eg the definitive parser) will not be available */
#ifndef TTYEDEN
	if (!libLocation) {
		fprintf(
				stderr,
				"%s: Could not find library files location (try using the -l argument)\n",
				progname);
		exit(-1);
	}
#endif /* TTYEDEN */

#ifdef DISTRIB
	if (socketPort > 9099) {
		/* The user enters a number from 0-99, but actually we use a
		 socket number from 9000 upwards :) [Ash, with Patrick] */
		fprintf(stderr, "Channel number must be less than 100\n");
		printUsage();
		exit(-1);
	}

	if (isServer) {
		if (strcmp(loginname, "") != 0) {
			fprintf(stderr, "-w option not valid with -s (login name is for client only, not for server)\n");
			printUsage();
			exit(-1);
		}
	}
#endif

#ifdef TTYEDEN
#ifdef HAVE_READLINE
	initreadline();
#endif
	prompt = prompt1;
#endif
	setbuf(stdout, 0);
	clearEntryStack();
	clearMasterStack();
	pushMasterStack("system");

#ifdef DISTRIB
	(s = install("EveryOneAllowed", basecontext, VAR, INTEGER, 1))->changed = FALSE;
	EveryOneAllowed = &s->d.u.i;
#endif /* DISTRIB */

	
	
	(s = install("autocalc", basecontext, VAR, INTEGER, 1))->changed = FALSE;
	autocalc = &s->d.u.i;

	(s = install("eden_error_index_range", basecontext, VAR, INTEGER, 1))->changed
			= FALSE;
	eden_error_index_range = &s->d.u.i;

	(s = install("eden_notice_undef_reference", basecontext, VAR, INTEGER,
					0))->changed = FALSE;
	eden_notice_undef_reference = &s->d.u.i;

	(s = install("eden_backticks_dependency_hack", basecontext, VAR, INTEGER,
					0))->changed = FALSE;
	eden_backticks_dependency_hack = &s->d.u.i;

#ifdef TTYEDEN
	(s = install("eden_prompt", basecontext, VAR, INTEGER, 1))->changed = FALSE;
	eden_prompt_sym = s;
#endif /* TTYEDEN */

#ifdef DISTRIB
	(s = install("synchronize", basecontext, VAR, INTEGER, 0))->changed = FALSE;
	synchronize = &s->d.u.i;

	(s = install("higherPriority", basecontext, VAR, INTEGER, 0))->changed = FALSE;
	higherPriority = &s->d.u.i;

	(s = install("propagateType", basecontext, VAR, INTEGER, 1))->changed = FALSE;
	propagateType = &s->d.u.i;
#endif /* DISTRIB */

	initRunSet(&RS1);
	initRunSet(&RS2);
	RS = &RS1;
	
	

#ifndef TTYEDEN
	EXinit(argc, argv);
#endif

	/*signal(SIGINT, Control_C);*/
	eden_init();
	initcode();
	init_LocalVarList();
#ifdef DISTRIB
	init_LSDagentList(); /* initial LSDagentList  --sun */
#endif /* DISTRIB */

	init_eden();

#ifndef TTYEDEN
	init_donald();
	init_scout();
#ifdef WANT_SASAMI
	sa_init_sasami();
#endif /* WANT_SASAMI */
#endif

	popMasterStack();
	pushMasterStack("initialisation");

	/* Figure out where the end of the options (and start of the
	 filename list) is [Ash] */
	gargc = argc - usedArg;
	gargv = argv + optind;

	setjmp(start); /* set resume point */
	startValid++;

	while (gargc-- > 0) {
		/* Recall that argv is an array of pointers to the
		 already-separated command arguments [Ash] */
		name = *gargv++;

		/* We seem to end up with gargc = 1 on Mac OS X, but gargv is
		 NULL. */
		if (name == NULL)
			continue;

		if (streq(name, "-")) {
			/* - is shorthand for stdin */
			run(FILE_DEV, stdin, name);
			continue;
		}

#ifdef TTYEDEN
		if (streq(name, "-n")) {
			eden_prompt_sym->d.u.i = FALSE;
			change(eden_prompt_sym, TRUE);
			continue;
		}
		if (streq(name, "-i")) {
			eden_prompt_sym->d.u.i = TRUE;
			change(eden_prompt_sym, TRUE);
			continue;
		}
#endif

		if (streq(name, "-e")) {
			/* Execute the following argument as a string [Ash] */
			char *argIn, *argProcessed, *inp, *outp, c;

			argIn = *gargv++;
			gargc--; /* we've consumed another argument */

			/* translate "\n" to newline */
			argProcessed = emalloc(strlen(argIn) + 1);
			inp = argIn;
			outp = argProcessed;
			do {
				c = *inp++;

				if ((c == '\\') && (*inp == 'n')) {
					inp++;
					*outp++ = '\n';
				} else {
					*outp++ = c;
				}
			} while (c != '\0');

			appendHist(argProcessed);
			run(STRING_DEV, argProcessed, 0);

			free(argProcessed);
#ifdef TTYEDEN
			currentNotation = EDEN; /* fudge but which makes the prompt
			 correct [Ash] */
#else
			setprompt();
#endif
			continue;
		}

		if (*name == '-') {
			/* First character is -, but it isn't one of the above
			 options, so print a message and carry on */
			fprintf(stderr, "%s: unknown argument %s\n", progname, name);
			continue;
		}

		/* else a filename */
		if ((filein = fopen(name, "r"))) {
			/* Do this on WIN32 only, as it messes things up when starting
			 tkeden with filename arguments which include relative
			 pathnames - a problem on UNIX as most people use the
			 command line to start tkeden, but on WIN32 this hack is
			 usually necessary so that files can be dragged onto the
			 tkeden icon and then include other files from that
			 directory successfully.  [Ash] */
#ifdef __WIN32__
			/* cd into the directory containing the file, to give any Eden
			 include(...) procedures a chance [Ash] */
			sprintf(edenCmd, "%%eden\ncd(dirname(\"%s\"));\n", name);
			appendHist(edenCmd);
			run(STRING_DEV, edenCmd, 0);

			/* this whole command is a bit of a lie really, but it should
			 help the user to appreciate what is going on [Ash] */
			sprintf(edenCmd, "include(basename(\"%s\")); /* (invoked from command line) */\n", name);
			appendHist(edenCmd);
#else
			/* On UNIX, don't do the cd'ing but do leave something in the
			 history [Ash] */
			sprintf(
					edenCmd,
					"%%eden\ninclude(\"%s\"); /* (invoked from command line) */\n",
					name);
			appendHist(edenCmd);
#endif

			run(FILE_DEV, filein, name);
#ifndef TTYEDEN
			setprompt();
#endif
			continue;
		} else {
			fprintf(stderr, "%s: can't open %s\n", progname, name);
		}
	}

	popMasterStack();

#ifdef TTYEDEN
	pushMasterStack("input");
	setjmp(start); /* set resume point */
	run(FILE_DEV, stdin, "stdin");
	popMasterStack();

#else /* {d}tkeden */
	setjmp(start); /* set resume point */
	

#endif /* TTYEDEN */
}

void cadence_e_update() {
	Tcl_DoOneEvent(TCL_DONT_WAIT);
}


void cadence_e_finalise() {
	terminate(0);
}
