/*
 * $Id: store.c,v 1.11 2001/07/27 17:00:29 cssbz Exp $
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

static char rcsid[] = "$Id: store.c,v 1.11 2001/07/27 17:00:29 cssbz Exp $";

#include "../../../config.h"

#include <stdio.h>
#include <string.h> // [Richard] : For strlen() etc...
#include "symbol.h"
#include "tree.h"
#include "parser.h"

#include <tk.h>
extern Tcl_Interp *interp;

static char ScoutDefn[4096];
static char *store;

extern char *just_name(int);
extern char *scoutType(int);

/* function prototypes */
void storeDefn(tree *);
void storeSym(symbol *);
#ifdef DISTRIB
void propagateScoutDef(tree *);
#endif /* DISTRIB */

#ifdef __STDC__
#define show(field)\
if (w.field) {\
	sprintf(store, "\n    %s: ", #field);\
	store += strlen(store);\
	storeDefn(w.field);\
}

#define dot(x) \
storeDefn(t->l.t); \
sprintf(store, ".%s", #x);\
store += strlen(store)
#else
#define show(field)\
if (w.field) {\
	sprintf(store, "\n    field: ");\
	store += strlen(store);\
	storeDefn(w.field);\
}

#define dot(x) \
storeDefn(t->l.t); \
sprintf(store, ".x");\
store += strlen(store)
#endif

#ifdef DISTRIB
static int printStoreDefn = 1; /* something to do with \"s [Ash] */
#endif /* DISTRIB */
/*
 * storeDefn - unparse the tree t and print the result on fp
 */
void storeDefn(tree * t) {
	WinStruct w;
#ifdef DISTRIB
	/* !@!@ Perhaps we don't need this [Ash] */
	/* char       *s; */
#endif /* DISTRIB */
	tree *arg;

	if (!t) {
		sprintf(store, "@");
		store++;
		return;
	}
	switch (t->op) {
	case '=':
		if (t->r.t) {
			sprintf(store, "%s = ", t->l.v->name);
			store += strlen(store);
			storeDefn(t->r.t);
		} else {
			sprintf(store, "%s", t->l.v->name);
			store += strlen(store);
		}
		break;
	case NUMBER:
		if (t->l.i == (int) t->l.i)
			sprintf(store, "%d", (int)t->l.i);
		else
			sprintf(store, "%f", t->l.i);
		store += strlen(store);
		break;
	case ROW:
		storeDefn(t->l.t);
		sprintf(store, ".r");
		store += 2;
		break;
	case COLUMN:
		storeDefn(t->l.t);
		sprintf(store, ".c");
		store += 2;
		break;
	case STR:
#ifdef DISTRIB
		if (printStoreDefn)
		sprintf(store, "\"%s\"", t->l.s); /* "%s" */
		else {
			/* This is only done when doing propagation [Ash] */
			sprintf(store, "\\\"%s\\\"", t->l.s); /* \"%s\" */
		}
		store += strlen(store);
#else
		sprintf(store, "\"%s\"", t->l.s);
		store += strlen(store);
#endif /* DISTRIB */
		break;
	case IMGVAR:
	case UNKNOWN:
	case STRVAR:
	case INTVAR:
	case PTVAR:
	case BOXVAR:
	case FRAMEVAR:
	case WINVAR:
	case DISPVAR:
		sprintf(store, "%s", t->l.v->name);
		store += strlen(store);
		break;
	case FORMPT:
		sprintf(store, "{");
		store++;
		storeDefn(t->l.t);
		sprintf(store, ", ");
		store += 2;
		storeDefn(t->r.t);
		sprintf(store, "}");
		store++;
		break;
	case FORMBOX:
		sprintf(store, "[");
		store++;
		storeDefn(t->l.t);
		sprintf(store, ", ");
		store += 2;
		storeDefn(t->r.t);
		sprintf(store, "]");
		store++;
		break;
	case TEXTBOX:
		sprintf(store, "[");
		store++;
		storeDefn(t->l.t);
		sprintf(store, ", ");
		store += 2;
		storeDefn(t->r.t->l.t);
		sprintf(store, ", ");
		store += 2;
		storeDefn(t->r.t->r.t);
		sprintf(store, "]");
		store++;
		break;
	case '(':
	case FORMFRAME:
		sprintf(store, "(");
		store++;
		storeDefn(t->l.t);
		sprintf(store, ")");
		store++;
		break;
	case FORMWIN:
		w = t->l.w;
		sprintf(store, "{\n    type: ");
		store += strlen(store);
		switch (w.type) {
		/* !@!@ This should really use the constants array [Ash] */
		case 0:
			sprintf(store, "TEXT");
			break;
		case 1:
			sprintf(store, "DONALD");
			break;
		case 2:
			sprintf(store, "ARCA");
			break;
		case 3:
			sprintf(store, "IMAGE");
			break;
		case 4:
			sprintf(store, "TEXTBOX");
			break;
		default:
			sprintf(store, "UNKNOWN");
			break;
		}
		;
		store += strlen(store);
		show(frame)
		;
		show(string)
		;
		show(box)
		;
		show(pict)
		;
		show(xmin)
		;
		show(ymin)
		;
		show(xmax)
		;
		show(ymax)
		;
		show(font)
		;
		show(bgcolor)
		;
		show(fgcolor)
		;
		show(bdcolor)
		;
		show(border)
		;
		if (w.bdtype) {
			sprintf(store, "\n    relief: ");
			store += strlen(store);
			storeDefn(w.bdtype);
		}
		if (w.align != -1) {
			sprintf(store, "\n    alignment: %s", just_name(w.align));
			store += strlen(store);
		}
		if (w.sensitive) {
			sprintf(store, "\n    sensitive: ON");
			store += strlen(store);
		}
		sprintf(store, "\n}");
		store += strlen(store);
		break;
	case FORMDISP:
		sprintf(store, "<");
		store++;
		storeDefn(t->l.t);
		sprintf(store, ">");
		store++;
		break;
	case CONCAT:
		storeDefn(t->l.t);
		sprintf(store, " // ");
		store += 4;
		storeDefn(t->r.t);
		break;
	case STRCAT:
		sprintf(store, "strcat(");
		store += 7;
		storeDefn(t->l.t);
		sprintf(store, ", ");
		store += 2;
		storeDefn(t->r.t);
		sprintf(store, ")");
		store++;
		break;
	case STRLEN:
		sprintf(store, "strlen(");
		store += 7;
		storeDefn(t->l.t);
		sprintf(store, ")");
		store++;
		break;
	case SUBSTR:
		sprintf(store, "substr(");
		store += 7;
		storeDefn(t->l.t);
		sprintf(store, ", ");
		store += 2;
		storeDefn(t->r.t->l.t);
		sprintf(store, ", ");
		store += 2;
		storeDefn(t->r.t->r.t);
		sprintf(store, ")");
		store++;
		break;
	case TOSTRING:
		sprintf(store, "itos(");
		store += 5;
		storeDefn(t->l.t);
		sprintf(store, ")");
		store++;
		break;
	case BOXSHIFT:
		sprintf(store, "shift(");
		store += 6;
		storeDefn(t->l.t);
		sprintf(store, ", ");
		store += 2;
		storeDefn(t->r.t->l.t);
		sprintf(store, ", ");
		store += 2;
		storeDefn(t->r.t->r.t);
		sprintf(store, ")");
		store++;
		break;
	case BOXINTERSECT:
		sprintf(store, "intersect(");
		store += strlen(store);
		storeDefn(t->l.t);
		sprintf(store, ", ");
		store += 2;
		storeDefn(t->r.t);
		sprintf(store, ")");
		store++;
		break;
	case BOXCENTRE:
		sprintf(store, "centre(");
		store += strlen(store);
		storeDefn(t->l.t);
		sprintf(store, ", ");
		store += strlen(store);
		storeDefn(t->r.t);
		sprintf(store, ")");
		store++;
		break;
	case BOXENCLOSING:
		sprintf(store, "enclose(");
		store += strlen(store);
		storeDefn(t->l.t);
		sprintf(store, ", ");
		store += strlen(store);
		storeDefn(t->r.t);
		sprintf(store, ")");
		store++;
		break;
	case BOXREDUCE:
		sprintf(store, "reduce(");
		store += strlen(store);
		storeDefn(t->l.t);
		sprintf(store, ", ");
		store += strlen(store);
		storeDefn(t->r.t);
		sprintf(store, ")");
		store++;
		break;
	case DOTNE:
		dot(ne)
		;
		break;
	case DOTNW:
		dot(nw)
		;
		break;
	case DOTSE:
		dot(se)
		;
		break;
	case DOTSW:
		dot(sw)
		;
		break;
	case DOTN:
		dot(n)
		;
		break;
	case DOTE:
		dot(e)
		;
		break;
	case DOTS:
		dot(s)
		;
		break;
	case DOTW:
		dot(w)
		;
		break;
	case '.':
		storeDefn(t->l.t);
		sprintf(store, ".");
		store++;
		storeDefn(t->r.t);
		break;
	case DOTFRAME:
		dot(frame)
		;
		break;
	case DOTSTR:
		dot(string)
		;
		break;
	case DOTBOX:
		dot(box)
		;
		break;
	case DOTTYPE:
		dot(type)
		;
		break;
	case DOTPICT:
		dot(pict)
		;
		break;
	case DOTXMIN:
		dot(xmin)
		;
		break;
	case DOTYMIN:
		dot(ymin)
		;
		break;
	case DOTXMAX:
		dot(xmax)
		;
		break;
	case DOTYMAX:
		dot(ymax)
		;
		break;
	case DOTFONT:
		dot(font)
		;
		break;
	case DOTBG:
		dot(bgcolor)
		;
		break;
	case DOTFG:
		dot(fgcolor)
		;
		break;
	case DOTBDCOLOR:
		dot(bdcolor)
		;
		break;
	case DOTBORDER:
		dot(border)
		;
		break;
	case DOTBDTYPE:
		dot(relief)
		;
		break;
	case DOTALIGN:
		dot(alignment)
		;
		break;
	case DOTSENSITIVE:
		dot(sensitive)
		;
		break;
	case IMGFUNC:
		sprintf(store, "%s(", t->l.s);
		store += strlen(store);
		for (arg = t->r.t; arg->op; arg = arg->r.t) {
			storeDefn(arg->l.t);
			if (arg->r.t->op) {
				sprintf(store, ", ");
				store += strlen(store);
			}
		}
		sprintf(store, ")");
		store++;
		break;

	case APPEND:
		sprintf(store, "append(");
		store += strlen(store);
		storeDefn(t->l.t);
		sprintf(store, ", ");
		store += strlen(store);
		storeDefn(t->r.t->l.t);
		sprintf(store, ", ");
		store += strlen(store);
		storeDefn(t->r.t->r.t);
		sprintf(store, ")");
		store++;
		break;
	case DELETE:
		sprintf(store, "delete(");
		store += strlen(store);
		storeDefn(t->l.t);
		sprintf(store, ", ");
		store += strlen(store);
		storeDefn(t->r.t);
		sprintf(store, ")");
		store++;
		break;
	case '&':
		storeDefn(t->l.t);
		sprintf(store, " & ");
		store += strlen(store);
		storeDefn(t->r.t);
		break;
	case ',':
		storeDefn(t->l.t);
		sprintf(store, ", ");
		store += strlen(store);
		storeDefn(t->r.t);
		break;
	case '>':
		storeDefn(t->l.t);
		sprintf(store, " / ");
		store += strlen(store);
		storeDefn(t->r.t);
		break;
	case UMINUS:
		sprintf(store, "-");
		store++;
		storeDefn(t->l.t);
		break;
	case '+':
		storeDefn(t->l.t);
		sprintf(store, " + ");
		store += strlen(store);
		storeDefn(t->r.t);
		break;
	case '-':
		storeDefn(t->l.t);
		sprintf(store, " - ");
		store += strlen(store);
		storeDefn(t->r.t);
		break;
	case '*':
		storeDefn(t->l.t);
		sprintf(store, " * ");
		store += strlen(store);
		storeDefn(t->r.t);
		break;
	case '/':
		storeDefn(t->l.t);
		sprintf(store, " / ");
		store += strlen(store);
		storeDefn(t->r.t);
		break;
	case '%':
		storeDefn(t->l.t);
		sprintf(store, " %% ");
		store += strlen(store);
		storeDefn(t->r.t);
		break;
	case EQ:
		storeDefn(t->l.t);
		sprintf(store, " == ");
		store += strlen(store);
		storeDefn(t->r.t);
		break;
	case NE:
		storeDefn(t->l.t);
		sprintf(store, " != ");
		store += strlen(store);
		storeDefn(t->r.t);
		break;
	case LT:
		storeDefn(t->l.t);
		sprintf(store, " < ");
		store += strlen(store);
		storeDefn(t->r.t);
		break;
	case LE:
		storeDefn(t->l.t);
		sprintf(store, " <= ");
		store += strlen(store);
		storeDefn(t->r.t);
		break;
	case GT:
		storeDefn(t->l.t);
		sprintf(store, " > ");
		store += strlen(store);
		storeDefn(t->r.t);
		break;
	case GE:
		storeDefn(t->l.t);
		sprintf(store, " >= ");
		store += strlen(store);
		storeDefn(t->r.t);
		break;
	case OR:
		storeDefn(t->l.t);
		sprintf(store, " || ");
		store += strlen(store);
		storeDefn(t->r.t);
		break;
	case AND:
		storeDefn(t->l.t);
		sprintf(store, " && ");
		store += strlen(store);
		storeDefn(t->r.t);
		break;
	case IF:
		sprintf(store, "if ");
		store += strlen(store);
		storeDefn(t->l.t);
		sprintf(store, " then ");
		store += strlen(store);
		storeDefn(t->r.t->l.t);
		sprintf(store, " else ");
		store += strlen(store);
		storeDefn(t->r.t->r.t);
		sprintf(store, " endif");
		store += strlen(store);
		break;
	default:
		sprintf(store, "Unknown operator!");
		store += strlen(store);
		break;
	}
}

/*
 printsym - print the type, variable name and the definition
 of the variable pointed by v on file fp
 */
void storeSym(symbol * v) {
	Tcl_DString command;
	Tcl_DStringInit(&command);
	if (v->type != UNKNOWN) {
		store = ScoutDefn;
		if (v->def) {
			/* print the type and variable name */
			sprintf(store, "%s %s = ", scoutType(v->type), v->name);
			store += strlen(store);
			/* print the definition */
			storeDefn(v->def);
			sprintf(store, ";\n");
			store += strlen(store);
		} else {
			/* print the type and variable name */
			sprintf(store, "%s %s;\n", scoutType(v->type), v->name);
			store += strlen(store);
		}
		Tcl_DStringAppendElement(&command, "scoutDefn");
		Tcl_DStringAppendElement(&command, v->name);
		Tcl_DStringAppendElement(&command, ScoutDefn);
		Tcl_EvalEC(interp, command.string);
		Tcl_DStringFree(&command);
	}
}










#ifdef DISTRIB
void
propagateScoutDef(tree * t)
{
	extern void propagateScoutScr(char *, char *);

	if (t->l.v->type != UNKNOWN) {
		store = ScoutDefn;
		if (t->l.v->def) {
			/* print the type and variable name */
			sprintf(store, "%%scout\n%s %s = ", scoutType(t->l.v->type),
					t->l.v->name);
			store += strlen(store);
			/* print the definition */

			printStoreDefn = 0;
			storeDefn(t->l.v->def);
			printStoreDefn = 1;
			sprintf(store, ";\n%%eden\n");
			store += strlen(store);
		} else {
			/* print the type and variable name */
			sprintf(store, "%%scout\n%s %s;\n%%eden\n",
					scoutType(t->l.v->type), t->l.v->name);
			store += strlen(store);
		}
		propagateScoutScr(t->l.v->name, ScoutDefn);
	}
}
#endif /* DISTRIB */
