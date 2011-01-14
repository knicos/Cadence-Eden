/*
 * $Id: custom.c,v 1.8 2001/07/27 16:44:15 cssbz Exp $
 */

static char rcsid[] = "$Id: custom.c,v 1.8 2001/07/27 16:44:15 cssbz Exp $";

/***********************************
 *	SYSTEM INITIALIZATION      *
 ***********************************/

#include <stdio.h>

#include "../../../../../config.h"
#include "../Eden/eden.h"
#include "../Eden/yacc.h"
#include "custom.h"

#define INCLUDE 'H'
#include "../Eden/builtin.h"
#include "customlib.h"
#undef  INCLUDE

struct ILIBTBL ilibtbl[] = {
#define INCLUDE 'T'
#include "customlib.h"
#undef  INCLUDE
	{0, 0}
};

struct RLIBTBL rlibtbl[] = {
#define INCLUDE 'R'
#include "customlib.h"
#undef  INCLUDE
	{0, 0}
};

extern int basecontext;

void
install_custom_variables(void)
{
	/* PRE-DEFINED VARIABLES */
#define INSTALL_VAR(name, value) \
	(install(name,basecontext, VAR, INTEGER, value))->changed = FALSE

#if (defined(HAVE_CURSES) || defined(HAVE_NCURSES)) && defined(TTYEDEN)
	INSTALL_VAR("stdscr", (Int) stdscr);
	INSTALL_VAR("curscr", (Int) curscr);
#endif

	/** these are standard input/output file pointers **/
	/** don't change them **/
	INSTALL_VAR("stdin",  (Int) stdin);
	INSTALL_VAR("stdout", (Int) stdout);
	INSTALL_VAR("stderr", (Int) stderr);
}
