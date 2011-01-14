/*
 * $Id: customlib.h,v 1.14 2002/07/10 19:26:57 cssbz Exp $
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

#ifndef __WIN32__
#   if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__)
#	define __WIN32__
#   endif
#endif

/* ------------------------ library.h --------------------------- */

   /************************************************************\
  ****************************************************************
 **                                                              **
**	The following macros are intended to reduce the           **
**	complexity of binding EDEN functions with C functions.    **
**	Programmers are adviced to use these macros in their      **
**	header files. The macros are:   			  **
**								  **
**	Function(Ename,Cname) -- bind Eden name to Cname func.    **
**	SameFunc(Name) -- Eden's name is the same as C's name.    **
**	SpecialF(Ename,Type,Cname)                                **
**		-- if the type of C func isn't int,               **
**		   Type specifies the type.                       **
 **                                                              **
  ****************************************************************
   \************************************************************/

/* This file gets included from custom.c three times, with INCLUDE
   defined to something else each time.  In order:

   INCLUDE == 'H' (Header?) - included as global text,
              to make initial declarations
   INCLUDE == 'T' (Table?) - included into ilibtcl
              (an array of ILIBTBL structs)
   INCLUDE == 'R' (Real?) - included into rlibtcl
              (an array of RLIBTBL structs)

   [Ash] */

#if INCLUDE == 'H'
#define SpecialF(Ename,Type,Cname)extern Type Cname();
#define Function(Ename,Cname)extern Cname();
#define SameFunc(Name)extern Name();
#define RealFunc(Ename,Cname)extern double Cname();
#define SameReal(Name)extern double Name();
#endif

#if INCLUDE == 'T'
#ifdef __STDC__
/* Note the # is a preprocessor quoting operator: standard compilers
   now don't expand macro strings inside "" [Ash] */
#define SpecialF(Ename,Type,Cname){#Ename,(int(*)())Cname},
#define Function(Ename,Cname){#Ename,Cname},
#define SameFunc(Name){#Name,Name},
#else /* not __STDC__... */
#define SpecialF(Ename,Type,Cname){"Ename",(int(*)())Cname},
#define Function(Ename,Cname){"Ename",Cname},
#define SameFunc(Name){"Name",Name},
#endif
#endif

#if INCLUDE == 'T'
/* We should ignore real-valued functions */
#define RealFunc(Ename,Cname)
#define SameReal(Name)
#endif

#if INCLUDE == 'R'
#define SpecialF(Ename,Type,Cname)
#define Function(Ename,Cname)
#define SameFunc(Name)
#ifdef __STDC__
#define RealFunc(Ename,Cname){#Ename,Cname},
#define SameReal(Name){#Name,Name},
#else /* not __STDC__... */
#define RealFunc(Ename,Cname){"Ename",Cname},
#define SameReal(Name){"Name",Name},
#endif
#endif

   /************************************************************\
  ****************************************************************
 **                                                              **
**	The following include files containes EDEN/C binding      **
**	header files.                                             **
**	If programmers do not use the macros, the format of the   **
** 	header files should be:                                   **
**	        #if INCLUDE == 'H'                                **
**	        extern C_name();	C function declaration    **
**              ...                     etc.                      **
**	        #endif                                            **
**	        #if INCLUDE == 'T'                                **
**	        {"Eden_name",C_name,0},	binding declaration       **
**					the 0 means int function  **
**					    1 means double	  **
**              ...                     etc.                      **
**	        #endif                                            **

... Ash's note: looks like the third parameter in the binding declaration
here (specifying type) was never implemented.

 **                                                              **
  ****************************************************************
   \************************************************************/

#if (defined(HAVE_CURSES) || defined(HAVE_NCURSES)) && defined(TTYEDEN)
#include "curses.h"
#endif

#include <stdlib.h>
#include <stdio.h>

#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

#ifdef __APPLE__
#include <math.h>
#endif /* __APPLE__ */

/*      <<<<------ Any more header files should be inserted here */



   /************************************************************\
  ****************************************************************
 **                                                              **
**	The following declarations bind some standard             **
**	C functions to EDEN names.				  **
**								  **
**	Note: no commas or semicolons are needed                  **
**	      after the macros.                                   **
 **                                                              **
  ****************************************************************
   \************************************************************/

/* fprintf and sprintf are a special case as gcc >=3.4 refuses to accept
   a definition of the form extern fprintf();, giving errors "a parameter
   list with an ellipsis can't match an empty parameter name list
   declaration". [Ash, 17 June 2005] */
#if INCLUDE == 'H'
extern fprintf(FILE * stream, const char * format, ...);
extern sprintf(char * str, const char * format, ...);
#elif INCLUDE == 'T'
{"fprintf",(int(*)())fprintf},	// This used to be {"fprintf",fprintf}, and returned an error so I've copied same structure from rand below as both return the same type - compiler does not complain [Richard] 
{"sprintf",(int(*)())sprintf},	// This used to be {"sprintf",sprintf}, and returned an error so I've copied same structure from rand below as both return the same type - compiler does not complain [Richard]
#endif

SameFunc(system)

SpecialF(popen,FILE *,popen)
SameFunc(pclose)
SpecialF(fopen,FILE *,fopen)
SameFunc(fclose)
SameFunc(fseek)

/*
SameFunc(fflush)
SpecialF(fdopen,FILE *,fdopen)
*/

SpecialF(setbuf,void,setbuf)
SpecialF(trace,void,user_trace)	/* produce trace message */
SpecialF(eager,void,eager)	/* eagerly eval all def's and action's */

/* the followings are math functions */

/* random is declared in stdlib.h and has various return types on
   various systems.  As it is declared by stdlib.h already,
   declaring it again with the correct type simply adds
   complexity - let's try this.  [Ash] */

#if INCLUDE == 'T'
{"rand", (int(*)())random},
{"srand", (int(*)())srandom},
#endif

SameReal(sqrt)
SameReal(sin)
SameReal(cos)
SameReal(tan)
SameReal(asin)
SameReal(acos)
SameReal(atan)
SameReal(atan2)
SameReal(exp)
SameReal(log)
SameReal(log10)
SameReal(pow)
SameReal(sinh)
SameReal(cosh)
SameReal(tanh)

#if INCLUDE == 'H'
	extern void *doste_lookup();
	extern int doste_init();
	extern void doste_update();
	extern char *doste_name();
	extern void *doste_edenoid();
	extern void *doste_oid();
	extern int doste_a();
	extern int doste_b();
	extern int doste_c();
	extern int doste_d();
	extern void doste_set();
	extern void doste_parse();
#endif

#if INCLUDE == 'T'
	{"doste_init", (int(*)()) doste_init},
	{"doste_update", (int(*)()) doste_update},
	{"doste_lookup", (int(*)()) doste_lookup},
	{"doste_name", (int(*)()) doste_name},
	{"doste_edenoid", (int(*)()) doste_edenoid},
	{"doste_oid", (int(*)()) doste_oid},
	{"doste_a", (int(*)()) doste_a},
	{"doste_b", (int(*)()) doste_b},
	{"doste_c", (int(*)()) doste_c},
	{"doste_d", (int(*)()) doste_d},
	{"doste_set", (int(*)()) doste_set},
	{"doste_parse", (int(*)()) doste_parse},
	
#endif

/* end of macros */

#undef SpecialF
#undef Function
#undef SameFunc
#undef RealFunc
#undef SameReal


