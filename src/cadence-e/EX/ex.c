/*
 * $Id: ex.c,v 1.26 2002/02/27 16:31:55 cssbz Exp $
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

/* [Ben] Check for Win32 */
#ifndef __WIN32__
#   if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__)
#	define __WIN32__
#   endif
#endif

static char rcsid[] = "$Id: ex.c,v 1.26 2002/02/27 16:31:55 cssbz Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <tcl.h>
#include <tk.h>

#include <limits.h>		/* for PATH_MAX */

#include <unistd.h>		/* for exec...() */

#include "../../../../../config.h"
#include "../Eden/notation.h"
#include "../Eden/eden.h"
#include "../version.h"
#include "../Eden/runset.h"

#ifdef DISTRIB
#include <netdb.h>		/* for MAXHOSTNAMELEN on Solaris [Ash] */
#include <sys/param.h>		/* for MAXHOSTNAMELEN on Linux [Ash] */
#endif

#include "../Eden/emalloc.h"
#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

#ifdef __CYGWIN__
#include <process.h> /* for spawnvp / _P_OVERLAY ... */
#endif

extern int  run(short, void *, char *);
extern void warningf(char *, ...);
extern void pushMasterStack(char *);
extern void popMasterStack(void);
extern void evaluate(char *, char *);
extern void terminate(int);



#ifdef DISTRIB
extern int  dumpLSD(ClientData, Tcl_Interp *, int, char *[]);
extern int  getNotation(ClientData, Tcl_Interp *, int, char *[]);  /* sun */
#endif /* DISTRIB */

#include <tk.h>
/* Tcl caches environment variables: so we must use Tcl's versions of
   the getenv and putenv functions [Ash] */
/* Tcl 8.2.0 Makefile implies don't use the Tcl getenv [Ash] */
#define putenv Tcl_PutEnv
#define setenv TclSetEnv
#define unsetenv TclUnsetEnv
#include <tcl.h>

Tcl_Interp *interp;
Tk_Window   mainWindow;

#ifdef USE_TCL_CONST84_OPTION

static int  edenCmd(ClientData, Tcl_Interp *, int, CONST84 char *[]);
static int  wedenCmd(ClientData, Tcl_Interp *, int, CONST84 char *[]); // Processes weden commands [richard]
static int  todoCmd(ClientData, Tcl_Interp *, int, CONST84 char *[]);
static int  evaluateCmd(ClientData, Tcl_Interp *, int, CONST84 char *[]);
static int  quitCmd(ClientData, Tcl_Interp *, int, CONST84 char *[]);
static int  refreshCmd(ClientData, Tcl_Interp *, int, CONST84 char *[]);
static int  gotoCmd(ClientData, Tcl_Interp *, int, CONST84 char *[]);
static int  restartCmd(ClientData, Tcl_Interp *, int, CONST84 char *[]);

extern int  eden_interrupt_handler(ClientData, Tcl_Interp *, int, CONST84 char *[]);
extern int  dumpeden(ClientData, Tcl_Interp *, int, CONST84 char *[]);
extern int  dumpscout(ClientData, Tcl_Interp *, int, CONST84 char *[]);
extern int  dumpdonald(ClientData, Tcl_Interp *, int, CONST84 char *[]);
extern int  viewOptions(ClientData, Tcl_Interp *, int, CONST84 char *[]);

#else

static int  edenCmd(ClientData, Tcl_Interp *, int, char *[]);
static int  wedenCmd(ClientData, Tcl_Interp *, int, char *[]); // Processes weden commands [richard]
static int  todoCmd(ClientData, Tcl_Interp *, int, char *[]);
static int  evaluateCmd(ClientData, Tcl_Interp *, int, char *[]);
static int  quitCmd(ClientData, Tcl_Interp *, int, char *[]);
static int  refreshCmd(ClientData, Tcl_Interp *, int, char *[]);
static int  gotoCmd(ClientData, Tcl_Interp *, int, char *[]);
static int  restartCmd(ClientData, Tcl_Interp *, int, char *[]);

extern int  eden_interrupt_handler(ClientData, Tcl_Interp *, int, char *[]);
extern int  dumpeden(ClientData, Tcl_Interp *, int, char *[]);
extern int  dumpscout(ClientData, Tcl_Interp *, int, char *[]);
extern int  dumpdonald(ClientData, Tcl_Interp *, int, char *[]);
extern int  viewOptions(ClientData, Tcl_Interp *, int, char *[]);

#endif

int         Tcl_AppInit(Tcl_Interp *);
void        EXinit(int, char **);

extern char *ex_sptr, ex_sbuf[];
char       *sptr;
extern char *progname;
extern char *libLocation;

#define STRING_DEV      0

/* This implements the Tcl "eden" command [Ash] */
#ifdef USE_TCL_CONST84_OPTION
static int edenCmd(ClientData clientData, Tcl_Interp * interp, int argc, CONST84 char *argv[])
#else
static int edenCmd(ClientData clientData, Tcl_Interp * interp, int argc, char *argv[])
#endif

{
    notationType notation;

    if (argc != 2) {
    	warningf("Tcl: eden: wrong # of arguments - expecting 1, got %d", argc - 1);
    }
    notation = currentNotation;
    changeNotation(EDEN);
    pushMasterStack("user");
    int runResult = run(STRING_DEV, argv[1], 0);
    popMasterStack();
    changeNotation(notation);
    return TCL_OK;
}

#ifdef USE_TCL_CONST84_OPTION
static int todoCmd(ClientData clientData, Tcl_Interp * interp, int argc, CONST84 char *argv[])
#else
static int todoCmd(ClientData clientData, Tcl_Interp * interp, int argc, char *argv[])
#endif

{
    if (argc != 2) {
	warningf("Tcl: todo: wrong # of arguments - expecting 1, got %d",
		 argc - 1);
    }
#ifndef TTYEDEN
    queue(argv[1], "interface", NULL);
#else
    queue(argv[1], "interface", NULL);
#endif
    return TCL_OK;
}

// Handels eden input commands [richard]
#ifdef USE_TCL_CONST84_OPTION
static int wedenCmd(ClientData clientData, Tcl_Interp * interp, int argc, CONST84 char *argv[])
#else
static int wedenCmd(ClientData clientData, Tcl_Interp * interp, int argc, char *argv[])
#endif

{
    if (argc != 2) {
    	warningf("Tcl: weden: wrong # of arguments - expecting 1, got %d", argc - 1);
    }
    evaluate(argv[1], "input");
    return TCL_OK;
}

#ifdef USE_TCL_CONST84_OPTION
static int evaluateCmd(ClientData clientData, Tcl_Interp * interp, int argc, CONST84 char *argv[])
#else
static int evaluateCmd(ClientData clientData, Tcl_Interp * interp, int argc, char *argv[])
#endif

{
    if (argc != 2) {
	warningf("Tcl: evaluate: wrong # of arguments - expecting 1, got %d",
		argc - 1);
    }
    evaluate(argv[1], "input");
    return TCL_OK;
}

#ifdef USE_TCL_CONST84_OPTION
static int quitCmd(ClientData clientData, Tcl_Interp * interp, int argc, CONST84 char *argv[])
#else
static int quitCmd(ClientData clientData, Tcl_Interp * interp, int argc, char *argv[])
#endif

{
    terminate(0);
    return TCL_ERROR;
}

#ifdef USE_TCL_CONST84_OPTION
static int restartCmd(ClientData clientData, Tcl_Interp * interp, int argc, CONST84 char *argv[])
#else
static int restartCmd(ClientData clientData, Tcl_Interp * interp, int argc, char *argv[])
#endif

{
    extern int startupargc;
    extern char **startupargv;
    extern char *startupcwd;
    pid_t pid;

/* 
    fprintf(stderr, "startupargc = %d\n", startupargc);
    fprintf(stderr, "startupargv[0] = %s\n", startupargv[0]);
    fprintf(stderr, "startupargv[1] = %s\n", startupargv[1]);
 */

    /* Need to restore the initial working directory as it is changed
       whilst EDEN runs by include()'ing models etc */
    chdir(startupcwd);

#ifdef __CYGWIN__
    /* The fork / execvp stuff below works on cygwin, but we are left with
       a tkeden process in some busy loop that takes 100% CPU.  I suspect
       this is some bug or other in cygwin.  The following seems to work,
       though.  spawnvp doesn't seem to be documented by cygwin (sigh): the
       only docs I can find are at 
         http://www.qnx.com/developers/docs/momentics621_docs/neutrino/
           lib_ref/s/spawnvp.html
       [Ash] */
    spawnvp(_P_OVERLAY, startupargv[0], startupargv);

#else /* not CYGWIN */
    /* Seems can't just exec the EDEN binary again, perhaps because
       file descriptors aren't closed?  So fork a new EDEN, and stop
       this one. */
    switch (pid = fork()) {
      case -1:
        perror("can't fork");
        exit(-1);
      case 0:
        /* this process is the newly forked process -- start up again
           with the original arguments */
	fprintf(stderr, "starting new EDEN process...\n");
        if (execvp(startupargv[0], startupargv) == -1) {
          perror("can't exec");
          exit(-1);
        }
      default:
        /* this process is the old process -- just stop */
        fprintf(stderr, "stopping old EDEN process...\n");
        exit(0);
    }
#endif

    /* should never get here */
    return TCL_ERROR;
}

#ifdef USE_TCL_CONST84_OPTION
static int refreshCmd(ClientData clientData, Tcl_Interp * interp, int argc, CONST84 char *argv[])
#else
static int refreshCmd(ClientData clientData, Tcl_Interp * interp, int argc, char *argv[])
#endif

{
    Tk_Window   tkwin;
    XEvent      e;

    if (argc != 2) {
	warningf("Tcl: refresh: wrong # of arguments - expecting 1, got %d",
		 argc - 1);
    }
    tkwin = Tk_NameToWindow(interp, argv[1], mainWindow);
    e.xexpose.type = Expose;
    e.xexpose.send_event = True;
    e.xexpose.display = Tk_Display(tkwin);
    e.xexpose.window = Tk_WindowId(tkwin);
    e.xexpose.x = 0;
    e.xexpose.y = 0;
    e.xexpose.width = Tk_Width(tkwin);
    e.xexpose.height = Tk_Height(tkwin);
    e.xexpose.count = 1;
    XSendEvent(Tk_Display(tkwin), Tk_WindowId(tkwin), True, NoEventMask, &e);
    return TCL_OK;
}

#ifdef USE_TCL_CONST84_OPTION
static int gotoCmd(ClientData clientData, Tcl_Interp * interp, int argc, CONST84 char *argv[])
#else
static int gotoCmd(ClientData clientData, Tcl_Interp * interp, int argc, char *argv[])
#endif

{
  /* I guess removed from Windows by [Ben] due to different
     interaction style there... although I'm not even sure that this
     function is used at all. [Ash] */
#ifndef __WIN32__
    Tk_Window   tkwin;

    if (argc != 4) {
	interp->result = "wrong # args";
	return TCL_ERROR;
    }
    if (strlen(argv[1]) == 0) {
	XWarpPointer(Tk_Display(mainWindow), 0, None,
		     0, 0, 0, 0, atoi(argv[2]), atoi(argv[3]));
    } else {
	tkwin = Tk_NameToWindow(interp, argv[1], mainWindow);
	XWarpPointer(Tk_Display(tkwin), 0, Tk_WindowId(tkwin),
		     0, 0, 0, 0, atoi(argv[2]), atoi(argv[3]));
    }
    interp->result = "";
    return TCL_OK;
#endif
}

/* Tcl_Eval with Error Check - uses Tcl_EvalEx for speed if we have it
   [Ash] */
#ifdef HAVE_TCL_EVALEX
#define TCL_EVALEX(i, s, n, f) Tcl_EvalEx(i, s, n, f)
#else
#define TCL_EVALEX(i, s, n, f) Tcl_Eval(i, s)
#endif

inline void Tcl_EvalEC(Tcl_Interp * interp, char *script) {
#ifdef DEBUG
  if (Debug & 128)
    fprintf(stderr, "Tcl_EvalEC(\"%s\")\n", script);
#endif
  /* sendCompleteFormattedWedenMessage("<b><item type=\"tcloutput\"><![CDATA[TCL_Eval: %s\n]]></item></b>", script);  [REENABLE] */
  if (TCL_EVALEX(interp, script, -1, 0) != TCL_OK) {
    errorf("Tcl error (Tcl_EvalEC): %s", Tcl_GetStringResult(interp));
  }
}

/* Tcl_GlobalEval with Error Check - uses Tcl_EvalEx for speed [Ash] */
inline void Tcl_GlobalEvalEC(Tcl_Interp * interp, char *script) {
#ifdef DEBUG
  if (Debug & 128)
    fprintf(stderr, "Tcl_GlobalEvalEC(\"%s\")\n", script);
#endif
  if (TCL_EVALEX(interp, script, -1, TCL_EVAL_GLOBAL) != TCL_OK) {
    errorf("Tcl error (Tcl_GlobalEvalEC): %s", Tcl_GetStringResult(interp));
  }
}

#ifdef __CYGWIN__
int cygwin_conv_to_full_win32_path_tcl(ClientData clientData,
				       Tcl_Interp *interp,
				       int argc, char *argv[]) {
  extern cygwin_conv_to_full_win32_path(const char *path, char *win32_path);
  char winpath[PATH_MAX];

  if (argc != 2) {
    warningf("cygwin_conv_to_full_win32_path: requires 1 argument, got %d",
	     argc - 1);
  }

  if (!cygwin_conv_to_full_win32_path(argv[1], winpath)) {
    Tcl_SetResult(interp, winpath, TCL_STATIC);
    return TCL_OK;
  } else {
    return TCL_ERROR;
  }
}
#endif


int Tcl_AppInit(Tcl_Interp * interp)
{
    if (Tcl_Init(interp) == TCL_ERROR)
	return TCL_ERROR;
    if (Tk_Init(interp) == TCL_ERROR)
	return TCL_ERROR;

#ifdef __APPLE__
    TkMacOSXInitAppleEvents(interp);
    // The next line is not required with Mac OS X 10.2.4, Tcl/Tk 8.4.2
    //    TkMacOSXInitMenus(interp);
#endif

    
    
    Tcl_CreateCommand(interp, "quit", (Tcl_CmdProc *)quitCmd, NULL, NULL);
    Tcl_CreateCommand(interp, "evaluate", (Tcl_CmdProc *)evaluateCmd, NULL, NULL);
    Tcl_CreateCommand(interp, "eden", (Tcl_CmdProc *)edenCmd, NULL, NULL);
    Tcl_CreateCommand(interp, "weden", (Tcl_CmdProc *)wedenCmd, NULL, NULL);
    Tcl_CreateCommand(interp, "todo", (Tcl_CmdProc *)todoCmd, NULL, NULL);
    Tcl_CreateCommand(interp, "refresh", (Tcl_CmdProc *)refreshCmd, NULL, NULL);
    Tcl_CreateCommand(interp, "goto", (Tcl_CmdProc *)gotoCmd, NULL, NULL);
    Tcl_CreateCommand(interp, "dumpeden", (Tcl_CmdProc *)dumpeden, NULL, NULL);
    Tcl_CreateCommand(interp, "dumpscout", (Tcl_CmdProc *)dumpscout, NULL, NULL);
    Tcl_CreateCommand(interp, "dumpdonald", (Tcl_CmdProc *)dumpdonald, NULL, NULL);
    Tcl_CreateCommand(interp, "setupViewOptions", (Tcl_CmdProc *)viewOptions, NULL, NULL);
    Tcl_CreateCommand(interp, "interrupt", (Tcl_CmdProc *)eden_interrupt_handler, NULL, NULL);
    Tcl_CreateCommand(interp, "restart", (Tcl_CmdProc *)restartCmd, NULL, NULL);
#ifdef DISTRIB
    Tcl_CreateCommand(interp, "dumpLSD", (Tcl_CmdProc *)dumpLSD, NULL, NULL);
    Tcl_CreateCommand(interp, "getOldNotation", (Tcl_CmdProc *)getNotation, NULL, NULL);
    Tcl_CreateCommand(interp, "getCurrentNotation", (Tcl_CmdProc *)getNotation, NULL, NULL);
#endif /* DISTRIB */

#ifdef __WIN32__
    Tcl_CreateCommand(interp, "cygwin_conv_to_full_win32_path", 
		      cygwin_conv_to_full_win32_path_tcl, NULL, NULL);
#endif

#ifdef FAILED_CONSOLE_ATTEMPT
    /* This doesn't work: tcl output via puts etc goes to this console
       window (and Tcl commands typed there are executed!), but output
       from Eden to stdout etc does not */
#ifdef __APPLE__
    /*
     * If we don't have a TTY, then use the Tk based console
     * interpreter instead.
     */

    if (ttyname(0) == NULL) {
      Tk_InitConsoleChannels(interp);
      /*      Tcl_RegisterChannel(interp, Tcl_GetStdChannel(TCL_STDIN));
      Tcl_RegisterChannel(interp, Tcl_GetStdChannel(TCL_STDOUT));
      Tcl_RegisterChannel(interp, Tcl_GetStdChannel(TCL_STDERR));
      */
      Tcl_RegisterChannel(interp, Tcl_MakeFileChannel(stdin, TCL_READABLE));
      Tcl_RegisterChannel(interp, Tcl_MakeFileChannel(stdout, TCL_WRITABLE));
      Tcl_RegisterChannel(interp, Tcl_MakeFileChannel(stderr, TCL_WRITABLE));

      if (Tk_CreateConsoleWindow(interp) == TCL_ERROR) {
	return TCL_ERROR;
      }
      Tcl_Eval(interp, "console show");
    }
#endif
#endif /* FAILED_CONSOLE_ATTEMPT */

    return TCL_OK;
}


void
EXinitTcl(int argc, char *argv[])
{
    interp = Tcl_CreateInterp();

    /* The following is copied from
       /dcs/emp/empublic/src/tk8.3.3/generic/tkMain.c
    */

    Tcl_FindExecutable(argv[0]);

    /*
     * Set the "tcl_interactive" variable.
     */

    Tcl_SetVar(interp, "tcl_interactive", "1", TCL_GLOBAL_ONLY);


    /* Tcl_SetVar(interp, "argv0", argv[0], TCL_GLOBAL_ONLY); */

    /*
     * Invoke application-specific initialization.
     */
    if (Tcl_AppInit(interp) != TCL_OK) {
      fprintf(stderr, "Tcl_AppInit failed: %s\n", interp->result);
      exit(1);
    }


    /* Set the "tcl_interactive" variable. */
    //Tcl_SetVar(interp, "tcl_interactive", "1", TCL_GLOBAL_ONLY);

    /* Invoke application-specific initialization. */
    //if (Tcl_AppInit(interp) != TCL_OK) {
    //fprintf(stderr, "Tcl_AppInit failed: %s\n", interp->result);
    //exit(1);
    //    }
}

void
EXinit(int argc, char *argv[])
{
    extern char *progname;
#ifdef DISTRIB
    extern char hostName[MAXHOSTNAMELEN]; /* provided in Eden/main.c [Ash] */
    extern int socketPort;  /* for distributed tkEden --sun */
    extern char *current_Notation;
    extern char *hostNamePtr;
    extern Int *synchronize;
    extern int isServer;
    extern char *loginnamePtr;
#endif /* DISTRIB */
#ifdef WEDEN_ENABLED
    char       *name = "/wedenio.tcl";
#else
    char       *name = "/edenio.tcl";
#endif

    extern char currentAgentName[128];
    extern char currentAgentType[3];
    extern char *currentAgentNamePtr;
    extern char *currentAgentTypePtr;
    char        fullname[255];
    /* This seems safe to do: if $DISPLAY is unset, Tcl_AppInit will
       have failed earlier [Ash] */
    char       *display = getenv("DISPLAY");
    char       *class = (char *) ckalloc((unsigned) (strlen(progname) + 1));
    char *envcontents;
    /* These must be static so they don't go out of scope and
       disappear, leaving Tcl saying "where are they", causing Seg
       faults etc.  D'oh!  [Ash] */
    static char *version;		/* version of tkeden [Ash] */
    static char *svnversion;	/* Subversion revision [Ash] */
    static char *variant;		/* whether tty, tk, d, ? [Ash] */
    static char *webSite;		/* address of the EM web site [Ash] */
    static char *win32version;	/* Win32 version number [Ben] */
    static char *sasamiAvail;	/* whether we have sasami [Ash] */
    static char *apple;		/* whether we are running on an Apple */

    /* EXinitTcl(argc, argv); */

    strcpy(class, progname);
    class[0] = toupper((unsigned char) class[0]);

    /* EXinitTcl() should have already been called by main.c [Ash] */

    mainWindow = Tk_MainWindow(interp);
    ckfree(class);
    if (mainWindow == NULL) {
	fprintf(stderr, "%s\n", interp->result);
	exit(1);
    }

    /* Work out file name of Tcl interface code */
    strcpy(fullname, libLocation);
    strcat(fullname, name);

    currentAgentNamePtr = currentAgentName;
    currentAgentTypePtr = currentAgentType;

#ifdef DISTRIB
    if (isServer) {
      /* DISTRIB, server */
       Tcl_LinkVar(interp, "svcPort", (char *) &socketPort, TCL_LINK_INT);
       Tcl_LinkVar(interp, "eshost", (char *) &hostNamePtr, TCL_LINK_STRING);
       Tcl_LinkVar(interp, "oldNotation", (char *) &current_Notation,
		   TCL_LINK_STRING);
       Tcl_LinkVar(interp, "oldAgentType", (char *) &currentAgentTypePtr,
		   TCL_LINK_STRING);
       Tcl_LinkVar(interp, "oldAgentName", (char *) &currentAgentNamePtr,
		   TCL_LINK_STRING);
    } else {
      /* DISTRIB, client */
       Tcl_LinkVar(interp, "esport", (char *) &socketPort, TCL_LINK_INT);
       Tcl_LinkVar(interp, "eshost", (char *) &hostNamePtr, TCL_LINK_STRING);
       Tcl_LinkVar(interp, "currentNotation", (char *) &current_Notation,
		   TCL_LINK_STRING);
       Tcl_LinkVar(interp, "dtkedenloginname", (char *) &loginnamePtr,
           TCL_LINK_STRING);
    }
    /* printf("sockPort = %i", socketPort); */
    Tcl_LinkVar(interp, "synchronize", (char *) synchronize, TCL_LINK_INT);

    Tcl_LinkVar(interp, "_dtkeden_isServer", (char *) &isServer,
		TCL_LINK_BOOLEAN | TCL_LINK_READ_ONLY);
#endif /* DISTRIB */

    /* Create Tcl variables... */
    version = Tcl_Alloc(TKEDEN_VERSION_MAX_LEN);
    *version = 0;
    strcat(version, TKEDEN_VERSION);
    Tcl_LinkVar(interp, "_tkeden_version", (char *) &version,
		TCL_LINK_STRING | TCL_LINK_READ_ONLY);

    svnversion = Tcl_Alloc(50); /* assume svnversion string will never be huge */
    *svnversion = 0;
    strcat(svnversion, EDEN_SVNVERSION);
    Tcl_LinkVar(interp, "_eden_svnversion", (char *) &svnversion,
		TCL_LINK_STRING | TCL_LINK_READ_ONLY);

   variant = Tcl_Alloc(TKEDEN_VARIANT_MAX_LEN);
    *variant = 0;
    strcat(variant, TKEDEN_VARIANT);
    Tcl_LinkVar(interp, "_tkeden_variant", (char *) &variant,
		TCL_LINK_STRING | TCL_LINK_READ_ONLY);

    webSite = Tcl_Alloc(TKEDEN_WEB_SITE_MAX_LEN);
    *webSite = 0;
    strcat(webSite, TKEDEN_WEB_SITE);
    Tcl_LinkVar(interp, "_tkeden_web_site", (char *) &webSite,
		TCL_LINK_STRING | TCL_LINK_READ_ONLY);

    sasamiAvail = Tcl_Alloc(2);
    *sasamiAvail = 0;
#ifdef WANT_SASAMI
    strcat(sasamiAvail, "1");
#else
    strcat(sasamiAvail, "0");
#endif
    Tcl_LinkVar(interp, "_tkeden_sasamiAvail", (char *) &sasamiAvail,
		TCL_LINK_STRING | TCL_LINK_READ_ONLY);

    /* [Ben] Create _tkeden_win32_version variable (used in TCL to
             check for Win32)
             _tkeden_win32_version is "0.0" if not running on Win32
             This is set in version.h */

    win32version = Tcl_Alloc(TKEDEN_WIN32_VERSION_MAX_LEN);
    *win32version = 0;
    strcat(win32version, TKEDEN_WIN32_VERSION);
    Tcl_LinkVar(interp, "_tkeden_win32_version", (char *) &win32version,
	 	TCL_LINK_STRING | TCL_LINK_READ_ONLY);

    /* Hack something so that Tcl can tell when we are running on an
       Apple, and do the Apple menu etc */
    apple = Tcl_Alloc(5);
    *apple = 0;
#ifdef __APPLE__
    strcat(apple, "1");
#else
    strcat(apple, "0");
#endif
    Tcl_LinkVar(interp, "_tkeden_apple", (char *) &apple,
		TCL_LINK_STRING | TCL_LINK_READ_ONLY);

    if (Tcl_EvalFile(interp, fullname) != TCL_OK) {
      fprintf(stderr, "%s: Tcl error whilst evaluating %s\n", progname,
	      fullname);
      fprintf(stderr, "Tcl errorInfo (from EXinit): %s\n",
	      Tcl_GetVar(interp, "errorInfo", 0));
      exit(1);
    }

#ifdef DISTRIB
/*     setSocketPort(); */
#endif /* DISTRIB */

}
