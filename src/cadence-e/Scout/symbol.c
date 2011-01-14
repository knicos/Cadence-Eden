/*
 * $Id: symbol.c,v 1.14 2001/08/02 16:26:11 cssbz Exp $
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

static char rcsid[] = "$Id: symbol.c,v 1.14 2001/08/02 16:26:11 cssbz Exp $";

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../../../config.h"
#include "symbol.h"
#include "tree.h"
#include "parser.h"
#include "../EX/script.h"
#include <tk.h>

#include "../Eden/emalloc.h"
#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

extern Tcl_Interp *interp;
extern char *progname;

extern void scout_err(char *);
extern void appendEden(char *, Script *);
extern char *scoutType(int);
extern void storeSym(symbol *);
extern void printsym(symbol *, FILE *);
extern void *getheap(int);
#ifdef DISTRIB
/* for distributed tkEden --sun */
/*
 Perhaps we don't need this [Ash]
 extern void substr(void);
 */
#endif /* DISTRIB */

Script *st_script;
static char edenStr[160]; /* Increased from 80 as the "VB" code
 below writes more than 80
 characters into this array
 sometimes, depending on the length
 of Scout symbol names that we have
 hanging around :(.  Really could do
 with a better solution to
 this. [Ash]. */
static char ss[2];

char scoutErrorStr[80];

/* function prototypes */
int hash(char *);
symbol *new_sym(int, char *);
symbol *lookUp(char *);
void declare_image_action(char *);
int declare(int, tree *);
void appendTreeList(tree *);
void clearTreeList(void);
void translate(tree *, int);
void define(tree *);
void listsym(char *);

#ifdef USE_TCL_CONST84_OPTION
int dumpscout(ClientData, Tcl_Interp *, int, CONST84 char *[]);
#else
int dumpscout(ClientData, Tcl_Interp *, int, char *[]);
#endif



static int symbolcmp(/* symbol **, symbol ** */);
#ifdef DISTRIB
void propagateScoutScr(char *, char *); /*  for distributed --sun */
static int scoutDefAgencyCheck(tree *, int);
static void checkScoutDefOracle(tree *, int);
#endif /* DISTRIB */

#ifdef __STDC__
#define out(func, type) \
	appendEden(#func, st_script); \
	appendEden("(", st_script); \
	translate(t->l.t, type); \
	appendEden(")", st_script)

#define dot(i) \
	appendEden("dotint(", st_script); \
	translate(t->l.t, WINVAR); \
	appendEden(", ", st_script); \
	appendEden(#i, st_script); \
	appendEden(")", st_script)
#else
#define out(func, type) \
	appendEden("func(", st_script); \
	translate(t->l.t, type); \
	appendEden(")", st_script)

#define dot(i) \
	appendEden("dotint(", st_script); \
	translate(t->l.t, WINVAR); \
	appendEden(", i)", st_script)
#endif

/*
 symalloc - allocate space for a new entry of symbol table
 */
#define symalloc() (symbol *)emalloc(sizeof(symbol))

/*
 table[] - symbol table
 */
static symbol *table[26];

/*
 hash - hashing function which maps from the first two characters
 of a variable to one of the 26 lists in table[]
 */
int hash(char *name) {
	return ( (isupper(name[0]) ? tolower(name[0]) - 'a' : name[0] - 'a')
			+ name[1] ) % 26;
}

/*
 new_sym - add a variable, name, to the list table[h]
 */
symbol * new_sym(int h, char *name) {
	symbol *v;

	v = symalloc();
	v->next = table[h];
	table[h] = v;
	v->name = (char *) emalloc(sizeof(char) * (strlen(name) + 1));
	strcpy(v->name, name);
	v->type = UNKNOWN;
	v->def = 0;
	return v;
}

/*
 lookUp - find the location of the variable name, in the
 symbol table.  If it is not there, create one
 */
symbol * lookUp(char *name) {
	int h;
	symbol *v;

	h = hash(name);
	for (v = table[h]; v != 0; v = v->next) {
		/* Not sure if the 'v &&' below is required, but safer to leave
		 it [Ash] */
		if (v && !strcmp(v->name, name))
			break;
	}
	return v ? v : new_sym(h, name);
}

/* Install a new symbol of type integer into the symbol table with the
 stated initial value [Ash] */
void installIntVar(char *name, int value) {
	symbol * sym;

	sym = lookUp(name);
	sym->type = INTVAR;
	sym->def = int_tree((double)value);
}

/* generating Eden action for handling image */
void declare_image_action(char *name) { /* the image variable name */
	char s[256]; /* buffer for generating eden script */

	sprintf(s, "proc P_%s : I_%s, %s { ShowImage(&I_%s, &%s); }\n", name, name, name, name, name);
	appendEden(s, st_script);
}

/*
 list is the parse tree denoting a list of variable of type unknown.
 declare - declare the variables in list to be of type `type'
 */
int declare(int type, tree * list) {
#ifdef DISTRIB
	extern int handle_check1(char *);
#endif /* DISTRIB */
	tree *p;

	
#ifdef DISTRIB
	for (p = list; p->op == ','; p = p->l.t) {
		if (handle_check1(p->r.t->l.v->name)) { /* declare is allowed --sun */
			if (p->r.t->l.v->type == UNKNOWN) {
				/* declare the type of the variable */
				p->r.t->l.v->type = type;
				if (type == IMGVAR)
				declare_image_action(p->r.t->l.v->name);
			} else if (p->r.t->l.v->type != type) {
				sprintf(scoutErrorStr, "can't redeclare %s %s to %s\n",
						scoutType(p->r.t->l.v->type),
						p->r.t->l.v->name, scoutType(type));
				return 0;
			}
			/*send ScoutDeclaration to others --sun */

			sprintf(edenStr, "%%scout\n%s %s;\n%%eden",
					scoutType(p->r.t->l.v->type), p->r.t->l.v->name);
			propagateScoutScr(p->r.t->l.v->name, edenStr);
		}
	}
	if (handle_check1(p->l.v->name)) { /* declare is allowed */
		if (p->l.v->type == UNKNOWN) {
			/* declare the type of the variable */
			p->l.v->type = type;
			if (type == IMGVAR)
			declare_image_action(p->l.v->name);
		} else if (p->l.v->type != type) {
			sprintf(scoutErrorStr, "can't redeclare %s %s to %s\n",
					scoutType(p->l.v->type), p->l.v->name, scoutType(type));
			return 0;
		}
		/*send ScoutDeclaration to others --sun */

		sprintf(edenStr, "%%scout\n%s %s;\n%%eden",
				scoutType(p->l.v->type), p->l.v->name);
		propagateScoutScr(p->l.v->name, edenStr);
		return 1;
	}
#else /* not DISTRIB */
	for (p = list; p->op == ','; p = p->l.t) {
		if (p->r.t->l.v->type == UNKNOWN) {
			/* declare the type of the variable */
			p->r.t->l.v->type = type;
			if (type == IMGVAR)
				declare_image_action(p->r.t->l.v->name);
		} else if (p->r.t->l.v->type != type) {
			sprintf(scoutErrorStr, "can't redeclare %s %s to %s\n",
					scoutType(p->r.t->l.v->type), p->r.t->l.v->name,
					scoutType(type));
			return 0;
		}
	}
	if (p->l.v->type == UNKNOWN) {
		/* declare the type of the variable */
		p->l.v->type = type;
		if (type == IMGVAR)
			declare_image_action(p->l.v->name);
	} else if (p->l.v->type != type) {
		sprintf(scoutErrorStr, "can't redeclare %s %s to %s\n",
				scoutType(p->l.v->type), p->l.v->name, scoutType(type));
		return 0;
	}
	return 1;
#endif /* DISTRIB */
}

static int stage = 1; /* controlling the translation process */
static char *defname; /* name of the variable to be defined */
static tree **TreeList; /* list of pointers to the image expressions */
static int treeListSize = 0;
static int TopTreeList = 0;
static char tempname[80]; /* name of a temp. image variable */
#ifdef DISTRIB
/* name of the variable to be given at first */
/*
 Perhaps not needed [Ash]
 static char *givenName;
 */
#endif /* DISTRIB */

void appendTreeList(tree * t) {
	if (treeListSize == 0) {
		treeListSize = 4;
		TreeList = (tree **) emalloc(sizeof(tree *) * treeListSize);
	} else if (TopTreeList == treeListSize) {
		treeListSize += 4;
		TreeList = (tree **) erealloc(TreeList, sizeof(tree *) * treeListSize);
	}
	TreeList[TopTreeList++] = t;
}

void clearTreeList(void) {
	TopTreeList = 0;
}

/*
 translate - translate the tree t to EDEN notation.
 the tree is expected to denote an expression of type `type'
 */
void translate(tree * t, int type) {
	WinStruct w;
	char *s;
	tree *arg;
	extern int inPrefix;
	extern char agentName[];
	/* For VB style features [Ash] */
	char autogen_proc_name[128], procClickName[128];

	ss[1] = '\0';
	if (!t) {
		appendEden("@", st_script);
		return;
	}
	switch (t->op) {
	case '=':
		defname = t->l.v->name;
		if (type == IMGVAR) {
			sprintf(edenStr, "I_%s is ", defname);
		} else {
			sprintf(edenStr, "%s is ", defname);
		}
		appendEden(edenStr, st_script);
		translate(t->r.t, type);
		break;
	case NUMBER:
		sprintf(edenStr, (t->l.i == (int)t->l.i) ? "%.1f" : "%f", t->l.i);
		appendEden(edenStr, st_script);
		break;
	case ROW:
		out(row, INTVAR)
		;
		break;
	case COLUMN:
		out(column, INTVAR)
		;
		break;
	case STR:
		ss[0] = '"';
		appendEden(ss, st_script);
		s = t->l.s;
		while (*s) {
			if (*s == '\\') { /* protect \ */
				s++;
				appendEden("\\\\", st_script);
			} else if (*s == '"') { /* protect " */
				s++;
				appendEden("\\\"", st_script);
			} else {
				ss[0] = *s++;
				appendEden(ss, st_script);
			}
		}
		ss[0] = '"';
		appendEden(ss, st_script);
		break;
	case UNKNOWN:
		scout_err("unknown operator");
		break;
	case STRVAR:
	case INTVAR:
	case PTVAR:
	case BOXVAR:
	case FRAMEVAR:
	case WINVAR:
	case DISPVAR:
		sprintf(edenStr, "%s", t->l.v->name);
		appendEden(edenStr, st_script);
		break;
	case IMGVAR:
		sprintf(edenStr, "I_%s", t->l.v->name);
		appendEden(edenStr, st_script);
		break;
	case FORMPT:
		appendEden("[", st_script);
		translate(t->l.t, INTVAR);
		appendEden(", ", st_script);
		translate(t->r.t, INTVAR);
		appendEden("]", st_script);
		break;
	case FORMBOX:
		appendEden("formbox(", st_script);
		translate(t->l.t, PTVAR);
		appendEden(", ", st_script);
		translate(t->r.t, PTVAR);
		appendEden(")", st_script);
		break;
	case TEXTBOX:
		appendEden("textbox(", st_script);
		translate(t->l.t, PTVAR);
		appendEden(", ", st_script);
		translate(t->r.t->l.t, INTVAR);
		appendEden(", ", st_script);
		translate(t->r.t->r.t, INTVAR);
		appendEden(")", st_script);
		break;
	case FORMFRAME:
		appendEden("[", st_script);
		translate(t->l.t, BOXVAR);
		appendEden("]", st_script);
		break;
	case FORMWIN:
		w = t->l.w;

		/* Eden list index 1: type */
		sprintf(edenStr, "[%d, ", w.type);
		appendEden(edenStr, st_script);

		/* Eden list index 2: frame */
		if (w.frame)
			translate(w.frame, FRAMEVAR);
		else
			appendEden("[[0,0,100,100]]", st_script);
		appendEden(", ", st_script);

		/* Eden list index 3: string */
		if (w.string)
			translate(w.string, STRVAR);
		else
			appendEden("\"\"", st_script);
		appendEden(", ", st_script);

		/* Eden list index 4: box */
		if (w.box)
			translate(w.box, BOXVAR);
		else
			appendEden("[0,0,100,100]", st_script);
		appendEden(", ", st_script);

		/* Eden list index 5: pict */
		if (w.pict)
			translate(w.pict, STRVAR);
		else
			appendEden("\"pict1\"", st_script);
		appendEden(", ", st_script);

		/* Eden list index 6: xmin */
		if (w.xmin)
			translate(w.xmin, INTVAR);
		else
			appendEden("DFxmin", st_script);
		appendEden(", ", st_script);

		/* Eden list index 7: ymin */
		if (w.ymin)
			translate(w.ymin, INTVAR);
		else
			appendEden("DFymin", st_script);
		appendEden(", ", st_script);

		/* Eden list index 8: xmax */
		if (w.xmax)
			translate(w.xmax, INTVAR);
		else
			appendEden("DFxmax", st_script);
		appendEden(", ", st_script);

		/* Eden list index 9: ymax */
		if (w.ymax)
			translate(w.ymax, INTVAR);
		else
			appendEden("DFymax", st_script);
		appendEden(", ", st_script);

		/* Eden list index 10: bgcolor */
		if (w.bgcolor)
			translate(w.bgcolor, STRVAR);
		else
			appendEden("DFbgcolor", st_script);
		appendEden(", ", st_script);

		/* Eden list index 11: fgcolor */
		if (w.fgcolor)
			translate(w.fgcolor, STRVAR);
		else
			appendEden("DFfgcolor", st_script);
		appendEden(", ", st_script);

		/* Eden list index 12: border */
		if (w.border)
			translate(w.border, INTVAR);
		else
			appendEden("DFborder", st_script);
		appendEden(", ", st_script);

		/* Eden list index 13: align */
		if (w.align != -1) {
			sprintf(edenStr, "%d", w.align);
			appendEden(edenStr, st_script);
		} else
			appendEden("DFalign", st_script);
		appendEden(", ", st_script);

		/* Eden list index 14: sensitive */
		if (w.sensitive)
			translate(w.sensitive, INTVAR);
		else
			appendEden("DFsensitive", st_script);
		appendEden(", ", st_script);

		/* Eden list index 15: bdcolor */
		if (w.bdcolor)
			translate(w.bdcolor, STRVAR);
		else
			appendEden("DFbdcolor", st_script);
		appendEden(", ", st_script);

		/* Eden list index 16: font */
		if (w.font)
			translate(w.font, STRVAR);
		else
			appendEden("DFfont", st_script);
		appendEden(", ", st_script);

		/* Eden list index 17: bdtype */
		if (w.bdtype)
			translate(w.bdtype, STRVAR);
		else
			appendEden("DFrelief", st_script);

		/* Eden list index 18: defname */
		sprintf(edenStr, ", \"%s\"", defname);
		appendEden(edenStr, st_script);

		/*if (w.type == 4) {
		 sprintf(edenStr, ", %s_TEXT_1", defname);
		 appendEden(edenStr, st_script);
		 } */
		appendEden("]", st_script);

		/* for like-VB interface  --sun */
		if (w.sensitive && (w.type == 4 || w.type == 0 || w.type == 1)) {
			*procClickName = 0;
			if (inPrefix) {
				strcpy(procClickName, defname);
				strcat(procClickName, "_click");
			} else {
				strncat(procClickName, defname, strlen(defname)
						-strlen(agentName));
				strcat(procClickName, "click_");
				strcat(procClickName, agentName);
			}
			if (lookup(procClickName) == 0) {
				sprintf(edenStr, ";\nproc %s {}", procClickName);
				appendEden(edenStr, st_script);
			}
			/* *autogen_proc_name = 0;
			 strcpy(autogen_proc_name, defname);
			 strcat(autogen_proc_name, "_mouseDown");
			 if (lookup(autogen_proc_name) == 0) {
			 sprintf(edenStr, ";\nproc %s_mouseDown {}", defname);
			 appendEden(edenStr, st_script);
			 } *//*   proc for MouseDown   */
			sprintf(edenStr, ";\nproc %s_mouseButtonPress : ", defname);
			appendEden(edenStr, st_script);
			if (w.type ==1)
				sprintf(edenStr, "%s_mouse, %s_mouseClick {\n", defname,
						defname);
			else
				sprintf(edenStr, "%s_mouse_1, %s_mouseClick {\n", defname,
						defname);
			appendEden(edenStr, st_script);
			sprintf(edenStr, "   if (%s_mouseClick) {\n", defname);
			appendEden(edenStr, st_script);
			sprintf(edenStr, "       %s();\n", procClickName);
			appendEden(edenStr, st_script);
			sprintf(edenStr, "       %s_mouseClick = FALSE;\n", defname);
			appendEden(edenStr, st_script);
			/* sprintf(edenStr, "       %s_mouseDown(", defname);
			 appendEden(edenStr, st_script);
			 sprintf(edenStr, "%s_mouse_1[4], ", defname);
			 appendEden(edenStr, st_script);
			 sprintf(edenStr, "%s_mouse_1[5]);\n", defname);
			 appendEden(edenStr, st_script);  *//* proc for MouseDown */
			appendEden("    }", st_script);
			appendEden("}", st_script);
		}
		if (w.type == 4) {
			*autogen_proc_name = 0;
			strcpy(autogen_proc_name, defname);
			strcat(autogen_proc_name, "_change");
			if (lookup(autogen_proc_name) == 0) {
				sprintf(edenStr, ";\nproc %s_change {}", defname);
				appendEden(edenStr, st_script);
			}
			sprintf(edenStr, ";\nproc %s_textCHANGE : %s_TEXT_1 {\n", defname, defname);
			appendEden(edenStr, st_script);
			sprintf(edenStr, "   %s_change();\n}", defname);
			appendEden(edenStr, st_script);
			sprintf(edenStr, ";\nproc %s_setText {\n", defname);
			appendEden(edenStr, st_script);
			sprintf(edenStr, "   auto currText;\n\n", defname);
			appendEden(edenStr, st_script);
			sprintf(edenStr, "   currText = $1;\n", defname);
			appendEden(edenStr, st_script);
			sprintf(edenStr, "   tcl(\"$%s_boxName delete 1.0 end\");\n", defname);
			appendEden(edenStr, st_script);
			sprintf(edenStr, "   if ($1!=@) {\n      tcl(\"$%s_boxName insert end \\\"\"//$1//\"\\\"\");", defname);
			appendEden(edenStr, st_script);
			sprintf(edenStr, "\n      %s_TEXT_1 = currText;\n   }\n}", defname);
			appendEden(edenStr, st_script);
			sprintf(edenStr, ";\nproc %s_getText {\n", defname);
			appendEden(edenStr, st_script);
			sprintf(edenStr, "   return %s_TEXT_1;\n", defname);
			appendEden(edenStr, st_script);
			appendEden("}", st_script);
		}
		break;
	case FORMDISP:
		appendEden("[", st_script);
		translate(t->l.t, INTVAR);
		appendEden("]", st_script);
		break;
	case CONCAT:
		translate(t->l.t, STRVAR);
		appendEden(" // ", st_script);
		translate(t->r.t, STRVAR);
		break;
	case STRCAT:
		appendEden("strcat(", st_script);
		translate(t->l.t, STRVAR);
		appendEden(", ", st_script);
		translate(t->r.t, STRVAR);
		appendEden(")", st_script);
		break;
	case STRLEN:
		appendEden("((", st_script);
		translate(t->l.t, STRVAR);
		appendEden(")#)", st_script);
		break;
	case SUBSTR:
		appendEden("substr(", st_script);
		translate(t->l.t, STRVAR);
		appendEden(", ", st_script);
		translate(t->r.t->l.t, INTVAR);
		appendEden(", ", st_script);
		translate(t->r.t->r.t, INTVAR);
		appendEden(")", st_script);
		break;
	case TOSTRING:
		appendEden("str(", st_script);
		translate(t->l.t, INTVAR);
		appendEden(")", st_script);
		break;
	case BOXSHIFT:
		appendEden("boxshift(", st_script);
		translate(t->l.t, BOXVAR);
		appendEden(", ", st_script);
		translate(t->r.t->l.t, INTVAR);
		appendEden(", ", st_script);
		translate(t->r.t->r.t, INTVAR);
		appendEden(")", st_script);
		break;
	case BOXINTERSECT:
		appendEden("boxop(", st_script);
		translate(t->l.t, BOXVAR);
		appendEden(", ", st_script);
		translate(t->r.t, BOXVAR);
		appendEden(", BOXINTERSECT)", st_script);
		break;
	case BOXCENTRE:
		appendEden("boxop(", st_script);
		translate(t->l.t, BOXVAR);
		appendEden(", ", st_script);
		translate(t->r.t, BOXVAR);
		appendEden(", BOXCENTRE)", st_script);
		break;
	case BOXENCLOSING:
		appendEden("boxop(", st_script);
		translate(t->l.t, BOXVAR);
		appendEden(", ", st_script);
		translate(t->r.t, BOXVAR);
		appendEden(", BOXENCLOSING)", st_script);
		break;
	case BOXREDUCE:
		appendEden("boxop(", st_script);
		translate(t->l.t, BOXVAR);
		appendEden(", ", st_script);
		translate(t->r.t, BOXVAR);
		appendEden(", BOXREDUCE)", st_script);
		break;
	case DOTNE:
		out(dotne, BOXVAR)
		;
		break;
	case DOTNW:
		out(dotnw, BOXVAR)
		;
		break;
	case DOTSE:
		out(dotse, BOXVAR)
		;
		break;
	case DOTSW:
		out(dotsw, BOXVAR)
		;
		break;
	case DOTN:
		out(dotn, BOXVAR)
		;
		break;
	case DOTE:
		out(dote, BOXVAR)
		;
		break;
	case DOTS:
		out(dots, BOXVAR)
		;
		break;
	case DOTW:
		out(dotw, BOXVAR)
		;
		break;
	case '.':
		appendEden("dotint(", st_script);
		translate(t->l.t, PTVAR);
		appendEden(", ", st_script);
		translate(t->r.t, INTVAR);
		appendEden(")", st_script);
		break;
	case DOTTYPE:
		dot(1)
		;
		break;
	case DOTFRAME:
		dot(2)
		;
		break;
	case DOTSTR:
		dot(3)
		;
		break;
	case DOTBOX:
		dot(4)
		;
		break;
	case DOTPICT:
		dot(5)
		;
		break;
	case DOTXMIN:
		dot(6)
		;
		break;
	case DOTYMIN:
		dot(7)
		;
		break;
	case DOTXMAX:
		dot(8)
		;
		break;
	case DOTYMAX:
		dot(9)
		;
		break;
	case DOTBG:
		dot(10)
		;
		break;
	case DOTFG:
		dot(11)
		;
		break;
	case DOTBDCOLOR:
		dot(15)
		;
		break;
	case DOTBORDER:
		dot(12)
		;
		break;
	case DOTALIGN:
		dot(13)
		;
		break;
	case DOTSENSITIVE:
		dot(14)
		;
		break;
	case DOTFONT:
		dot(16)
		;
		break;
	case DOTBDTYPE:
		dot(17)
		;
		break;
	case IMGFUNC:
		if (stage == 1) { /* replace the function with a variable */
			appendTreeList(t);
			sprintf(tempname, "I_%s_temp%d", defname, TopTreeList);
			appendEden(tempname, st_script);
			break;
		}
		/* stage 2 - actually translate the image function */
		stage = 1; /* suppress expanding image
		 sub-expressions */
		appendEden(t->l.s, st_script);
		appendEden("(\"", st_script);
		appendEden(tempname, st_script);
		appendEden("\"", st_script);
		for (arg = t->r.t; arg->op != 0; arg = arg->r.t) {
			appendEden(", ", st_script);
			translate(arg->l.t, arg->op);
		}
		appendEden(")", st_script);
		break;
	case APPEND:
		appendEden("app(", st_script);
		translate(t->l.t, type);
		appendEden(", ", st_script);
		translate(t->r.t->l.t, INTVAR);
		appendEden(", ", st_script);
		translate(t->r.t->r.t, type == DISPVAR ? WINVAR : BOXVAR);
		appendEden(")", st_script);
		break;
	case DELETE:
		appendEden("del(", st_script);
		translate(t->l.t, type);
		appendEden(", ", st_script);
		translate(t->r.t, INTVAR);
		appendEden(")", st_script);
		break;
	case '&': /* new function for appending a window to a screen --sun */
		/* appendEden("list_append(", st_script); */
		/* using list_append easily causes the problem of heap
		 overflow. -- sun */
		translate(t->l.t, type);
		/* appendEden(", ", st_script); */
		appendEden(" // ", st_script);
		translate(t->r.t, type);
		/* appendEden(")", st_script); */
		break;
	case '(':
		appendEden("(", st_script);
		translate(t->l.t, INTVAR);
		appendEden(")", st_script);
		break;
	case '>':
	case ',':
		translate(t->l.t, type);
		appendEden(", ", st_script);
		translate(t->r.t, type);
		break;
	case UMINUS:
		appendEden("-", st_script);
		translate(t->l.t, INTVAR);
		break;
	case '+':
		if (type == INTVAR) {
			translate(t->l.t, type);
			appendEden(" + ", st_script);
			translate(t->r.t, type);
		} else {
			appendEden("pt_add(", st_script);
			translate(t->l.t, PTVAR);
			appendEden(", ", st_script);
			translate(t->r.t, PTVAR);
			appendEden(")", st_script);
		}
		break;
	case '-':
		if (type == INTVAR) {
			translate(t->l.t, type);
			appendEden(" - ", st_script);
			translate(t->r.t, type);
		} else {
			appendEden("pt_subtract(", st_script);
			translate(t->l.t, PTVAR);
			appendEden(", ", st_script);
			translate(t->r.t, PTVAR);
			appendEden(")", st_script);
		}
		break;
	case '*':
		translate(t->l.t, INTVAR);
		appendEden(" * ", st_script);
		translate(t->r.t, INTVAR);
		break;
	case '/':
		appendEden("float(", st_script);
		translate(t->l.t, INTVAR);
		appendEden(") / ", st_script);
		translate(t->r.t, INTVAR);
		break;
	case '%':
		translate(t->l.t, INTVAR);
		appendEden(" % ", st_script);
		translate(t->r.t, INTVAR);
		break;
	case EQ:
		translate(t->l.t, INTVAR);
		appendEden(" == ", st_script);
		translate(t->r.t, INTVAR);
		break;
	case NE:
		translate(t->l.t, INTVAR);
		appendEden(" != ", st_script);
		translate(t->r.t, INTVAR);
		break;
	case LT:
		translate(t->l.t, INTVAR);
		appendEden(" < ", st_script);
		translate(t->r.t, INTVAR);
		break;
	case LE:
		translate(t->l.t, INTVAR);
		appendEden(" <= ", st_script);
		translate(t->r.t, INTVAR);
		break;
	case GT:
		translate(t->l.t, INTVAR);
		appendEden(" > ", st_script);
		translate(t->r.t, INTVAR);
		break;
	case GE:
		translate(t->l.t, INTVAR);
		appendEden(" >= ", st_script);
		translate(t->r.t, INTVAR);
		break;
	case OR:
		translate(t->l.t, INTVAR);
		appendEden(" || ", st_script);
		translate(t->r.t, INTVAR);
		break;
	case AND:
		translate(t->l.t, INTVAR);
		appendEden(" && ", st_script);
		translate(t->r.t, INTVAR);
		break;
	case IF:
		appendEden("(int(", st_script);
		translate(t->l.t, INTVAR);
		appendEden(") ? ", st_script);
		translate(t->r.t->l.t, t->r.t->op);
		appendEden(" : ", st_script);
		translate(t->r.t->r.t, t->r.t->op);
		appendEden(")", st_script);
		break;
	default:
		scout_err("unknown operator");
		break;
	}
}

/*
 t is assumed to be the parse tree of a definition
 define - translate the parse tree into EDEN definition
 */
void define(tree * t) {
	int start, top, i;
	extern int useOldTree; /* for the &= operator, we need to check
	 whether a display tree already exists
	 that we need to append to [Ash, with Patrick] */
#ifdef DISTRIB
	extern int handle_check1(char *);
	extern void propagateScoutDef(tree *);
#endif /* DISTRIB */

	if (t->l.v->def && !useOldTree)
		freetree(t->l.v->def);
	stage = 1; /* to control the translation process
	 translate image function as a temporary
	 variable */
#ifdef DISTRIB
	if (!handle_check1(t->l.v->name)) /* not be allowed to change --sun */
	return;
	if (!scoutDefAgencyCheck(t->r.t, t->l.v->type))
	/* some variables are not allowed to be observed --sun */
	return;
#endif /* DISTRIB */
	t->l.v->def = t->r.t;
	translate(t, t->l.v->type);
	appendEden(";\n", st_script);
	top = 0;
	do {
		start = top + 1;
		top = TopTreeList;
		for (i = TopTreeList; i >= start; --i) {
			sprintf(tempname, "I_%s_temp%d", t->l.v->name, i);
			stage = 2; /* to control the translation process;
			 truly transate the image functions;
			 stage may reset to 1 if the expr
			 contains image subexpressions */
			appendEden(tempname, st_script);
			appendEden(" is ", st_script);
			translate(TreeList[i - 1], IMGVAR);
			appendEden(";\n", st_script);
		}
	} while (top != TopTreeList); /* if there are image subexpr */
	clearTreeList();

#ifdef DISTRIB
	propagateScoutDef(t); /* propagate Scout Definition to others --sun */
#endif /* DISTRIB */

	free(t);

	useOldTree = 0;
}

/*
 listsym - if s is 0 then list all variables in symbol table
 else s is a variable name which is going to be
 listed out
 */
void listsym(char *s) {
	int i;
	symbol *p;

	if (s == 0)
		for (i = 0; i < 26; i++)
			for (p = table[i]; p != 0; p = p->next)
				printsym(p, stderr);
	else
		printsym(lookUp(s), stderr);
}

#ifdef USE_TCL_CONST84_OPTION
int dumpscout(ClientData clientData, Tcl_Interp * interp, int argc, CONST84 char *argv[])
#else
int dumpscout(ClientData clientData, Tcl_Interp * interp, int argc,
		char *argv[])
#endif

{
	extern char *hptr;
	char *saveHeapPtr;
	int i, count;
	symbol *sp;
	symbol **p, **pptr;

	count = 0;
	for (i = 0; i < 26; i++)
		for (sp = table[i]; sp != 0; sp = sp->next)
			count++;
	saveHeapPtr = hptr;
	p = (symbol **) getheap(count * sizeof(symbol *));

	pptr = p;
	for (i = 0; i < 26; i++)
		for (sp = table[i]; sp != 0; sp = sp->next)
			*pptr++ = sp;

	qsort(p, count, sizeof(symbol *), symbolcmp);

	Tcl_EvalEC(interp, "cleanup scout");
	for (i = 0; i < count; i++) {
		storeSym(p[i]);
	}
	hptr = saveHeapPtr;
	Tcl_EvalEC(interp, ".scout.t.text mark set insert 1.0");
	Tcl_EvalEC(interp, ".scout.t.text see insert");
	return TCL_OK;
}

static int symbolcmp(symbol ** s1, symbol ** s2) {
	return strcmp((*s1)->name, (*s2)->name);
}









#ifdef DISTRIB
/*--------------------------------------------------------------------------*/
/*    for agency --sun */

static int ScoutDefOracle;

static void
checkScoutDefOracle(tree * t, int type)
{
	extern int oracle_check1(char *);
	WinStruct w;
	char *s;
	tree *arg;

	if (!t) {
		return;
	}
	switch (t->op) {
		case '=':
		checkScoutDefOracle(t->r.t, type);
		break;
		case NUMBER:
		break;
		case ROW:
		case COLUMN:
		checkScoutDefOracle(t->l.t, INTVAR);
		break;
		case STR:
		break;
		case UNKNOWN:
		scout_err("unknown operator");
		break;
		case STRVAR:
		case INTVAR:
		case PTVAR:
		case BOXVAR:
		case FRAMEVAR:
		case WINVAR:
		case DISPVAR:
		case IMGVAR:
		if (!oracle_check1(t->l.v->name)) ScoutDefOracle = 0;
		break;
		case FORMPT:
		checkScoutDefOracle(t->l.t, INTVAR);
		checkScoutDefOracle(t->r.t, INTVAR);
		break;
		case FORMBOX:
		checkScoutDefOracle(t->l.t, PTVAR);
		checkScoutDefOracle(t->r.t, PTVAR);
		break;
		case TEXTBOX:
		checkScoutDefOracle(t->l.t, PTVAR);
		checkScoutDefOracle(t->r.t->l.t, INTVAR);
		checkScoutDefOracle(t->r.t->r.t, INTVAR);
		break;
		case FORMFRAME:
		checkScoutDefOracle(t->l.t, BOXVAR);
		break;
		case FORMWIN:
		w = t->l.w;
		if (w.frame)
		checkScoutDefOracle(w.frame, FRAMEVAR);
		if (w.string)
		checkScoutDefOracle(w.string, STRVAR);
		if (w.box)
		checkScoutDefOracle(w.box, BOXVAR);
		if (w.pict)
		checkScoutDefOracle(w.pict, STRVAR);
		if (w.xmin)
		checkScoutDefOracle(w.xmin, INTVAR);
		if (w.ymin)
		checkScoutDefOracle(w.ymin, INTVAR);
		if (w.xmax)
		checkScoutDefOracle(w.xmax, INTVAR);
		if (w.ymax)
		checkScoutDefOracle(w.ymax, INTVAR);
		if (w.bgcolor)
		checkScoutDefOracle(w.bgcolor, STRVAR);
		if (w.fgcolor)
		checkScoutDefOracle(w.fgcolor, STRVAR);
		if (w.border)
		checkScoutDefOracle(w.border, INTVAR);
		if (w.bdcolor)
		checkScoutDefOracle(w.bdcolor, STRVAR);
		if (w.font)
		checkScoutDefOracle(w.font, STRVAR);
		if (w.bdtype)
		checkScoutDefOracle(w.bdtype, STRVAR);
		break;
		case FORMDISP:
		checkScoutDefOracle(t->l.t, INTVAR);
		break;
		case CONCAT:
		case STRCAT:
		checkScoutDefOracle(t->l.t, STRVAR);
		checkScoutDefOracle(t->r.t, STRVAR);
		break;
		case STRLEN:
		checkScoutDefOracle(t->l.t, STRVAR);
		break;
		case SUBSTR:
		checkScoutDefOracle(t->l.t, STRVAR);
		checkScoutDefOracle(t->r.t->l.t, INTVAR);
		checkScoutDefOracle(t->r.t->r.t, INTVAR);
		break;
		case TOSTRING:
		checkScoutDefOracle(t->l.t, INTVAR);
		break;
		case BOXSHIFT:
		checkScoutDefOracle(t->l.t, BOXVAR);
		checkScoutDefOracle(t->r.t->l.t, INTVAR);
		checkScoutDefOracle(t->r.t->r.t, INTVAR);
		break;
		case BOXINTERSECT:
		case BOXCENTRE:
		case BOXENCLOSING:
		case BOXREDUCE:
		checkScoutDefOracle(t->l.t, BOXVAR);
		checkScoutDefOracle(t->r.t, BOXVAR);
		break;
		case DOTNE:
		case DOTNW:
		case DOTSE:
		case DOTSW:
		case DOTN:
		case DOTE:
		case DOTS:
		case DOTW:
		checkScoutDefOracle(t->l.t, BOXVAR);
		break;
		case '.':
		checkScoutDefOracle(t->l.t, PTVAR);
		checkScoutDefOracle(t->r.t, INTVAR);
		break;
		case DOTTYPE:
		case DOTFRAME:
		case DOTSTR:
		case DOTBOX:
		case DOTPICT:
		case DOTXMIN:
		case DOTYMIN:
		case DOTXMAX:
		case DOTYMAX:
		case DOTBG:
		case DOTFG:
		case DOTBDCOLOR:
		case DOTBORDER:
		case DOTALIGN:
		case DOTSENSITIVE:
		case DOTFONT:
		case DOTBDTYPE:
		checkScoutDefOracle(t->l.t, WINVAR);
		break;
		case IMGFUNC:
		for (arg = t->r.t; arg->op != 0; arg = arg->r.t) {
			checkScoutDefOracle(arg->l.t, arg->op);
		}
		break;
		case APPEND:
		checkScoutDefOracle(t->l.t, type);
		checkScoutDefOracle(t->r.t->l.t, INTVAR);
		checkScoutDefOracle(t->r.t->r.t,
				type == DISPVAR ? WINVAR : BOXVAR);
		break;
		case DELETE:
		checkScoutDefOracle(t->l.t, type);
		checkScoutDefOracle(t->r.t, INTVAR);
		break;
		case '&':
		checkScoutDefOracle(t->l.t, type);
		checkScoutDefOracle(t->r.t, type);
		break;
		case '(':
		checkScoutDefOracle(t->l.t, INTVAR);
		break;
		case '>':
		case ',':
		checkScoutDefOracle(t->l.t, type);
		checkScoutDefOracle(t->r.t, type);
		break;
		case UMINUS:
		checkScoutDefOracle(t->l.t, INTVAR);
		break;
		case '+':
		case '-':
		if (type == INTVAR) {
			checkScoutDefOracle(t->l.t, type);
			checkScoutDefOracle(t->r.t, type);
		} else {
			checkScoutDefOracle(t->l.t, PTVAR);
			checkScoutDefOracle(t->r.t, PTVAR);
		}
		break;
		case '*':
		case '/':
		case '%':
		case EQ:
		case NE:
		case LT:
		case LE:
		case GT:
		case GE:
		case OR:
		case AND:
		checkScoutDefOracle(t->l.t, INTVAR);
		checkScoutDefOracle(t->r.t, INTVAR);
		break;
		case IF:
		checkScoutDefOracle(t->l.t, INTVAR);
		checkScoutDefOracle(t->r.t->l.t, t->r.t->op);
		checkScoutDefOracle(t->r.t->r.t, t->r.t->op);
		break;
		default:
		scout_err("unknown operator");
		break;
	}
}

static int
scoutDefAgencyCheck(tree * def, int type)
{
	ScoutDefOracle = 1;
	checkScoutDefOracle(def, type);
	return ScoutDefOracle;
}

void
propagateScoutScr(char *name, char *s)
{
	extern void appendEden(char *, Script *);

	appendEden("propagate(\"", st_script);
	appendEden(name, st_script);
	appendEden("\", \"", st_script);
	appendEden(s, st_script);
	appendEden("\");\n", st_script);
}
#endif /* DISTRIB */
