/*
 * $Id: builtin.c,v 1.47 2002/07/10 19:22:18 cssbz Exp $
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

static char rcsid[] = "$Id: builtin.c,v 1.47 2002/07/10 19:22:18 cssbz Exp $";

/*************************************
 *	SOME BUILTIN FUNCTIONS       *
 *************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>		/* for stat() */
#include <float.h>		/* for DBL_DIG */
#include <assert.h>
#include <stdarg.h>		// For custom formatting for debugMessage() and 

#include "../../../../../config.h"
#include "eden.h"
#include "machine.h"
#include "builtin.h"
#include "yacc.h"
#include "symptr.q.h"
#include "hash.h"
#include "notation.h"

#if !defined(__APPLE__) && !defined(__CYGWIN__)
#include <stropts.h> /* for I_PUSH */
#endif

#include <fcntl.h> /* for O_RDWR */
#include <termios.h> /* for ECHO, termios etc */

#ifdef ipc
#include <sys/ipc.h>
#include <sys/msg.h>
#endif

#include <unistd.h>
#include <limits.h>		/* for PATH_MAX */
#include <errno.h>

#ifndef TTYEDEN
#include <tk.h>
/* Tcl caches environment variables: so we must use Tcl's versions of
 the getenv and putenv functions [Ash] */
/* Tcl 8.2.0 Makefile implies don't use the Tcl getenv [Ash] */
#define putenv Tcl_PutEnv
#define setenv TclSetEnv
#define unsetenv TclUnsetEnv
#include <tcl.h>
extern Tcl_Interp *interp;
#endif

#include "emalloc.h"
#include "runset.h"

extern void call(symptr, Datum, char *);
extern char *topMasterStack(void);
extern int topEntryStack(void);
extern int appAgentName; /* VA [Ash] */
extern char *typename();
#ifdef DISTRIB
void tkdefine1(symptr); /* for propagation --sun */
void propagate_agency(symptr);
void propagateAgency(symptr, char *);
void propagateAgency1(char *, char *);
void SendServer(char *, char *);
void SendClient(char *, char *);
void SendotherClients(char *, char *);
#endif /* DISTRIB */

#ifndef streq
#define	streq(X,Y)		(strcmp(X,Y)==0)
#endif

struct BLIBTBL blibtbl[] = {
#define INCLUDE 'T'
#include "builtin.h"
#undef INCLUDE
		{ 0, 0 } };




extern void sendCompleteWedenMessage(char[]);
extern void sendCompleteFormattedWedenMessage(char *, ...);
extern void startWedenMessage();
extern void appendWedenMessage(char[]);
extern void appendFormattedWedenMessage(char *, ...);
extern void endWedenMessage();
extern void debugMessage(char *fmt, ...);

extern int basecontext; /* DOSTE context [Nick] */



#ifdef WEDEN_ENABLED

//extern void sendWedenMessage(char[]);
//extern void sendFormattedWedenMessage(char *, ...);
//extern void debugMessage(char *fmt, ...);

void sendCompleteWedenMessage(char msg[]) {
	// Initialise the command
	Tcl_DString command;
	Tcl_DStringInit(&command);

	// Put together the command and pass the message to send as a list
	// Tcl_DStringAppend(&command, "weden_SendMessage ", -1);
	// Now we send messages in real time
	Tcl_DStringAppend(&command, "weden_SendCompleteRealTimeMessage ", -1);
	
	Tcl_DStringAppendElement(&command, msg);

	// Try to run the send message command and display any errors
	if (Tcl_Eval(interp, command.string) != TCL_OK) {
		fprintf(stdout, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\nERROR SENDING WEDEN MESSAGE\n%s\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", command.string);
	}

	// Free up any memory
	Tcl_DStringFree(&command);
}

void sendCompleteFormattedWedenMessage(char *fmt, ...) {
	va_list argp;
	char *messageToSend, *newMessageToSend;
	Tcl_DString tclCommand; // need to use Tcl_DStrings as want to ensure spaces and other characters are properly dealt with
	int messageSize = 100;
	int actualLength;

	// Get some space to store the message to send
	messageToSend = emalloc(messageSize);

	// Specify the command we will call
	Tcl_DStringInit(&tclCommand);
	// Put together the command and pass the message to send as a list
	// Tcl_DStringAppend(&tclCommand, "weden_SendMessage ", -1);
	// Now we send messages in real time
	Tcl_DStringAppend(&tclCommand, "weden_SendCompleteRealTimeMessage ", -1);

	// Format the message
	if ((messageToSend = malloc (messageSize)) == NULL) {
		// Not enough room so clear up and we will need to die due to insufficent memory
		free(messageToSend);
		fprintf(stderr, "Insufficent memory to run sendCompleteFormattedWedenMessage(). EDEN Terminating.");
		exit(-1);
	}

	// We will now try to format the string to return
	while (1) {

		// Try to print in the allocated space.
		va_start(argp, fmt);
		actualLength = vsnprintf(messageToSend, messageSize, fmt, argp);
		va_end(argp);

		// If that worked we can continue on and send the message
		if (actualLength > -1 && actualLength < messageSize)
			break;

		// Otherwise not enough space so we need to make some more room
		if (actualLength > -1) // We know the length we need to use this time
			messageSize = actualLength + 1; // So this should be precisely the space we need
		else // We still do not know how much room so doube the space
			messageSize *= 2;

		// Try and get the extra space we need to try again
		if ((newMessageToSend = realloc (messageToSend, messageSize)) == NULL) {
			// Not enough room so clear up and we will need to die due to insufficent memory
			free(messageToSend);
			fprintf(stderr, "Insufficent memory to run sendCompleteFormattedWedenMessage(). EDEN Terminating.");
			exit(-1);
		} else {
			// We have the required memory so point to it
			messageToSend = newMessageToSend;
		}
	}

	// Append the message to the command as a TCL List
	Tcl_DStringAppendElement(&tclCommand, messageToSend);

	// Try to send the message
	if (Tcl_Eval(interp, tclCommand.string) != TCL_OK) {

		// If there are any errors then report them
		fprintf(stderr, "Tcl error (sendCompleteFormattedWedenMessage): %s\n", Tcl_GetStringResult(interp));
		exit(-1);

	}

	// Free up resources we used
	Tcl_DStringFree(&tclCommand);
	free(messageToSend);

}

void startWedenMessage() {

	// Initialise the command
	Tcl_DString command;
	Tcl_DStringInit(&command);

	// Put together the command and pass the message to send as a list
	// Tcl_DStringAppend(&command, "weden_SendMessage ", -1);
	// Now we send messages in real time
	Tcl_DStringAppend(&command, "weden_startRealTimeMessage", -1);
	
	// Try to run the send message command and display any errors
	if (Tcl_Eval(interp, command.string) != TCL_OK) {
		fprintf(stdout, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\nERROR SENDING WEDEN MESSAGE\n%s\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", command.string);
	}

	// Free up any memory
	Tcl_DStringFree(&command);

}


void appendWedenMessage(char msg[]) {
	
	// Initialise the command
	Tcl_DString command;
	Tcl_DStringInit(&command);

	// Put together the command and pass the message to send as a list
	// Tcl_DStringAppend(&command, "weden_SendMessage ", -1);
	// Now we send messages in real time
	Tcl_DStringAppend(&command, "weden_appendRealTimeMessage ", -1);
	
	Tcl_DStringAppendElement(&command, msg);

	// Try to run the send message command and display any errors
	if (Tcl_Eval(interp, command.string) != TCL_OK) {
		fprintf(stdout, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\nERROR SENDING WEDEN MESSAGE\n%s\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", command.string);
	}

	// Free up any memory
	Tcl_DStringFree(&command);
	
}

void appendFormattedWedenMessage(char *fmt, ...) {
	
	va_list argp;
		char *messageToSend, *newMessageToSend;
		Tcl_DString tclCommand; // need to use Tcl_DStrings as want to ensure spaces and other characters are properly dealt with
		int messageSize = 100;
		int actualLength;

		// Get some space to store the message to send
		messageToSend = emalloc(messageSize);

		// Specify the command we will call
		Tcl_DStringInit(&tclCommand);
		// Put together the command and pass the message to send as a list
		// Tcl_DStringAppend(&tclCommand, "weden_SendMessage ", -1);
		// Now we send messages in real time
		Tcl_DStringAppend(&tclCommand, "weden_appendRealTimeMessage ", -1);

		// Format the message
		if ((messageToSend = malloc (messageSize)) == NULL) {
			// Not enough room so clear up and we will need to die due to insufficent memory
			free(messageToSend);
			fprintf(stderr, "Insufficent memory to run appendFormattedWedenMessage(). EDEN Terminating.");
			exit(-1);
		}

		// We will now try to format the string to return
		while (1) {

			// Try to print in the allocated space.
			va_start(argp, fmt);
			actualLength = vsnprintf(messageToSend, messageSize, fmt, argp);
			va_end(argp);

			// If that worked we can continue on and send the message
			if (actualLength > -1 && actualLength < messageSize)
				break;

			// Otherwise not enough space so we need to make some more room
			if (actualLength > -1) // We know the length we need to use this time
				messageSize = actualLength + 1; // So this should be precisely the space we need
			else // We still do not know how much room so doube the space
				messageSize *= 2;

			// Try and get the extra space we need to try again
			if ((newMessageToSend = realloc (messageToSend, messageSize)) == NULL) {
				// Not enough room so clear up and we will need to die due to insufficent memory
				free(messageToSend);
				fprintf(stderr, "Insufficent memory to run appendFormattedWedenMessage(). EDEN Terminating.");
				exit(-1);
			} else {
				// We have the required memory so point to it
				messageToSend = newMessageToSend;
			}
		}

		// Append the message to the command as a TCL List
		Tcl_DStringAppendElement(&tclCommand, messageToSend);

		// Try to send the message
		if (Tcl_Eval(interp, tclCommand.string) != TCL_OK) {

			// If there are any errors then report them
			fprintf(stderr, "Tcl error (appendFormattedWedenMessage): %s\n", Tcl_GetStringResult(interp));
			exit(-1);

		}

		// Free up resources we used
		Tcl_DStringFree(&tclCommand);
		free(messageToSend);
	
}

void endWedenMessage() {

	// Initialise the command
	Tcl_DString command;
	Tcl_DStringInit(&command);

	// Put together the command and pass the message to send as a list
	// Tcl_DStringAppend(&command, "weden_SendMessage ", -1);
	// Now we send messages in real time
	Tcl_DStringAppend(&command, "weden_endRealTimeMessage", -1);
	
	// Try to run the send message command and display any errors
	if (Tcl_Eval(interp, command.string) != TCL_OK) {
		fprintf(stdout, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\nERROR SENDING WEDEN MESSAGE\n%s\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++", command.string);
	}

	// Free up any memory
	Tcl_DStringFree(&command);

}



void debugMessage(char *fmt, ...) {

	va_list argp;
	char *debugMessage, *newDebugMessage;
	int messageSize = 100;
	int actualLength;

	// Get some space to store the message to send
	debugMessage = emalloc(messageSize);


	// Format the message
	if ((debugMessage = malloc (messageSize)) == NULL) {
		// Not enough room so clear up and we will need to die due to insufficent memory
		free(debugMessage);
		fprintf(stderr, "Insufficent memory to run debugMessage(). EDEN Terminating.");
		exit(-1);
	}

	// We will now try to format the string to return
	while (1) {

		// Try to print in the allocated space.
		va_start(argp, fmt);
		actualLength = vsnprintf(debugMessage, messageSize, fmt, argp);
		va_end(argp);

		// If that worked we can continue on and send the message
		if (actualLength > -1 && actualLength < messageSize)
			break;

		// Otherwise not enough space so we need to make some more room
		if (actualLength > -1) // We know the length we need to use this time
			messageSize = actualLength + 1; // So this should be precisely the space we need
		else // We still do not know how much room so doube the space
			messageSize *= 2;

		// Try and get the extra space we need to try again
		if ((newDebugMessage = realloc (debugMessage, messageSize)) == NULL) {
			// Not enough room so clear up and we will need to die due to insufficent memory
			free(debugMessage);
			fprintf(stderr, "Insufficent memory to run debugMessage(). EDEN Terminating.");
			exit(-1);
		} else {
			// We have the required memory so point to it
			debugMessage = newDebugMessage;
		}
	}
	
	// Send the debug message	
	sendCompleteFormattedWedenMessage("<debug>%s</debug>", debugMessage);
	
	free(debugMessage);

}

#else // Not using Web EDEN

void outputMessage(char msg[]) {

#ifdef TTYEDEN
	fprintf(stdout, msg);
#else
	// Initialise the command
	Tcl_DString command;
	Tcl_DStringInit(&command);

	// Put together the command and pass the message to send as a list
	Tcl_DStringAppend(&command, "outputMessage ", -1);
	
	Tcl_DStringAppendElement(&command, msg);

	// Try to run the send message command and display any errors
	if (Tcl_Eval(interp, command.string) != TCL_OK) {
		fprintf(stdout, "+++\nERROR SENDING OUTPUT MESSAGE\n%s\n+++", command.string);
	}

	// Free up any memory
	Tcl_DStringFree(&command);

#endif
}

void outputFormattedMessage(char *fmt, ...) {

#ifdef TTYEDEN
	va_list ptr;
	va_start(ptr, fmt);
	vprintf(fmt,ptr);
	va_end(ptr);
#else	
	va_list argp;
	char *messageToSend, *newMessageToSend;
	Tcl_DString tclCommand; // need to use Tcl_DStrings as want to ensure spaces and other characters are properly dealt with
	int messageSize = 100;
	int actualLength;

	// Get some space to store the message to send
	messageToSend = emalloc(messageSize);

	// Specify the command we will call
	Tcl_DStringInit(&tclCommand);
	// Put together the command and pass the message to send as a list
	Tcl_DStringAppend(&tclCommand, "outputMessage ", -1);

	// Format the message
	if ((messageToSend = malloc (messageSize)) == NULL) {
		// Not enough room so clear up and we will need to die due to insufficent memory
		free(messageToSend);
		fprintf(stderr, "Insufficent memory to run outputFormattedMessage(). EDEN Terminating.");
		exit(-1);
	}

	// We will now try to format the string to return
	while (1) {

		// Try to print in the allocated space.
		va_start(argp, fmt);
		actualLength = vsnprintf(messageToSend, messageSize, fmt, argp);
		va_end(argp);

		// If that worked we can continue on and send the message
		if (actualLength > -1 && actualLength < messageSize)
			break;

		// Otherwise not enough space so we need to make some more room
		if (actualLength > -1) // We know the length we need to use this time
			messageSize = actualLength + 1; // So this should be precisely the space we need
		else // We still do not know how much room so doube the space
			messageSize *= 2;

		// Try and get the extra space we need to try again
		if ((newMessageToSend = realloc (messageToSend, messageSize)) == NULL) {
			// Not enough room so clear up and we will need to die due to insufficent memory
			free(messageToSend);
			fprintf(stderr, "Insufficent memory to run outputFormattedMessage(). EDEN Terminating.");
			exit(-1);
		} else {
			// We have the required memory so point to it
			messageToSend = newMessageToSend;
		}
	}

	// Append the message to the command as a TCL List
	Tcl_DStringAppendElement(&tclCommand, messageToSend);

	// Try to send the message
	if (Tcl_Eval(interp, tclCommand.string) != TCL_OK) {

		// If there are any errors then report them
		fprintf(stderr, "Tcl error (outputMessage): %s\n", Tcl_GetStringResult(interp));
		exit(-1);

	}

	// Free up resources we used
	Tcl_DStringFree(&tclCommand);
	free(messageToSend);
#endif	
}

void startWedenMessage() {
	error("Error sending a Web EDEN Message - Not Running Web EDEN");
}

void endWedenMessage() {
	error("Error sending a Web EDEN Message - Not Running Web EDEN");
}
void appendWedenMessage(char msg[]) {
	error("Error sending a Web EDEN Message - Not Running Web EDEN");
}

void appendFormattedWedenMessage(char *fmt, ...) {
	error("Error sending a Web EDEN Message - Not Running Web EDEN");
}


void sendWedenMessage(char msg[]) {
	error("Error sending a Web EDEN Message - Not Running Web EDEN");
}

void sendFormattedWedenMessage(char *fmt, ...) {
	error("Error sending a Formatted Web EDEN Message - Not Running Web EDEN");
}

void debugMessage(char *fmt, ...) {
	
	// Get the arguments
	va_list ap;
	va_start (ap, fmt);
	
	// Print the debug message
	vprintf(fmt, ap);
	
	// Clean up
	va_end (ap);
	
}

#endif

void usage(char *s) { /* internal subroutine */
#ifdef WEDEN_ENABLED
	sendCompleteFormattedWedenMessage("<b><item type=\"commandusage\">%s</item></b>", s);
#else
	outputFormattedMessage("usage: ", s);
#endif
}

#ifdef NO_CHECK_CIRCULAR
int NCC = 0; /* 1 for disable circular check */

void dcc(void)
{ /* NCC mode on/off */
	extern int NCC;

	if (paracount > 0) {
		mustint(para(1), "dcc()");
		NCC = para(1).u.i;
	} else
	NCC = 1;
	pushUNDEF();
}

#endif

#ifdef DEBUG
int Debug = 0; /* 1 for debugging */
extern int yydebug;
extern int st_debug;
extern int dd_debug;

void
debug(void)
{ /* debug modes on/off:
 1:  various
 2:  RunSet debugging
 4:  Eden parser debugging (=yydebug)
 8:  Sasami
 16: Donald
 32: Scout parser debugging (=st_debug)
 64: malloc debugging (emalloc macro)
 128: Tcl_Eval debugging
 256: execute debugging
 512: func/proc/procmacro call debugging
 1024: VMWRIT debugging (in 1 also)
 2048: Donald parser debugging (=dd_debug)
 4096: Print errors on stderr as well as the error window
 8192: regular expression debugging
 16384: notation debugging
 32768: timers C code debugging
 ... add these debug values together
 to find the argument to give to the
 debug() function.  [Ash] */
	extern int Debug;

	if (paracount > 0) {
		mustint(para(1), "debug()");
		Debug = para(1).u.i;
		yydebug = Debug & 4;
#ifndef TTYEDEN
		st_debug = Debug & 32;
		dd_debug = Debug & 2048;
#endif
	} else
	Debug = 1;
	pushUNDEF();
}
#endif /* DEBUG */

#ifdef INSTRUMENT
#include <sys/time.h>
extern hrtime_t insSchedule;
extern hrtime_t insCycle;
extern hrtime_t insStart;

// 																					<<<------- TODO: TO STILL DO - WHAT IS THIS FOR???
// Not to sure what this is actually used for. Can not find any reference to it in any on/offline documentation. We just print it as a 'standard output'
void insPrint(void) {

	hrtime_t sofar = gethrtime() - insStart;

#ifdef WEDEN_ENABLED
	startWedenMessage();
	appendFormattedWedenMessage("<b><item type=\"stdoutput\">Time so far      = %20lld nsec\n", sofar);
	appendFormattedWedenMessage("insSchedule time = %20lld nsec (%Lf%%)\n", insSchedule, ((long double)insSchedule / (long double)sofar) * 100.0);
	appendFormattedWedenMessage("insCycle time    = %20lld nsec (%Lf%%)\n</item></b>", insCycle, ((long double)insCycle / (long double)sofar) * 100.0);
	endWedenMessage();
#else

	printf("Time so far      = %20lld nsec\n", sofar);
	printf("insSchedule time = %20lld nsec (%Lf%%)\n", insSchedule, ((long double)insSchedule / (long double)sofar) * 100.0);
	printf("insCycle time    = %20lld nsec (%Lf%%)\n", insCycle, ((long double)insCycle / (long double)sofar) * 100.0);

#endif

	pushUNDEF();
}

void insReset(void) {
	insSchedule = insCycle = 0;
	insStart = gethrtime();

	pushUNDEF();
}
#endif

#ifndef TTYEDEN

/* Need to keep a record of active timers so that they can be removed by
 deletetimer() and deletealltimers().  Before Tcl activates them, we
 store information in the 'PreQueueTimer' linked list (which starts at
 pqtlisthead).  When Tcl activates a timer (by calling timerhandler),
 we remove the information from this list and tag the Action created
 with the timer token, enabling the action to be removed by
 deletetimer() and deletealltimers() before it is executed. */
typedef struct PreQueueTimer {
	char *cmdstring;
	Tcl_TimerToken tok;
	struct PreQueueTimer *next;
	struct PreQueueTimer *prev;
} PreQueueTimer;

PreQueueTimer *pqtlisthead = NULL;

/* I am assuming that this Tcl TimerHandler only gets called when EDEN
 is quiescent -- in particular, that it is not called when EDEN is
 already in the queue() proc.  This should be true, given that both
 EDEN and Tcl run in a single thread.  gdb on Mac OS X shows that above
 this TimerHandler in the call stack is Tk_MainLoop, called from main,
 so this seems to be true on Mac OS X at least.  */
void timerhandler(ClientData clientData) {
	PreQueueTimer *pqt;

	pqt = (PreQueueTimer *)clientData;
	assert(pqt->tok != NULL);

	queue(pqt->cmdstring, "timerhandler", pqt->tok);

	/* Remove this timer information from the list */
	pqt->prev->next = pqt->next;
	if (pqt->next != NULL)
		pqt->next->prev = pqt->prev;

	free(pqt->cmdstring); /* queue strdups cmdstring, so this is OK */
	free(pqt);
}

void createtimer(void) {
	Tcl_TimerToken tok; /* geddit? :) */
	Datum d;
	PreQueueTimer *pqt;

	if (paracount != 2) {
		usage("token createtimer(cmdstring, timemillis)");
	} else {
		muststr(para(1), "createtimer");
		mustint(para(2), "createtimer");
	}

	/* make sure never to give a value of 0 milliseconds to
	 Tcl_CreateTimerHandler, or else nothing else ever seems to happen. */
	if (para(2).u.i < 1)
		errorf("time must be >=1");

	if (pqtlisthead == NULL) {
		/* Initialise.  The first PreQueueTimer in the list is a dummy, just there
		 to hold the 'next' pointer. */
		pqtlisthead = emalloc(sizeof(PreQueueTimer));
		pqtlisthead->cmdstring = NULL;
		pqtlisthead->tok = NULL;
		pqtlisthead->next = NULL;
		pqtlisthead->prev = NULL;
	}

	/* The token needs to be available to timerhandler so that it can enter
	 that information in the RunSet so that timer-created Actions may later
	 be deleted by deletetimer.  The token isn't actually available until
	 after the timer has been created -- fill it in then.  'timerhandler'
	 is responsible for freeing all of this structure.  The structure is
	 "PreQueue" because it exists only until timerhandler enters the
	 cmdstring into the todo queue (RunSet action list) -- thereafter the
	 information exists in the queue itself. */
	pqt = emalloc(sizeof(PreQueueTimer));
	pqt->cmdstring = strdup(para(1).u.s);
	pqt->tok = NULL;

	/* Add this timer information to the head of the list */
	pqt->next = pqtlisthead->next;
	pqt->prev = pqtlisthead;

	if (pqtlisthead->next != NULL)
		pqtlisthead->next->prev = pqt;

	pqtlisthead->next = pqt;

	tok = Tcl_CreateTimerHandler(para(2).u.i, timerhandler, pqt);

	pqt->tok = tok;

	/* treating Tcl_TimerToken as an integer even though it is a Tcl
	 opaque type (see tcl.h) */
	dpush(d, INTEGER, tok);
}

void deletetimer(void) {
	extern RunSet RS1, RS2;
	int found = 0;
	PreQueueTimer *pqt, *nextpqt;
	Tcl_TimerToken tok;

	if (paracount != 1) {
		usage("deletetimer(token)");
	}

	/* ignore a @ argument */
	if (para(1).type != UNDEF) {
		/* if not @, argument must be an integer */
		mustint(para(1), "deletetimer");

		tok = (Tcl_TimerToken)para(1).u.i;

		Tcl_DeleteTimerHandler(tok);

		if (pqtlisthead == NULL)
			errorf("you must call createtimer() before deletetimer()");

		/* delete this token from the pqtlist */
		pqt = pqtlisthead->next;
		while (pqt != NULL) {
			nextpqt = pqt->next;

			if (pqt->tok == tok) {
				assert(found == 0); /* shouldn't ever be two pqts in the list with 
				 the same token */
				found = 1;

#ifdef DEBUG
				if (Debug & 32768)
					debugMessage("deletetimer: removing token <%d> cmdstring <%s>"
						" from prequeuetimers list\n", pqt->tok, pqt->cmdstring);
#endif

				pqt->prev->next = pqt->next;
				if (pqt->next != NULL)
					pqt->next->prev = pqt->prev;

				free(pqt->cmdstring);
				free(pqt);
			}

			pqt = nextpqt;
		}

#ifdef DEBUG
		if (Debug & 32768) {
			if (found == 0) {
				debugMessage("deletetimer: failed to find any timers with token <%d>"
						" in the prequeuetimers list\n", tok);
			}
		}
#endif

		/* need to search through the queues of Actions (RunSets) in case we
		 are deleting a timer after timerhandler was called but before the
		 cmdstring was executed and removed from the queue. */
		deleteActionWithToken((Tcl_TimerToken)para(1).u.i, &RS1);
		deleteActionWithToken((Tcl_TimerToken)para(1).u.i, &RS2);
	}

	pushUNDEF();
}

void deletealltimers(void) {
	extern RunSet RS1, RS2;

	if (paracount != 0) {
		usage("deletealltimers()");
	}

	if (pqtlisthead == NULL) {
		// 																					<<<------- TO STILL DO (Customise this)
		/* list not yet initialised by first call to createtimer */
#ifdef DEBUG
		if (Debug & 32768)
			debugMessage("deletealltimers: no timers list to delete from\n");
#endif
		return;
	}
	// 																					<<<------- TO STILL DO (Customise this)
	while (pqtlisthead->next != NULL) {
#ifdef DEBUG
		if (Debug & 32768)
			debugMessage("deletealltimers: removing token <%d> cmdstring <%s>"
				" from prequeuetimers list\n",
				pqtlisthead->next->tok,
				pqtlisthead->next->cmdstring);
#endif

		Tcl_DeleteTimerHandler(pqtlisthead->next->tok);

		free(pqtlisthead->next->cmdstring);
		free(pqtlisthead->next);
		/* no need to keep prev pointers up to date as removing the whole list */
		pqtlisthead->next = pqtlisthead->next->next;
	}

	deleteAllTokenActions(&RS1);
	deleteAllTokenActions(&RS2);

	pushUNDEF();
}

#endif /* NOT TTYEDEN */

/* Return Inp_Dev->lineno so Eden code can determine the current line
 number in the Eden (or other?) code.  This should allow
 implementation (for example) of a code profiler in Eden which is
 able to describe the performance of particular numbered lines of
 code.  [Ash] */
#include <setjmp.h>
#include "input_device.h"
void inpdevlineno(void) {
	Datum d;

	dpush(d, INTEGER, Inp_Dev->lineno);
}

void inpdevname(void) {
	Datum d;
	char *t;

	if (Inp_Dev->name) {
		t = getheap(strlen(Inp_Dev->name) + 1);
		strcpy(t, Inp_Dev->name);
		dpush(d, STRING, t);
	} else {
		/* name was null */
		pushUNDEF();
	}
}

/* From Autoconf manual [Ash] */
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifdef __CYGWIN__
extern void cygwin_split_path(const char *, char *, char *);
#else
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif /* HAVE_LIBGEN_H */
#endif

/* if we don't have cygwin or libgen, this function will end up
 undefined, and an Eden version of it will be created in main.c */
#if defined(__CYGWIN__) || defined(HAVE_LIBGEN_H)
void _eden_internal_dirname(void) {
	char *result, *myHeap;
	Datum d;
#ifdef __CYGWIN__
	char *basename;
#endif

	if (paracount != 1) {
		usage("string _eden_internal_dirname(string)");
	} else {
		muststr(para(1), "_eden_internal_dirname");

#ifdef __CYGWIN__
		result = emalloc(PATH_MAX);
		basename = emalloc(PATH_MAX);
		cygwin_split_path(para(1).u.s, result, basename);
#else /* not CYGWIN */
		result = dirname(para(1).u.s);
#endif /* __CYGWIN__ */

		myHeap = getheap(strlen(result) + 1);
		strcpy(myHeap, result);
		dpush(d, STRING, myHeap);

#ifdef __CYGWIN__
		free(result);
		free(basename);
#endif
	}
}

void _eden_internal_basename(void) {
	char *result, *myHeap;
	Datum d;
#ifdef __CYGWIN__
	char *dirname;
#endif

	if (paracount != 1) {
		usage("string _eden_internal_basename(string)");
	} else {
		muststr(para(1), "_eden_internal_basename");

#ifdef __CYGWIN__
		dirname = emalloc(PATH_MAX);
		result = emalloc(PATH_MAX);
		cygwin_split_path(para(1).u.s, dirname, result);
#else /* not CYGWIN */
		result = basename(para(1).u.s);
#endif /* __CYGWIN__ */

		myHeap = getheap(strlen(result) + 1);
		strcpy(myHeap, result);
		dpush(d, STRING, myHeap);

#ifdef __CYGWIN__
		free(result);
		free(dirname);
#endif
	}
}
#endif /* __CYGWIN__ or HAVE_LIBGEN_H */

// Get the internal current working directory for EDEN
void _eden_internal_cwd(void) {
	char *cwd;
	Datum d;
	char *myHeap;

	if (paracount != 0) {
		usage("string eden_internal_cwd()");
	}

	if ((cwd = getcwd(NULL, PATH_MAX)) == NULL) {
		errorf("failed to get cwd");
	} else {
		myHeap = getheap(strlen(cwd) + 1);
		strcpy(myHeap, cwd);
		dpush(d, STRING, myHeap);
		free(cwd);
	}
}

// Change the current working directory
void _eden_internal_cd(void) {
	
	if (paracount == 1) {
		muststr(para(1), "_eden_internal_cd");

		if (chdir(para(1).u.s)) {
			errorf("failed to cd into '%s': %s", para(1).u.s, strerror(errno));
		}
	} else {
		usage("void _eden_internal_cd(string)");
	}
}

/* local time */
void gettime(void) {
	Datum sec, min, hour, mday, mon, year, wday;
	struct tm *clock;
	time_t tloc;

	time(&tloc);
	clock = localtime(&tloc);
	dpush(sec, INTEGER, clock->tm_sec);
	dpush(min, INTEGER, clock->tm_min);
	dpush(hour, INTEGER, clock->tm_hour);
	dpush(mday, INTEGER, clock->tm_mday);
	dpush(mon, INTEGER, clock->tm_mon + 1);
	dpush(year, INTEGER, clock->tm_year);
	dpush(wday, INTEGER, clock->tm_wday);
	makearr(7);
}

/* time elapsed since Jan 1, 1970 in seconds */
void inttime(void) {
	Datum seconds;
	time_t tloc;

	dpush(seconds, INTEGER, time(&tloc));
}

/* time elapsed since Jan 1, 1970 in milli-seconds */
void finetime(void) {
	Datum seconds, milli;
	struct timeval tloc;
	struct timezone tzone;

	gettimeofday(&tloc, &tzone);
	dpush(seconds, INTEGER, tloc.tv_sec);
	dpush(milli, INTEGER, tloc.tv_usec / 1000);
	makearr(2);
}

void f_eof(void) {
#ifdef WEDEN_ENABLED
	if (paracount > 0) {
		sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>f_eof</builtinname><edenname>feof</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");
		Datum d;
		dpush(d, INTEGER, (-1));
	} else {
		usage("int feof(stream)");
	}

#else

	Datum d;
	FILE *stream;

	if (paracount > 0) {
		mustint(para(1), "feof()");
		stream = (FILE *) para(1).u.i;
		dpush(d, INTEGER, feof(stream));
	} else
		usage("int feof(stream)");
#endif	// End WEDEN Not Enabled
}

void get_char(void) {
#ifdef WEDEN_ENABLED
	sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>get_char</builtinname><edenname>getchar</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");
	Datum d;
	dpush(d, INTEGER, (-1));
#else
	Datum d;
	dpush(d, INTEGER, getchar());
#endif	// End WEDEN Not Enabled
}

void fget_char(void) {

#ifdef WEDEN_ENABLED
	if (paracount > 0) {
		sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>fget_char</builtinname><edenname>fgetc</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");
	} else {
		usage("int fgetc(stream)");
	}
	Datum d;
	dpush(d, INTEGER, (-1));

#else
	Datum d;
	FILE *stream;

	if (paracount > 0) {
		mustint(para(1), "fgetc()");
		stream = (FILE *) para(1).u.i;
		dpush(d, INTEGER, fgetc(stream));
	} else
		usage("int fgetc(stream)");
#endif	// End WEDEN Not Enabled
}

void file_stat(void) {
#ifdef WEDEN_ENABLED
	sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>file_stat</builtinname><edenname>stat</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");
#else

	char *filename;
	struct stat buf;
	int ret;
	Datum d;

	if (paracount != 1)
		usage("stat(filename);");

	muststr(para(1), "stat()");
	filename = para(1).u.s;

	ret = stat(filename, &buf);
	if (ret == 0) {
		/* stuff the info returned into a list */
		dpush(d, INTEGER, buf.st_uid);
		dpush(d, INTEGER, buf.st_gid);
		dpush(d, INTEGER, buf.st_size);
		dpush(d, INTEGER, buf.st_atime);
		dpush(d, INTEGER, buf.st_mtime);
		dpush(d, INTEGER, buf.st_ctime);
		makearr(6);

	} else {
		/* there was an error */
		errorf("failed to stat %s: %s", filename, strerror(errno));
	}
#endif	// End WEDEN Not Enabled
}

#define MAXSTRLEN 256

void get_string(void) {
#ifdef WEDEN_ENABLED

	if (paracount > 0) {
		usage("string gets()");
	}
	sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>get_string</builtinname><edenname>gets</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");

	pushUNDEF();
#else

	Datum d;
	char *status;
	char *t;
	char *s = (char *) malloc(MAXSTRLEN);
	int slength;

	if (paracount > 0) {
		usage("string gets()");
	}
	status = fgets(s, MAXSTRLEN, stdin);

	/* Remove the \n that is added by fgets() (thus keeping gets()
	 semantics) [Ash] */
	slength = strlen(s);
	if (slength > 0) {
		s[strlen(s)-1] = '\0';
	}

	if (status) {
		t = getheap(strlen(s) + 1);
		strcpy(t, s);
		dpush(d, STRING, t);
	} else
		pushUNDEF();
	free(s);
#endif	// End WEDEN Not Enabled
}

void fget_string(void) {

#ifdef WEDEN_ENABLED

	if (paracount == 0) {
		usage("string fgets(n, stream)");
	}
	sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>fget_string</builtinname><edenname>fgets</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");

	pushUNDEF();
#else

	Datum d;
	int n;
	FILE *stream;
	char *status;
	char *t;
	char *s;

	if (paracount > 0) {
		mustint(para(1), "fgets()");
		n = para(1).u.i;
		mustint(para(2), "fgets()");
		stream = (FILE *) para(2).u.i;
		s = (char *) malloc(n);
		status = fgets(s, n, stream);
		if (status) {
			t = getheap(strlen(s) + 1);
			strcpy(t, s);
			dpush(d, STRING, t);
		} else
			pushUNDEF();
		free(s);
	} else
		usage("string fgets(n, stream)");
#endif	// End WEDEN Not Enabled
}

void unget_char(void) {
#ifdef WEDEN_ENABLED

	if (paracount == 0) {
		usage("void ungetc(c, stream)");
	}
	sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>unget_char</builtinname><edenname>ungetc</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");

#else

	int c;
	FILE *stream;

	if (paracount > 0) {
		mustint(para(1), "ungetc()");
		c = para(1).u.i;
		mustint(para(2), "ungetc()");
		stream = (FILE *) para(2).u.i;
		ungetc(c, stream);
	} else
		usage("void ungetc(c, stream)");
#endif	// End WEDEN Not Enabled
}

void scan_f(void)
{
#ifdef WEDEN_ENABLED

	if (paracount > 10) {
		usage("scanf can handle at most 9 variables");
	} else if (paracount == 0) {
		usage("int scanf(format [, pointer ... ])");
	}

	sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>scan_f</builtinname><edenname>scanf</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");

	Datum d;
	dpush(d, INTEGER, 0);

#else

	Datum d;
	void *arg[9];
	int i;
	char *format;

	if (paracount > 10) {
		usage("scanf can handle at most 9 variables");
	} else if (paracount > 0) {
		muststr(para(1), "scanf()");
		for (i = paracount; i > 1; --i)
			mustaddr(para(i), "scanf()");
		format = para(1).u.s;
		for (i = paracount; i > 1; --i)
			if (para(i).u.a->type == STRING)
				arg[i - 2] = (void *) (para(i).u.a->u.i);
			else
				arg[i - 2] = (void *) &(para(i).u.a->u);
		i = scanf(format, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7], arg[8]);
	} else
		usage("int scanf(format [, pointer ... ])");
	dpush(d, INTEGER, i);

#endif	// End WEDEN Not Enabled
}

	void fscan_f(void)
	{
#ifdef WEDEN_ENABLED

		if (paracount > 10) {
			usage("fscanf can handle at most 9 variables");
		} else if (paracount == 0) {
			usage("int fscanf(stream, format [, pointer ... ])");
		}

		sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>fscan_f</builtinname><edenname>fscanf</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");

		Datum d;
		dpush(d, INTEGER, 0);

#else

		Datum d;
		void *arg[9];
		int i;
		FILE *stream;
		char *format;

		if (paracount > 10)
			usage("fscanf can handle at most 9 variables");
		else if (paracount > 0) {
			mustint(para(1), "fscanf()");
			muststr(para(2), "fscanf()");
			for (i = paracount; i > 2; --i)
				mustaddr(para(i), "fscanf()");
			stream = (FILE *) para(1).u.i;
			format = para(2).u.s;
			for (i = paracount; i > 2; --i)
				if (para(i).u.a->type == STRING)
					arg[i - 3] = (void *) para(i).u.a->u.i;
				else
					arg[i - 3] = (void *) &(para(i).u.a->u);
			i = fscanf(stream, format, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7], arg[8]);
		} else
			usage("int fscanf(stream, format [, pointer ... ])");
		dpush(d, INTEGER, i);

#endif	// End WEDEN Not Enabled
	}

	void sscan_f(void)
	{
#ifdef WEDEN_ENABLED

		if (paracount > 10) {
			usage("sscanf can handle at most 9 variables");
		} else if (paracount == 0) {
			usage("int sscanf(string, format [, pointer ... ])");
		}

		sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>sscan_f</builtinname><edenname>sscanf</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");

		Datum d;
		dpush(d, INTEGER, 0);

#else

		Datum d;
		void *arg[9];
		int i;
		char *str;
		char *format;

		if (paracount > 10)
			usage("sscanf can handle at most 9 variables");
		else if (paracount > 0) {
			muststr(para(1), "sscanf()");
			muststr(para(2), "sscanf()");
			for (i = paracount; i > 2; --i)
			mustaddr(para(i), "sscanf()");
			str = para(1).u.s;
			format = para(2).u.s;
			for (i = paracount; i > 2; --i)
			if (para(i).u.a->type == STRING)
			arg[i - 3] = (void *) para(i).u.a->u.i;
			else
			arg[i - 3] = (void *) &(para(i).u.a->u);
			i = sscanf(str, format, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7], arg[8]);
		} else
			usage("int sscanf(string, format [, pointer ... ])");
		dpush(d, INTEGER, i);

#endif	// End WEDEN Not Enabled
	}

	void substr(void)
	{ /* sub-string function */
		Datum d;
		int from, to, len, i;
		char *s, *t, *str;
		Datum ctos(Datum);

		if (paracount >= 3) {
			para(1) = ctos(para(1));
			str = para(1).u.s; /* the string */
			len = strlen(str); /* the strlen */
			mustint(para(2), "substr()"); /* from */
			mustint(para(3), "substr()"); /* to   */
			from = para(2).u.i - 1;
			to = para(3).u.i - 1;

			if (from < 0)
				error("substr: index out of range (from < 0)");

			i = (from <= to) ? (to - from + 1) : 0;

			for (t = s = getheap(i + 1); from <= to; from++)
			*t++ = (from < len) ? str[from] : ' ';
			*t = '\0';
		} else
			usage("substr(s, from, to)");
		dpush(d, STRING, s);
	}
	
	void indexOf(void)
	{ /* indexOf function */
		Datum d;
		int index;
		char *str, *search_str, *position;
		Datum ctos(Datum);

		if (paracount >= 2) {
			para(1) = ctos(para(1));
			str = para(1).u.s; /* the string */
			para(2) = ctos(para(2));
			search_str = para(2).u.s;	/* the string whose starting index is to be found */
					
			position = strstr(str, search_str);

			if(position==NULL) {
				index = -1;
			} else {
				index = str - position; /* pointer arithmetic to find the start position */

			    if(index < 0) {
			      index = -index;       /* take the positive value of index */
			    }
			}
		} else {
			usage("indexOf(string, findString)");
		}
				
		dpush(d, INTEGER, index);
	}

#if !defined(TTYEDEN) && !defined(DISTRIB) 
	void getTclVariable(void)
	{ 
		Datum d;
		int index;
		char *tclVarName, *result;
		Datum ctos(Datum);

		if (paracount == 1) {
			para(1) = ctos(para(1));
			tclVarName = para(1).u.s; /* the variable name */

			
			if ( (result = Tcl_GetVar(interp, tclVarName, TCL_GLOBAL_ONLY)) == NULL) {
				error2("tcl variable not found: ", tclVarName);
			}
			d.type = STRING;
			d.u.s = result;
			push(d);
			
		} else {
			usage("getTCLVarValue(varName)");
		}
				
	}
#endif
	
	
	// RICHARD
	void trimstr(void)
	{ /* string trimming function */
		Datum d;
		Datum ctos(Datum);
		char *oringStr;
		char *newStr = NULL;
		char *last = NULL;

		if (paracount >= 1) {

			// Get the input string
			para(1) = ctos(para(1));
			oringStr = para(1).u.s; /* the string */

			// Find and store the first instance of a non whitespace character
			oringStr += strspn(oringStr, " \t\v\f");
			newStr = oringStr;

			// Now look through the rest of the string for ending white space
			while (*oringStr)
			{
				oringStr += strcspn(oringStr, " \t\v\f"); last = oringStr;
				oringStr += strspn(oringStr, " \t\v\f");
			}

			// Chop off any trailing white space characters
			if (last != oringStr)
				last[0] = '\0';

		} else {
			usage("trimstr(str)");
		}

		// Returns the new trimmed string
		dpush(d, STRING, newStr);
	}


	
	// END RICHARD

	void sublist(void)
	{
		Datum *a;
		int i, listlen, from, to;

		if (paracount >= 3) {
			mustlist(para(1), "sublist()");
			mustint(para(2), "sublist()");
			mustint(para(3), "sublist()");
			a = para(1).u.a;
			listlen = a[0].u.i;
			from = para(2).u.i;
			to = para(3).u.i;
			if (from < 1)
			error("sublist: index out of range (from < 1)");
			for (i = from; i <= to; i++)
			push(i <= listlen ? a[i] : UndefDatum);
			makearr(from <= to ? to - from + 1 : 0);
		} else
			usage("sublist(L, from, to)");
	}

#ifdef WEDEN_ENABLED

	/* internal function: print a datum.  reflect should be true if the
	 printed string may be used as code to re-execute: ie a string will
	 be printed with quotes.  */
	void print(Datum d, int reflect) {

		/* printf("Using the Print() function\n"); */
		
		int i;

		if (is_symbol(d)) {

			appendFormattedWedenMessage("&%s", symbol_of(d)->name);

		} else if (is_local(d)) {
			appendFormattedWedenMessage("(local variable #%d, location 0x%x, value ", local(d), d.u.v.x);

			switch (address_type(d)) {
				case DPTR:
					print(*dptr(d), reflect);
					break;
				case CPTR:
					if (reflect) {
						appendFormattedWedenMessage("\"%s\"", cptr(d));
					} else {
						appendFormattedWedenMessage("%s", cptr(d));
					}
					break;
				default:
					appendFormattedWedenMessage("oops - internal error - unknown type of pointer");
			}

			appendFormattedWedenMessage(")");

		} else {
			switch (d.type) {
				case REAL:
#define Str(x) #x
#define Xstr(x) Str(x)
				/* Need to specify the precision, otherwise 169.9999 comes
				 out as 170 [Ash] */
					appendFormattedWedenMessage("%." Xstr(DBL_DIG) "g", d.u.r);
					break;

				case INTEGER:
					appendFormattedWedenMessage("%d", d.u.i);
					break;

				case MYCHAR:
					if (reflect) {
						appendFormattedWedenMessage("\'%c\'", d.u.i);
					} else {
						appendFormattedWedenMessage("%c", d.u.i);
					}
					break;

				case STRING:
					if (d.u.s) {
						if (reflect) {
							appendFormattedWedenMessage("\"%s\"", d.u.s);
						} else {
							appendFormattedWedenMessage("%s", d.u.s);
						}
					}
					break;

				case LIST:

					appendWedenMessage("[");
					for (i = 1; i <= d.u.a[0].u.i; i++) {
						print(d.u.a[i], 1);
						if (i < d.u.a[0].u.i) {
							appendWedenMessage(",");
						}
					}
					appendWedenMessage("]");
					return;
					break;

				case VAR:
					appendFormattedWedenMessage("(%s is variable)", d.u.sym->name);
					break;

				case BLTIN:
					appendFormattedWedenMessage("(%s is built-in function)", d.u.sym->name);
					break;

				case LIB:
					appendFormattedWedenMessage("(%s is int-valued C function)", d.u.sym->name);
					break;

				case LIB64:
					appendFormattedWedenMessage("(%s is 64bit int-valued C function)", d.u.sym->name);
					break;

				case RLIB:
					appendFormattedWedenMessage("(%s is real-valued C function)", d.u.sym->name);
					break;

				case FORMULA:
					appendFormattedWedenMessage("@");
					/* fprintf(f,"(%s is formula)", d.u.sym->name); */
					break;

				case FUNCTION:
					appendFormattedWedenMessage("(%s is user-defined function)", d.u.sym->name);
					break;

				case PROCEDURE:
					appendFormattedWedenMessage("(%s is user-defined procedure)", d.u.sym->name);
					break;

				case PROCMACRO:
					appendFormattedWedenMessage("(%s is user-defined procmacro)", d.u.sym->name);
					break;

				case UNDEF:
					//fprintf(stderr, "Sending an Undefined");
					appendFormattedWedenMessage("@");
					break;

				default:
					appendFormattedWedenMessage("(type=%d, val=%d)", d.type, d.u.i);
					break;
			}
		}

	}

	void writeln(void)
	{ /* pascal-like writeln */
		startWedenMessage();
		appendWedenMessage("<b><item type=\"stdoutput\"><![CDATA[");

		int i;
		for (i = 1; i <= paracount; i++) {
			printf("Calling Print() function\n");
			print(para(i), 0);
			printf("Finished Calling Print() function\n");
		}
		pushUNDEF();

		appendWedenMessage("\n]]></item></b>");
		endWedenMessage();
	}

	void b_write(void)
	{
		startWedenMessage();
		appendWedenMessage("<b><item type=\"stdoutput\"><![CDATA[");

		int i;
		for (i = 1; i <= paracount; i++) {
			print(para(i), 0);
		}
		pushUNDEF();
		appendWedenMessage("]]></item></b>");
		endWedenMessage();
	}

	/* for use from gdb */
	void printstdout(Datum d) {
		startWedenMessage();
		appendWedenMessage("<b><item type=\"stdoutput\"><![CDATA[");
		print(d, 1);
		appendWedenMessage("]]></item></b>");
		endWedenMessage();
	}

#else // Not using WebEDEN

	/* internal function: print a datum.  reflect should be true if the
	 printed string may be used as code to re-execute: ie a string will
	 be printed with quotes.  */
	void print(Datum d, int reflect) {

		/* printf("Using the Print() function\n"); */
		
		int i;

		if (is_symbol(d)) {

			outputFormattedMessage("&%s", symbol_of(d)->name);

		} else if (is_local(d)) {
			outputFormattedMessage("(local variable #%d, location 0x%x, value ", local(d), d.u.v.x);

			switch (address_type(d)) {
				case DPTR:
					print(*dptr(d), reflect);
					break;
				case CPTR:
					if (reflect) {
						outputFormattedMessage("\"%s\"", cptr(d));
					} else {
						outputFormattedMessage("%s", cptr(d));
					}
					break;
				default:
					outputFormattedMessage("oops - internal error - unknown type of pointer");
			}

			outputFormattedMessage(")");

		} else {
			switch (d.type) {
				case REAL:
#define Str(x) #x
#define Xstr(x) Str(x)
				/* Need to specify the precision, otherwise 169.9999 comes
				 out as 170 [Ash] */
					outputFormattedMessage("%." Xstr(DBL_DIG) "g", d.u.r);
					break;

				case INTEGER:
					outputFormattedMessage("%d", d.u.i);
					break;

				case MYCHAR:
					if (reflect) {
						outputFormattedMessage("\'%c\'", d.u.i);
					} else {
						outputFormattedMessage("%c", d.u.i);
					}
					break;

				case STRING:
					if (d.u.s) {
						if (reflect) {
							outputFormattedMessage("\"%s\"", d.u.s);
						} else {
							outputFormattedMessage("%s", d.u.s);
						}
					}
					break;

				case LIST:

					outputMessage("[");
					for (i = 1; i <= d.u.a[0].u.i; i++) {
						print(d.u.a[i], 1);
						if (i < d.u.a[0].u.i) {
							outputMessage(",");
						}
					}
					outputMessage("]");
					return;
					break;

				case VAR:
					outputFormattedMessage("(%s is variable)", d.u.sym->name);
					break;

				case BLTIN:
					outputFormattedMessage("(%s is built-in function)", d.u.sym->name);
					break;

				case LIB:
					outputFormattedMessage("(%s is int-valued C function)", d.u.sym->name);
					break;

				case LIB64:
					outputFormattedMessage("(%s is 64bit int-valued C function)", d.u.sym->name);
					break;

				case RLIB:
					outputFormattedMessage("(%s is real-valued C function)", d.u.sym->name);
					break;

				case FORMULA:
					outputFormattedMessage("@");
					/* fprintf(f,"(%s is formula)", d.u.sym->name); */
					break;

				case FUNCTION:
					outputFormattedMessage("(%s is user-defined function)", d.u.sym->name);
					break;

				case PROCEDURE:
					outputFormattedMessage("(%s is user-defined procedure)", d.u.sym->name);
					break;

				case PROCMACRO:
					outputFormattedMessage("(%s is user-defined procmacro)", d.u.sym->name);
					break;

				case UNDEF:
					//fprintf(stderr, "Sending an Undefined");
					outputFormattedMessage("@");
					break;

				default:
					outputFormattedMessage("(type=%d, val=%d)", d.type, d.u.i);
					break;
			}
		}

	}

	void writeln(void)
	{ /* pascal-like writeln */
		int i;
		for (i = 1; i <= paracount; i++) {
			print(para(i), 0);
		}
		outputMessage("\n");
		pushUNDEF();
}

	void b_write(void)
	{
		int i;
		for (i = 1; i <= paracount; i++) {
			print(para(i), 0);
		}
		pushUNDEF();
	}

	/* for use from gdb */
	void printstdout(Datum d) {
		print(d, 1);
	}

#endif // NOT WebEDEN

	void nameof(void)
	{ /* built-in function: symbol name of a
	 pointer */
		Datum d, addr;
		char *s, *name;

		if (paracount != 1)
		usage("s = nameof(&var);");

		addr = para(1);
		if (!is_symbol(addr))
		errorf("nameof(): address needed (got %s)", typename(addr.type));

		name = symbol_of(addr)->name;
		s = getheap(strlen(name) + 1);
		strcpy(s, name);

		dpush(d, STRING, s);
	}

	void scat(void)
	{ /* string concatenation */
		Datum d;
		int i;
		int len;
		char *s;
		Datum ctos(Datum);

		len = 0;
		for (i = 1; i <= paracount; i++) {
			para(i) = ctos(para(i));
			len += strlen(para(i).u.s);
		}

		s = getheap(len + 1);

		for (i = 1, *s = '\0'; i <= paracount; i++)
		strcat(s, para(i).u.s);

		dpush(d, STRING, s);
	}

	void lcat(void)
	{
		int i, j, total, listlen;
		Datum *a;

		total = 0;
		for (i = 1; i <= paracount; i++) {
			mustlist(para(i), "lcat()");
			a = para(i).u.a;
			total += (listlen = a[0].u.i);
			for (j = 1; j <= listlen; j++)
			push(a[j]);
		}
		makearr(total);
	}

	void b_exit(void)
	{ /* builtin: exit the interpreter */
		extern void terminate(int);

		terminate(paracount > 0 && isint(para(1)) ? para(1).u.i : 0);
		pushUNDEF();
	}

	/* Eden execute() command */
	void exec_string(void)
	{ /* builtin: execute a string as a program */
		int result;
		Datum d;
		notationType n;
		int run(short, void *, char *);

		if (paracount == 0)
			usage("execute(string_expr);");

		muststr(para(1), "execute()");

#ifdef DEBUG
		if (Debug&256)
			debugMessage("execute(\"%s\");\n", para(1).u.s);
#endif

		pushEntryStack(EDEN);
		n = currentNotation;
		notationPushPop(+1);

		result = run(STRING_DEV, para(1).u.s, 0);

		notationPushPop(-1);
		changeNotation(n);
		popEntryStack();
		dpush(d, INTEGER, result);
	}

	void exec_file(void)
	{ /* builtin: execute a file as a program */

		FILE *filein;
		char *name;
		Datum d;
		int result;
		int run(short, void *, char *);
		int i, n;

		if (paracount == 0)
		usage("include(filename[, filename...]);");

		n = paracount; /* don't rely on being able to read paracount
		 (which is on the stack) after calling run() [Ash] */

		for (i = 1; i <= n; i++) {
			muststr(para(i), "include()");
			name = para(i).u.s;

			if ((filein = fopen(name, "r"))) {
				pushEntryStack(EDEN);

				/* notationPushPop not required here as the semantics of
				 include are that changes of notation persist afterwards */
				result = run(FILE_DEV, filein, name);

				/* ``run'' will close the file */

				popEntryStack();
			} else {
				error2("can't read file ", name);
			}
		}

#ifndef TTYEDEN
		setprompt();
#endif

		dpush(d, INTEGER, result);
	}

	/* call a function with a list as its argument */
	void apply(void)
	{
		if (paracount >= 2) {
			switch (para(1).type) {
				case FUNCTION:
				case PROCEDURE:
				case PROCMACRO:
				case BLTIN:
				case LIB:
				case LIB64:
				case RLIB:
				if (para(2).type == LIST)
				call(para(1).u.sym, para(2), 0);
				break;

				default:
				usage("apply(function, list);");
				break;
			}
		} else
		usage("apply(function, list);");
	}

	static void pushlist(symptr_QUEUE * Q)
	{
		int count;
		Datum d;
		symptr_ATOM A;

		count = 0;
		FOREACH(A, Q) {
			dpush(d, STRING, A->obj->name);
			count++;
		}
		makearr(count);
	}

#ifdef DISTRIB
	/* Only used for oracle, handle, state: the three item agent list [Ash] */
	static void
	pushlist1(agent_QUEUE * Q)
	{
		int count;
		Datum d;
		agent_ATOM A;

		count = 0;
		FOREACH(A, Q) {
			dpush(d, STRING, A->obj.name);
			count++;
		}
		makearr(count);
	}
#endif /* DISTRIB */

	void
	symboldetail(void)
	{
		extern symptr hashtable[];
		extern int hashindex(char *);
		extern char *typename();
		symptr sp;
		Datum d;
		char *name;
		int i;

		if (paracount < 1)
		usage("symboldetail(symbol);");

		if (!is_symbol(para(1)) && para(1).type != STRING)
		errorf("symboldetail(): address or symbol name needed (got %s)",
				typename(para(1).type));

		if (is_symbol(para(1))) {
			/* If we've been passed a pointer, then just look it up [Ash] */
			sp = symbol_of(para(1));
		} else {
			/* If we've been passed a string, find the symbol [Ash] */
			name = para(1).u.s;
			i = hashindex(name);
			for (sp = hashtable[i]; sp != (symptr) 0; sp = sp->next)
			if (strcmp(sp->name, name) == 0) {
				break;
			}
			if (sp == 0)
			error2("symboldetail(): no such variable ", name);
		}

#ifdef DISTRIB
		/* if (!oracle_check(sp))
		 error2("no Oracle privilege on observable ", name);*//* --sun */
#endif /* DISTRIB */

		/* sp is a pointer to the symbol we want to examine [Ash] */
		dpush(d, STRING, sp->name);
		dpush(d, STRING, typename(sp->stype));
		if (sp->text) {
			dpush(d, STRING, sp->text);
		} else {
			dpush(d, STRING, "");
		}
		pushlist(&sp->targets);
		pushlist(&sp->sources);
		dpush(d, STRING, sp->master);
#ifdef DISTRIB
		pushlist1(&sp->OracleOf);
		pushlist1(&sp->HandleOf);
		pushlist1(&sp->StateOf);
		makearr(9);
#else
		makearr(6);
#endif /* DISTRIB */
	}

	/* symboldetail creates a load of stuff on the stack in the Chris Roe
	 case when there are lots of targets and sources around, so this
	 function simply returns what Chris needs: the text that was used to
	 define this symbol originally.  Basically this is the same as
	 symboldetail(summat)[3], but this version will not run out of stack
	 space if you call it with a symbol with lots of targets and
	 sources. [Ash, 8/3/2000] */
	void symboltext(void)
	{
		extern symptr hashtable[];
		extern int hashindex(char *);
		extern char *typename();
		symptr sp;
		Datum d;
		char *name;
		int i;

		if (paracount < 1)
		usage("symboltext(symbol);");

		if (!is_symbol(para(1)) && para(1).type != STRING)
		errorf("symboltext(): address or symbol name needed (got %s)",
				typename(para(1).type));

		if (is_symbol(para(1))) {
			/* If we've been passed a pointer, then just look it up [Ash] */
			sp = symbol_of(para(1));
		} else {
			/* If we've been passed a string, find the symbol [Ash] */
			name = para(1).u.s;
			i = hashindex(name);
			for (sp = hashtable[i]; sp != (symptr) 0; sp = sp->next)
			if (strcmp(sp->name, name) == 0) {
				break;
			}
			if (sp == 0)
			error2("symboltext(): no such variable ", name);
		}

#ifdef DISTRIB
		/* if (!oracle_check(sp))
		 error2("no Oracle privilege on observable ", name);*//* --sun */
#endif /* DISTRIB */

		/* sp is a pointer to the symbol we want to examine [Ash] */
		if (sp->text) {
			dpush(d, STRING, sp->text);
		} else {
			dpush(d, STRING, "");
		}
	}

	void symbols(void)
	{
		extern symptr hashtable[];
		extern int typeno();
		symptr sp;
		Datum d;
		Datum *p;
		Int count;
		Int i;
		Int any, pointer, type;

		if (paracount < 1 || para(1).type != STRING)
		usage("symbols(type);");

		any = (strcmp(para(1).u.s, "any") == 0);
		pointer = (strcmp(para(1).u.s, "pointer") == 0);
		if (!any && !pointer) {
			type = typeno(para(1).u.s);
		}
		count = 0;
		for (i = 0; i <= HASHSIZE; i++) {
			for (sp = hashtable[i]; sp != (symptr) 0; sp = sp->next) {
				if (any || type == sp->stype || (pointer && is_address(sp->d)) || type == sp->d.type)
				count++;
			}
		}
		p = (Datum *) getheap((count + 1) * sizeof(Datum));
		p[0].type = INTEGER;
		p[0].u.i = count;

		count = 1;
		for (i = 0; i <= HASHSIZE; i++) {
			for (sp = hashtable[i]; sp != (symptr) 0; sp = sp->next) {
				if (any || type == sp->stype || (pointer && is_address(sp->d)) || type == sp->d.type) {
					dpush(d, STRING, sp->name);
					p[count++] = pop();
				}
			}
		}
		d.type = LIST;
		d.u.a = p;
		push(d);
	}

	/* transform the internal symbol table into a list */
	void symtbl2list(void)
	{
		extern symptr hashtable[];
		extern char *typename(int);
		symptr sp;
		Datum d;
		Datum *p;
		Int count;
		Int i;

		count = 0;
		for (i = 0; i <= HASHSIZE; i++)
		for (sp = hashtable[i]; sp != (symptr) 0; sp = sp->next)
		count++;
		p = (Datum *) getheap((count + 1) * sizeof(Datum));
		p[0].type = INTEGER;
		p[0].u.i = count;

		count = 1;
		for (i = 0; i <= HASHSIZE; i++) {
			for (sp = hashtable[i]; sp != (symptr) 0; sp = sp->next) {
				dpush(d, STRING, sp->name);
				dpush(d, STRING, typename(sp->stype));
				if (sp->text) {
					dpush(d, STRING, sp->text);
				} else {
					dpush(d, STRING, "");
				}
				pushlist(&sp->targets);
				pushlist(&sp->sources);
				dpush(d, STRING, sp->master);
				makearr(6);
				p[count++] = pop();
			}
		}
		d.type = LIST;
		d.u.a = p;
		push(d);
	}

	// 																					<<<------- TODO: What to do with this...should it be avalaible as noted as a debug feature
	void printhash(void)
	{ /* for debugging */
		extern symptr hashtable[];
		symptr sp;
		int i;
#ifdef WEDEN_ENABLED
		startWedenMessage();
		appendWedenMessage("<b><item type=\"stdoutput\">");
		for (i = 0; i <= HASHSIZE; i++) {
			appendFormattedWedenMessage("%d ----------------------\n", i);
			for (sp = hashtable[i]; sp != (symptr) 0; sp = sp->next) {
				appendFormattedWedenMessage("%s\n", sp->name);
			}
		}
		appendWedenMessage("</item></b>");
		endWedenMessage();
		
#else
		for (i = 0; i <= HASHSIZE; i++) {
			outputFormattedMessage("%d ----------------------\n", i);
			for (sp = hashtable[i]; sp != (symptr) 0; sp = sp->next) {
				outputFormattedMessage("%s\n", sp->name);
			}
		}
#endif
		pushUNDEF();
	}

	// 																					<<<------- TO STILL DO (Customise this)
#ifndef TTYEDEN
	static char defn[4096];

	void tkdefineDatum(Datum d, char *s)
	{
		int i;
		char *t;

		if (is_symbol(d)) {
			sprintf(s, "&%s", symbol_of(d)->name);
			s += strlen(s);
		} else if (is_local(d)) {
			sprintf(s, "(local variable #%d, ", local(d));
			s += strlen(s);
			sprintf(s, address_type(d) == DPTR ? "data pointer: %d)" : "char pointer: %d)", d.u.v.x);
			s += strlen(s);
		} else {
			switch (d.type) {

				case REAL:
				/* See similar code in print() - Used for formatting decimals and prevent rounding [Ash] */
#define Str(x) #x
#define Xstr(x) Str(x)
				sprintf(s, "%." Xstr(DBL_DIG) "g", d.u.r);
				s += strlen(s);
				break;

				case INTEGER:
				sprintf(s, "%d", d.u.i);
				s += strlen(s);
				break;

				case MYCHAR:
				sprintf(s, "'%c'", d.u.i);
				s += strlen(s);
				break;

				case STRING:
				if (d.u.s) {
					*s++ = '"';
					for (t = d.u.s; *t != '\0'; t++) {
						switch (*t) {
							case '\\':
							*s++ = '\\';
							*s++ = '\\';
							break;
							case '"':
							*s++ = '\\';
							*s++ = '"';
							break;
							default:
							*s++ = *t;
							break;
						}
					}
					*s++ = '"';
					*s++ = '\0';
				}
				break;

				case LIST:
				sprintf(s, "[");
				s++;
				for (i = 1; i <= d.u.a[0].u.i; i++) {
					tkdefineDatum(d.u.a[i], s);
					s += strlen(s);
					if (i < d.u.a[0].u.i) {
						sprintf(s, ",");
						s++;
					}
				}
				sprintf(s, "]");
				s++;
				break;

				case VAR:
				sprintf(s, "(%s is variable)", d.u.sym->name);
				s += strlen(s);
				break;

				case BLTIN:
				sprintf(s, "(%s is built-in function)", d.u.sym->name);
				s += strlen(s);
				break;

				case LIB:
				case LIB64:
				case RLIB:
				sprintf(s, "(%s is C function)", d.u.sym->name);
				s += strlen(s);
				break;

				case FORMULA:
				sprintf(s, "(%s is formula)", d.u.sym->name);
				s += strlen(s);
				break;

				case FUNCTION:
				sprintf(s, "(%s is user-defined function)", d.u.sym->name);
				s += strlen(s);
				break;

				case PROCEDURE:
				sprintf(s, "(%s is user-defined procedure)", d.u.sym->name);
				s += strlen(s);
				break;

				case PROCMACRO:
				sprintf(s, "(%s is user-defined procmacro)", d.u.sym->name);
				s += strlen(s);
				break;

				case UNDEF:
				sprintf(s, "@");
				s++;
				break;

				default:
				sprintf(s, "(type=%d, val=%d)", d.type, d.u.i);
				break;
			}
		}
	}

	// 																					<<<------- TODO: What does this do...Note: the 'edenDefn' function is in edenio.tcl
	void tkdefine(symptr sp)
	{
		Tcl_DString command, message;

		if (*sp->name == '\0')
		return; /* why are there such cases? */

		Tcl_DStringInit(&command);
		Tcl_DStringInit(&message);

		Tcl_DStringAppend(&command, "edenDefn ", -1);
		Tcl_DStringAppendElement(&command, sp->name);
		Tcl_DStringAppendElement(&message,
				sp->entry == INTERNAL ? "internal" :
				sp->entry == ARCA ? "arca" :
				sp->entry == SCOUT ? "scout" :
				sp->entry == DONALD ? "donald" : "eden");
		Tcl_DStringAppendElement(&message, sp->master);
		Tcl_DStringAppendElement(&command, message.string);
		Tcl_DStringFree(&message);
		switch (sp->stype) {
			case FORMULA:
			Tcl_DStringAppend(&message, sp->name, -1);
			Tcl_DStringAppend(&message, " is", -1);
			Tcl_DStringAppend(&message, sp->text, -1);
			Tcl_DStringAppend(&message, "\n", -1);
			break;
			case FUNCTION:
			case PROCMACRO:
			case PROCEDURE: {
				symptr_QUEUE *P, *Q;
				char *s = defn;

				Q = &sp->sources;
				for (P = FRONT(Q); P; P = NEXT(Q, P)) {
					sprintf(s, "%s %s", P == Q->next ? " :" : ",", P->obj->name);
					s += strlen(s);
				}
				*s++ = ' ';
				*s = '\0';
			}
			Tcl_DStringAppend(&message, sp->stype == FUNCTION ? "func " : (sp->stype == PROCMACRO ? "procmacro " : "proc "), -1);
			Tcl_DStringAppend(&message, sp->name, -1);
			Tcl_DStringAppend(&message, defn, -1);
			if (sp->text) {
				Tcl_DStringAppend(&message, sp->text, -1);
				Tcl_DStringAppend(&message, "\n", -1);
			}
			break;

			default:
			tkdefineDatum(sp->d, defn);
			Tcl_DStringAppend(&message, sp->name, -1);
			Tcl_DStringAppend(&message, " = ", -1);
			Tcl_DStringAppend(&message, defn, -1);
			Tcl_DStringAppend(&message, ";\n", -1);
			break;
		}
		Tcl_DStringAppendElement(&command, message.string);
		if (Tcl_Eval(interp, command.string) != TCL_OK) {
			Tcl_DStringFree(&command);
			Tcl_DStringFree(&message);
			error2("Tcl errorInfo (from tkdefine): ", Tcl_GetVar(interp, "errorInfo", 0));
		}
		Tcl_DStringFree(&command);
		Tcl_DStringFree(&message);
	}

	// 																					<<<------- IGNORE THIS...We dont work if we are not TkEDEN with WEDEN Enabled
#ifdef DISTRIB
	void tkdefine1(symptr sp)
	{
		extern void push_text(char *, int);

		if (*sp->name == '\0')
		return; /* why are there such cases? */

		push_text("%eden\n", 6);

		switch (sp->stype) {
			case FORMULA:
			push_text(sp->name, strlen(sp->name));
			push_text(" is ", 4);
			push_text(sp->text, strlen(sp->text));
			push_text("\n", 1);
			break;
			case FUNCTION:
			case PROCMACRO:
			case PROCEDURE:
			{
				symptr_QUEUE *P, *Q;
				char *s = defn;

				Q = &sp->sources;
				for (P = FRONT(Q); P; P = NEXT(Q, P)) {
					sprintf(s, "%s %s", P == Q->next ? " :" : ",", P->obj->name);
					s += strlen(s);
				}
				*s++ = ' ';
				*s = '\0';
			}
			switch (sp->stype) {
				case FUNCTION: push_text("func ", 5); break;
				case PROCEDURE: push_text("proc ", 5); break;
				case PROCMACRO: push_text("procmacro ", 10); break;
			}
			push_text(sp->name, strlen(sp->name));
			push_text(defn, strlen(defn));
			if (sp->text) push_text(sp->text, strlen(sp->text));
			push_text("\n", 1);
			break;

			default:
			tkdefineDatum(sp->d, defn);
			push_text(sp->name, strlen(sp->name));
			push_text(" = ", 3);
			push_text(defn, strlen(defn));
			push_text(";\n", 2);
			break;
		}
		push_text("\0", 1);
	}
#endif /* DISTRIB */

	int isUserDefined(symptr sp)
	{
		switch (sp->stype) {
			case FORMULA:
			case FUNCTION:
			case PROCMACRO:
			case PROCEDURE:
			return 1;
			default:
			switch (sp->d.type) {
				case REAL:
				case INTEGER:
				case MYCHAR:
				case STRING:
				case LIST:
				case VAR:
				return 1;
				case UNDEF:
				if (sp->changed == FALSE)
				return 1;
			}
		}
		return 0;
	}

	int isNotYetDefined(symptr sp)
	{
		switch (sp->stype) {
			case FORMULA:
			case FUNCTION:
			case PROCMACRO:
			case PROCEDURE:
			return 0;
			default:
			switch (sp->d.type) {
				case UNDEF:
				if (sp->changed)
				return 1;
			}
		}

		return 0;
	}

	/* AcceptTable holds a string representing a Tcl list containing the
	 numerical indices of all the elements in the View Options listbox
	 (ie agent names) that the user has selected.  [Ash] */
	static char *AcceptTable = 0;
	/* assuming this is enough to hold a large number of agent names */
#define ACCEPTTABLESIZE 2048

	// 																					<<<------- TODO: Handle UI BuildAcceptTable - Need to determin what this does

	static void buildAcceptTable(void)
	{
		char *indices, *s;
		if (AcceptTable)
		free(AcceptTable);
		Tcl_Eval(interp, "winfo exists .view.left.list");
		if (strcmp(interp->result, "0")) {
			/* window exists */
			Tcl_EvalEC(interp, ".view.left.list curselection");
			indices = strdup(Tcl_GetStringResult(interp));

			/* find elements from listbox that correspond to the indices we have */
			AcceptTable = emalloc(ACCEPTTABLESIZE);
			AcceptTable[0] = '\0';
			s = strtok(indices, " ");
			while (s) {
				Tcl_VarEval(interp, ".view.left.list get ", s, 0);
				strncat(AcceptTable, interp->result, ACCEPTTABLESIZE);
				strncat(AcceptTable, " ", ACCEPTTABLESIZE);
				s = strtok(0, " ");
			}
			free(indices);
		} else {
			/* window does not exist */
			AcceptTable = 0;
		}
	}

	/* Return 1 if the given agent name was selected for query by the user
	 [Ash] */
	static int acceptable(char *name)
	{
		char *s1, *s;

		if (!AcceptTable)
		return 1;
		s1 = strdup(AcceptTable);
		s = strtok(s1, " ");
		while (s) {
			if (strcmp(s, name) == 0) {
				free(s1);
				return 1;
			}
			s = strtok(0, " ");
		}
		free(s1);
		return 0;
	}

	static int symcmp( /* symptr *, symptr * */); /* qsort doesn't like full
	 prototype */

	// 																					<<<------- TODO: DumpEden - Prints the internal symbol table into a Tk text Window
	/* print the internal symbol table into a Tk text Window */
	
#ifdef USE_TCL_CONST84_OPTION
int dumpeden(ClientData clientData, Tcl_Interp * interp, int argc, CONST84 char *argv[])
#else
int dumpeden(ClientData clientData, Tcl_Interp * interp, int argc, char *argv[])
#endif
	{
		extern char *hptr;
		char *saveHeapPtr;
		extern symptr hashtable[];
		symptr sp;
		symptr *p;
		symptr *pptr;
		Int count;
		Int i;
		char entry, show_not_yet_defined;
		char *lastMaster = "";

		if (argc != 3) {
			Tcl_AppendResult(interp, "wrong # args: should be \"",
					"dumpeden notation-option show-not-yet-define\"", 0);
			return TCL_ERROR;
		}
		entry = atoi(argv[1]);
		show_not_yet_defined = atoi(argv[2]);

		/* Setting the fourth bit in the first argument causes dumpeden to
		 use the same options as last time [Ash] */
		if (!(entry & 0x8)) {
			buildAcceptTable();
		}
		count = 0;
		for (i = 0; i <= HASHSIZE; i++)
		for (sp = hashtable[i]; sp != (symptr) 0; sp = sp->next)
		if ((isUserDefined(sp) ||
						(show_not_yet_defined && isNotYetDefined(sp)))
				&& acceptable(sp->master)
				&& (((entry & 0x1) && sp->entry == SCOUT) ||
						((entry & 0x2) && sp->entry == DONALD) ||
						sp->entry <= EDEN))
		count++;
		saveHeapPtr = hptr;
		p = (symptr *) getheap(count * sizeof(symptr));

		pptr = p;
		for (i = 0; i <= HASHSIZE; i++)
		for (sp = hashtable[i]; sp != (symptr) 0; sp = sp->next)
		if ((isUserDefined(sp) ||
						(show_not_yet_defined && isNotYetDefined(sp)))
				&& acceptable(sp->master)
				&& (((entry & 0x1) && sp->entry == SCOUT) ||
						((entry & 0x2) && sp->entry == DONALD) ||
#ifdef WANT_SASAMI
						((entry & 0x4) && sp->entry == SASAMI) ||
#endif
						sp->entry <= EDEN))
		*pptr++ = sp;

		qsort(p, count, sizeof(symptr), symcmp);

		Tcl_EvalEC(interp, "cleanup eden");
		for (i = 0; i < count; i++) {
			if (strcmp(p[i]->master, lastMaster)) {
				lastMaster = p[i]->master;
				Tcl_EvalEC(interp, ".eden.t.text config -state normal");
				Tcl_VarEval(interp, ".eden.t.text insert end {AGENT ", lastMaster, "\n} masteragent", 0);
				Tcl_EvalEC(interp, ".eden.t.text config -state disabled");
			}
			tkdefine(p[i]);
		}
		hptr = saveHeapPtr;
		Tcl_EvalEC(interp, ".eden.t.text mark set insert 1.0");
		Tcl_EvalEC(interp, ".eden.t.text see insert");
		return TCL_OK;
	}

	// 																					<<<------- TODO: viewOptions - Builds a list of agents to be used in the View Options Window
	/* build a list of agents to be used in the View Options window */
#ifdef USE_TCL_CONST84_OPTION
int viewOptions(ClientData clientData, Tcl_Interp * interp, int argc, CONST84 char *argv[])
#else
int viewOptions(ClientData clientData, Tcl_Interp * interp, int argc, char *argv[])
#endif
	{
		extern char *hptr;
		char *saveHeapPtr;
		extern symptr hashtable[];
		symptr sp;
		symptr *p;
		symptr *pptr;
		Int count;
		Int i;
		char *lastMaster;

		if (argc != 1) {
			Tcl_AppendResult(interp, "wrong # args: should be \"",
					"setupViewOptions\"", 0);
			return TCL_ERROR;
		}
		lastMaster = "";
		count = 0;
		for (i = 0; i <= HASHSIZE; i++)
		for (sp = hashtable[i]; sp != (symptr) 0; sp = sp->next) {
			if (strcmp(sp->master, lastMaster) && isUserDefined(sp)) {
				count++;
				lastMaster = sp->master;
			}
		}

		saveHeapPtr = hptr;
		p = (symptr *) getheap(count * sizeof(symptr));

		lastMaster = "";
		pptr = p;
		for (i = 0; i <= HASHSIZE; i++)
		for (sp = hashtable[i]; sp != (symptr) 0; sp = sp->next) {
			if (strcmp(sp->master, lastMaster) && isUserDefined(sp)) {
				*pptr++ = sp;
				lastMaster = sp->master;
			}
		}

		qsort(p, count, sizeof(symptr), symcmp);

		lastMaster = "";
		for (i = 0; i < count; i++) {
			if (strcmp(p[i]->master, lastMaster)) {
				lastMaster = p[i]->master;
				Tcl_VarEval(interp, ".view.left.list insert end ",
						lastMaster, 0);
			}
		}
		hptr = saveHeapPtr;
		return TCL_OK;
	}

	static int symcmp(symptr * s1, symptr * s2)
	{
		int m;

		if (*s1 == *s2)
		return 0;
		if ((m = strcmp((*s1)->master, (*s2)->master)))
		return m;
		if ((*s1)->stype > (*s2)->stype)
		return 1;
		if ((*s1)->stype == (*s2)->stype)
		return strcmp((*s1)->name, (*s2)->name);
		return -1;
	}

#endif /* not TTYEDEN ? */

	/* remove a symbol from the symbol table */
#define		OK		0
#define		NOTFOUND	1
#define		FAIL		2

	void forget(void)
	{
		extern symptr hashtable[];
		extern int hashindex(char *);
		extern symptr_QUEUE formula_queue, action_queue;
		extern void refer_to(symptr, symptr_QUEUE *);
		Datum d;
		int error_code;
		char *s;
		symptr sp, last;
		int i;
		symptr_QUEUE *Q;

		if (paracount < 1 || (para(1).type != STRING && !is_symbol(para(1))))
		errorf("forget(): pointer or symbol name needed (got %s)",
				typename(para(1).type));

		if (is_symbol(para(1))) {
			/* Find the textual name used when defining this pointer [Ash] */
			s = symbol_of(para(1))->name;
		} else {
			s = para(1).u.s;
		}

		/* Find the location of the symbol in the hashtable, and the
		 previous symbol in the hashtable linked list so that we can
		 remove the symbol from the list [Ash] */
		i = hashindex(s);
		last = (symptr) 0;
		for (sp = hashtable[i]; sp != (symptr) 0; sp = (last = sp)->next) {
			if (strcmp(sp->name, s) == 0) {
				break; /* terminate the for loop */
			}
		}
		if (sp == (symptr)0) {
			/* Didn't find it.  This will never happen when a pointer is given
			 to forget() as use of a pointer reference causes Eden to create
			 the pointed-to item (as @) if it does not already exist.  [Ash] */
			dpush(d, INTEGER, NOTFOUND);
			return;
		}

		if (Q_EMPTY(&sp->targets)) {
			/* this sp has no targets, so nothing references it, so it
			 is safe to forget it [Ash] */
			if (sp->name)
			free(sp->name);

			/* if (sp->inst) free(sp->inst); */

			if (sp->text)
			free(sp->text);
			freedatum(sp->d);
			/* sp->targets is already EMPTY */
			refer_to(sp, &sp->targets); /* free sources */

			if (last)
			last->next = sp->next;
			else
			hashtable[i] = sp->next;

			free(sp);
			error_code = OK; /* forgotten OK */
		} else {
			/* some objects refer to this sp (it has targets) */
			/* error("can't forget", sp->name); */
			error_code = FAIL; /* FAILure */
		}

		/* remove object from evaluation (formula / action) queue */
		switch (sp->stype) {
			case FORMULA:
			Q = &formula_queue;
			break;

			case BLTIN:
			case PROCEDURE:
			case FUNCTION:
			case PROCMACRO:
			Q = &action_queue;
			break;

			default: /* Is it an error ? */
			Q = 0;
			break; /* don't queue up */
		}
		if (Q) {
			DELETE_symptr_ATOM(Q, sp->Qloc);
			sp->Qloc = 0;
		}

		dpush(d, INTEGER, error_code);
	}

	static void packpara(Datum d, char *err_msg)
	{
		if (is_address(d))
		*((Int *) getheap(sizeof(Int))) = (Int) & (dptr(d)->u.i);
		else
		switch (d.type) {
			case REAL:
			*((float *) getheap(sizeof(float))) = d.u.r;
			break;

			case INTEGER:
			*((int *) getheap(sizeof(int))) = d.u.i;
			break;

			case MYCHAR:
			*((char *) getheap(sizeof(char))) = d.u.i;
			break;

			case STRING:
			*((char **) getheap(sizeof(char **))) = d.u.s;
			break;

			case LIST:
			{
				Int n, i;

				n = d.u.a[0].u.i; /* no. of items */
				for (i = 1; i <= n; i++)
				packpara(d.u.a[i], err_msg);
			}
			break;

			default:
			usage(err_msg);
			break;
		}
	}

#define parameters (*fp->stackp)
	void pack(void)
	{
		char *mem;
		Datum d;

		mem = getheap(0); /* find the begin of memory block */
		packpara(parameters, "address=pack(data,...);");
		dpush(d, INTEGER, mem);
	}

	void array(void)
	{
		Int n;
		Datum d;

		if (paracount < 1)
		usage("list = array(n, data);");

		mustint(para(1), "array()");
		n = para(1).u.i;
		if (n < 0)
		error("listcat: -ve number in 1st argument");

		if (paracount > 1)
		d = para(2);
		else
		d = UndefDatum;

		while (n--)
		push(d);

		makearr(para(1).u.i);
	}

	void user_error(void)
	{ /* generate an error */
#ifdef WEDEN_ENABLED
		
		startWedenMessage();
		if (paracount > 0) {
			muststr(para(1), "user_error");
			appendFormattedWedenMessage("<b><item type=\"error\"><info>%s</info></item></b>", para(1).u.s);
		} else {
			appendWedenMessage("<b><item type=\"error\"><info>run-time error</info></item></b>");
		}
		endWedenMessage();
#else

		if (paracount > 0) {
			muststr(para(1), "user_error");
			error(para(1).u.s);
		} else
			error("run-time error");
		/* don't need a push since "error" never return */
#endif	// End WEDEN Not Enabled
	}
	
	
	void user_warning(void)
	{ /* generates a warning */
		extern void warningf(char *fmt, ...);
		
		if (paracount > 0) {
			muststr(para(1), "user_warning");
			warningf("%s", para(1).u.s);
		} else
			usage("warning(\"warning message\")");
	}
	
	void user_notice(void)
		{ /* generates a warning */
			
			if (paracount > 0) {
				muststr(para(1), "user_notice");
				// the noticef() function happens to format the data in a way that does not leave
				// full freedom to the user so we simply send a notice out with the contents of the users command
				startWedenMessage();
				appendFormattedWedenMessage("<b><item type=\"notice\"><![CDATA[%s]]></item></b>", para(1).u.s);
				endWedenMessage();
			} else
				usage("notice(\"notification message\")");
		}

	/*-	touch variables (pointed to by a pointer)
	 only their parents shall be re-evaluated
	 -*/
	void touch(void)
	{
		extern void schedule_parents_of(symptr);
		extern void eval_formula_queue(void);
		int i;

		for (i = 1; i <= paracount; i++) {
			mustaddr(para(i), "touch()");
			schedule_parents_of((symptr) para(i).u.v.y);
		}
		eval_formula_queue();
		pushUNDEF();
	}

	void get_environ(void)
	{
		/*#ifdef WEDEN_ENABLED
		 
		 if (paracount < 1)
		 usage("string = getenv(\"env_name\")");
		 muststr(para(1), "getenv()");
		 // We will not give user the environment so we return undefined value
		 sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>get_environ</builtinname><edenname>getenv</edenname><reason code=\"1\">Potential Security Risk - Will return undefined (@)</reason></item></b>");
		 pushUNDEF();
		 
		 #else
		 */
		Datum d;
		char *s;

		if (paracount < 1)
			usage("string = getenv(\"env_name\")");

		muststr(para(1), "getenv()");
		if ((s = getenv(para(1).u.s))) {
			dpush(d, STRING, s);
		} else
		pushUNDEF();
		//#endif	// End WEDEN Not Enabled
	}

	void put_environ(void)
	{
#ifdef WEDEN_ENABLED

		Datum d;

		if (paracount < 1)
		usage("ok = putenv(\"env_name=value\")");
		muststr(para(1), "putenv()");

		// We will not let the user set the environment
		sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>put_environ</builtinname><edenname>putenv</edenname><reason code=\"1\">Potential Security Risk - Will return -1</reason></item></b>");
		dpush(d, INTEGER, (-1));

#else
		Datum d;
		char *s;

		if (paracount < 1)
		usage("ok = putenv(\"env_name=value\")");

		muststr(para(1), "putenv()");
		s = getheap(strlen(para(1).u.s) + 1);
		strcpy(s, para(1).u.s);
		dpush(d, INTEGER, putenv(s));
#endif	// End WEDEN Not Enabled
	}

	/*** errno ***/
#include <errno.h>
	void error_no(void)
	{
#ifdef WEDEN_ENABLED
		sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>error_no</builtinname><edenname>error_no</edenname><reason code=\"1\">Potential Security Risk - Will always return 0 (zero)</reason></item></b>");
		Datum d;
		dpush(d, INTEGER, 0);
#else
		Datum d;
		dpush(d, INTEGER, errno);
#endif	// End WEDEN Not Enabled
	}

	/*** process ***/
#undef FAIL
#define FAIL (-1)
	// 																							<<<------- TODO: Check this causes no other issues by locking it down
	void backgnd(void)
	{
#ifdef WEDEN_ENABLED
		sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>backgnd</builtinname><edenname>backgnd</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");
#else
		int pid;
		Datum d;
		Datum *p;
		int argc, i;
		char **argv;

		p = &para(0);
		argc = p[0].u.i;

		if (argc < 2)
		usage("pid = backgnd(\"cmd\", \"cmd\", \"arg1\", ...);");

		argv = (char **) emalloc(sizeof(char *) * argc);
		for (i = 2; i <= argc; i++) {
			muststr(p[i], "backgnd()");
			argv[i - 2] = p[i].u.s;
		}
		argv[argc - 1] = (char *) 0;

		pid = fork();
		switch (pid) {
			case 0: /* child process */
			{
				FILE *f;

				f = fopen("/dev/null", "r");
				close(0); /* close stdin */
				dup2(fileno(f), 0); /* redirect stdin to /dev/null */
				fclose(f);
			}
			pid = execvp(para(1).u.s, argv);
			if (pid == FAIL) { /* can't execute */
				noticef("backgnd: can't execute %s\n", para(1).u.s);
			}
			break;

			default: /* parent process */
			dpush(d, INTEGER, pid);
			break;
		}
		free(argv);
#endif	// End WEDEN Not Enabled
	}

	// 																					<<<------- TODO: Check this causes no other issues by locking it down
	void pipe_process(void)
	{

#ifdef WEDEN_ENABLED
		sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>pipe_process</builtinname><edenname>pipe</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");
#else

		int pid;
		Datum d;
		Datum *p;
		int argc, i;
		char **argv;
		int fdes[2];

		p = &para(0);
		argc = p[0].u.i;

		if (argc < 2)
		usage("pid = pipe(\"cmd\", \"cmd\", \"arg1\", ...);");

		muststr(p[1], "pipe()");

		argv = (char **) getheap(sizeof(char *) * argc);
		for (i = 2; i <= argc; i++) {
			muststr(p[i], "pipe()");
			argv[i - 2] = p[i].u.s;
		}
		argv[argc - 1] = (char *) 0;

		if (pipe(fdes) != FAIL) {
			pid = fork();
			switch (pid) {
				case 0: /* child process */
				i = fileno(stdin);
				close(i); /* close stdin */
				dup2(fdes[0], i); /* redirect stdin */
				close(fdes[1]); /* close other end */
				pid = execvp(p[1].u.s, argv);
				if (pid == FAIL) { /* can't execute */
					noticef("pipe(): can't execute %s\n", p[1].u.s);
				}
				break;

				default: /* parent process */
				i = fileno(stdout);
				close(i); /* close stdout */
				dup2(fdes[1], i); /* redirect stdout */
				close(fdes[0]); /* close other end */
				dpush(d, INTEGER, pid);
				break;
			}
		}
#endif	// End WEDEN Not Enabled
	}

	// 																					<<<------- TODO: Check this causes no other issues by locking it down

	/* ipopen: Interactive Process Open.  Creates a process running
	 concurrently with Eden, and returns file descriptors to this child
	 processes stdin, stdout and stderr.  A pseudo-tty is used to
	 communicate with the child processes stdin and stdout in order to
	 get the child processes stdio library to use terminal-style
	 blocking.  This code is largely taken from David A. Curry, "UNIX
	 Systems Programming for SVR4" (1996) O'Reilly, appendix D.  [Ash] */
	void ipopen(void) {

#ifdef WEDEN_ENABLED
		sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>ipopen</builtinname><edenname>ipopen</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");
#else

		int pid;
		Datum d;
		Datum *p;
		int argc, i;
		char **argv;
		int pipefds[2];
#ifdef __APPLE__
		char mastername[32]; /* pty clone device   */
		char slavename[32];
#else
		char *mastername;
		char *slavename;
#endif
		int master, slave;
		struct termios modes;
		char *s, *t;

		p = &para(0);
		argc = p[0].u.i;

		if (argc < 2)
		usage("fileidlist = ipopen(\"cmd\", \"cmd\", \"arg1\", ...);");

		muststr(p[1], "ipopen()");

		/* Check the command supplied is actually executable */
		if (access(p[1].u.s, X_OK) < 0) {
			errorf("ipopen(): can't execute command file %s: %s",
					p[1].u.s, strerror(errno));
		}

		argv = (char **) getheap(sizeof(char *) * argc);
		for (i = 2; i <= argc; i++) {
			muststr(p[i], "ipopen()");
			argv[i - 2] = p[i].u.s;
		}
		argv[argc - 1] = (char *) 0;

		/* Get a master pseudo-tty */
#ifdef __APPLE__
		/* BSD pttys */
		for (s = "pqrs"; *s != '\0'; s++) {
			for (t = "0123456789abcdef"; *t != '\0'; t++) {
				sprintf(mastername, "/dev/pty%c%c", *s, *t);
				if ((master = open(mastername, O_RDWR)) >= 0)
				goto foundMaster;
			}
		}
		foundMaster:
		if (*s == '\0' && *t == '\0') {
			errorf("ipopen(): failed to open master ptty\n");
		}
#else
		/* SVR4 pttys */
		mastername = "/dev/ptmx";
		if ((master = open(mastername, O_RDWR)) < 0) {
			errorf("ipopen(): failed to open master ptty %s\n", mastername);
		}

		/* Set the permissions on the slave */
		if (grantpt(master) < 0) {
			close(master);
			errorf("ipopen(): failed to set permissions on slave ptty\n");
		}

		/* Unlock the slave */
		if (unlockpt(master) < 0) {
			close(master);
			errorf("ipopen(): failed to unlock the slave ptty\n");
		}
#endif /* SVR4 pttys */

		/* Create a pipe to link Eden with the child's stderr */
		if (pipe(pipefds) == FAIL) {
			close(master);
			errorf("ipopen(): can't make pipe\n");
		}

		/* Start a child process */
		if ((pid = fork()) < 0) {
			close(master);
			errorf("ipopen(): fork failed\n");
		}

		if (pid == 0) {
			/* Child process.  The child process will open the slave, which
			 will become its controlling terminal. */

			/* Get rid of our current controlling terminal */
			setsid();

			/* Get the name of the slave ptty */
#ifdef __APPLE__
			sprintf(slavename, "/dev/tty%c%c", *s, *t);
#else
			if ((slavename = ptsname(master)) == NULL) {
				close(master);
				perror("ipopen(): couldn't get name of slave ptty");
				exit(1);
			}
#endif

			/* Open the slave ptty */
			if ((slave = open(slavename, O_RDWR)) < 0) {
				close(master);
				perror("ipopen(): couldn't open slave ptty");
				exit(1);
			}

#if !defined(__APPLE__) && !defined(__CYGWIN__)
			/* Push the hardware emulation module */
			if (ioctl(slave, I_PUSH, "ptem") < 0) {
				close(master);
				close(slave);
				perror("ipopen(): ioctl couldn't push ptem module");
				exit(1);
			}

			/* Push the line discipline module */
			if (ioctl(slave, I_PUSH, "ldterm") < 0) {
				close(master);
				close(slave);
				perror("ipopen(): ioctl couldn't push ldterm module");
				exit(1);
			}
#endif

			/* Need to turn off ECHO in the child, otherwise the input sent
			 to it is echo'd back */
			if (tcgetattr(slave, &modes) < 0) {
				close(master);
				close(slave);
				perror("ipopen(): couldn't get terminal attributes");
				exit(1);
			}
			modes.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHOKE);
			if (tcsetattr(slave, TCSANOW, &modes) < 0) {
				close(master);
				close(slave);
				perror("ipopen(): couldn't set terminal attributes");
				exit(1);
			}

			/* Close the master: this is not needed in the child */
			close(master);

			/* Set the child's stdin and stdout to be the slave, then get
			 rid of the original file descriptor */
			dup2(slave, 0);
			dup2(slave, 1);
			close(slave);

			/* Connect the child's stderr to the writing end of the pipe */
			i = fileno(stderr);
			close(i);
			dup2(pipefds[1], i);
			close(pipefds[0]);

			/* Execute the command */
			pid = execvp(p[1].u.s, argv);
			/* If exec returns, there has been an error.  But now we have
			 closed stdin and stdout, so there is no way that the child
			 can signal this problem to the parent in the usual way.  So
			 we open /dev/console and assign stderr (2) to it to make the
			 output from perror come out somewhere sensible.  (/dev/tty
			 doesn't seem to work). */
			i = open("/dev/console", O_WRONLY);
			dup2(i, 2);
			perror(p[1].u.s);
			exit(1);
			/* ...should never get here */
		}

		/* Parent process */
		/* Close the writing end of the pipe -- the parent only needs the
		 reading end */
		close(pipefds[1]);

		/* Return a list comprising fileids for stdin, stdout and stderr
		 of the child process to the Eden caller of this function. */
		dpush(d, INTEGER, master);
		dpush(d, INTEGER, master);
		dpush(d, INTEGER, pipefds[0]);
		makearr(3);

#endif	// End WEDEN Not Enabled
	}

	// 																					<<<------- TODO: Check this causes no other issues by locking it down
	void ipclose(void) {

#ifdef WEDEN_ENABLED
		sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>ipclose</builtinname><edenname>ipclose</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");
#else

		int rfd, wfd, efd;

		if (paracount != 1)
		usage("ipclose([rfd, wfd, efd]);");
		mustlist(para(1), "ipclose()");
		if (para(1).u.a->u.i != 3)
		usage("ipclose([rfd, wfd, efd]);");

		mustint(para(1).u.a[1], "ipclose()");
		mustint(para(1).u.a[2], "ipclose()");
		mustint(para(1).u.a[3], "ipclose()");

		rfd = para(1).u.a[1].u.i;
		wfd = para(1).u.a[2].u.i;
		efd = para(1).u.a[3].u.i;

		/*
		 debugMessage("ipclose: closing fds %d %d %d\n", rfd, wfd, efd);
		 */

		close(rfd);
		close(wfd);
		close(efd);
#endif	// End WEDEN Not Enabled
	}

	// 																					<<<------- TODO: Check this causes no other issues by locking it down

	void fdready(void) {
#ifdef WEDEN_ENABLED
		sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>fdready</builtinname><edenname>fdready</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");
#else
		int fd, sec, milli, block, ret;
		char type;
		fd_set readfds, writefds, exceptfds;
		struct timeval tv;
		struct timeval *tvp;
		Datum d;

		if (paracount != 3)
		usage("fdready(fd, type, [sec, milli]);");

		mustint(para(1), "fdready()");
		mustchar(para(2), "fdready()");
		if (para(3).type != UNDEF)
		mustlist(para(3), "fdready()");

		fd = para(1).u.i;
		type = para(2).u.i;

		if (para(3).type == UNDEF) {
			block = 1;
		} else {
			block = 0;
			if (para(3).u.a->u.i != 2) {
				usage("fdready(fd, type, [sec, milli]);");
			} else {
				mustint(para(3).u.a[1], "fdready()");
				mustint(para(3).u.a[2], "fdready()");
				sec = para(3).u.a[1].u.i;
				milli = para(3).u.a[2].u.i;
			}
		}

		FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		FD_ZERO(&exceptfds);

		switch (type) {
			case 'r': FD_SET(fd, &readfds); break;
			case 'w': FD_SET(fd, &writefds); break;
			case 'e': FD_SET(fd, &exceptfds); break;
			default:
			errorf("fdready(): second type argument must be one of 'r', 'w' or 'e'");
		}

		if (block) {
			tvp = NULL;
		} else {
			tv.tv_sec = sec;
			tv.tv_usec = milli;
			tvp = &tv;
		}

		/*
		 if (!block) {
		 debugMessage("fdready %d %c %d %d\n", fd, type, sec, milli);
		 } else {
		 debugMessage("fdready %d %c block\n", fd, type, sec, milli);
		 }
		 */

		ret = select(fd+1, &readfds, &writefds, &exceptfds, tvp);

		/*
		 debugMessage("returning %d\n", ret);
		 */

		dpush(d, INTEGER, ret);
#endif	// End WEDEN Not Enabled
	}

	// 																					<<<------- TODO: Check this causes no other issues by locking it down
	void rawread(void) {
#ifdef WEDEN_ENABLED
		sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>rawread</builtinname><edenname>rawread</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");
#else
		char *s, *t;
		int fd, maxbytes;
		ssize_t status;
		Datum d;

		if (paracount != 2)
		usage("rawread(fd, maxbytes);");

		mustint(para(1), "rawread()");
		mustint(para(2), "rawread()");

		fd = para(1).u.i;
		maxbytes = para(2).u.i;

		s = (char *) malloc(maxbytes);

		status = read(fd, s, maxbytes);

		if (status == -1) {
			errorf("rawread(): %s", strerror(errno));
			pushUNDEF();
		} else {
			/* Make a permanently accessible Eden string from the result from read */
			t = getheap((int)status + 1);
			strncpy(t, s, status);
			t[(int)status] = '\0'; /* terminate the string t as strncpy might not */
			dpush(d, STRING, t);
		}

		free(s);
#endif	// End WEDEN Not Enabled
	}

	// 																					<<<------- TODO: Check this causes no other issues by locking it down
	void rawwrite(void) {
#ifdef WEDEN_ENABLED
		sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>rawwrite</builtinname><edenname>rawwrite</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");
#else
		int fd;
		char *s;
		size_t status, n;

		if (paracount != 2)
		usage("rawwrite(fd, string);");

		mustint(para(1), "rawwrite()");
		muststr(para(2), "rawwrite()");

		fd = para(1).u.i;
		s = para(2).u.s;

		n = strlen(s);
		status = write(fd, s, n);

		if (status != n)
		errorf("rawwrite(): %s", strerror(errno));

		pushUNDEF();
#endif	// End WEDEN Not Enabled
	}

	// 																					<<<------- TODO: Check this causes no other issues by locking it down
#ifdef linux

	void rawserialopen(void) {
#ifdef WEDEN_ENABLED
		sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>rawserialopen</builtinname><edenname>rawserialopen</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");
#else
		char * file;
		int fd;
		Datum d;
		struct termios new;
		int r;

		if (paracount != 1)
		usage("rawserialopen(file - eg /dev/ttyS0);");

		muststr(para(1), "rawserialopen()");
		file = para(1).u.s;

		fd = open(file, O_RDWR|O_NONBLOCK|O_EXCL);
		if (fd < 0)
		perror("rawserialopen: couldn't open file");

		r = tcgetattr(fd, &new);
		if (r != 0) perror("rawserialopen: tcgetattr");

		cfmakeraw(&new);

		r = cfsetospeed(&new, B9600);
		if (r != 0) perror("rawserialopen: cfsetospeed");

		r = cfsetispeed(&new, B9600);
		if (r != 0) perror("rawserialopen: cfsetispeed");

		r = tcsetattr(fd, TCSANOW, &new);
		if (r != 0) perror("rawserialopen: tcsetattr");

		dpush(d, INTEGER, fd);
#endif	// End WEDEN Not Enabled
	}

	// 																					<<<------- TODO: Check this causes no other issues by locking it down
	void rawserialclose(void) {
#ifdef WEDEN_ENABLED
		sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>rawserialclose</builtinname><edenname>rawserialclose</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");
#else
		int fd;

		if (paracount != 1)
		usage("rawserialclose(fd);");

		mustint(para(1), "rawserialclose()");

		fd = para(1).u.i;

		if (close(fd) < 0)
		perror("rawserialclose(): couldn't close file");

		pushUNDEF();
#endif	// End WEDEN Not Enabled
	}

	// 																					<<<------- TODO: Check this causes no other issues by locking it down
	void rawserialread(void) {
#ifdef WEDEN_ENABLED
		sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>rawserialread</builtinname><edenname>rawserialread</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");
#else
		char *s, *t;
		int fd, maxbytes;
		ssize_t status;
		Datum d;

		if (paracount != 2)
		usage("rawserialread(fd, maxbytes);");

		mustint(para(1), "rawserialread()");
		mustint(para(2), "rawserialread()");

		fd = para(1).u.i;
		maxbytes = para(2).u.i;

		s = (char *) malloc(maxbytes);

		status = read(fd, s, maxbytes);

		if (status == -1) {
			if (errno == 11) {
				/* ignore error 11: resource unavailable, since we seem to get this
				 instead of EOF with Rod's railway PIC */
				pushUNDEF();
			} else {
				errorf("rawread(): %d", errno);
				pushUNDEF();
			}
		} else {
			/* Make a permanently accessible Eden string from the result from read */
			t = getheap((int)status + 1);
			strncpy(t, s, status);
			t[(int)status] = '\0'; /* terminate the string t as strncpy might not */
			dpush(d, STRING, t);
		}

		free(s);
#endif	// End WEDEN Not Enabled
	}

	// 																					<<<------- TODO: Check this causes no other issues by locking it down
	void usbhidopen(void) {
#ifdef WEDEN_ENABLED
		sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>usbhidopen</builtinname><edenname>usbhidopen</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");
#else
		char *file;
		int fd;
		Datum d;

		if (paracount != 1)
		usage("usbhidopen(file - eg /dev/input/event0);");

		muststr(para(1), "usbhidopen()");
		file = para(1).u.s;

		if ((fd = open(file, O_RDONLY)) < 0)
		perror("usbhidopen(): couldn't open file");

		dpush(d, INTEGER, fd);
#endif	// End WEDEN Not Enabled
	}

	// 																					<<<------- TODO: Check this causes no other issues by locking it down
#include <linux/input.h>
	void usbhidread(void) {
#ifdef WEDEN_ENABLED
		sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>usbhidread</builtinname><edenname>usbhidread</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");
#else
		int fd;
		fd_set readfds;
		struct timeval tv;
		struct timeval *tvp;
		size_t read_bytes;
		struct input_event ev;
		Datum d;

		if (paracount != 1)
		usage("usbhidread(fd);");

		mustint(para(1), "usbhidread()");

		fd = para(1).u.i;

		FD_ZERO(&readfds);
		FD_SET(fd, &readfds);

		tv.tv_sec = 0;
		tv.tv_usec = 0;
		tvp = &tv;

		if (select(fd+1, &readfds, 0, 0, tvp)) {
			/* there is something to be read */

			read_bytes = read(fd, &ev, sizeof(struct input_event));
			if (read_bytes < (int) sizeof(struct input_event)) {
				/* !@!@ perhaps use errorf instead of perror */
				perror("usbhidread(): short read");
			}

			/* time output in same form as ftime (Eden), finetime (C, above) */
			dpush(d, INTEGER, ev.time.tv_sec);
			dpush(d, INTEGER, ev.time.tv_usec / 1000);
			makearr(2);
			dpush(d, INTEGER, ev.type);
			dpush(d, INTEGER, ev.code);
			dpush(d, INTEGER, ev.value);
			makearr(4);

		} else {
			/* fd can't be read from currently */
			makearr(0);

		}
#endif	// End WEDEN Not Enabled
	}

	// 																					<<<------- TODO: Check this causes no other issues by locking it down
	void usbhidclose(void) {
#ifdef WEDEN_ENABLED
		sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>usbhidclose</builtinname><edenname>usbhidclose</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");
#else
		int fd;

		if (paracount != 1)
		usage("usbhidclose(fd);");

		mustint(para(1), "usbhidclose()");

		fd = para(1).u.i;

		if (close(fd) < 0)
		perror("usbhidclose(): couldn't close file");

		pushUNDEF();
#endif	// End WEDEN Not Enabled
	}
#endif

#include "../pcre-3.9/pcre.h"
	void regmatch(void)
	{
		pcre *re;
		const char *error;
		int erroffset;
#define OVECCOUNT 30 /* size of output vector: must be a multiple of 3 */
		int ovector[OVECCOUNT];
		int rc, i;
		char *pattern, *subject;
		char *substring_start;
		int substring_length;
		char *s, *t;
		Datum d;

		if (paracount != 2)
		usage("regmatch(pattern, subject)");

		muststr(para(1), "regmatch");
		muststr(para(2), "regmatch");

		pattern = para(1).u.s;
		subject = para(2).u.s;

		re = pcre_compile(pattern,
				0, /* default options */
				&error, /* for error message */
				&erroffset, /* for error offset */
				NULL); /* use default character tables */

		if (re == NULL) {
			errorf("couldn't compile regular expression '%s': failed at offset %d: %s",
					pattern, erroffset, error);
		}

		/* !@!@ need to run this multiple times and give a controlling limit
		 parameter... return result needs some thought: list of matches,
		 each of which can be a list of subpattern matches? */

		rc = pcre_exec(re, /* the compiled pattern */
				NULL, /* we didn't study the pattern */
				subject, /* the subject string */
				(int)strlen(subject), /* length of the subject */
				0, /* start at offset 0 in the subject */
				0, /* default options */
				ovector, /* vector for substring information */
				OVECCOUNT); /* number of elements in the vector */

#ifdef DEBUG
		if (Debug & 8192) {
			fprintf(stderr,
					"regmatch: pcre_exec p='%s' s='%s' rc=%d o[0]=%d o[1]=%d\n",
					pattern, subject, rc, ovector[0], ovector[1]);
		}
#endif

		if (rc == PCRE_ERROR_NOMATCH) {
			/* no matches found: return an empty list */
			makearr(0);

		} else if (rc < 0) {
			/* these are internal errors only: a human-readable version isn't
			 very important */
			errorf("internal matching error %d", rc);

		} else if (rc == 0) {
			/* the output vector wasn't big enough */
			/* !@!@ could remove this error case by using pcre_fullinfo after
			 compiling the RE and form a dynamically sized ovector */
			errorf("ovector overflow: found more than %d matches", (OVECCOUNT / 3) - 1);

		} else {
			for (i = 0; i < rc; i++) {
				if ((ovector[2*i] == -1) && (ovector[2*i + 1] == -1)) {
					/* subpattern was not used at all, eg second result in
					 (a|(z))(bc) when matching "abc" */
					pushUNDEF();

				} else {
					substring_start = subject + ovector[2*i];
					substring_length = ovector[2*i + 1] - ovector[2*i];

					t = getheap(substring_length + 1);
					strncpy(t, substring_start, substring_length);
					t[substring_length] = '\0';
					dpush(d, STRING, t);
				}
			}

			makearr(rc);
		}

		free(re);
		return;
	}

	/* Functions to maintain a malloc'd string which can grow, whilst
	 trying to make sensible requests of malloc and realloc */
	typedef struct StringBuf {
		char * s;
		int l;
	}StringBuf;

	StringBuf * newStringBuf(int requestedL) {
		StringBuf *res;
		int l;

		res = emalloc(sizeof(StringBuf));

		l = 128;
		while (l < requestedL) l *= 2;

		res->s = emalloc(l);
		res->l = l;

#ifdef DEBUG
		if (Debug & 8192) {
			debugMessage("newStringBuf 0x%x s=0x%x l=%d %s\n",
					res, res->s, res->l, res->s);
		}
#endif
		return res;
	}

	void reallocStringBuf(StringBuf *sb, int requestedL) {
		int l;
#ifdef DEBUG
		int reallocDone = 0;;
#endif

		l = sb->l;
		while (l < requestedL) l *= 2;

		if (l != sb->l) {
			sb->s = erealloc(sb->s, l);
			sb->l = l;
#ifdef DEBUG
			reallocDone = 1;
#endif
		}

#ifdef DEBUG
		if (Debug & 8192) {
			debugMessage("reallocStringBuf 0x%x s=0x%x l=%d rl=%d %s %s\n",
					sb, sb->s, sb->l, requestedL, reallocDone ? "Y" : "N", sb->s);
		}
#endif
	}

	void freeStringBuf(StringBuf *sb) {
#ifdef DEBUG
		if (Debug & 8192) {
			debugMessage("freeStringBuf 0x%x s=0x%x l=%d %s\n",
					sb, sb->s, sb->l, sb->s);
		}
#endif

		free(sb->s);
		/* mark pointers and data invalid to help track down nasty bugs */
		sb->s = 0;
		sb->l = 0;
		free(sb);
		sb = 0;
	}

	/* Replace references in the form "$n" (0<=n<=99) in the "replace"
	 string with substrings from the "subject" string, the exact
	 locations of which are given in ovector (which "n" is a reference
	 into).  rc is the number of substring locations stored in ovector
	 (two values in ovector for each substring).  The result is a new
	 StringBuf, which should be freed by the caller. */
	StringBuf * replaceRefs(char * replace,
			char * subject,
			char * pattern, /* this just for error output */
			int ovector[OVECCOUNT],
			int rc) {
		int n, i;
		char *rp; /* pointer into replace */
		StringBuf *result;
		int resi = 0; /* index into result */
		char *start;
		int length;

		result = newStringBuf(strlen(replace) + 1);

		for (rp = replace; *rp != '\0';) {
			if (*rp == '\\') {
				if (*(rp+1) == '$') {
					/* \$ is a literal $ */
					result->s[resi++] = '$';
					rp += 2;
				} else {
					/* \ followed by a char that isn't $ */
					result->s[resi++] = *rp++;
				}
			} else if (*rp == '$') {
				/* $ without preceding \ */
				if (isdigit(*++rp)) {
					n = *rp - '0';
					if (isdigit(*(rp+1))) {
						n = n*10 + (*(rp+1) - '0');
						rp++;
					}
				} else {
					errorf("$ in replacement denotes a reference and must be followed by an integer (found %c)",
							*rp);
				}

				if (n<0)
				errorf("reference cannot be negative (encountered %d)", n);
				if (n>=rc)
				errorf("reference number greater than the number of subpatterns (reference %d, subpatterns %d in pattern \"%s\")",
						n, rc-1, pattern);
				if ((ovector[2*n] == -1) && (ovector[2*n + 1] == -1))
				errorf("reference (%d) to subpattern that was not used", n);

				start = subject + ovector[2*n];
				length = ovector[2*n + 1] - ovector[2*n];

				reallocStringBuf(result, resi + length);

				for (i = 0; i < length; i++) {
					result->s[resi++] = start[i];
				}

				rp++;

			} else {
				/* just copy a normal character across from replace to result */
				result->s[resi++] = *rp++;
			}
		}
		result->s[resi] = '\0';

		return result;
	}

	
	
	void regreplace(void)
	{
		pcre *re;
		const char *error;
		int erroffset;
#define OVECCOUNT 30 /* size of output vector: must be a multiple of 3 */
		int ovector[OVECCOUNT];
		int rc, i;
		char *pattern, *subject, *replace;
		int limit, count;
		char *substring_start;
		int substring_length;
		char *s, *t;
		Datum d;
		int si; /* index into subject */
		StringBuf *newsubject;
		StringBuf *replaced;

		if ((paracount != 3) && (paracount != 4))
		usage("regreplace(pattern, replacement, subject [, limit])");

		muststr(para(1), "regreplace");
		muststr(para(2), "regreplace");
		muststr(para(3), "regreplace");

		pattern = para(1).u.s;
		replace = para(2).u.s;
		subject = para(3).u.s;

		if (paracount == 4) {
			mustint(para(4), "regreplace");
			limit = para(4).u.i;
		} else {
			limit = -1; /* unlimited by default: replace all occurrances */
		}
		count = 0;

		re = pcre_compile(pattern,
				0, /* default options */
				&error, /* for error message */
				&erroffset, /* for error offset */
				NULL); /* use default character tables */

		if (re == NULL) {
			errorf("couldn't compile regular expression '%s': failed at offset %d: %s",
					pattern, erroffset, error);
		}

		newsubject = newStringBuf(strlen(subject) + 1);
		*newsubject->s = '\0';
		si = 0;

		do {
			/* rc is the number of substrings captured by the match, including
			 the substring that matched the entire expression */
			rc = pcre_exec(re, /* the compiled pattern */
					NULL, /* we didn't study the pattern */
					subject, /* the subject string */
					(int)strlen(subject), /* length of the subject */
					si, /* start at offset 0 in the subject */
					0, /* default options */
					ovector, /* vector for substring information */
					OVECCOUNT); /* number of elements in the vector */

			if (rc > 0) {
				/* found a match: replace this match of the complete pattern in
				 the subject string with the replacement string (with
				 appropriate substitutions in the replacement string) */

				/* replace $ns in replace string */
				replaced = replaceRefs(replace, subject, pattern, ovector, rc);

#ifdef DEBUG
				if (Debug & 8192) {
					debugMessage("regreplace: p=%s r=%s s=%s o[0]=%d o[1]=%d\n",
							pattern, replaced->s, subject, ovector[0], ovector[1]);
				}
#endif

				/* make sure enough memory is allocated.  This calculation is a
				 simplified overestimate. */
				reallocStringBuf(newsubject,
						strlen(newsubject->s) +
						strlen(subject + si) +
						strlen(replaced->s) + 1);

				/* append stuff in subject from after the end of the last match
				 until the beginning of this match to newsubject.  ie from
				 subject[si] to subject[ovector[0]]. */
				strncat(newsubject->s, subject + si, ovector[0] - si);

				/* append replacement to newsubject */
				strcat(newsubject->s, replaced->s);

				/* store location of end of match */
				si = ovector[1];

				/* where the match is of zero length, need to make sure we keep
				 progressing.  (Don't really understand what I've done here,
				 but hopefully it works). */
				if (ovector[0] == ovector[1]) si++;

				freeStringBuf(replaced);

			} else if (rc == 0) {
				/* the output vector wasn't big enough */
				/* !@!@ could remove this error case by using pcre_fullinfo after
				 compiling the RE and form a dynamically sized ovector */
				errorf("ovector overflow: found more than %d matches",
						(OVECCOUNT / 3) - 1);

			} else if (rc == PCRE_ERROR_NOMATCH) {
				/* dealt with by while below */

			} else if (rc < 0) {
				/* these are internal errors only: a human-readable version isn't
				 very important */
				errorf("internal matching error %d", rc);

			}

		} while ((rc != PCRE_ERROR_NOMATCH) &&
				((limit == -1) || (++count < limit)));

		/* no matches found now - return the rest of the subject string unaltered */
		reallocStringBuf(newsubject,
				strlen(newsubject->s) + strlen(subject + si) + 1);
		strcat(newsubject->s, subject + si);
		t = getheap(strlen(newsubject->s) + 1);
		strcpy(t, newsubject->s);
		dpush(d, STRING, t);

		freeStringBuf(newsubject);
		free(re);
		return;
	}

	void todo(void)
	{
		if (paracount < 1)
		usage("todo(string);");

		muststr(para(1), "todo()");

#ifdef DEBUG
		if (Debug&2)
		debugMessage("todo called: queuing <%s>\n", para(1).u.s);
#endif

		/* keep to the same master, timer token is not not available */
#ifndef TTYEDEN
		queue(para(1).u.s, topMasterStack(), NULL);
#else
		queue(para(1).u.s, topMasterStack());
#endif
		pushUNDEF();
	}

	// 																					<<<------- TODO: Check this causes no other issues by locking it down
	void get_msgq(void)
	{
#ifdef WEDEN_ENABLED
		sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>get_msgq</builtinname><edenname>get_msgq</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");
#else
#ifdef ipc
		Datum d;

		if (paracount < 2)
		usage("msqid = get_message_queue(key,msgflg);");
		mustint(para(1), "get_msgq()");
		mustint(para(2), "get_msgq()");
		dpush(d, INTEGER, msgget((key_t) para(1).u.i, para(2).u.i));

#endif	// Not Def IPC
#endif	// End WEDEN Not Enabled
	}

#define MSG_SIZE 1024

	// 																					<<<------- TODO: Check this causes no other issues by locking it down
	void send_message(void)
	{
#ifdef WEDEN_ENABLED
		sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>send_message</builtinname><edenname>send_message</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");
#else

#ifdef ipc
		int msqid, msgflg;
		int result;
		int msize;
		Datum *p;
		Datum d;
		static struct msgbuf msg;

		p = &para(0);
		if (p[0].u.i < 3) /* paracount */
		usage("ok = send_msg(msqid,[mtype,mtext],msgflg);");

		mustint(p[1], "send_msg()");
		mustlist(p[2], "send_msg()");
		if (p[2].u.a[0].u.i < 2)
		usage("ok = send_msg(msqid,[mtype,mtext],msgflg);");
		mustint(p[2].u.a[1], "send_msg()");
		muststr(p[2].u.a[2], "send_msg()");
		mustint(p[3], "send_msg()");

		msqid = p[1].u.i;
		msgflg = p[3].u.i;
		msg.mtype = (long) p[2].u.a[1].u.i;
		strncpy(msg.mtext, p[2].u.a[2].u.s, MSG_SIZE);

		msize = strlen(msg.mtext) + 1;
		if (msize > MSG_SIZE)
		msize = MSG_SIZE;

		result = msgsnd(msqid, &msg, msize, msgflg);
		if (result == -1)
		perror("send_msg");
		dpush(d, INTEGER, result);

#endif	// Not Def IPC
#endif	// End WEDEN Not Enabled
	}

	// 																					<<<------- TODO: Check this causes no other issues by locking it down
	void receive_message(void)
	{
#ifdef WEDEN_ENABLED
		sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>receive_message</builtinname><edenname>receive_message</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");
#else
#ifdef ipc
		Datum *p;
		Datum d;
		int i, size;
		static struct msgbuf msg;

		p = &para(0);
		if (p[0].u.i < 3) /* paracount */
		usage("[mtype,mtext] = receive_message(msqid,mtype,msgflg);");

		for (i = 1; i <= 3; i++)
		mustint(p[i], "receive_message()");

		size = msgrcv(p[1].u.i, &msg, MSG_SIZE, (long) p[2].u.i, p[3].u.i);
		if (size == -1) {
			pushUNDEF();
		} else {
			/* terminate the message with null character */
			msg.mtext[(size >= MSG_SIZE) ? MSG_SIZE - 1 : size] = '\0';
			dpush(d, INTEGER, msg.mtype);
			dpush(d, STRING, msg.mtext);
			makearr(2);
		}

#endif	// Not Def IPC
#endif	// End WEDEN Not Enabled
	}

	// 																					<<<------- TODO: Check this causes no other issues by locking it down
	void remove_msgq(void)
	{
#ifdef WEDEN_ENABLED
		sendCompleteWedenMessage("<b><item type=\"disabledcommand\"><builtinname>remove_msgq</builtinname><edenname>remove_msgq</edenname><reason code=\"1\">Potential Security Risk</reason></item></b>");
#else
#ifdef ipc
		Datum d;

		if (paracount < 1)
		usage("remove_msgq(msqid);");
		mustint(para(1), "remove_msgq()");
		dpush(d, INTEGER, msgctl(para(1).u.i, IPC_RMID, 0));

#endif	// Not Def IPC
#endif	// End WEDEN Not Enabled
	}

	/* Generate a random number between 0.0 and 1.0, including 0 but excluding 1.
	 ** [Ant][10/06/05] */

	void realrand(void)
	{
		Datum d;

		if (paracount != 0) {
			usage("real realrand()");
		}
		else {
			d.type = REAL;
			d.u.r = (double)rand() / ((double)(RAND_MAX)+1.0);
			push(d);
		}
	}


	
	
	
	
	
	
	
	// TODO: Need to configure the X Window stuff in Builtin.c

#ifndef TTYEDEN

	/* X Windows Stuff */

	/************************
	 * 	EX Commands	*
	 ************************/

	void E_Disp2PS(void)
	{
		if (paracount < 2)
			usage("Disp2PS(screen, file);");
		pushUNDEF();
	}

	void E_StringRemain(void)
	{
		if (paracount < 2)
		usage("StringRemain(screen, box);");
		pushUNDEF();
	}

	void E_tcl(void)
	{
		extern void warning(char *, char *);
		Datum d;

		if (paracount < 1)
			usage("string tcl(string)");
		/*         printf("E_tcl %s\n", para(1).u.s); */

		if (isundef(para(1))) {
			errorf("Eden tcl() called with @ parameter");
		}
		
		/* sendCompleteFormattedWedenMessage("<b><item type=\"tcloutput\"><![CDATA[TCL: %s\n]]></item></b>", para(1).u.s );  [REENABLE] */

		if (Tcl_GlobalEval(interp, para(1).u.s) == TCL_OK) {
			dpush(d, STRING, interp->result);
		} else {
			char * errorInfo;
			errorInfo = interp->result;

			/* Can't use errorf etc as this uses Tcl, which might be in a mess... */
			outputFormattedMessage("Eden tcl() command, whilst executing `%s`, got Tcl errorInfo: %s\n", para(1).u.s, errorInfo);
			pushUNDEF();
		}
	}

	static void Append(char *v, char *name, char *box)
	{
		Datum screenName, boxName;
		char *s;
		symptr sp;
		Datum *a;

		sp = lookup(v, basecontext);
		if (sp == NULL)
		sp = install(v, basecontext, VAR, UNDEF, 0);
		if (isundef(sp->d)) {
			makearr(0);
			sp->d = newdatum(pop());
		}
		push(sp->d);
		s = (char *) getheap(strlen(name) + 1);
		strcpy(s, name);
		dpush(screenName, STRING, s);
		s = (char *) getheap(strlen(box) + 1);
		strcpy(s, box);
		dpush(boxName, STRING, s);
		makearr(2);
		a = (Datum *) erealloc(sp->d.u.a, (sp->d.u.a->u.i + 2) * sizeof(Datum));
		a[++a->u.i] = newdatum(pop());
		sp->d.u.a = a;
		sp->entry = topEntryStack();
		sp->master = topMasterStack();
		change(sp, FALSE);
	}

	static void Remove(char *v, char *name, char *box)
	{
		Datum d;
		int i, j;
		symptr sp;

		sp = lookup(v,basecontext);
		for (i = sp->d.u.a[0].u.i; i > 0; i--) {
			d = sp->d.u.a[i];
			if (d.u.a[0].u.i == 2
					&& isstr(d.u.a[1]) && strcmp(d.u.a[1].u.s, name) == 0
					&& isstr(d.u.a[2]) && strcmp(d.u.a[2].u.s, box) == 0)
			break;
		}
		if (i > 0) {
			freedatum(d);
			for (j = i + 1; j <= sp->d.u.a[0].u.i; j++)
			sp->d.u.a[j - 1] = sp->d.u.a[j];
			--sp->d.u.a[0].u.i;
			sp->entry = topEntryStack();
			sp->master = topMasterStack();
			change(sp, FALSE);
		}
	}

	
	
	
	// 																					<<<<<<<<<<<<<<<<<<<<-------------- DISPLAY SCREEN HERE
	
	
#include "screen.q.h"      /* create a screen queue for bug fix --sun  */

	static ScoutScreenQ ScoutScreenList = EMPTYQUEUE(ScoutScreenList);

#include "screen.q.c"      /* create a screen queue - bug fix --sun  */

void DisplayScreen(void)
{
	Datum screen;
	int noOfscreen, noOfoScreen, next_ref, select;
	int k, l, r;
	int lskip, rskip;
	intQ *newref, *oldref, *new, discard, *ref, *r2;
	char *name;
	int lastBox;
	char s[256];
	char *t;
	winfo *WinInfo;
	Datum oScreen; /*  changed to variable rather than static --sun*/
	winfo *oWinInfo;
	int MaxRef;
	intQ *Refer;
	ScoutScreen_ATOM ssptr;
	ScoutScreenQ ssq;

	if (paracount < 2 || !is_symbol(para(1)) || para(2).type != STRING)
		usage("DisplayScreen(&screen, screen_name);");

	screen = symbol_of(para(1))->d;
	name = para(2).u.s;

	if (isundef(screen)) error("DisplayScreen called with screen undefined");
	if (Q_EMPTY(&ScoutScreenList)) {init_ScoutScreenList();}
	if (!(ssptr = search_ScoutScreenQ(name))) {ssptr = add_ScoutScreen(name);}

	/* initialization [sy] */
	oScreen = ssptr->obj.oScreen;
	Refer = ssptr->obj.Refer;
	MaxRef = ssptr->obj.MaxRef;
	oWinInfo = ssptr->obj.oWinInfo;

	/* trimming the size of the differences [sy] */
	noOfscreen = screen.u.a[0].u.i;/* no of widgets in screen [Ash] */
	noOfoScreen = oScreen.u.a[0].u.i; /* no of widgets in previous screen [Ash] */
	/* find smaller no of widgets [Ash] */
	k = (noOfscreen < noOfoScreen) ? noOfscreen : noOfoScreen;
	/* iterate through the widget list from the start until we find
	 one that has changed.  lskip is then the number of items on the
	 start of the list that can be ignored (?) [Ash] */
	for (l = 1; l <= k; l++) {
		if (datacmp(screen.u.a[l], oScreen.u.a[l])) {
			break;
		}
	}
	lskip = l - 1;
	
	/* iterate through the widget list from the end of each until we
	 find one that has changed.  rskip is then the number of items
	 on the end of the list that can be ignored (?) [Ash] */
	for (r = 0; r < k - lskip; r++) {
		if (datacmp(screen.u.a[noOfscreen - r], oScreen.u.a[noOfoScreen - r])) {
			break;
		}
	}
	rskip = r;

	/* printf("here %i %i %i %i %s %i\n", noOfscreen, noOfoScreen, lskip,
	 rskip, name, MaxRef); */

	/* match new windows to old windows [sy] */
	/* for each of the positions in the non-skipped screen lists (both
	 new and old), construct an empty linked list which will contain
	 integers [Ash] */
	newref = (intQ *) getheap(sizeof(intQ) * (noOfscreen - rskip - lskip));
	for (l = 0; l < noOfscreen - rskip - lskip; l++)
		newref[l].prev = newref[l].next = newref + l;
	oldref = (intQ *) getheap(sizeof(intQ) * (noOfoScreen - rskip - lskip));
	for (l = 0; l < noOfoScreen - rskip - lskip; l++)
		oldref[l].prev = oldref[l].next = oldref + l;

	/* k runs through previous screen, l through new screen.  Each
	 runs over the region of things that may have changed.  For each
	 l in k... try and identify where the old stuff has ended up in
	 the new.  At each position, add information to the two lists:
	 oldref records where the new widget has moved to; newref
	 records where the old widget was.  [Ash] */
	for (k = lskip; k < noOfoScreen - rskip; k++) {
		for (l = lskip; l < noOfscreen - rskip; l++) {
			if (datacmp(oScreen.u.a[k + 1], screen.u.a[l + 1]) == 0) {
				/* found new (l) widget in old (k): insert l in oldref, k in newref [Ash] */
				/* identical window, take precedence [sy] */
				new = (intQ *) getheap(sizeof(intQ));
				new->obj = l;
				INSERT_Q(&oldref[k - lskip], new);

				new = (intQ *) getheap(sizeof(intQ));
				new->obj = k;
				INSERT_Q(&newref[l - lskip], new);

			} else if ((oScreen.u.a[k + 1].u.a[1].u.i != 0 &&
							oScreen.u.a[k + 1].u.a[1].u.i != 4)
					&& (screen.u.a[l + 1].u.a[1].u.i != 0 &&
							screen.u.a[l + 1].u.a[1].u.i != 4) &&
					strcmp(oScreen.u.a[k + 1].u.a[5].u.s,
							screen.u.a[l + 1].u.a[5].u.s) == 0) {
				/* new (l) widget doesn't match old (k), but they are not
				 TEXT (or TEXTBOX, added by Patrick), and value of pict is
				 the same as before: append l after the current item in
				 oldref, append k after the current item in newref [Ash] */
				/* same picture, lower in precedence [sy] */
				new = (intQ *) getheap(sizeof(intQ));
				new->obj = l;
				APPEND_Q(&oldref[k - lskip], new);

				new = (intQ *) getheap(sizeof(intQ));
				new->obj = k;
				APPEND_Q(&newref[l - lskip], new);
			}
		}
	}

	CLEAN_Q(&discard);
	/* Run through the old screen list, and put any widgets that no
	 longer exist on the discard list.  [Ash] */
	for (l = lskip; l < noOfoScreen - rskip; l++) {
		if (Q_EMPTY(&oldref[l - lskip])) {
			new = (intQ *) getheap(sizeof(intQ));
			new->obj = l;
			APPEND_Q(&discard, new);
		}
	}

	next_ref = noOfoScreen + 1; /* this doesn't seem to be usefully used at all [Ash] */
	/* Run through the new screen list.  [Ash] */
	for (l = lskip; l < noOfscreen - rskip; l++) {
		if (Q_EMPTY(&newref[l - lskip])) {
			
			/* This widget is new (it didn't previously exist) [Ash] */
			if (Q_EMPTY(&discard)) {
				select = next_ref++; /* this can't seem to have any effect [Ash] */
			} else {
				select = discard.next->obj;
				discard.next = discard.next->next;
				discard.next->prev = &discard;
				new = (intQ *) getheap(sizeof(intQ));
				new->obj = l;
				INSERT_Q(&oldref[select - lskip], new);
			}

		} else {
			
			/* If this new widget previously existed [Ash] */
			select = newref[l - lskip].next->obj;
			for (ref = newref[l - lskip].next->next;
					ref != &newref[l - lskip];
					ref = ref->next) {
				for (r2 = oldref[ref->obj - lskip].next;
						r2 != &oldref[ref->obj - lskip];
						r2 = r2->next) {
					if (r2->obj == l) {
						r2 = r2->prev;
						r2->next = r2->next->next;
						r2->next->prev = r2;
					}
				}
			}
			for (ref = oldref[select - lskip].next;
					ref != &oldref[select - lskip];
					ref = ref->next) {
				for (r2 = newref[ref->obj - lskip].next;
						r2 != &newref[ref->obj - lskip];
						r2 = r2->next) {
					if (r2->obj == select) {
						r2 = r2->prev;
						r2->next = r2->next->next;
						r2->next->prev = r2;
					}
				}
			}
		}
		newref[l - lskip].obj = select;
	}

	/* final judgment if any window should actually be discarded [sy, but not in initial versions] */
	/* Build the discard list again: it is a list of the widgets for which the oldref list is empty [Ash] */
	CLEAN_Q(&discard);
	for (l = lskip; l < noOfoScreen - rskip; l++){ 
		if (Q_EMPTY(&oldref[l - lskip])) {
			new = (intQ *) getheap(sizeof(intQ));
			new->obj = l;
			APPEND_Q(&discard, new);
		}
	}

	/* remove unmatched old windows [sy] */
	/* For each widget in the discard list, destroy it and any
	 subboxes, and add it to the Refer list.  [Ash] */
	for (ref = discard.next; ref != &discard; ref = ref->next) {
		new = (intQ *) emalloc(sizeof(intQ));
		new->obj = oWinInfo[ref->obj].ref;
		APPEND_Q(Refer, new);
		for (r = oWinInfo[ref->obj].nbox; r > 0; --r) {
			
			sprintf(s, "catch {destroy .%s.b%d_%d}", name, oWinInfo[ref->obj].ref, r);
			Tcl_EvalEC(interp, s);
#ifdef WEDEN_ENABLED
			sendCompleteFormattedWedenMessage("<s><item type=\"removeobject\"><object id=\".%s.b%d_%d\" /></item></s>", name, oWinInfo[ref->obj].ref, r); // [TO_MAKE_WebEdenDecision]
#endif
		}
		if ((oScreen.u.a[ref->obj + 1].u.a[1].u.i != 0) &&
				(oScreen.u.a[ref->obj + 1].u.a[1].u.i != 4)) {
			/* Non-TEXT and TEXTBOX widgets (DONALD, ARCA, IMAGE) seem to
			 have an extra symbol in the symbol table, perhaps
			 representing their contained image (?), which must now be
			 removed [Ash] */
			/* This only seems to remove content from memory. It does not do
			 * anything with the actual interface (at least that I can see) [Richard] */
			sprintf(s, "b%d_1", oWinInfo[ref->obj].ref);
			Remove(oScreen.u.a[ref->obj + 1].u.a[5].u.s, name, s);
		}
	}

	/* setup new WinInfo [sy] */
	WinInfo = (winfo *) emalloc(sizeof(winfo) * noOfscreen);
	/* Copy the information from the unchanged and ignored widgets on
	 the left and right across.  The stuff in the middle... well... [Ash] */
	for (r = 0; r < lskip; r++){
		WinInfo[r] = oWinInfo[r];
	}
	for (r = lskip; r < noOfscreen - rskip; r++) {
		if (newref[r - lskip].obj > noOfoScreen) {
			if (Q_EMPTY(Refer)) {
				WinInfo[r].ref = MaxRef++;
			} else {
				WinInfo[r].ref = Refer->next->obj;
				Refer->next = Refer->next->next;
				free(Refer->next->prev);
				Refer->next->prev = Refer;
			}
			WinInfo[r].nbox = 0;
		} else {
			WinInfo[r] = oWinInfo[newref[r - lskip].obj];
		}
	}
	
	for (r = 1; r <= rskip; r++) {
		WinInfo[noOfscreen - r] = oWinInfo[noOfoScreen - r];
	}

	/* draw new windows [sy] */
	for (r = lskip; r < noOfscreen - rskip; r++) {
		
		Datum New, Old, win, screenName, winNo, boxName;

		if (newref[r - lskip].obj > noOfoScreen) {
			/* add window [sy] */
			win = screen.u.a[r + 1];
			switch (win.u.a[1].u.i) { /* looks like switching on CONTENT [Ash] */
				case 0:
					/* TEXT [Ash] */
					for (k = 1; k <= win.u.a[2].u.a[0].u.i; k++) {
						
						sprintf(s, "canvas .%s.b%d_%d", name, WinInfo[r].ref, k);
						Tcl_EvalEC(interp, s);
#ifdef WEDEN_ENABLED
						sendCompleteFormattedWedenMessage("<s><item type=\"createtextlabel\"><textlabel id=\".%s.b%d_%d\" /></item></s>", name, WinInfo[r].ref, k);		// [TO_MAKE_WebEdenDecision]
#endif
					}
					WinInfo[r].nbox = win.u.a[2].u.a[0].u.i;
					t = getheap(strlen(name) + 1);
					strcpy(t, name);
					dpush(screenName, STRING, t);
					dpush(winNo, INTEGER, r + 1);
					sprintf(s, "b%d", WinInfo[r].ref);
					t = getheap(strlen(s) + 1);
					strcpy(t, s);
					dpush(boxName, STRING, t);
					makearr(3);
					/* The next line is to make sure that the call command
					 will not cause agentName to be prefixed to variables --sun */
					appAgentName--;
					call(lookup("scout_show_canvas", basecontext), pop(), 0);
					appAgentName++;
					break;
				case 4:
					/* TEXTBOX [Ash] */
					for (k = 1; k <= win.u.a[2].u.a[0].u.i; k++) {
						sprintf(s, "text .%s.b%d_%d", name, WinInfo[r].ref, k);
						Tcl_EvalEC(interp, s);
#ifdef WEDEN_ENABLED
						sendCompleteFormattedWedenMessage("<s><item type=\"createtextbox\"><textbox id=\".%s.b%d_%d\" /></item></s>", name, WinInfo[r].ref, k);		// [TO_MAKE_WebEdenDecision]
#endif
					}
					WinInfo[r].nbox = win.u.a[2].u.a[0].u.i;
					t = getheap(strlen(name) + 1);
					strcpy(t, name);
					dpush(screenName, STRING, t);
					dpush(winNo, INTEGER, r + 1);
					sprintf(s, "b%d", WinInfo[r].ref);
					t = getheap(strlen(s) + 1);
					strcpy(t, s);
					dpush(boxName, STRING, t);
					makearr(3);
					appAgentName--;
					call(lookup("scout_show_text", basecontext), pop(), 0);
					appAgentName++;
					break;
				case 1:
					/* DONALD [Ash] */
					
					
				case 2:
					/* ARCA [Ash] */
				default:
					/* IMAGE is 3 [Ash] */
					sprintf(s, "canvas .%s.b%d_1", name, WinInfo[r].ref);
					Tcl_EvalEC(interp, s);
#ifdef WEDEN_ENABLED
					sendCompleteFormattedWedenMessage("<s><item type=\"createcanvas\"><canvas id=\".%s.b%d_1\" type=\"donald\" /></item></s>", name, WinInfo[r].ref);		// [TO_MAKE_WebEdenDecision]
#endif
					WinInfo[r].nbox = 1;
					t = getheap(strlen(name) + 1);
					strcpy(t, name);
					dpush(screenName, STRING, t); /* 1st argument to scout_show_2D [Ash] */
					dpush(winNo, INTEGER, r + 1); /* 2nd arg [Ash] */
					sprintf(s, "b%d", WinInfo[r].ref);
					t = getheap(strlen(s) + 1);
					strcpy(t, s);
					dpush(boxName, STRING, t); /* 3rd arg [Ash] */
					makearr(3);
					appAgentName--;
					call(lookup("scout_show_2D", basecontext), pop(), 0);
					appAgentName++;
					sprintf(s, "b%d_1", WinInfo[r].ref);
					Append(screen.u.a[r + 1].u.a[5].u.s, name, s);
					break;
			}

		} else if (datacmp(screen.u.a[r + 1], oScreen.u.a[newref[r - lskip].obj + 1])) {
			/* change existing window [sy] */
			New = screen.u.a[r + 1];
			Old = oScreen.u.a[newref[r - lskip].obj + 1];

			/* Destroy and recreate the widget if the type has changed to or from TEXTBOX [Ash] */
					
			if (((New.u.a[1].u.i == 4) && (Old.u.a[1].u.i != 4)) ||
					((New.u.a[1].u.i != 4) && (Old.u.a[1].u.i == 4))) {
				if (New.u.a[1].u.i != 4) {
					/* not TEXTBOX [Ash] */
					for (k = 1; k <= New.u.a[2].u.a[0].u.i; k++) {
																
						sprintf(s, "catch { destroy .%s.b%d_%d }", name, oWinInfo[r].ref, k);
						Tcl_EvalEC(interp, s);
#ifdef WEDEN_ENABLED
						sendCompleteFormattedWedenMessage("<s><item type=\"removeobject\"><object id=\".%s.b%d_%d\" /></item></s>", name, oWinInfo[r].ref, k); // [TO_MAKE_WebEdenDecision]
#endif	
						// Need to create the correct type of base object within Web EDEN
						// which has a distinction between a Text Label and a Drawing.
						// Not actually sure here is User can turn a TextBox into a Drawing.
						// This check is simply to be safe
						
						
						
						switch (New.u.a[1].u.i) {
							case 0: /* TextLabel */
								
								sprintf(s, "canvas .%s.b%d_%d", name, WinInfo[r].ref, k);
								Tcl_EvalEC(interp, s);
#ifdef WEDEN_ENABLED						
								sendCompleteFormattedWedenMessage("<s><item type=\"createtextlabel\"><textlabel id=\".%s.b%d_%d\" /></item></s>", name, WinInfo[r].ref, k); // [TO_MAKE_WebEdenDecision]
#endif
								break;
							case 1: /* Donald */
							case 2: /* Arca */
							case 3: /* Image */
								
								sprintf(s, "canvas .%s.b%d_%d", name, WinInfo[r].ref, k);
								Tcl_EvalEC(interp, s);
#ifdef WEDEN_ENABLED						
								sendCompleteFormattedWedenMessage("<s><item type=\"createcanvas\"><canvas id=\".%s.b%d_%d\" /></item></s>", name, WinInfo[r].ref, k);		// [TO_MAKE_WebEdenDecision]
#endif
								break;
						}
					}
				} else {
					/* TEXTBOX [Ash] */
					for (k = 1; k <= New.u.a[2].u.a[0].u.i; k++) {
						
						sprintf(s, "catch { destroy .%s.b%d_%d }", name, oWinInfo[r].ref, k);
						Tcl_EvalEC(interp, s);
#ifdef WEDEN_ENABLED				
						sendCompleteFormattedWedenMessage("<s><item type=\"removeobject\"><object id=\".%s.b%d_%d\" /></item></s>", name, oWinInfo[r].ref, k); // [TO_MAKE_WebEdenDecision]
#endif
						
						sprintf(s, "text .%s.b%d_%d", name, WinInfo[r].ref, k);
						Tcl_EvalEC(interp, s);
#ifdef WEDEN_ENABLED				
						sendCompleteFormattedWedenMessage("<s><item type=\"createtextbox\"><textbox id=\".%s.b%d_%d\" /></item></s>", name, WinInfo[r].ref, k);		// [TO_MAKE_WebEdenDecision]
#endif
					}
				}
			}

			/* Destroy and recreate subboxes (if "frame" has more than one item in its list) [Ash] */
			for (k = New.u.a[2].u.a[0].u.i + 1;	k <= Old.u.a[2].u.a[0].u.i; k++) {
				
				sprintf(s, "catch { destroy .%s.b%d_%d }", name, oWinInfo[r].ref, k);
				Tcl_EvalEC(interp, s);
#ifdef WEDEN_ENABLED		
				sendCompleteFormattedWedenMessage("<s><item type=\"removeobject\"><object id=\".%s.b%d_%d\" /></item></s>", name, oWinInfo[r].ref, k); // [TO_MAKE_WebEdenDecision]
#endif
				
			}
			
			/* I believe this could also be creating new 'drawing' displays instead of just Text Lables and Text Boxes and
			 * therefore I have reworked this to seperate drawing canvases from that of the original text items.
			 * I have included the old code here encase anyone should have a problem. [Richard] 
			for (k = Old.u.a[2].u.a[0].u.i + 1; k <= New.u.a[2].u.a[0].u.i; k++) {
				switch (New.u.a[1].u.i) {
					case 4:
						// TEXTBOX [Ash]
						sprintf(s, "text .%s.b%d_%d", name, WinInfo[r].ref, k);
						Tcl_EvalEC(interp, s);
						break;
					default:
						// TEXT [Ash]
						sprintf(s, "canvas .%s.b%d_%d", name, WinInfo[r].ref, k);
						Tcl_EvalEC(interp, s);
						break;
				}
			}
			* New for loop starts here
			*/
			for (k = Old.u.a[2].u.a[0].u.i + 1; k <= New.u.a[2].u.a[0].u.i; k++) {
				switch (New.u.a[1].u.i) {
					case 0: // TextLabel
						
						sprintf(s, "canvas .%s.b%d_%d", name, WinInfo[r].ref, k);
						Tcl_EvalEC(interp, s);
#ifdef WEDEN_ENABLED				
						sendCompleteFormattedWedenMessage("<s><item type=\"createtextlabel\"><textlabel id=\".%s.b%d_%d\" /></item></s>", name, WinInfo[r].ref, k); // [TO_MAKE_WebEdenDecision]
#endif
						
						break;
					case 1: // Donald
					case 2: // Arca
					case 3: // Image
						
						sprintf(s, "canvas .%s.b%d_%d", name, WinInfo[r].ref, k);
						Tcl_EvalEC(interp, s);
#ifdef WEDEN_ENABLED				
						sendCompleteFormattedWedenMessage("<s><item type=\"createcanvas\"><canvas id=\".%s.b%d_%d\" /></item></s>", name, WinInfo[r].ref, k);		// [TO_MAKE_WebEdenDecision]
#endif	
						break;
					case 4: // TextBoxes
						
						sprintf(s, "text .%s.b%d_%d", name, WinInfo[r].ref, k);
						Tcl_EvalEC(interp, s);
#ifdef WEDEN_ENABLED				
						sendCompleteFormattedWedenMessage("<s><item type=\"createtextbox\"><textbox id=\".%s.b%d_%d\" /></item></s>", name, WinInfo[r].ref, k);		// [TO_MAKE_WebEdenDecision]
#endif	
						break;
				}
			}
			
			
			
			WinInfo[r].nbox = New.u.a[2].u.a[0].u.i;
			if (((Old.u.a[1].u.i != 0) && (Old.u.a[1].u.i != 4)) ||
					((New.u.a[1].u.i != 0) && (New.u.a[1].u.i != 4)) ||
					strcmp(Old.u.a[5].u.s, New.u.a[5].u.s)) {
				if (Old.u.a[1].u.i != 0 && Old.u.a[1].u.i != 4) {
					sprintf(s, "b%d_1", oWinInfo[newref[r - lskip].obj].ref);
					Remove(Old.u.a[5].u.s, name, s);
				}
				if (New.u.a[1].u.i != 0 && New.u.a[1].u.i != 4) {
					sprintf(s, "b%d_1", WinInfo[r].ref);
					Append(New.u.a[5].u.s, name, s);
				}
			}

			switch (New.u.a[1].u.i) { /* looks like switching on CONTENT [Ash] */
				/* This appears to be the same code as above under "draw new
				 windows [sy]", but without the initial widget creation [Ash] */
			
				/* This simply 'updates' the definition for each widget which has changed since being last updated.
				 * Therefore we can assume the widget to exist on the interface and simply 'update' [Richard] */
				case 0:
					/* TEXT [Ash] */
					t = getheap(strlen(name) + 1);
					strcpy(t, name);
					dpush(screenName, STRING, t);
					dpush(winNo, INTEGER, r + 1);
					sprintf(s, "b%d", WinInfo[r].ref);
					t = getheap(strlen(s) + 1);
					strcpy(t, s);
					dpush(boxName, STRING, t);
					makearr(3);
					appAgentName--;
					call(lookup("scout_show_canvas", basecontext), pop(), 0);
					appAgentName++;
					break;
				case 4:
					/* TEXTBOX [Ash] */
					t = getheap(strlen(name) + 1);
					strcpy(t, name);
					dpush(screenName, STRING, t);
					dpush(winNo, INTEGER, r + 1);
					sprintf(s, "b%d", WinInfo[r].ref);
					t = getheap(strlen(s) + 1);
					strcpy(t, s);
					dpush(boxName, STRING, t);
					makearr(3);
					appAgentName--;
					call(lookup("scout_show_text", basecontext), pop(), 0);
					appAgentName++;
					break;
				case 1:
					/* DONALD [Ash] */
				case 2:
					/* ARCA [Ash] */
				default:
					/* IMAGE is 3 [Ash] */
					t = getheap(strlen(name) + 1);
					strcpy(t, name);
					dpush(screenName, STRING, t);
					dpush(winNo, INTEGER, r + 1);
					sprintf(s, "b%d", WinInfo[r].ref);
					t = getheap(strlen(s) + 1);
					strcpy(t, s);
					dpush(boxName, STRING, t);
					makearr(3);
					appAgentName--;
					call(lookup("scout_show_2D", basecontext), pop(), 0);
					appAgentName++;
					break;
			}
		}
	}

	/* restack display [sy] */
	lastBox = lskip == 0 ? 0 :
	WinInfo[lskip - 1].ref * 100 + WinInfo[lskip - 1].nbox;

	for (r = lskip; r < noOfscreen - rskip; r++) {
		for (l = 1; l <= WinInfo[r].nbox; l++) {
			if (lastBox == 0) {
				
				// This will raise the specified object to be above all the others
				sprintf(s, "raise .%s.b%d_%d", name, WinInfo[r].ref, l);
				Tcl_EvalEC(interp, s);
#ifdef WEDEN_ENABLED		
				sendCompleteFormattedWedenMessage("<s><item type=\"raiseobject\"><object id=\".%s.b%d_%d\" /></item></s>", name, WinInfo[r].ref, l);		// [TO_MAKE_WebEdenDecision]
#endif
			} else {
				// This will place the first object below the second object
				sprintf(s, "lower .%s.b%d_%d .%s.b%d_%d", name, WinInfo[r].ref, l, name, lastBox / 100, lastBox % 100);
				Tcl_EvalEC(interp, s);
#ifdef WEDEN_ENABLED		
				sendCompleteFormattedWedenMessage("<s><item type=\"lowerobject\"><object id=\".%s.b%d_%d\" higherobjectid=\".%s.b%d_%d\"/></item></s>", name, WinInfo[r].ref, l, name, lastBox / 100, lastBox % 100);// [TO_MAKE_WebEdenDecision]
#endif
			}
			lastBox = WinInfo[r].ref * 100 + l;
		}
	}

	/* clean up and save information for next change of display [sy] */
	ssptr->obj.oWinInfo = WinInfo;
	freedatum(ssptr->obj.oScreen); /* 	Bug fix - Patrick forgot this, meaning that each time
	 									DisplayScreen is called (because the screen changed), 
	 									we leak some memory. [Ash] */
	ssptr->obj.oScreen = newdatum(screen);
	ssptr->obj.MaxRef = MaxRef;
	ssptr->obj.Refer = Refer;
	
	/*   UPDATE_ScoutScreen(name, ssptr); */
	pushUNDEF();
	
}




	/********************************************************************
	 Functions for manipulating assocative memory
	 ********************************************************************/

	typedef struct btree {
		struct btree *l;
		struct btree *r;
		int key;
		Datum vp;
		Datum ent;
		Datum attr;
	}Btree;

	Btree *WidgetMap = 0;

	static Btree * searchKey(
			Btree * map,
			int key
	)
	{
		if (map == 0) {
			map = (Btree *) emalloc(sizeof(Btree));
			map->l = map->r = 0;
			map->key = 0;
			return map;
		}
		if (map->key == key)
		return map;
		if (map->key > key) {
			if (map->l)
			return searchKey(map->l, key);
			else
			return map;
		}
		else if (map->r)
		return searchKey(map->r, key);
		else
		return map;
	}

	
	static void replaceKey(Btree * map, int key, Datum vp, Datum ent, Datum attr)
	{
		Btree *ans;
		Btree *new;

		ans = searchKey(map, key);
		if (ans->key != key) { /* not there */
			new = (Btree *) emalloc(sizeof(Btree));
			new->key = key;
			new->vp = newdatum(vp);
			new->ent = newdatum(ent);
			new->attr = newdatum(attr);
			if (key > ans->key) {
				new->l = 0;
				new->r = ans->r;
				ans->r = new;
			} else {
				new->r = 0;
				new->l = ans->l;
				ans->l = new;
			}
		} else { /* found */
			freedatum(ans->vp);
			freedatum(ans->ent);
			freedatum(ans->attr);
			ans->vp = newdatum(vp);
			ans->ent = newdatum(ent);
			ans->attr = newdatum(attr);
		}
	}

	
	static char lookAttrResult[80];

	static void lookAttr(char *s, char *attr)
	{
		char *head;
		int ls, la;

		ls = strlen(s);
		la = strlen(attr);
		if (ls <= la + 1) {
			lookAttrResult[0] = '\0';
			return;
		}
		head = strchr(s, ',');
		if (head == 0)
		head = s + ls;
		if (strncmp(s, attr, la) == 0 && s[la] == '=') {
			strncpy(lookAttrResult, s + la + 1, (head - s) - la - 1);
			lookAttrResult[(head - s) - la - 1] = '\0';
		} else if (*head != '\0') {
			lookAttr(head + 1, attr);
		} else {
			lookAttrResult[0] = '\0';
		}
	}

	
	// 																					<<<<<<<<<<<<<<<<<<<<-------------- PLOT SHAPE HERE
	
	void PlotShape(void)
	{
		Datum viewport_list, viewport_name, vp_list, segPtr, attrPtr;
		Datum d;
		char *attr;
		int segid, i, j, fullUpdate;
		Btree *ans;
		char s[256];
		// Was char *screen, *v, *var; [Richard]
		// Now
		char *screen, *var;
		const char *v;			// We should not go and edit this...just look at it

		if (paracount < 3 || para(1).type != LIST || !is_symbol(para(3)))
			usage("PlotShape(viewport_list, segid, &attr);");

		viewport_list = para(1);
		segPtr = para(2);
		attrPtr = para(3);
		segid = is_symbol(segPtr) ? segPtr.u.v.x : segPtr.u.i;
		muststr(symbol_of(attrPtr)->d, "PlotShape()");
		attr = symbol_of(attrPtr)->d.u.s;

		ans = searchKey(WidgetMap, segid);
		if (WidgetMap == 0) {
			WidgetMap = ans;
		}
		
		if (ans->key != segid || datacmp(ans->ent, *dptr(segPtr)) || datacmp(ans->attr, *dptr(attrPtr))) {
			fullUpdate = 1;
		} else {
			/* only the viewport list has changed at most */
			fullUpdate = 0;
		}

		vp_list = newhdat(viewport_list);
		
		if (!fullUpdate) {
			for (i = vp_list.u.a[0].u.i; i > 0; --i) {
				for (j = ans->vp.u.a[0].u.i; j > 0; --j) {
					if (datacmp(vp_list.u.a[i], ans->vp.u.a[j]) == 0) {
						freedatum(ans->vp.u.a[j]);
						ans->vp.u.a[j] = UndefDatum;
						/* don't need to free vp_list.u.a[i] because it is created by newhdat() */
						vp_list.u.a[i] = UndefDatum;
						break; /* expect at most one match */
					}
				}
			}
		}
		
		lookAttr(attr, "locus");
		if (strcmp(lookAttrResult, "true")) {
			if (ans->key == segid) {
				for (i = 1; i <= ans->vp.u.a[0].u.i; i++) {
					Datum d;
	
					d = ans->vp.u.a[i];
					if (isundef(d))
						continue;
	
					// TODO: Get remove shape to create 2 element datnum
	
					/*
					 * OLD CODE - Not using donald.eden xdelete proc. Now it does
					 * 
					 sprintf(s, "if [ winfo exists .%s.%s ] { .%s.%s delete t%d }",
					 d.u.a[1].u.s, d.u.a[2].u.s,
					 d.u.a[1].u.s, d.u.a[2].u.s, segid);
					 
					 Tcl_EvalEC(interp, s);
					 */
	
					// This is a bit of a hack to generate a view port name. Should really define new datnum with 2 elements as used for other drawing shapes			
					sprintf(s, ".%s.%s", d.u.a[1].u.s, d.u.a[2].u.s);
	
					d.type = STRING;
					d.u.s = s;
					push(d);
	
					d.type = INTEGER;
					d.u.i = segid;
					push(d);
	
					makearr(2);
					appAgentName--;
					call(lookup("remove_shape", basecontext), pop(), 0);
					appAgentName++;
	
					// END ADDED RICHARD
	
				}
			}
		}
		lookAttr(attr, "color");
		if (strcmp(lookAttrResult, "transparent")) {
			for (i = 1; i <= vp_list.u.a[0].u.i; i++) {
				viewport_name = vp_list.u.a[i];

				/* TODO: consider printing an error message below instead
				 of just silently continuing in the undefined case.  But this
				 is probably necessary in more than just this location?  [Ash] */
				if (isundef(viewport_name))
					continue;

				screen = viewport_name.u.a[1].u.s; /* eg screen [Ash] */
				var = viewport_name.u.a[2].u.s; /* eg b7_1 [Ash] */

				push(viewport_name); /* viewport_name in draw_shape [Ash] */
				push(segPtr); /* SegName in draw_shape [Ash] */
				push(*dptr(segPtr)); /* entity in draw_shape [Ash] */
				push(attrPtr); /* attr in draw_shape [Ash] */

				sprintf(s, "%s.%s_xOrigin", screen, var);
				if ((v = Tcl_GetVar(interp, s, TCL_GLOBAL_ONLY)) == NULL)
					error2("tcl variable not found: ", s);
				d.type = REAL;
				d.u.r = atof(v);
				push(d); /* xOrigin in draw_shape [Ash] */

				sprintf(s, "%s.%s_yOrigin", screen, var);
				if ((v = Tcl_GetVar(interp, s, TCL_GLOBAL_ONLY)) == NULL)
					error2("tcl variable not found: ", s);
				d.type = REAL;
				d.u.r = atof(v);
				push(d); /* yOrigin in draw_shape [Ash] */

				sprintf(s, "%s.%s_xScale", screen, var);
				if ((v = Tcl_GetVar(interp, s, TCL_GLOBAL_ONLY)) == NULL)
					error2("tcl variable not found: ", s);
				d.type = REAL;
				d.u.r = atof(v);
				push(d); /* xScale in draw_shape [Ash] */

				sprintf(s, "%s.%s_yScale", screen, var);
				if ((v = Tcl_GetVar(interp, s, TCL_GLOBAL_ONLY)) == NULL)
					error2("tcl variable not found: ", s);
				d.type = REAL;
				d.u.r = atof(v);
				push(d); /* yScale in draw_shape [Ash] */

				makearr(8);
				appAgentName--;
				/* NB draw_shape is defined in donald.eden [Ash] */
				call(lookup("draw_shape", basecontext), pop(), 0);
				appAgentName++;
			}
		}
		replaceKey(WidgetMap, segid, viewport_list, *dptr(segPtr), *dptr(attrPtr));
	}

#endif /* TTYEDEN? */

	
	
	
	
	
	
	
	
	
	
	
	
	
/*							---------------------------------------------------------------------------
 * 
 * 								           BELOW CODE FOR Sasami and DTkEden...
 * 
 *							-------------------------------------------------------------------------*/

	
	
	
	
	
	
	
	
#ifdef DISTRIB
	/*---------------------------------------------------------------------------*/
	/*  for agency --sun */

	extern int isServer;
	extern char *topMasterStack(void);

	/* First argument to sendServer is actually ignored -- this is probably
	 due to Patrick avoiding a compatibility problem... [Ash] */
	void
	sendServer(void) /* for distributed tkEden --sun */
	{ /* send scripts to server by using Tcl  */

		if (paracount != 2)
		usage("sendServer(\"servername\", \"scripts\")");
		muststr(para(1), "sendServer()");
		muststr(para(2), "sendServer()");
		SendServer(para(1).u.s, para(2).u.s);
	}

	/* First argument to SendServer is actually ignored -- this is probably
	 due to Patrick avoiding a compatibility problem... [Ash] */
	void
	SendServer(char *serverName, char * sentScripts)
	{
		extern void warning(char *, char *);
		Tcl_DString command;

		if (isServer) return;
		if ( *sentScripts != 0) {
			Tcl_DStringInit(&command);
			Tcl_DStringAppend(&command, "sendServer ", -1);
			Tcl_DStringAppendElement(&command, sentScripts);
			Tcl_EvalEC(interp, command.string);
			Tcl_DStringFree(&command);
		}
	}

	void
	sendClient(void)
	{ /* send scripts to server by using Tcl  */
		if (paracount != 2)
		usage("sendClient(\"clientname\", \"scripts\")");
		muststr(para(1), "sendClient()");
		muststr(para(2), "sendClient()");
		SendClient(para(1).u.s, para(2).u.s);
	}

	void
	SendClient(char *clientName, char *sentScripts)
	{
		extern void warning(char *, char *);
		Tcl_DString command;
		char *s;

		if (!isServer) {
			s = (char *) malloc(strlen(clientName) + 1 + strlen(sentScripts) + 1 + 20);
			strcpy(s, "sendClient(\"");
			strcat(s, clientName);
			strcat(s, "\", \"");
			strcat(s, sentScripts);
			strcat(s, "\");");
			SendServer("", s);
			free(s);
			return;
		}
		if ( *sentScripts != 0 && *clientName !=0) {
			Tcl_DStringInit(&command);
			Tcl_DStringAppend(&command, "sendClient ", -1);
			Tcl_DStringAppendElement(&command, clientName);
			Tcl_DStringAppendElement(&command, sentScripts);
			Tcl_EvalEC(interp, command.string);
			Tcl_DStringFree(&command);
		}
	}

	void
	SendOtherClients(char *clientName, char *sentScripts)
	{
		extern void warning(char *, char *);
		Tcl_DString command;

		if ( *sentScripts != 0) {
			Tcl_DStringInit(&command);
			Tcl_DStringAppend(&command, "sendOtherClients ", -1);
			Tcl_DStringAppendElement(&command, clientName);
			Tcl_DStringAppendElement(&command, sentScripts);
			Tcl_EvalEC(interp, command.string);
			Tcl_DStringFree(&command);
		}
	}

	void
	renewObs(void)
	{
		extern char *textptr, textcode[];
		extern char agentName[128];
		extern symptr lookup(char *);
		extern int oracle_check(symptr);
		symptr sp;
		char *name, *s, *t;
		int errorSW;

		if (paracount != 1)
		usage("renewObs(\"Observablename\")");
		muststr(para(1), "renewObs()");
		name = para(1).u.s;
		if (!isServer) {
			/* Bloody awful use of magic numbers here by Patrick, I'm
			 afraid.  This caused a nasty bug: need to do a better way of
			 alloc-ing space for strings.  [Ash] */
			s = (char *) malloc(strlen(name) + 20);
			strcpy(s, "renewObs(\"");
			strcat(s, name);
			strcat(s, "\");");
			SendServer("", s);
			free(s);
			return;
		}

		/* isServer must now be true (note the return above) [Ash] */
		if ((sp = lookup(name)) == 0) {
			s = (char *) malloc(strlen(name) + 22 + 1);
			sprintf(s, "no such an observable %s", name);
			errorSW=1;
		} else {
			if (!oracle_check(sp)) {
				s = (char *) malloc(strlen(name) + 34 + 1);
				sprintf(s, "no ORACLE privilege on observable %s", name);
				errorSW=1;
			} else {
				s = textptr = textcode; /* set textptr at the begin of
				 textcode */
				*textptr = '\0';
				tkdefine1(sp); /* will put text into textcode and change textptr */
				errorSW = 0;
			}
		}
		if (*s == '\0') {error("unknown error in Query()");}
		if (*agentName == '\0') { /* for server */
			if (errorSW) {error(s); free(s);}
			else printf("%s", s);
		} else {
			if (errorSW) {
				t = (char *) malloc(strlen(s) + 50);
				sprintf(t, "tcl(\"show hist 1\");\n/*** renewObs ERROR \n%s\n***/\n", s);
				SendClient(agentName, t);
				free(t);
				free(s);
			} else {
				SendClient(agentName, s);
				/* This should not be here (duh!).  s is set above, and is
				 actually pointing to somewhere in textcode at this point,
				 which was declared in code.c as a statically allocated
				 array.  So we don't need to free it.  [Ash, Meurig and
				 Chris :] */
				/* free(s); */
			}
		}
	}

	void
	queryObs(void)
	{
		extern char *textptr, textcode[];
		extern char agentName[128];
		extern symptr lookup(char *);
		extern int oracle_check(symptr);
		symptr sp;
		char *name, *s, *t, *s1;
		int errorSW;

		if (paracount != 1)
		usage("queryObs(\"Observablename\")");
		muststr(para(1), "queryObs()");
		name = para(1).u.s;
		if (!isServer) {
			s = (char *) malloc(strlen(name) + 20);
			strcpy(s, "queryObs(\"");
			strcat(s, name);
			strcat(s, "\");");
			SendServer("", s);
			free(s);
			return;
		}
		if ((sp = lookup(name)) == 0) {
			s = (char *) malloc(strlen(name) + 22 + 1);
			sprintf(s, "no such an observable %s", name);
			errorSW=1;
		} else {
			if (!oracle_check(sp)) {
				s = (char *) malloc(strlen(name) + 34 + 1);
				sprintf(s, "no ORACLE privilege on observable %s", name);
				errorSW=1;
			} else {
				s = textptr = textcode; /* set textptr at the begin of textcode */
				*textptr = '\0';
				tkdefine1(sp); /* will put text into textcode and change textptr */
				/* get current value  */
				switch (sp->d.type) {
					case VAR:
					case BLTIN:
					case LIB:
					case LIB64:
					case RLIB:
					case FUNCTION:
					case PROCMACRO:
					case PROCEDURE:
					s1 = " ";
					break;
					case FORMULA:
					s1 = (char *) malloc(strlen(sp->name) + 30);
					sprintf(s1, "current state: %s = @\n", sp->name);
					break;
					default:
					tkdefineDatum(sp->d, defn);
					s1 = (char *) malloc(strlen(defn) + strlen(sp->name) + 30);
					sprintf(s1, "current state: %s = %s\n", sp->name, defn);
					break;
				}
				errorSW = 0;
			}
		}
		if (*s == '\0') {error("unknown error in Query()");}
		if (*agentName == '\0') { /* for server */
			if (errorSW) {error(s); free(s);}
			else {printf("%s%s", s, s1); free(s1);}
		} else {
			if (errorSW) {
				t = (char *) malloc(strlen(s) + 50);
				sprintf(t, "tcl(\"show hist 1\");\n/*** queryObs\n%s\n***/\n", s);
				SendClient(agentName, t);
				free(t);
			} else {
				t = (char *) malloc(strlen(s) + strlen(s1) + 50);
				/* sprintf(t, "writeln(\"%s\", \"%s\");\n", s, s1); */
				sprintf(t, "tcl(\"show hist 1\");\n/*** queryObs\n%s%s\n***/\n", s, s1);
				SendClient(agentName, t);
				free(t);
				free(s1);
			}
		}
	}

	void
	propagate(void)
	{

		if (paracount != 2)
		usage("propagate(\"Observablename\", \"scripts\")");
		muststr(para(1), "propagate()");
		muststr(para(2), "propagate()");
		if (isServer)
		propagateAgency1(para(1).u.s, para(2).u.s);
		else return;
		/* {
		 if (streq(topMasterStack(), "input"))
		 return;
		 else
		 error("no propagation in a client end. Please use sendServer command");
		 } */
	}

	void
	propagate_agency(symptr sp)
	{
		extern char *textptr, textcode[];
		char *s;

		/* printf("propagate %s", sp->name); */
		if (!isServer) return;
		if (streq(sp->master, "system")) return;
		s = textptr = textcode; /* set textptr at the begin of textcode */
		*textptr = '\0';
		tkdefine1(sp); /* will put text into textcode and change textptr */
		/* printf("tkdefine %s", s); */
		if (textptr) propagateAgency(sp, s);
	}

	void
	propagateAgency(symptr sp, char *s)
	{
		extern char agentName[128];
		extern char *everyone;
		extern agent_ATOM search_agent_Q(agent_QUEUE *, char *);
		agent_QUEUE *AQ = &sp->OracleOf;
		agent_ATOM A;
		extern Int *propagateType;
		char *temp = (char *) malloc(strlen(s)+1);

		if (!isServer) return;
		strcpy(temp, s); /* because textcode may be changed during propagation */
		if (temp) {
			if (*propagateType == 1) {
				if (Q_EMPTY(AQ)) {
					return; /* exclusive use */
					/* if (*autoPropagate) SendOtherClients(agentName, s); */
				} else {
					if (search_agent_Q(AQ, everyone)) {SendOtherClients(agentName, temp);}
					else {
						FOREACH(A, AQ) {
							if (!streq(A->obj.name, agentName) && !streq(A->obj.name, "SYSTEM"))
							{	SendClient(A->obj.name, temp);}
						}
					}
				}
			}
			if (*propagateType == 0) SendOtherClients(agentName, temp);
			if (*propagateType == -1) return;
		}
		free(temp);
	}

	void
	propagateAgency1(char *name, char *s)
	{
		extern char agentName[128];
		extern symptr lookup(char *);
		extern Int *EveryOneAllowed;
		extern Int *propagateType;
		symptr sp;

		if (!isServer) return;
		if ((sp = lookup(name)) != 0) propagateAgency(sp, s);
		else if (*EveryOneAllowed == 1 && *propagateType != -1)
		SendOtherClients(agentName, s);
		/* new variable --- In Donald openshspe, int, boolean, real and char are not defined in Eden yet , even they are declared in Donald. So at this moment, agency is dependent on EveryOneAllowed.. */
	}

	void
	addAgency(void)
	{
		Datum d;
		symptr sp;
		char *name, *type, *ObsName;
		int LSDtype;
		extern void add_Obs_Q(char *, int, char *);
		extern int isServer;

		if (!isServer) return; /*  If it is running in client end, it does nothing so far */
		if (paracount != 3)
		usage("addAgency(\"LSDagentName\", \"LSDtype\", \"observableName\")");
		muststr(para(1), "addAgency()");
		muststr(para(2), "addAgency()");
		muststr(para(3), "addAgency()");
		name = para(1).u.s;
		type = para(2).u.s;
		ObsName = para(3).u.s;
		if ((sp=lookup(ObsName)) == 0)
		sp = install(ObsName, VAR, UNDEF, 0);
		if (!strcmp(type, "oracle")) {
			add_agent_Q(&sp->OracleOf, name); LSDtype =1;}
		else {
			if (!strcmp(type, "handle")) {
				add_agent_Q(&sp->HandleOf, name); LSDtype = 2;}
			else {
				if (!strcmp(type, "state")) {
					add_agent_Q(&sp->StateOf, name); LSDtype = 3;}
				else dpush(d, INTEGER, 0);
			}
		}
		add_Obs_Q(name, LSDtype, ObsName);
		dpush(d, INTEGER, 1);
	}

	void
	removeAgency(void)
	{
		Datum d;
		symptr sp;
		char *name, *type, *ObsName;
		int LSDtype;
		extern void remove_Obs_Q(char *, int, char *);
		extern void delete_agent_Q(agent_QUEUE *, char *, char *);
		extern int isServer;

		if (!isServer) return; /*  If it is running in client end, it does nothing so far */
		if (paracount != 3)
		usage("removeAgency(\"LSDagentName\", \"LSDtype\", \"observableName\")");
		muststr(para(1), "removeAgency()");
		muststr(para(2), "removeAgency()");
		muststr(para(3), "removeAgency()");
		name = para(1).u.s;
		type = para(2).u.s;
		ObsName = para(3).u.s;

		if ((sp=lookup(ObsName)) == 0) error2("no such observable ", ObsName);
		if (!strcmp(type, "oracle")) {
			delete_agent_Q(&sp->OracleOf, name, ObsName); LSDtype = 1;}
		else {
			if (!strcmp(type, "handle")) {
				delete_agent_Q(&sp->HandleOf, name, ObsName); LSDtype =2;}
			else {
				if (!strcmp(type, "state")) {
					delete_agent_Q(&sp->StateOf, name, ObsName); LSDtype =3;}
				else dpush(d, INTEGER, 0);
			}
		}
		remove_Obs_Q(name, LSDtype, ObsName);
		dpush(d, INTEGER, 1);
	}

	void
	checkAgency(void)
	{
		Datum d;
		symptr sp;
		char *name, *type, *ObsName;
		agent_QUEUE *AQ;
		agent_ATOM A;
		extern agent_ATOM search_agent_Q(agent_QUEUE *, char *);
		extern int isServer;

		if (!isServer) return; /*  If it is running in client end, it does nothing so far */
		if (paracount != 3)
		usage("checkAgency(\"LSDagentName\", \"LSDtype\", \"observableName\")");
		muststr(para(1), "checkAgency()");
		muststr(para(2), "checkAgency()");
		muststr(para(3), "checkAgency()");
		name = para(1).u.s;
		type = para(2).u.s;
		ObsName = para(3).u.s;
		if ((sp=lookup(ObsName)) == 0) {
			dpush(d, INTEGER, 0); /* no such observable */
			return;
		}
		if (!strcmp(type, "oracle")) {
			AQ = &sp->OracleOf;}
		else {
			if (!strcmp(type, "handle")) {
				AQ = &sp->HandleOf;}
			else {
				if (!strcmp(type, "state")) {
					AQ = &sp->StateOf;}
				else {dpush(d, INTEGER, 0); /* no such LSDtype */
					return;}
			}
		}
		if ((A=search_agent_Q(AQ, name))!=0) {
			dpush(d, INTEGER, 1);
		} else {
			dpush(d, INTEGER, 0);
		}
		return;
	}
#endif /* DISTRIB */

	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
#if defined(WANT_SASAMI) && !defined(TTYEDEN)
	// --------------------------------------------------------------------------
	// Sasami functions - these are just stubs which call the real function code
	// over in Sasami/functions.c
	// See Sasami/functions.c for descriptions
	// --------------------------------------------------------------------------

	void ed_sasami_vertex(void)
	{
		int i;
		double x,y,z;
		if (paracount < 4)
		usage("sasami_vertex(n,x,y,z)");

		if (para(1).type!=INTEGER)
		usage("n must be an integer");
		if (para(2).type==REAL)
		{
			x=para(2).u.r;
		}
		else
		{
			x=para(2).u.i;
		}
		if (para(3).type==REAL)
		{
			y=para(3).u.r;
		}
		else
		{
			y=para(3).u.i;
		}
		if (para(4).type==REAL)
		{
			z=para(4).u.r;
		}
		else
		{
			z=para(4).u.i;
		}

		sasami_vertex(para(1).u.i,x,y,z);
	}

	void ed_sasami_set_bgcolour(void)
	{
		int i;
		double r,g,b;
		if (paracount < 3)
		usage("sasami_set_bgcolour(r,g,b)");

		if (para(1).type==REAL)
		{
			r=para(1).u.r;
		}
		else
		{
			r=para(1).u.i;
		}
		if (para(2).type==REAL)
		{
			g=para(2).u.r;
		}
		else
		{
			g=para(2).u.i;
		}
		if (para(3).type==REAL)
		{
			b=para(3).u.r;
		}
		else
		{
			b=para(3).u.i;
		}

		sasami_set_bgcolour(r,g,b);
	}

	void ed_sasami_poly_geom_vertex(void)
	{
		int i;
		double r,g,b;
		if (paracount < 3)
		usage("sasami_poly_geom_vertex(<poly>,<vertex>,<id>)");

		if (para(1).type!=INTEGER)
		usage("<poly> must be an integer");
		if (para(2).type!=INTEGER)
		usage("<vertex> must be an integer");
		if (para(3).type!=INTEGER)
		usage("<id> must be an integer");

		sasami_poly_geom_vertex(para(1).u.i,para(2).u.i,para(3).u.i);
	}

	void ed_sasami_poly_tex_vertex(void)
	{
		int i;
		double r,g,b;
		if (paracount < 3)
		usage("sasami_poly_tex_vertex(<poly>,<vertex>,<id>)");

		if (para(1).type!=INTEGER)
		usage("<poly> must be an integer");
		if (para(2).type!=INTEGER)
		usage("<vertex> must be an integer");
		if (para(3).type!=INTEGER)
		usage("<id> must be an integer");

		sasami_poly_tex_vertex(para(1).u.i,para(2).u.i,para(3).u.i);
	}

	void ed_sasami_object_poly(void)
	{
		int i;
		double r,g,b;
		if (paracount < 3)
		usage("sasami_object_poly(<object>,<poly>,<id>)");

		if (para(1).type!=STRING)
		usage("<object> must be a string");
		if (para(2).type!=INTEGER)
		usage("<poly> must be an integer");
		if (para(3).type!=INTEGER)
		usage("<id> must be an integer");

		sasami_object_poly(para(1).u.s,para(2).u.i,para(3).u.i);
	}

	void ed_sasami_poly_colour(void)
	{
		int i;
		double r,g,b,a;

		if (paracount < 5)
		usage("sasami_poly_colour(poly,r,g,b,a)");

		if (para(1).type!=INTEGER)
		usage("<poly> must be an integer");

		if (para(2).type==REAL)
		{
			r=para(2).u.r;
		}
		else
		{
			r=para(2).u.i;
		}
		if (para(3).type==REAL)
		{
			g=para(3).u.r;
		}
		else
		{
			g=para(3).u.i;
		}
		if (para(4).type==REAL)
		{
			b=para(4).u.r;
		}
		else
		{
			b=para(4).u.i;
		}
		if (para(5).type==REAL)
		{
			a=para(5).u.r;
		}
		else
		{
			a=para(5).u.i;
		}

		sasami_poly_colour(para(1).u.i,r,g,b,a);
	}

	void ed_sasami_poly_material(void)
	{
		int i;

		if (paracount < 2)
		usage("sasami_poly_material(<poly>,<material>)");

		if (para(1).type!=INTEGER)
		usage("<poly> must be an integer");
		if (para(2).type!=INTEGER)
		usage("<material> must be an integer");

		sasami_poly_material(para(1).u.i,para(2).u.i);
	}

	void ed_sasami_object_pos(void)
	{
		int i;
		double x,y,z;

		if (paracount < 4)
		usage("sasami_object_pos(object,x,y,z)");

		if (para(1).type!=STRING)
		usage("<object> must be a string");

		if (para(2).type==REAL)
		{
			x=para(2).u.r;
		}
		else
		{
			x=para(2).u.i;
		}
		if (para(3).type==REAL)
		{
			y=para(3).u.r;
		}
		else
		{
			y=para(3).u.i;
		}
		if (para(4).type==REAL)
		{
			z=para(4).u.r;
		}
		else
		{
			z=para(4).u.i;
		}

		sasami_object_pos(para(1).u.s,x,y,z);
	}

	void ed_sasami_object_rot(void)
	{
		int i;
		double x,y,z;

		if (paracount < 4)
		usage("sasami_object_rot(object,x,y,z)");

		if (para(1).type!=STRING)
		usage("<object> must be a string");

		if (para(2).type==REAL)
		{
			x=para(2).u.r;
		}
		else
		{
			x=para(2).u.i;
		}
		if (para(3).type==REAL)
		{
			y=para(3).u.r;
		}
		else
		{
			y=para(3).u.i;
		}
		if (para(4).type==REAL)
		{
			z=para(4).u.r;
		}
		else
		{
			z=para(4).u.i;
		}

		sasami_object_rot(para(1).u.s,x,y,z);
	}

	void ed_sasami_object_scale(void)
	{
		int i;
		double x,y,z;

		if (paracount < 4)
		usage("sasami_object_scale(object,x,y,z)");

		if (para(1).type!=STRING)
		usage("<object> must be a string");

		if (para(2).type==REAL)
		{
			x=para(2).u.r;
		}
		else
		{
			x=para(2).u.i;
		}
		if (para(3).type==REAL)
		{
			y=para(3).u.r;
		}
		else
		{
			y=para(3).u.i;
		}
		if (para(4).type==REAL)
		{
			z=para(4).u.r;
		}
		else
		{
			z=para(4).u.i;
		}

		sasami_object_scale(para(1).u.s,x,y,z);
	}

	void ed_sasami_viewport(void)
	{
		int x,y;

		if (paracount < 2)
		usage("sasami_viewport(x,y)");

		if (para(1).type!=INTEGER)
		usage("<x> must be an integer");

		if (para(2).type!=INTEGER)
		usage("<y> must be an integer");

		sasami_viewport(para(1).u.i,para(2).u.i);
	}

	void ed_sasami_setshowaxes(void)
	{
		int n;

		if (paracount < 1)
		usage("sasami_setshowaxes(n)");

		if (para(1).type!=INTEGER)
		usage("<n> must be an integer");

		sasami_setshowaxes(para(1).u.i);
	}

	void ed_sasami_material_ambient(void)
	{
		int i;
		double r,g,b,a;

		if (paracount < 5)
		usage("sasami_material_ambient(material,r,g,b,a)");

		if (para(1).type!=INTEGER)
		usage("<material> must be an integer");

		if (para(2).type==REAL)
		{
			r=para(2).u.r;
		}
		else
		{
			r=para(2).u.i;
		}
		if (para(3).type==REAL)
		{
			g=para(3).u.r;
		}
		else
		{
			g=para(3).u.i;
		}
		if (para(4).type==REAL)
		{
			b=para(4).u.r;
		}
		else
		{
			b=para(4).u.i;
		}
		if (para(5).type==REAL)
		{
			a=para(5).u.r;
		}
		else
		{
			a=para(5).u.i;
		}

		sasami_material_ambient(para(1).u.i,r,g,b,a);
	}

	void ed_sasami_material_diffuse(void)
	{
		int i;
		double r,g,b,a;

		if (paracount < 5)
		usage("sasami_material_diffuse(material,r,g,b,a)");

		if (para(1).type!=INTEGER)
		usage("<material> must be an integer");

		if (para(2).type==REAL)
		{
			r=para(2).u.r;
		}
		else
		{
			r=para(2).u.i;
		}
		if (para(3).type==REAL)
		{
			g=para(3).u.r;
		}
		else
		{
			g=para(3).u.i;
		}
		if (para(4).type==REAL)
		{
			b=para(4).u.r;
		}
		else
		{
			b=para(4).u.i;
		}
		if (para(5).type==REAL)
		{
			a=para(5).u.r;
		}
		else
		{
			a=para(5).u.i;
		}

		sasami_material_diffuse(para(1).u.i,r,g,b,a);
	}

	void ed_sasami_material_specular(void)
	{
		int i;
		double r,g,b,a;

		if (paracount < 5)
		usage("sasami_material_specular(material,r,g,b,a)");

		if (para(1).type!=INTEGER)
		usage("<material> must be an integer");

		if (para(2).type==REAL)
		{
			r=para(2).u.r;
		}
		else
		{
			r=para(2).u.i;
		}
		if (para(3).type==REAL)
		{
			g=para(3).u.r;
		}
		else
		{
			g=para(3).u.i;
		}
		if (para(4).type==REAL)
		{
			b=para(4).u.r;
		}
		else
		{
			b=para(4).u.i;
		}
		if (para(5).type==REAL)
		{
			a=para(5).u.r;
		}
		else
		{
			a=para(5).u.i;
		}

		sasami_material_specular(para(1).u.i,r,g,b,a);
	}

	void ed_sasami_material_texture(void)
	{
		int i;
		double r,g,b,a;

		if (paracount < 2)
		usage("sasami_material_texture(material,texture)");

		if (para(1).type!=INTEGER)
		usage("<material> must be an integer");

		if (para(2).type!=STRING)
		usage("<texture> must be a string");

		sasami_material_texture(para(1).u.i,para(2).u.s);
	}

	void ed_sasami_light_pos(void)
	{
		int i;
		double x,y,z;

		if (paracount < 4)
		usage("sasami_light_pos(light,x,y,z)");

		if (para(1).type!=INTEGER)
		usage("<light> must be an integer");

		if (para(2).type==REAL)
		{
			x=para(2).u.r;
		}
		else
		{
			x=para(2).u.i;
		}
		if (para(3).type==REAL)
		{
			y=para(3).u.r;
		}
		else
		{
			y=para(3).u.i;
		}
		if (para(4).type==REAL)
		{
			z=para(4).u.r;
		}
		else
		{
			z=para(4).u.i;
		}

		sasami_light_pos(para(1).u.i,x,y,z);
	}

	void ed_sasami_light_ambient(void)
	{
		int i;
		double r,g,b,a;

		if (paracount < 5)
		usage("sasami_light_ambient(light,r,g,b,a)");

		if (para(1).type!=INTEGER)
		usage("<light> must be an integer");

		if (para(2).type==REAL)
		{
			r=para(2).u.r;
		}
		else
		{
			r=para(2).u.i;
		}
		if (para(3).type==REAL)
		{
			g=para(3).u.r;
		}
		else
		{
			g=para(3).u.i;
		}
		if (para(4).type==REAL)
		{
			b=para(4).u.r;
		}
		else
		{
			b=para(4).u.i;
		}
		if (para(5).type==REAL)
		{
			a=para(5).u.r;
		}
		else
		{
			a=para(5).u.i;
		}

		sasami_light_ambient(para(1).u.i,r,g,b,a);
	}

	void ed_sasami_light_diffuse(void)
	{
		int i;
		double r,g,b,a;

		if (paracount < 5)
		usage("sasami_light_diffuse(light,r,g,b,a)");

		if (para(1).type!=INTEGER)
		usage("<light> must be an integer");

		if (para(2).type==REAL)
		{
			r=para(2).u.r;
		}
		else
		{
			r=para(2).u.i;
		}
		if (para(3).type==REAL)
		{
			g=para(3).u.r;
		}
		else
		{
			g=para(3).u.i;
		}
		if (para(4).type==REAL)
		{
			b=para(4).u.r;
		}
		else
		{
			b=para(4).u.i;
		}
		if (para(5).type==REAL)
		{
			a=para(5).u.r;
		}
		else
		{
			a=para(5).u.i;
		}

		sasami_light_diffuse(para(1).u.i,r,g,b,a);
	}

	void ed_sasami_light_specular(void)
	{
		int i;
		double r,g,b,a;

		if (paracount < 5)
		usage("sasami_light_specular(light,r,g,b,a)");

		if (para(1).type!=INTEGER)
		usage("<light> must be an integer");

		if (para(2).type==REAL)
		{
			r=para(2).u.r;
		}
		else
		{
			r=para(2).u.i;
		}
		if (para(3).type==REAL)
		{
			g=para(3).u.r;
		}
		else
		{
			g=para(3).u.i;
		}
		if (para(4).type==REAL)
		{
			b=para(4).u.r;
		}
		else
		{
			b=para(4).u.i;
		}
		if (para(5).type==REAL)
		{
			a=para(5).u.r;
		}
		else
		{
			a=para(5).u.i;
		}

		sasami_light_specular(para(1).u.i,r,g,b,a);
	}

	void ed_sasami_light_enabled(void)
	{
		if (paracount < 2)
		usage("sasami_light_specular(light,e)");

		if (para(1).type!=INTEGER)
		usage("<light> must be an integer");

		if (para(2).type!=INTEGER)
		usage("<e> must be an integer");

		sasami_light_enabled(para(1).u.i,para(2).u.i);
	}

	void ed_sasami_light_directional(void)
	{
		if (paracount < 2)
		usage("sasami_light_specular(light,d)");

		if (para(1).type!=INTEGER)
		usage("<light> must be an integer");

		if (para(2).type!=INTEGER)
		usage("<d> must be an integer");

		sasami_light_directional(para(1).u.i,para(2).u.i);
	}

	void ed_sasami_light_attenuation(void)
	{
		int i;
		double a;

		if (paracount < 2)
		usage("sasami_light_attenuation(light,attenuation)");

		if (para(1).type!=INTEGER)
		usage("<light> must be an integer");

		if (para(2).type==REAL)
		{
			a=para(2).u.r;
		}
		else
		{
			a=para(2).u.i;
		}

		sasami_light_attenuation(para(1).u.i,a);
	}

	void ed_sasami_object_visible(void)
	{
		if (paracount < 2)
		usage("sasami_object_visible(object,e)");

		if (para(1).type!=STRING)
		usage("<object> must be a string");

		if (para(2).type!=INTEGER)
		usage("<e> must be an integer");

		sasami_object_visible(para(1).u.s,para(2).u.i);
	}

	void ed_sasami_cube(void)
	{
		double w,h,d;

		if (paracount < 4)
		usage("sasami_cube(cube,width,height,depth)");

		if (para(1).type!=STRING)
		usage("<cube> must be a string");
		if (para(2).type == REAL)
		w = para(2).u.r;
		else
		w = para(2).u.i;
		if (para(3).type == REAL)
		h = para(3).u.r;
		else
		h = para(3).u.i;
		if (para(4).type == REAL)
		d = para(4).u.r;
		else
		d = para(4).u.i;

		sasami_cube(para(1).u.s,w,h,d);
	}

	void ed_sasami_face_material(void)
	{
		if (paracount < 3)
		usage("sasami_face_material(object,face,material)");

		if (para(1).type!=STRING)
		usage("<object> must be a string");
		if (para(2).type!=INTEGER)
		usage("<face> must be an integer");
		if (para(3).type!=INTEGER)
		usage("<material> must be an integer");

		sasami_face_material(para(1).u.s,para(2).u.i,para(3).u.i);
	}

	void ed_sasami_object_delete(void)
	{
		int i;
		if (paracount < 1)
		usage("sasami_object_delete(object)");

		if (para(1).type!=STRING)
		usage("<object> must be a string");

		sasami_object_delete(para(1).u.s);
	}

	void ed_sasami_cylinder(void)
	{
		double l,r1,r2;

		if (paracount < 5)
		usage("sasami_cylinder(cube,length,radius1,radius2,segments)");

		if (para(1).type!=STRING)
		usage("<cube> must be a string");
		if (para(2).type == REAL)
		l = para(2).u.r;
		else
		l = para(2).u.i;
		if (para(3).type == REAL)
		r1 = para(3).u.r;
		else
		r1 = para(3).u.i;
		if (para(4).type == REAL)
		r2 = para(4).u.r;
		else
		r2 = para(4).u.i;
		if (para(5).type!=INTEGER)
		usage("<segments> must be an integer");
		if (para(5).u.i < 2)
		usage("<segments> must be > 1");

		sasami_cylinder(para(1).u.s,l,r1,r2,para(5).u.i);
	}

	void ed_sasami_primitive_material(void)
	{
		int i;
		if (paracount < 2)
		usage("sasami_primitive_material(primitive,material)");

		if (para(1).type!=STRING)
		usage("<primitive> must be a string");
		if (para(2).type!=INTEGER)
		usage("<material> must be an integer");

		sasami_primitive_material(para(1).u.s,para(2).u.i);
	}

	void ed_sasami_sphere(void)
	{
		double r;

		if (paracount < 3)
			usage("sasami_sphere(sphere,radius,segments)");

		if (para(1).type!=STRING)
			usage("<sphere> must be a string");
		if (para(2).type == REAL)
			r = para(2).u.r;
		else
			r = para(2).u.i;
		if (para(3).type!=INTEGER)
			usage("<segments> must be an integer");
		if (para(3).u.i < 2)
			usage("<segments> must be > 1");

		sasami_sphere(para(1).u.s,r,para(3).u.i);
	}

#endif /* defined(WANT_SASAMI) && !defined(TTYEDEN) */
