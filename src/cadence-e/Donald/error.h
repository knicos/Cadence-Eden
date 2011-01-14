/*
 * $Id: error.h,v 1.6 1999/11/16 21:20:40 ashley Rel1.10 $
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

/*----- error code definitions -----*/

enum errorcodes {
        Unclassified    ,
        Impossible      ,
        SyntaxError     ,
        OutOfMemory     ,
        UndefinedID     ,
        IdListExpect    ,
        IdExprUnmatch   ,
        RedeclareID     ,
        UndeclareID     ,
        StackOverflow   ,
        StackUnderflow  ,
        NotOpenshapeOrGraph ,
        TypeMismatch	,
	FuncExpect
} ;

#define ERRORSTRINGS                            \
        "%s"                                    ,\
        "impossible error occurs in %s"         ,\
        "syntax error"                          ,\
        "out of memory"                         ,\
        "undefined id: %s"                      ,\
        "id list expected"                      ,\
        "id list and expr list unmatched"       ,\
        "redeclared identifier: %s"             ,\
        "undeclared identifier: %s"             ,\
        "%sstack overflow"                      ,\
        "%sstack underflow"                     ,\
        "not openshape or graph: %s"            ,\
        "type mismatched"			,\
	"%s is not declared as a function"

extern void don_err(enum errorcodes, char *);
