/*
 * $Id: builtinf.h,v 1.8 2002/07/10 19:23:25 cssbz Exp $
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
 * builtin or predefined function table
 *
 */

/* builtin function table */
{ "autocalc" },
{ "exit" },
{ "execute" }, 
{ "include" }, 
{ "eager" },
{ "include_file" },
{ "_eden_internal_cwd" },
{ "_eden_internal_cd" },

#if defined(__CYGWIN__) || defined(HAVE_LIBGEN_H)
{ "_eden_internal_dirname" },
{ "_eden_internal_basename" },
#endif

{ "cd" }, /* unfortunately having to define the non-_internal versions
             as well due to VA problems - this means that the user
             cannot then redefine the initial versions of these
             functions :( [Ash] */
{ "cwd" },
{ "dirname" },
{ "basename" },

{ "apply" },
{ "substr" },
{ "strcat" },
{ "trimstr" },
{ "sublist" },
{ "listcat" },
{ "type" },
{ "int" },
{ "float" },
{ "char" }, 
{ "str" },
{ "_type_convert" },
{ "write" },
{ "writeln" },
{ "sendClient" },
{ "sendServer" },
{ "propagate" },
{ "addAgency" },
{ "removeAgency" },
{ "checkAgency" },
{ "FALSE" },   
{ "TRUE" },   
{ "ON" },   
{ "OFF" },
{ "MOTION" },
{ "ENTER" },
{ "LEAVE" },
{ "propagateType" },
{ "EveryOneAllowed" },
{ "synchronize" },
{ "higherPriority" },
{ "queryObs" },
{ "renewObs" },
{ "array" },
{ "nameof" },
{ "formula_list" },
{ "action_list" },
{ "symbols" },	
{ "symboldetail" },
{ "symboltable" },
{ "forget" },	
{ "pack" },
{ "error" },
{ "get_msgq" },	
{ "send_msg" },	
{ "receive_msg" },
{ "remove_msgq" },
{ "realrand" },	
{ "error_no" },
{ "backgnd" },	
{ "pipe" },
{ "ipopen" },
{ "ipclose" },
{ "fdready" },
{ "rawread" },
{ "rawwrite" },
{ "regmatch" },
{ "regreplace" },
{ "getenv" },
{ "putenv" },
{ "printhash" },
{ "touch" },	
{ "gettime" },	
{ "time" },	
{ "ftime" },	
{ "feof" },	
{ "getchar" },	
{ "fgetc" },
{ "stat" },
{ "gets" },	
{ "fgets" },	
{ "ungetc" },	
{ "scanf" },	
{ "fscanf" },	
{ "sscanf" },
{ "system" },

#ifdef NO_CHECK_CIRCULAR
{ "dcc" },	
#endif /* NO_CHECK_CIRCULAR */

#ifdef DEBUG
{ "debug" }, 	
#endif /* DEBUG */

#ifndef TTYEDEN
{ "Disp2PS" },	
{ "StringRemain" },		
{ "tcl" },
{ "DisplayScreen" },
{ "OpenDisplay" },
{ "PlotShape" },
{ "DoNaLDdefaultWin" },	
#endif

{ "todo" },	


/*  for Donald  */	
{ "cart" },
{ "CART" },
{ "label" },
{ "trans" },
{ "line" },
{ "circle" },
{ "rectangle" },
{ "arc" },
{ "PI" },
{ "str" },
{ "int" },
{ "float" },
{ "intersect" },
{ "parallel" },
{ "perpend" },
{ "between" },
{ "colinear" },
{ "intersects" },
{ "separates" },
{ "includes" },
{ "distlarger" },
{ "distsmaller" },
{ "dist" },
{ "midpoint" },
{ "sqrt" },
{ "cos" },
{ "sin" },
{ "tan" },
{ "asin" },
{ "acos" },
{ "atan" },
{ "log" },
{ "exp" },
{ "incident" },
{ "ellipse" },
{ "DD_random" },
{ "rtos" },
{ "dotx" },
{ "doty" },
{ "dot1" },
{ "dot2" },
{ "dotrad" },
{ "dotarg" },
{ "polar" },
{ "rot" },
{ "scale" },
{ "vector_add" },
{ "vector_sub" },
{ "pt_add" },
{ "pt_subtract" },
{ "scalar_mult" },
{ "scalar_div" },
{ "scalar_mod" },
/* for adm  */
{ "sysClock" },
#ifdef linux
{ "usbhidopen" },
{ "usbhidread" },
{ "usbhidclose" },
{ "rawserialopen" },
{ "rawserialclose" },
{ "rawserialread" },
#endif
#ifdef INSTRUMENT
{ "insPrint" },
{ "insReset" },
#endif
{ "inpdevlineno" },
{ "inpdevname" },
{ "createtimer" },
{ "deletetimer" },
{ "deletealltimers" },

/* For DOSTE */
{ "edenclock" },
{ "_doste_tick" },
