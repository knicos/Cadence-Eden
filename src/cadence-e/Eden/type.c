/*
 * $Id: type.c,v 1.19 2001/12/06 22:50:22 cssbz Exp $
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

static char rcsid[] = "$Id: type.c,v 1.19 2001/12/06 22:50:22 cssbz Exp $";

/***************************************************************
 * 		    TYPE CONVERSION FUNCTIONS 		       *
 ***************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>	// [Richard] : For strlen() etc..

#include "../../../../../config.h"
#include "eden.h"
#include "builtin.h"
#include "yacc.h"

#include "emalloc.h"
#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

void t_str1(Datum);

extern Datum newhdat(Datum);

static struct {
    short       type;
    char       *name;
}           datatype[] = {

    { UNDEF, "@" },
    { REAL, "float" },
    { INTEGER, "int" },
    { MYCHAR, "char" },
    { STRING, "string" },
    { LIST, "list" },
    { FORMULA, "formula" },
    { FUNCTION, "func" },
    { PROCEDURE, "proc" },
    { PROCMACRO, "procmacro" },
    { BLTIN, "builtin" },
    { LIB, "C-func" },
    { LIB64, "64-bit C-func" },
    { RLIB, "Real-func" },
    { VAR, "var" },
    { 0, "???" }
};

Datum
ctos(Datum d)
{				/* convert char to string */
    if (ischar(d)) {		/* only if Datum is char */
	char       *s;

	s = (char *) getheap(2);/* put the string on heap */
	*s = d.u.i;
	s[1] = '\0';
	d.u.s = s;
	d.type = STRING;
    }
    if (!isstr(d))
	error("string needed");
    return d;
}

int
typeno(char *typename)
{
    int         i;

    for (i = 0; datatype[i].type; i++) {
	if (strcmp(typename, datatype[i].name) == 0)
	    break;
    }
    if (datatype[i].type == 0)
	error("unknown data type");
    return datatype[i].type;
}

char       *
typename(int type)
{
    int         i;

    for (i = 0; datatype[i].type; i++) {
	if (type == datatype[i].type)
	    break;
    }
    return datatype[i].name;
}

/* type(data): type of datum in the form of a string */
void
t_type(void)
{				/* data type name */
    Datum       d;
    short       type;
    int         i;

    if (paracount == 0)
	usage("s = type(expr);");
    type = para(1).type;
    if (type & ADDRESSMASK) {
	dpush(d, STRING, "pointer");
    } else {
	for (i = 0; datatype[i].type; i++) {
	    if (type == datatype[i].type)
		break;
	}
	dpush(d, STRING, datatype[i].name);
    }
}

/* int(data): convert datum to an integer */
void
t_int(void)
{
    Datum       d;

    if (paracount == 0)
	usage("i = int(expr);");
    d = para(1);

    if (is_symbol(d)) {
	d.u.i = d.u.v.x;
    } else {
	switch (d.type) {
	case REAL:
	    d.u.i = d.u.r;
	case INTEGER:
	case MYCHAR:
	    break;

	case STRING:
	    d.u.i = atoi(d.u.s);
	    break;

	default:
	    pushUNDEF();	/* return @ */
	    return;
	}
    }

    d.type = INTEGER;
    push(d);
}

/* float(data): builtin function convert datum to an integer */
void
t_float(void)
{
    Datum       d;

    if (paracount == 0)
	usage("f = float(expr);");
    d = para(1);

    if (is_symbol(d)) {
	d.u.r = d.u.v.x;
    } else {
	switch (d.type) {
	case INTEGER:
	case MYCHAR:
	    d.u.r = d.u.i;

	case REAL:
	    break;

	case STRING:
	    d.u.r = atof(d.u.s);
	    break;

	default:
	    pushUNDEF();	/* return @ */
	    return;
	}
    }
    d.type = REAL;
    push(d);
}

/* char(datum): builtin function convert datum to a character */
void
t_char(void)
{				/* convert to char */
    Datum       d;

    if (paracount == 0)
	usage("c = char(expr);");
    d = para(1);
    switch (d.type) {
    case REAL:
	d.u.i = d.u.r;
    case INTEGER:
    case MYCHAR:
	break;

    case STRING:		/* the 1st char of string */
	d.u.i = *d.u.s & 0377;	/* '\0' if s == "" */
	break;

    default:
	pushUNDEF();		/* return @ */
	return;
    }
    d.type = MYCHAR;
    push(d);
}

/* str(datum): builtin function convert datum to a string */
void
t_str(void)
{
    Datum       d;

    if (paracount == 0)
	usage("c = str(expr);");
    d = para(1);
    t_str1(d);
}

/* separating t_str to two subs. the latter can be used by other
   programs --sun */
void
t_str1(Datum d)
{
  /* Static buffer, used for type conversions likely to result in a
     small resulting string [Ash] */
  char buf[1024];
  /* Dynamic buffer, used when creating a string from a list [Ash] */
  char *dynbuf;
  int dynbufsize; /* current /buffer/ allocated size */
  int dynbufstrlen; /* current buffer /contents/ size */
  int         i;
  int type;
  Datum       d1;

  if (is_symbol(d)) {
    sprintf(buf, "&%s", symbol_of(d)->name);
  } else if (is_local(d)) {
    sprintf(buf, "%d", d.u.v.x);
  } else {

    switch (d.type) {
    case STRING:
      push(d);
      return;

    case LIST:
      /* Patrick added a list -> string conversion function [Ash] */
      /* All of the code in this file is similar to the print()
	 function in builtin.c, which performs the task for writeln()
	 and so on.  However, print does not need to store the result
	 anywhere, simply write it out. [Ash] */
      /* Initial size and allocation units of dynbuf [Ash] */
#define STR_DYNBUF_LEN_UNITS 1028
      dynbuf = (char *) emalloc(STR_DYNBUF_LEN_UNITS);
      dynbufsize = STR_DYNBUF_LEN_UNITS;
      *dynbuf=0;
      strcat(dynbuf, "[");
      dynbufstrlen=1;
      /* For each item in the list... [Ash] */
      for (i = 1; i <= d.u.a[0].u.i; i++) {
	type = d.u.a[i].type;	/* remember the type for later use [Ash] */
	t_str1(d.u.a[i]);	/* Convert that item to a string [Ash] */
	d1 = pop();

	/* Add to culmative length of string in the buffer [Ash] */
	dynbufstrlen += strlen(d1.u.s) + 2;

	/* Double the buffer allocation size if it looks like we might
           need to.  (10 is a fudge factor).  Here re-working some
           code that I added in 1.0 to try and do this -- but I didn't
           get it right first time.  [Ash] */
	while ((dynbufstrlen + 10) > dynbufsize) {
	  dynbufsize *= 2;
	}
	dynbuf = erealloc(dynbuf, dynbufsize);

	if (type == STRING) {
	  /* Strings in lists come out with double quote separators [Ash] */
	  strcat(dynbuf, "\"");
	  strcat(dynbuf, d1.u.s);
	  strcat(dynbuf, "\"");
	} else if (type == MYCHAR) {
	  /* Chars in lists come out with single quote separators [Ash] */
	  strcat(dynbuf, "\'");
	  strcat(dynbuf, d1.u.s);
	  strcat(dynbuf, "\'");
	} else {
	  strcat(dynbuf, d1.u.s);
	}

	if (i < d.u.a[0].u.i)
	  strcat(dynbuf, ",");

      }
      strcat(dynbuf, "]");

      d.type = STRING;
      d.u.s = dynbuf;
      d = newhdat(d);		/* copy it to the heap */
      push(d);
      free(dynbuf);
      return;

    case UNDEF:
      buf[0] = '@';
      buf[1] = '\0';
      break;

    case REAL:
      sprintf(buf, "%f", d.u.r);
      break;

    case INTEGER:
      sprintf(buf, "%d", d.u.i);
      break;

    case MYCHAR:
      buf[0] = d.u.i;
      buf[1] = '\0';
      break;

    default:
      pushUNDEF();		/* return @ */
      return;
    }
  }

  d.type = STRING;
  d.u.s = buf;
  d = newhdat(d);		/* copy it to the heap */
  push(d);
}

/*----
	_type_convert(datum, typename): builtin function convert datum
	to type specified by typename.

	! ! !   DANGER --- MAY CAUSE CLASH IF NOT CAREFUL   ! ! !

	no type checking. no memory allocation. extremely dangerous to
	convert into string or list.
----*/
void
t_super(void)
{				/* super type convertion */
    Datum       d;
    Datum       t;
    char       *s;
    int         i;
    extern void muststr(Datum);

    if (paracount < 2)
	usage("data = _type_convert(data, \"type\");");
    d = para(1);
    t = para(2);
    muststr(t);
    s = t.u.s;
    for (i = 0; datatype[i].type; i++) {
	if (strcmp(s, datatype[i].name) == 0)
	    break;
    }
    d.type = datatype[i].type;
    push(d);
}
