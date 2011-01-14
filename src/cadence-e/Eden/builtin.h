/*
 * $Id: builtin.h,v 1.16 2002/07/10 19:22:59 cssbz Exp $
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

/*
   Adding a new built-in function to Eden:
     - declare it here in builtin.h
     - declare it in builtinf.h
*/

#if INCLUDE != 'T'

#define paracount	fp->stackp->u.a[0].u.i
#define para(n)		fp->stackp->u.a[n]
#define dpush(x,y,z)	x.type=y; x.u.i=(Int)(z); push(x)

struct BLIBTBL {
	char   *name;
	void	(*func)();
};

extern struct BLIBTBL blibtbl[];

extern void usage();

#endif

/*---------------------------------------------------------*/

#if INCLUDE == 'T'
#define BIND(name,func) {name,func},
#else
#define BIND(name,func) extern void func();
#endif

/*---------------------------------------------------------*/

BIND(	"exit",	 					b_exit		           )	// Still Avaliable - From builtin.c - Need to decide how this should work though?
BIND(	"execute", 					exec_string	           )	// Still Avaliable - From builtin.c
BIND(	"include", 					exec_file	           )	// STILL NEED TO SORT OUT FILE INCLUDES!
BIND(	"include_file",				exec_file	           )	// STILL NEED TO SORT OUT FILE INCLUDES!
BIND(   "_eden_internal_cwd",      _eden_internal_cwd      )	// This is kind of safe but might disable
BIND(   "_eden_internal_cd",       _eden_internal_cd       )	// We might want this disabled

#if defined(__CYGWIN__) || defined(HAVE_LIBGEN_H)
BIND(   "_eden_internal_dirname",  _eden_internal_dirname  )	// Is safe...check out what main.c does with this though
BIND(   "_eden_internal_basename", _eden_internal_basename )	// Is safe...check out what main.c (or a .eden) file does to set this up if undefined
#endif

#ifdef linux 
BIND(   "usbhidopen",     	usbhidopen      )	// Disabled in webeden
BIND(   "usbhidread",       usbhidread      )	// Disabled in webeden
BIND(   "usbhidclose",      usbhidclose     )	// Disabled in webeden
BIND(   "rawserialopen",    rawserialopen   )	// Disabled in webeden
BIND(   "rawserialclose",   rawserialclose  )	// Disabled in webeden
BIND(   "rawserialread",    rawserialread   )	// Disabled in webeden
#endif

BIND(	"apply",	 		apply			)	// Still Avaliable - From builtin.c
BIND(	"substr",	 		substr			)	// Still Avaliable - From builtin.c
BIND(	"strcat",	 		scat			)	// Still Avaliable - From builtin.c
BIND(	"indexof",			indexOf			)	// NEW FEATURE
BIND(   "trimstr",      	trimstr     	)	// NEW FEATURE
#if !defined(TTYEDEN) && !defined(DISTRIB)
BIND(   "getTclVariable",	getTclVariable	)	// NEW FEATURE
#endif
BIND(	"sublist",			sublist			)	// Still Avaliable - From builtin.c
BIND(	"listcat",			lcat			)	// Still Avaliable - From builtin.c
BIND(	"type", 			t_type			)	// Still Avaliable - From type.c
BIND(	"int",				t_int			)	// Still Avaliable - From type.c
BIND(	"float", 			t_float			)	// Still Avaliable - From type.c
BIND(	"char", 			t_char			)	// Still Avaliable - From type.c
BIND(	"str",				t_str			)	// Still Avaliable - From type.c
BIND(	"_type_convert",	t_super			)	// Still Avaliable - From type.c - Note the danger warning!!!
BIND(	"write",			b_write			)	// Supported in webeden
BIND(	"writeln", 			writeln			)	// Supported in webeden
BIND(	"array",			array			)	// Still Avaliable - From builtin.c
BIND(	"nameof",			nameof			)	// Still Avaliable - From builtin.c
BIND(	"formula_list",		formula_list	)	// Still Avaliable - From eval.c
BIND(	"action_list",		action_list		)	// Still Avaliable - From eval.c
BIND(	"symbols",			symbols			)	// Still Avaliable - From builtin.c  <-- Could use working out what this does/how it works
BIND(	"symboldetail",		symboldetail	)	// Still Avaliable - From builtin.c
BIND(	"symboltext",		symboltext		)	// Still Avaliable - From builtin.c
BIND(	"symboltable",		symtbl2list		)	// Needs to have formatting changed  <-- Not XML Valid - Find in builtin.c
BIND(	"forget",			forget			)	// Still Avaliable - From builtin.c
BIND(	"pack",				pack			)	// Still Avaliable - From builtin.c
BIND(	"error",			user_error		)	// Supported in webeden 			 <-- Need to consider if this should be releasing stack
BIND(	"warning",			user_warning	)	// NEW FEATURE - warning("my warning");
BIND(	"notice",			user_notice		)	// NEW FEATURE
BIND(	"get_msgq",			get_msgq		)	// Disabled in webeden
BIND(	"send_msg",			send_message	)	// Disabled in webeden
BIND(	"receive_msg",		receive_message	)	// Disabled in webeden
BIND(	"remove_msgq",		remove_msgq		)	// Disabled in webeden
BIND(	"realrand",			realrand		)	// Still Avaliable - From builtin.c
BIND(	"error_no",			error_no		)	// Disabled in weden - From builtin.c <-- Returns 0 every time
BIND(	"backgnd",			backgnd			)	// Disabled in webeden
BIND(	"pipe",				pipe_process	)	// Disabled in webeden
BIND(   "ipopen",			ipopen          )	// Disabled in webeden
BIND(   "ipclose",          ipclose         )	// Disabled in webeden
BIND(   "fdready",          fdready         )	// Disabled in webeden
BIND(   "rawread",          rawread         )	// Disabled in webeden
BIND(   "rawwrite",         rawwrite        )	// Disabled in webeden
BIND(   "regmatch",         regmatch        )	// Still Avaliable - From builtin.c
BIND(   "regreplace",       regreplace      )	// Still Avaliable - From builtin.c
BIND(	"getenv",			get_environ		)	// Still Avaliable - From builtin.c <-- Consider disabling this. BUT: Needed by 'loader' possibly add a 'system loaded' flag to disable???
BIND(	"putenv",			put_environ		)	// Disabled in webeden - Returns -1 and raises
BIND(	"printhash",		printhash		)	// Supported Currently - Might need to disable this ???
BIND(	"touch",			touch			)	// Still Avaliable - From builtin.c
BIND(	"gettime",			gettime			)	// Still Avalaible - From builtin.c
BIND(	"time",				inttime			)	// Still Avaliable - From builtin.c
BIND(	"ftime",			finetime		)	// Still Avaliable - From builtin.c
BIND(	"feof",				f_eof			)	// Disabled in webeden - Returns Datum INT with value (-1)
BIND(	"getchar",			get_char		)	// Disabled in webeden - Returns Datum INT with value (-1)
BIND(	"fgetc",			fget_char		)	// Disabled in webeden - Returns Datum INT with value (-1)
BIND(   "stat",             file_stat       )	// Disabled in webeden
BIND(	"gets",				get_string		)	// Disabled in webeden - Returns Undefined @
BIND(	"fgets",			fget_string		)	// Disabled in webeden - Returns Undefined @
BIND(	"ungetc",			unget_char		)	// Disabled in webeden
BIND(	"scanf",			scan_f			)	// Disabled in webeden - Returns Datum INT with value (0)
BIND(	"fscanf",			fscan_f			)	// Disabled in webeden - Returns Datum INT with value (0)
BIND(	"sscanf",			sscan_f			)	// Disabled in webeden - Returns Datum INT with value (0)
BIND(	"todo",				todo			)	// Still Avaliable - From builtin.c
BIND(	"inpdevlineno", 	inpdevlineno	)	// Still Avaliable - From builtin.c
BIND(	"inpdevname", 		inpdevname		)	// Still Avaliable - From builtin.c


#ifdef NO_CHECK_CIRCULAR
BIND(	"dcc",				dcc				)	// Still Avaliable - From builtin.c
#endif /* NO_CHECK_CIRCULAR */


#ifdef DEBUG
BIND(	"debug", 			debug			)	// Still Avalaible - From builtin.c. NOTE: A number of debug sections not avalaible (if (DEBUG) && !defined(WEDEN_ENABLED))
#endif /* DEBUG */


#ifndef TTYEDEN
BIND(	"Disp2PS",			E_Disp2PS		)
BIND(	"StringRemain",		E_StringRemain	)
BIND(	"tcl",				E_tcl			)
BIND(	"DisplayScreen",	DisplayScreen	)
BIND(	"PlotShape",		PlotShape		)
BIND(	"createtimer",		createtimer		)
BIND(	"deletetimer",		deletetimer		)
BIND(	"deletealltimers",	deletealltimers	)
#endif


#ifdef INSTRUMENT
BIND(   "insPrint", 		insPrint        )	// Still Avaliable - From builtin.c - Not sure what its for. ALSO: to enable compile with -DINSTRUMENT
BIND(   "insReset",         insReset        )	// Still Avaliable - From builtin.c - Not sure what its for. ALSO: to enable compile with -DINSTRUMENT
#endif /* INSTRUMENT */




#if defined(DISTRIB) && !defined(WEDEN_ENABLED)
BIND(   "sendServer",       sendServer      )	// Not Supported in webeden
BIND(   "sendClient",       sendClient      )	// Not Supported in webeden
BIND(   "propagate",        propagate       )	// Not Supported in webeden
BIND(   "addAgency",        addAgency       )	// Not Supported in webeden
BIND(   "removeAgency",     removeAgency    )	// Not Supported in webeden
BIND(   "checkAgency",      checkAgency     )	// Not Supported in webeden
BIND(   "queryObs",         queryObs        )	// Not Supported in webeden
BIND(   "renewObs",         renewObs        )	// Not Supported in webeden
#endif /* DISTRIB */


// NO SASAMI FEATURES ARE CURRENTLY SUPPORTED IN WebEDEN

#if defined(WANT_SASAMI) && !defined(TTYEDEN) && !defined(WEDEN_ENABLED)
/* Sasami functions */
BIND(	"sasami_vertex",			ed_sasami_vertex			)	// Not Supported in webeden
BIND(	"sasami_set_bgcolour",		ed_sasami_set_bgcolour		)	// Not Supported in webeden
BIND(	"sasami_poly_geom_vertex",	ed_sasami_poly_geom_vertex	)	// Not Supported in webeden
BIND(	"sasami_poly_tex_vertex",	ed_sasami_poly_tex_vertex	)	// Not Supported in webeden
BIND(	"sasami_poly_colour",		ed_sasami_poly_colour		)	// Not Supported in webeden
BIND(	"sasami_poly_material",		ed_sasami_poly_material		)	// Not Supported in webeden
BIND(	"sasami_object_poly",		ed_sasami_object_poly		)	// Not Supported in webeden
BIND(	"sasami_object_pos",		ed_sasami_object_pos		)	// Not Supported in webeden
BIND(	"sasami_object_rot",		ed_sasami_object_rot		)	// Not Supported in webeden
BIND(	"sasami_object_scale",		ed_sasami_object_scale		)	// Not Supported in webeden
BIND(	"sasami_object_visible",	ed_sasami_object_visible	)	// Not Supported in webeden
BIND(	"sasami_viewport",			ed_sasami_viewport			)	// Not Supported in webeden
BIND(	"sasami_setshowaxes",		ed_sasami_setshowaxes		)	// Not Supported in webeden
BIND(	"sasami_material_ambient",	ed_sasami_material_ambient	)	// Not Supported in webeden
BIND(	"sasami_material_diffuse",	ed_sasami_material_diffuse	)	// Not Supported in webeden
BIND(	"sasami_material_specular",	ed_sasami_material_specular	)	// Not Supported in webeden
BIND(	"sasami_material_texture",	ed_sasami_material_texture	)	// Not Supported in webeden
BIND(	"sasami_light_pos",			ed_sasami_light_pos			)	// Not Supported in webeden
BIND(	"sasami_light_ambient",		ed_sasami_light_ambient		)	// Not Supported in webeden
BIND(	"sasami_light_diffuse",		ed_sasami_light_diffuse		)	// Not Supported in webeden
BIND(	"sasami_light_specular",	ed_sasami_light_specular	)	// Not Supported in webeden
BIND(	"sasami_light_enabled",		ed_sasami_light_enabled		)	// Not Supported in webeden
BIND(	"sasami_light_attenuation",	ed_sasami_light_attenuation	)	// Not Supported in webeden
BIND(	"sasami_light_directional",	ed_sasami_light_directional	)	// Not Supported in webeden
BIND(	"sasami_cube",				ed_sasami_cube				)	// Not Supported in webeden
BIND(	"sasami_face_material",		ed_sasami_face_material		)	// Not Supported in webeden
BIND(	"sasami_object_delete",		ed_sasami_object_delete		)	// Not Supported in webeden
BIND(	"sasami_cylinder",			ed_sasami_cylinder			)	// Not Supported in webeden
BIND(	"sasami_primitive_material",ed_sasami_primitive_material)	// Not Supported in webeden
BIND(	"sasami_sphere",			ed_sasami_sphere			)	// Not Supported in webeden
#endif /* defined(WANT_SASAMI) && !defined(TTYEDEN) && !defined(WEDEN_ENABLED) */ 


#undef BIND
