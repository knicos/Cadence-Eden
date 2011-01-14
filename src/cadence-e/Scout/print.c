/*
 * $Id: print.c,v 1.8 2001/07/27 16:58:33 cssbz Exp $
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

static char rcsid[] = "$Id: print.c,v 1.8 2001/07/27 16:58:33 cssbz Exp $";

#include "../../../../../config.h"

#include <stdio.h>

#include "symbol.h"
#include "tree.h"
#include "parser.h"

/* function prototypes */
char *scoutType(int);
char *just_name(int);
void printdef(tree *, FILE *);
void printsym(symbol *, FILE *);

#ifdef __STDC__
#define show(field)\
if (w.field) {\
	fprintf(fp, "\n    %s: ", #field);\
	printdef(w.field, fp);\
}

#define dot(x) \
printdef(t->l.t, fp); \
fprintf(fp, ".%s", #x)
#else
#define show(field)\
if (w.field) {\
	fprintf(fp, "\n    field: ");\
	printdef(w.field, fp);\
}

#define dot(x) \
printdef(t->l.t, fp); \
fprintf(fp, ".x")
#endif

/*
 scoutType - return the type name of type
 */
char * scoutType(int type) {
	switch (type) {
	case STRVAR:
		return "string";
	case INTVAR:
		return "integer";
	case PTVAR:
		return "point";
	case BOXVAR:
		return "box";
	case FRAMEVAR:
		return "frame";
	case WINVAR:
		return "window";
	case DISPVAR:
		return "display";
	case UNKNOWN:
		return "unknown";
	case IMGVAR:
		return "image";
	default:
		return "error !!!";
	}
}

/*
 just_name - return the corresponding string of just
 */
char * just_name(int just) {
	switch (just) {
	case 0:
		return "NOADJ";
	case 1:
		return "LEFT";
	case 2:
		return "RIGHT";
	case 3:
		return "CENTRE";
	case 4:
		return "EXPAND";
	default:
		return "error !!!";
	}
}

/*
 printdef - unparse the tree t and print the result on fp
 */
void printdef(tree * t, FILE * fp) {
	WinStruct w;
	tree *arg;

	if (!t) {
		fprintf(fp, "@");
		return;
	}
	switch (t->op) {
	case '=':
		if (t->r.t) {
			fprintf(fp, "%s = ", t->l.v->name);
			printdef(t->r.t, fp);
		} else {
			fprintf(fp, "%s", t->l.v->name);
		}
		break;
	case NUMBER:
		fprintf(fp, "%g", t->l.i);
		break;
	case ROW:
		printdef(t->l.t, fp);
		fprintf(fp, ".r");
		break;
	case COLUMN:
		printdef(t->l.t, fp);
		fprintf(fp, ".c");
		break;
	case STR:
		fprintf(fp, "\"%s\"", t->l.s);
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
		fprintf(fp, "%s", t->l.v->name);
		break;
	case FORMPT:
		fprintf(fp, "{");
		printdef(t->l.t, fp);
		fprintf(fp, ", ");
		printdef(t->r.t, fp);
		fprintf(fp, "}");
		break;
	case FORMBOX:
		fprintf(fp, "[");
		printdef(t->l.t, fp);
		fprintf(fp, ", ");
		printdef(t->r.t, fp);
		fprintf(fp, "]");
		break;
	case TEXTBOX:
		fprintf(fp, "[");
		printdef(t->l.t, fp);
		fprintf(fp, ", ");
		printdef(t->r.t->l.t, fp);
		fprintf(fp, ", ");
		printdef(t->r.t->r.t, fp);
		fprintf(fp, "]");
		break;
	case '(':
	case FORMFRAME:
		fprintf(fp, "(");
		printdef(t->l.t, fp);
		fprintf(fp, ")");
		break;
	case FORMWIN:
		w = t->l.w;
		fprintf(fp, "{\n    type: ");
		/* !@!@ This should really use the constants array [Ash] */
		switch (w.type) {
		case 0:
			fprintf(fp, "TEXT");
			break;
		case 1:
			fprintf(fp, "DONALD");
			break;
		case 2:
			fprintf(fp, "ARCA");
			break;
		case 3:
			fprintf(fp, "IMAGE");
			break;
		case 4:
			fprintf(fp, "TEXTBOX");
			break;
		default:
			fprintf(fp, "UNKNOWN");
			break;
		}
		;
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
			fprintf(fp, "\n    relief: ");
			printdef(w.bdtype, fp);
		}
		if (w.align != -1)
			fprintf(fp, "\n    alignment: %s", just_name(w.align));
		if (w.sensitive)
			fprintf(fp, "\n    sensitive: ON");
		fprintf(fp, "\n}");
		break;
	case FORMDISP:
		fprintf(fp, "<");
		printdef(t->l.t, fp);
		fprintf(fp, ">");
		break;
	case CONCAT:
		printdef(t->l.t, fp);
		fprintf(fp, " // ");
		printdef(t->r.t, fp);
		break;
	case STRCAT:
		fprintf(fp, "strcat(");
		printdef(t->l.t, fp);
		fprintf(fp, ", ");
		printdef(t->r.t, fp);
		fprintf(fp, ")");
		break;
	case STRLEN:
		fprintf(fp, "strlen(");
		printdef(t->l.t, fp);
		fprintf(fp, ")");
		break;
	case SUBSTR:
		fprintf(fp, "substr(");
		printdef(t->l.t, fp);
		fprintf(fp, ", ");
		printdef(t->r.t->l.t, fp);
		fprintf(fp, ", ");
		printdef(t->r.t->r.t, fp);
		fprintf(fp, ")");
		break;
	case TOSTRING:
		fprintf(fp, "itos(");
		printdef(t->l.t, fp);
		fprintf(fp, ")");
		break;
	case BOXSHIFT:
		fprintf(fp, "shift(");
		printdef(t->l.t, fp);
		fprintf(fp, ", ");
		printdef(t->r.t->l.t, fp);
		fprintf(fp, ", ");
		printdef(t->r.t->r.t, fp);
		fprintf(fp, ")");
		break;
	case BOXINTERSECT:
		fprintf(fp, "intersect(");
		printdef(t->l.t, fp);
		fprintf(fp, ", ");
		printdef(t->r.t, fp);
		fprintf(fp, ")");
		break;
	case BOXCENTRE:
		fprintf(fp, "centre(");
		printdef(t->l.t, fp);
		fprintf(fp, ", ");
		printdef(t->r.t, fp);
		fprintf(fp, ")");
		break;
	case BOXENCLOSING:
		fprintf(fp, "enclose(");
		printdef(t->l.t, fp);
		fprintf(fp, ", ");
		printdef(t->r.t, fp);
		fprintf(fp, ")");
		break;
	case BOXREDUCE:
		fprintf(fp, "reduce(");
		printdef(t->l.t, fp);
		fprintf(fp, ", ");
		printdef(t->r.t, fp);
		fprintf(fp, ")");
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
		printdef(t->l.t, fp);
		fprintf(fp, ".");
		printdef(t->r.t, fp);
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
		fprintf(fp, "%s(", t->l.s);
		for (arg = t->r.t; arg->op; arg = arg->r.t) {
			printdef(arg->l.t, fp);
			if (arg->r.t->op)
				fprintf(fp, ", ");
		}
		fprintf(fp, ")");
		break;
	case APPEND:
		fprintf(fp, "append(");
		printdef(t->l.t, fp);
		fprintf(fp, ", ");
		printdef(t->r.t->l.t, fp);
		fprintf(fp, ", ");
		printdef(t->r.t->r.t, fp);
		fprintf(fp, ")");
		break;
	case DELETE:
		fprintf(fp, "delete(");
		printdef(t->l.t, fp);
		fprintf(fp, ", ");
		printdef(t->r.t, fp);
		fprintf(fp, ")");
		break;
	case '&':
		printdef(t->l.t, fp);
		fprintf(fp, " & ");
		printdef(t->r.t, fp);
		break;
	case ',':
		printdef(t->l.t, fp);
		fprintf(fp, ", ");
		printdef(t->r.t, fp);
		break;
	case '>':
		printdef(t->l.t, fp);
		fprintf(fp, " / ");
		printdef(t->r.t, fp);
		break;
	case UMINUS:
		fprintf(fp, "-");
		printdef(t->l.t, fp);
		break;
	case '+':
		printdef(t->l.t, fp);
		fprintf(fp, " + ");
		printdef(t->r.t, fp);
		break;
	case '-':
		printdef(t->l.t, fp);
		fprintf(fp, " - ");
		printdef(t->r.t, fp);
		break;
	case '*':
		printdef(t->l.t, fp);
		fprintf(fp, " * ");
		printdef(t->r.t, fp);
		break;
	case '/':
		printdef(t->l.t, fp);
		fprintf(fp, " / ");
		printdef(t->r.t, fp);
		break;
	case '%':
		printdef(t->l.t, fp);
		fprintf(fp, " %% ");
		printdef(t->r.t, fp);
		break;
	case EQ:
		printdef(t->l.t, fp);
		fprintf(fp, " == ");
		printdef(t->r.t, fp);
		break;
	case NE:
		printdef(t->l.t, fp);
		fprintf(fp, " != ");
		printdef(t->r.t, fp);
		break;
	case LT:
		printdef(t->l.t, fp);
		fprintf(fp, " < ");
		printdef(t->r.t, fp);
		break;
	case LE:
		printdef(t->l.t, fp);
		fprintf(fp, " <= ");
		printdef(t->r.t, fp);
		break;
	case GT:
		printdef(t->l.t, fp);
		fprintf(fp, " > ");
		printdef(t->r.t, fp);
		break;
	case GE:
		printdef(t->l.t, fp);
		fprintf(fp, " >= ");
		printdef(t->r.t, fp);
		break;
	case OR:
		printdef(t->l.t, fp);
		fprintf(fp, " || ");
		printdef(t->r.t, fp);
		break;
	case AND:
		printdef(t->l.t, fp);
		fprintf(fp, " && ");
		printdef(t->r.t, fp);
		break;
	case IF:
		fprintf(fp, "if ");
		printdef(t->l.t, fp);
		fprintf(fp, " then ");
		printdef(t->r.t->l.t, fp);
		fprintf(fp, " else ");
		printdef(t->r.t->r.t, fp);
		fprintf(fp, " endif");
		break;
	default:
		fprintf(fp, "Unknown operator!");
		break;
	}
}

/*
 printsym - print the type, variable name and the definition
 of the variable pointed by v on file fp
 */
void printsym(symbol * v, FILE * fp) {
	if (v->type != UNKNOWN) {
		if (v->def) {
			/* print the type and variable name */
			fprintf(fp, "%s %s = ", scoutType(v->type), v->name);
			/* print the definition */
			printdef(v->def, fp);
			fprintf(fp, ";\n");
		} else {
			/* print the type and variable name */
			fprintf(fp, "%s %s;\n", scoutType(v->type), v->name);
		}
	}
}
