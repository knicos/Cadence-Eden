/*
 * $Id: version.h,v 1.26 2002/07/10 19:17:49 cssbz Exp $
 */

/* This is the version number of the entire {tty|tk|dtk}eden package,
   which must be incremented whenever a release is given out for use.
   This number is centrally defined here, so it only needs to be
   changed once here.  The version number can contain letters as well
   as numbers, but no spaces.  [Ash] */
#define TKEDEN_VERSION "1.0"

#define TKEDEN_VERSION_MAX_LEN 10

#include "svnversion.h"

#if defined DISTRIB
#define TKEDEN_VARIANT "dtkeden"
#elif defined TTYEDEN
#define TKEDEN_VARIANT "ttyeden"
#elif defined TKEDEN
#define TKEDEN_VARIANT "Cadence-e"
#else
#error Variant not defined
#endif

#define TKEDEN_VARIANT_MAX_LEN 10

#define TKEDEN_WEB_SITE "http://go.warwick.ac.uk/EDEN/"
#define TKEDEN_WEB_SITE_MAX_LEN 60

/* [Ben] Set the version number of the Win32 port */

/* [Ben] Check for Win32 */
#ifndef __WIN32__
#   if defined(_WIN32) || defined(WIN32)
#	define __WIN32__
#   endif
#endif

#define TKEDEN_WIN32_VERSION_MAX_LEN 10
#ifdef __WIN32__
#define TKEDEN_WIN32_VERSION "0.6"
#else
#define TKEDEN_WIN32_VERSION "0.0" /* 0.0 indicates that this is not a
                                      WIN32 version */
#endif
