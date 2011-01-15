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

/* This handles the encoding of Donald definitions in Eden [Ash, with Sun] */

static char rcsid[] = "$Id: symbol.c,v 1.14 2001/08/02 16:26:11 cssbz Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tk.h>

#include "../../../config.h"
#include "error.h"
#include "tree.h"
#include "symbol.h"
#include "oper.h"
#include "parser.h"
#include "../EX/script.h"

#include "../Eden/emalloc.h"
#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

#include <inttypes.h>

extern Tcl_Interp *interp;

extern void *getmem(int);
extern char *map_token_to_name(intptr_t);
#ifdef DISTRIB
extern void propagateDonaldDef(char *, char *);
extern int handle_check1(char *);
extern int withinHandle;
#endif

/* DISTRIB */

/*	Code for debugging
appendEden(char *s, Script *d)
{
appendEden(s, d); printf("%s", s);
}
*/

Script     *dd_script;
static char temp[256];

/* maximum number of openshapes */
#define MAXCONTEXT 128

int         _changed = 0;	/* to prevent multiple redefinition of _
				   on e.g. int i,j */

static symbol *GlobalSymbols = SymEnd;

static struct c {
    symbol     *sym;
    symbol    **table;
}           context[MAXCONTEXT] = {

    {
	SymEnd, &GlobalSymbols
    },
    {
	0, 0
    }
};

/* to do with monitor, now obsolete
struct d {
    char       *name;
    struct d   *next;
}          *MonTable = (struct d *) 0;
*/

#define PARENT  0
#define SymbolTable *context[indx].table
static struct c NORMAL = {SymEnd};

static int  indx = 0;		/* always points at the toppest context */
static int  backindx = 0;	/* always points at the toppest
				   backcontext */
static struct c backcontext[MAXCONTEXT];

 /* contains what change of context should takes when resume_context() */

char        viewport_name[64];

/* function prototypes */
void        change_prompt(void);
void        change_context(symbol *);
void        resume_context(void);
int         reset_context(void);
void        backup_context(void);
void        restore_context(void);
static void change_scope2(tree);
void        change_scope(tree);
void        resume_scope(void);
symbol     *symbol_search(char *);
symbol     *context_search(char *);
symbol     *new_symbol(char *);
void        free_symbol(symbol *);
symbol     *look_up(char *);
symbol     *look_up_tree(tree);
symbol     *look_prev(char *);
void        declare_openshape(void);
void        declare_action(char *, char *, char *);
void        eden_delete(symbol *, char *);
void        eden_declare(char *, int, char *);
void        Declare(int, tree);
void        DeclareGraph(tree);
void        Delete(tree);
void        print_symbol_table(int, symbol *);
void        print_all_symbols(void);
char       *donald_full_name(char *);
char       *eden_full_name(char *);
char       *expr_to_donald_name(tree);
char       *expr_to_eden_name(tree);
#ifdef DISTRIB
int         isGraph = 0;  /* variables of graph do not be propagated,
                             because we don't understand how the
                             Donald graph function works! [Ash, with
                             Sun] */
#endif /* DISTRIB */
static int  isGraphAttr(char *);
static void indent(int);
static void insert_nl(void);
static void dumpDonaldDeclaration(int, symbol *, int);
static void dumpDonaldDefinition(int, symbol *);
int         dumpdonald(void);


/*---------------------------------------------+---------------+
                                               | change_prompt |
                                               +---------------*/
void
change_prompt(void)
{				/* set dd_prompt to the current context */
    extern char *dd_prompt;
    char       *s;
    char       *t;
    int         i;
    int         len;
    int         prompt_len;

    prompt_len = strlen(dd_prompt);
    for (len = 0, i = 1; i <= indx; i++, len += strlen(t)) {
	t = context[i].sym->name;
    }
    if (prompt_len < len + 3 + indx) {
	dd_prompt = (char *) erealloc(dd_prompt, prompt_len + len + 3 + indx);
    }
    s = dd_prompt;
    strcpy(s, "[");
    s += 1;
    for (i = 1; i <= indx; i++) {
	t = context[i].sym->name;
	len = strlen(t);
	*s++ = '/';
	strcpy(s, t);
	s += len;
    }
    if (indx)
	strcpy(s, "]");
    else
	strcpy(s, "/]");
}


/*--------------------------------------------+----------------+
                                              | change_context |
                                              +----------------*/
void
change_context(symbol * sym)
{				/* change context to sym */
    if (sym == SymEnd) {	/* ~ */
	if (backindx > MAXCONTEXT)
	    don_err(StackOverflow, "context ");
	backcontext[backindx++] = context[indx];
	if (indx == 0)
	    don_err(StackUnderflow, "context ");
	else
	    --indx;
    } else {
	if (backindx > MAXCONTEXT)
	    don_err(StackOverflow, "context ");
	backcontext[backindx++] = NORMAL;
	if (indx++ >= MAXCONTEXT)
	    don_err(StackOverflow, "context ");
	context[indx].sym = sym;
	context[indx].table = &sym->derived;
    }
}

/*--------------------------------------------+----------------+
                                              | resume_context |
                                              +----------------*/
void
resume_context(void)
{
    struct c    cnt;

    if (backindx <= 0)
	don_err(StackUnderflow, "context ");
    cnt = backcontext[--backindx];
    if (cnt.sym == SymEnd) {
	if (indx == 0)
	    don_err(StackUnderflow, "context ");
	else
	    --indx;
    } else {
	if (indx++ >= MAXCONTEXT)
	    don_err(StackOverflow, "context ");
	context[indx] = cnt;
    }
}

/*---------------------------------------------+---------------+
                                               | reset_context |
                                               +---------------*/
int
reset_context(void)
{				/* cd root */
    int         ix = indx;

    indx = 0;			/* root */
    backindx = 0;
    return ix;
}

static int  MaxBackupContext = 0;
static int  backupContextSize = 0;
static struct c *backupContext = 0;
static int  MaxBackupIndx = 0;
static int  backupIndxSize = 0;
static int *backupIndx = 0;

/*--------------------------------------------+----------------+
                                              | backup_context |
                                              +----------------*/
void
backup_context(void)
{
    int         i;

    if (backupContext == 0) {
	backupContext = (struct c *) emalloc(sizeof(struct c)
					     * (backindx + indx + 1));
	MaxBackupContext = backindx + indx + 1;
    } else if (MaxBackupContext < backupContextSize + backindx + indx + 1) {
	backupContext = (struct c *) erealloc(backupContext, sizeof(struct c)
				* (backupContextSize + backindx + indx + 1));
	MaxBackupContext = backupContextSize + backindx + indx + 1;
    }
    if (backupIndx == 0) {
	backupIndx = (int *) emalloc(sizeof(int) * 2);
	MaxBackupIndx = 2;
    } else if (MaxBackupIndx < backupIndxSize + 2) {
	backupIndx = (int *) erealloc(backupIndx, sizeof(int) *
				      (backupIndxSize + 2));
	MaxBackupIndx = backupIndxSize + 2;
    }
    for (i = 0; i <= indx; i++)
	backupContext[backupContextSize + i] = context[i];
    backupIndx[backupIndxSize++] = indx;
    for (i = 0; i < backindx; i++)
	backupContext[backupContextSize + indx + 1 + i] = backcontext[i];
    backupIndx[backupIndxSize++] = backindx;
    backupContextSize += indx + 1 + backindx;
}

/*-------------------------------------------+-----------------+
                                             | restore_context |
                                             +-----------------*/
void
restore_context(void)
{
    int         i;

    for (i = (backindx = backupIndx[--backupIndxSize]) - 1; i >= 0; --i)
	backcontext[i] = backupContext[--backupContextSize];
    for (i = (indx = backupIndx[--backupIndxSize]); i >= 0; --i)
	context[i] = backupContext[--backupContextSize];
}

/*---------------------------------------------+---------------+
                                               | change_scope2 |
                                               +---------------*/
/* actually change the scope, called by change_scope */
static void
change_scope2(tree expr)
{
    /* int         ix = indx; */

    switch (expr->op) {
	case OP_ID:
	change_context(context_search((char *) Lexpr));
	break;

    case OP_SLASH:
	change_context(context_search((char *) Lexpr->left));
	change_scope2(Rexpr);
	break;

    case OP_GLOBAL:
	reset_context();
	change_scope2(Lexpr);
	break;

    default:
	restore_context();
	don_err(Impossible, "change_scope");
    }
}

/*----------------------------------------------+--------------+
                                                | change_scope |
                                                +--------------*/
void
change_scope(tree expr)
{
    backup_context();
    change_scope2(expr);
}

/*----------------------------------------------+--------------+
                                                | resume_scope |
                                                +--------------*/
void
resume_scope(void)
{
    restore_context();
}

/*---------------------------------------------+---------------+
                                               | symbol_search |
                                               +---------------*/
symbol     *
symbol_search(char *name)
{
    symbol     *sym;

    if (!(sym = look_up(name)))
	don_err(UndeclareID, donald_full_name(name));
    return sym;
}

/*--------------------------------------------+----------------+
                                              | context_search |
                                              +----------------*/
symbol     *
context_search(char *name)
{
    symbol     *sym;

    if (name == PARENT)
	return SymEnd;
    if (!(sym = look_up(name)))
	don_err(UndeclareID, donald_full_name(name));
    if (sym->type != OPENSHAPE && sym->type != GRAPH)
	don_err(NotOpenshapeOrGraph, donald_full_name(name));
    return sym;
}

/*------------------------------------------------+------------+
                                                  | new_symbol |
                                                  +------------*/
symbol     *
new_symbol(char *name)
{				/* creat a new symbol in current context */
    symbol     *sym;

    /* Make sure it is new */
    sym = (symbol *) getmem(sizeof(symbol));
    sym->name = name;
    sym->type = UNDEFINED;	/* not defined */
    sym->defn.t = 0;		/* not defined */
    sym->master = topMasterStack();
    sym->derived = SymEnd;
    sym->next = SymbolTable;
    return SymbolTable = sym;
}

/*-----------------------------------------------+-------------+
                                                 | free_symbol |
                                                 +-------------*/
void
free_symbol(symbol * sym)
{				/* free the storage occupied by sym */
    extern void freeTree(tree);

    if (sym->name)
	free(sym->name);
    if (sym->viewport)
	free(sym->viewport);
    if (sym->defn.t && sym->type != GRAPH)
	freeTree(sym->defn.t);
    free(sym);
}

/* to do with monitor, now obsolete
|*------------------------------------------------+------------+
                                                  | mon_symbol |
                                                  +------------*|
void
mon_symbol(char *name)		|* creat a new symbol in current context *|
{
    struct d   *d;

    |* Make sure it is new *|
    d = (struct d *) getmem(sizeof(struct d));
    d->name = name;
    d->next = MonTable;
    MonTable = d;
}
*/

/*---------------------------------------------------+---------+
                                                     | look_up |
                                                     +---------*/
symbol     *
look_up(char *name)
{				/* look up the symbol in current context */
    symbol     *sym;

    for (sym = SymbolTable; sym != SymEnd; sym = sym->next) {
	if (strcmp(sym->name, name) == 0)
	    break;		/* found */
    }
    return sym;			/* sym == SymEnd if not found */
}


/*----------------------------------------------+--------------+
                                                | look_up_tree |
                                                +--------------*/
symbol     *
look_up_tree(tree expr)
{				/* look up the symbol in current context */
    symbol     *sym;
    int         count;

    if (expr->op == OP_GLOBAL) {
	backup_context();
	reset_context();
	sym = look_up_tree(Lexpr);
	restore_context();
	return sym;
    }
    for (count = 0; expr->op == OP_SLASH; count++, expr = Rexpr)
	change_context(context_search((char *) Lexpr->left));

    /* Now, expr->op should be OP_ID */
    for (sym = SymbolTable; sym != SymEnd; sym = sym->next) {
	if (Lexpr && strcmp(sym->name, (char *) Lexpr) == 0)
	    break;		/* found */
    }
    while (count--)
	resume_context();
    return sym;			/* sym == SymEnd if not found */
}


/*---------------------------------------------------+-----------+
                                                     | look_prev |
                                                     +-----------*/
symbol     *
look_prev(char *name)
{				/* look up the symbol before name in
				   current context */
    symbol     *sym, *sym_prev;

    sym_prev = SymEnd;
    for (sym = SymbolTable; sym != SymEnd; sym = sym->next) {
	if (strcmp(sym->name, name) == 0)
	    break;		/* found */
	sym_prev = sym;
    }
    return sym_prev;		/* sym == SymEnd if not found */
}

static char x_default_graph[] = "SetGraph(\"%s\",\"%s\"); %s_viewport = \"%s\";\n";

#define declare_graph(id, did, vp) \
	sprintf(temp, x_default_graph, id, did, id, vp);\
	appendEden(temp, dd_script);

/*------------------------------------------+-------------------+
                                            | declare_openshape |
                                            +-------------------*/
 /* sym is [ OPENSHAPE, sym/a, sym/b, ... ] */
void
declare_openshape(void)
{
    symbol     *s;

    /*----- print the context name -----*/
    /* indx == 0 --> root */
    sprintf(temp, "%s is [ OPENSHAPE", indx ? eden_full_name(0) : "_");
    appendEden(temp, dd_script);

    s = indx ? context[indx].sym->derived : *context[0].table;

    for (; s != SymEnd; s = s->next) {
	if (strcmp(s->name, "x<i>") == 0) {
	   /* printf("xi = %s", s->name); */
	    sprintf(temp, ", &%s", eden_full_name("_xi_"));
	} else if (strcmp(s->name, "f<i>") == 0) {
	   /* printf("fi = %s", s->name); */
	    sprintf(temp, ", &%s", eden_full_name("_fi_"));
	} else {
	   /* printf("wwi = %s", s->name); */
	    sprintf(temp, ", &%s", eden_full_name(s->name));
	}
	appendEden(temp, dd_script);
    }
    appendEden(" ];\n", dd_script);
}

/*-----------------------------------------------+--------------+
                                                 | eden_declare |
                                                 +--------------*/
static char x_default_attr[] = "A%s = NullStr;\n";
static char x_draw_act[] = "proc P%s:%s, A%s, %s\n{\n\t%s(%s, &%s, &A%s);\n}\n";

#define declare_attribute(id) \
	sprintf(temp, x_default_attr, id);\
	appendEden(temp, dd_script);

void
declare_action(char *id, char *proc_name, char *vp)
/* viewport name */
{
    if (!*vp) {
    	strcpy(viewport_name, "DoNaLD");
    	vp = "DoNaLD";
    	appendEden("InitDoNaLDViewport();\n", dd_script);
    }
    sprintf(temp, x_draw_act, id, id, id, vp, proc_name, vp, id, id);
    appendEden(temp, dd_script);
}

static char x_delete_attr[] = "A%s = @; forget(\"A%s\");\n";
static char x_delete_act[] = "forget(\"P%s\");\ndd_delete(&%s,&%s,&A%s);\n";
static char x_delete_openshape[] = "%s = @; forget(\"%s\");\n";
static char x_delete_graph[] = "forget(\"P%s\");\n";

#define delete_attribute(id) \
	sprintf(temp, x_delete_attr, id, id);\
	appendEden(temp, dd_script);

#define delete_action(id, vp) \
	sprintf(temp, x_delete_act, id, id, vp, id);\
	appendEden(temp, dd_script);

#define delete_openshape(id) \
	sprintf(temp, x_delete_openshape, id, id);\
	appendEden(temp, dd_script);

#define delete_graph(id) \
	sprintf(temp, x_delete_graph, id);\
	appendEden(temp, dd_script);

void
eden_delete(symbol * sym, char *vp)
 /* vp - viewport name */
{
    symbol     *s;
    char       *id = eden_full_name(sym->name);

    switch (sym->type) {
    case POINT:
    case LINE:
    case ARC:
    case CIRCLE:
    case RECTANGLE:
    case ELLIPSE:
    case LABEL:
    case SHAPE:
	delete_action(id, vp);
	delete_attribute(id);
	break;

    case GRAPH:
	delete_graph(id);
    case OPENSHAPE:
	change_context(sym);
	s = indx ? context[indx].sym->derived : *context[0].table;
	for (; s != SymEnd; s = s->next) {
	    eden_delete(s, vp);
	    SymbolTable = s->next;
	    free_symbol(s);
	}
	resume_context();
	delete_openshape(eden_full_name(sym->name));
	break;

    case VIEWPORT:
	break;
    }
    _changed++;
}

void
eden_declare(char *id, int type, char *vp)
 /* vp - viewport name */
{

	// Note: Uncomment below line to see declerations [richard]
	
/*    printf("eden_declare %s %i %s \n ", id, type, vp); */

    switch (type) {
	case POINT:
	declare_attribute(id);
	declare_action(id, "plot_point", vp);
	break;

    case LINE:
	declare_attribute(id);
	declare_action(id, "plot_line", vp);
	break;

    case ARC:
	declare_attribute(id);
	declare_action(id, "plot_arc", vp);
	break;

    case CIRCLE:
	declare_attribute(id);
	declare_action(id, "plot_circle", vp);
	break;

    case RECTANGLE:
	declare_attribute(id);
	declare_action(id, "plot_rectangle", vp);
	break;

    case ELLIPSE:
	declare_attribute(id);
	declare_action(id, "plot_ellipse", vp);
	break;

    case LABEL:
	declare_attribute(id);
	declare_action(id, "plot_label", vp);
	break;

    case SHAPE:
	declare_attribute(id);
	declare_action(id, "plot_shape", vp);
	break;

#ifdef DISTRIB
/*    case INT:
    case REAL:
    case BOOLEAN:
    case MYCHAR:
        sprintf(temp, "%s = @;\n", id);
        appendEden(temp, dd_script);
        break; */

	/* this block will cause declaration becomes definition. In
   other words, variable's value will be changed and affect other
   variables due to dependency */
#endif /* DISTRIB */

    case OPENSHAPE:
    case GRAPH:
	break;

    case VIEWPORT:
#ifdef DISTRIB
	if (handle_check1((char *) id)) { /* declare is allowed */
	   if (!*viewport_name && !strcmp((char *) id, "DoNaLD")) {
	      appendEden("InitDoNaLDViewport();\n", dd_script);
	   }
	   strcpy(viewport_name, (char *) id);
	   if (withinHandle > 0) { /* excluding Within clause */
              sprintf(temp, "%%donald\nviewport %s\n%%eden\n", (char *) id);
              propagateDonaldDef((char *) id, temp);
           }
	}
#else
	if (!*viewport_name && !strcmp((char *) id, "DoNaLD")) {
	    appendEden("InitDoNaLDViewport();\n", dd_script);
	}
	strcpy(viewport_name, (char *) id);
#endif /* DISTRIB */
	break;
    }
    _changed++;
}

/*---------------------------------------------------+---------+
                                                     | Declare |
                                                     +---------*/
void
Declare(int type, tree expr)
{	       /* declare variables, only to Donald not to Eden  */

    /* type - type of variable */
    /* expr - identifier expr(-list) */
    symbol     *sym;
    int         dc;		/* degree of changes */

    if (expr != EMPTYTREE) {
	switch (expr->op) {
	case OP_ID:
#ifdef DISTRIB
	if (handle_check1(eden_full_name((char *) Lexpr))) {
	  /* declare is allowed */
#endif /* DISTRIB */
	  sym = look_up((char *) Lexpr);
	  if (sym == SymEnd) {
	    sym = new_symbol((char *) Lexpr);
	  } else if (sym->type != type) {
	    don_err(RedeclareID, donald_full_name((char *) Lexpr));
	  }
	  sym->type = type;
	  /* can redeclare the same variable to a new viewport only if
	     it is of the same type */
	  eden_declare(eden_full_name((char *) Lexpr), type, viewport_name);
	  sym->viewport = strdup(viewport_name);
#ifdef DISTRIB
	  if (sym != SymEnd && withinHandle > 0 && !isGraph) {
	    /* send donaldDeclaration to others, but not graph */
	    sprintf(temp, "%%donald\n%s %s\n%%eden\n",
		    map_token_to_name(sym->type), sym->name);
	    propagateDonaldDef(eden_full_name(sym->name), temp);
	  }
	}
#endif /* DISTRIB */
	break;

	case OP_SLASH:
	    change_context(context_search((char *) Lexpr->left));
	    dc = _changed;
	    Declare(type, Rexpr);
	    if (_changed > dc) {
		declare_openshape();	/* take special care on context
					   change */
		_changed = dc;
	    }
	    resume_context();
	    break;

	case OP_GLOBAL:
	    backup_context();
	    reset_context();
	    dc = _changed;
	    Declare(type, Lexpr);
	    if (_changed > dc) {
		declare_openshape();	/* take special care on context
					   change */
		_changed = dc;
	    }
	    restore_context();
	    break;

	case OP_COMMA:
	    Declare(type, Lexpr);
	    Declare(type, Rexpr);
	    break;

	default:
	    don_err(IdListExpect, 0);
	    break;
	}
    }
}


/*----------------------------------------------+--------------+
                                                | DeclareGraph |
                                                +--------------*/
void
DeclareGraph(tree expr)
{				/* declare variables */
    /* identifier expr(-list) */
    symbol     *sym;
    int         dc;		/* degree of changes */
    tree        t;

    if (expr != EMPTYTREE) {
	switch (expr->op) {
        case OP_ID:
#ifdef DISTRIB
	if (handle_check1(eden_full_name((char *) Lexpr))) {
	  /* declare is allowed */
#endif /* DISTRIB */
	  sym = look_up((char *) Lexpr);
	  if (sym == SymEnd) {
	    sym = new_symbol((char *) Lexpr);
	  } else if (sym->type != GRAPH) {
	    don_err(RedeclareID, donald_full_name((char *) Lexpr));
	  }
	  sym->type = GRAPH;
	  /* can redeclare the same variable to a new viewport only if
	     it is of the same type */
	  declare_graph(eden_full_name((char *) Lexpr),
			donald_full_name((char *) Lexpr),
			(*viewport_name == '\0') ? "DoNaLD" : viewport_name);
	  sym->viewport = strdup(viewport_name);
#ifdef DISTRIB
	  if (sym != SymEnd && withinHandle > 0) {
	    /* send donaldDeclaration to others */
	    sprintf(temp, "%%donald\n%s %s\n%%eden\n",
		    map_token_to_name(sym->type), sym->name);
	    propagateDonaldDef(eden_full_name(sym->name), temp);
	  }
#endif /* DISTRIB */

	  /* sym->defn.f = f; */

	  /* declare variables of graph */
#ifdef DISTRIB
	  /* these variable should not be propagated */
	  isGraph = 1;
#endif /* DISTRIB */
	  change_context(sym);
	  t = dtree1(OP_ID, (tree) "nSegment");
	  Declare(INT, t);
	  t->left = (tree) "x<i>";
	  Declare(REAL, t);
	  t->left = (tree) "f<i>";
	  Declare(REAL, t);
	  t->left = (tree) "node";
	  Declare(GSPEC, t);
	  t->left = (tree) "segment";
	  Declare(GSPEC, t);
	  t->left = (tree) "viewport";
	  Declare(MYCHAR, t);
	  free(t);
	  resume_context();
#ifdef DISTRIB
	  isGraph = 0;
	}
#endif /* DISTRIB */
	break;

	case OP_SLASH:
	    change_context(context_search((char *) Lexpr->left));
	    dc = _changed;
	    DeclareGraph(Rexpr);
	    if (_changed > dc) {
		declare_openshape();	/* take special care on context
					   change */
		_changed = dc;
	    }
	    resume_context();
	    break;

	case OP_GLOBAL:
	    backup_context();
	    reset_context();
	    dc = _changed;
	    DeclareGraph(Lexpr);
	    if (_changed > dc) {
		declare_openshape();	/* take special care on context
					   change */
		_changed = dc;
	    }
	    restore_context();
	    break;

	case OP_COMMA:
	    DeclareGraph(Lexpr);
	    DeclareGraph(Rexpr);
	    break;

	default:
	    don_err(IdListExpect, 0);
	    break;
	}
    }
}

/* to do with monitor, now obsolete
|*---------------------------------------------------+----------+
                                                     | declare2 |
                                                     +----------*|
void
declare2(int type, tree expr)		|* declare variables *|
    |* type - type of variable *|
    |* expr - identifier expr(-list) *|
{
    symbol     *sym;

    if (expr != EMPTYTREE) {
	switch (expr->op) {
	case OP_ID:
	    if (!look_up(Lexpr)) {
		sym = new_symbol(Lexpr);
		sym->type = type;
		eden_declare(eden_full_name(Lexpr), type);
		mon_symbol(Lexpr);
		mon_declare();
	    }
	    break;

	case OP_SLASH:
	    change_context(context_search(Lexpr->left));
	    Declare(type, Rexpr);
	    resume_context();
	    break;

	case OP_GLOBAL:
	    backup_context();
	    reset_context();
	    Declare(type, Lexpr);
	    restore_context();
	    break;

	case OP_COMMA:
	    Declare(type, Lexpr);
	    Declare(type, Rexpr);
	    break;

	default:
	    don_err(IdListExpect, 0);
	    break;
	}
    }
}
*/


/*---------------------------------------------------+--------+
                                                     | Delete |
                                                     +--------*/
void
Delete(tree expr)
{				/* Delete variables */
    /* expr - identifier expr(-list) */
    symbol     *sym, *sym_prev;
    int         dc;		/* degree of changes */

    if (expr != EMPTYTREE) {
	switch (expr->op) {
	case OP_ID:
	    if ((sym = look_up((char *) Lexpr)) == 0)
		don_err(UndeclareID, donald_full_name((char *) Lexpr));
	    eden_delete(sym, viewport_name);
	    sym_prev = look_prev((char *) Lexpr);
	    if (sym_prev == SymEnd)
		SymbolTable = sym->next;
	    else
		sym_prev->next = sym->next;
	    free_symbol(sym);
	    break;

	case OP_SLASH:
	    change_context(context_search((char *) Lexpr->left));
	    dc = _changed;
	    Delete(Rexpr);
	    if (_changed > dc) {
		declare_openshape();	/* take special care on context
					   change */
		_changed = dc;
	    }
	    resume_context();
	    break;

	case OP_GLOBAL:
	    backup_context();
	    reset_context();
	    Delete(Lexpr);
	    restore_context();
	    break;

	case OP_COMMA:
	    Delete(Lexpr);
	    Delete(Rexpr);
	    break;

	default:
	    don_err(IdListExpect, 0);
	    break;
	}
    }
}


/* to do with monitor, now obsolete
|*------------------------------------------+-------------------+
                                            | mon_declare       |
                                            +-------------------*|
 |* sym is [ OPENSHAPE, sym/a, sym/b, ... ] *|
void
mon_declare(void)
{
    struct d   *s;

    |*----- print the context name -----*|
    |* indx == 0 --> root *|
    appendEden("_monmesg is [ ", dd_script);

    for (s = MonTable; s != (struct d *) 0; s = s->next) {
	if (s->next == NULL)
	    sprintf(temp, " &_%s", s->name);
	else
	    sprintf(temp, " &_%s,", s->name);
	appendEden(temp, dd_script);
    }
    appendEden(" ];\n", dd_script);
}
*/

/*-----------------------------------------+--------------------+
                                           | print_symbol_table |
                                           +--------------------*/
void
print_symbol_table(int level, symbol * symtab)
{
    symbol     *sym;
    int         i;

    for (sym = symtab; sym != SymEnd; sym = sym->next) {
	for (i = level; i; --i)
	    fprintf(stderr, "|   ");
	fprintf(stderr, "%s %d\n", sym->name, sym->type);
	if (sym->type == OPENSHAPE || sym->type == GRAPH)
	    print_symbol_table(level + 1, sym->derived);
    }
}

/*------------------------------------------+-------------------+
                                            | print_all_symbols |
                                            +-------------------*/
void
print_all_symbols(void)
{
    fprintf(stderr, "Symbol Table :-\n");
    print_symbol_table(0, *context[0].table);
    fprintf(stderr, "Symbol Table End.\n");
}

/*-------------------------------------------+------------------+
                                             | donald_full_name |
                                             +------------------*/
char       *
donald_full_name(char *id)
{
    static char name[256];
    char       *s = name;
    char       *t;
    int         i;
    int         len;
    int         id_len;

    id_len = id ? strlen(id) + 2 : 1;

    *s++ = '/';
    for (i = 1; i <= indx; i++) {
	t = context[i].sym->name;
	len = strlen(t);
	if (&name[256] - s < id_len + len)
	    don_err(Unclassified, "variable name too long");
	strcpy(s, t);
	s += len;
	*s++ = '/';
    }
    strcpy(s, id);
    return name;
}

/*---------------------------------------------+----------------+
                                               | eden_full_name |
                                               +----------------*/
char       *
eden_full_name(char *id)
{
    static char name[256];
    char       *s = name;
    char       *t;
    int         i;
    int         len;
    int         id_len;

    /* if (id == 0) ==> context name */
    id_len = id ? strlen(id) + 2 : 0;

    for (i = 1; i <= indx; i++) {
	t = context[i].sym->name;
	/* printf("eden name %s %i", t, i); */
	len = strlen(t);
	if (&name[256] - s < id_len + len)
	    don_err(Unclassified, "variable name too long");
	*s++ = '_';
#ifdef DISTRIB
	/* if (i==1 && !inPrefix) {
	   len = strlen(t) -strlen(agentName);
	   strncat(s, t, len);
	} else */
#endif /* DISTRIB */
	strcpy(s, t);
	s += len;
    }
    if (id) {
	*s++ = '_';
	strcpy(s, id);
    }
    return name;
}

/*----------------------------------------+---------------------+
                                          | expr_to_donald_name |
                                          +---------------------*/
static char expr_name[256];

char       *
expr_to_donald_name(tree expr)
 /* turn an id-expr into a string */
{				/* assume expr was checked */
    *expr_name = '\0';
    if (expr->op == OP_GLOBAL) {
	strcat(expr_name, "/");
	expr = Lexpr;
    }
    for (; expr->op == OP_SLASH; expr = Rexpr) {
	strcat(expr_name, Lexpr->left ? (char *) Lexpr->left : "~");
	strcat(expr_name, "/");
    }
    strcat(expr_name, (char *) Lexpr);
    return expr_name;
}


/*------------------------------------------+-------------------+
                                            | expr_to_eden_name |
                                            +-------------------*/
/* not very sophisticated, can't handle global id with ~ */
char       *
expr_to_eden_name(tree expr)
 /* turn an id-expr into a string */
{				/* assume expr was checked */
    tree        e;
    int         count;
    char       *name;

    if (expr->op == OP_GLOBAL) {
	backup_context();
	reset_context();
	name = expr_to_eden_name(Lexpr);
	restore_context();
	return name;
    }
    for (count = 0, e = expr; e->op == OP_SLASH; count++, e = e->right)
	change_context(context_search((char *) e->left->left));

    /* Now, e->op should be OP_ID */
    name = eden_full_name((char *) e->left);
    while (count--)
	resume_context();
    return name;
}

/*-------------------------------------------------+------------+
                                                   | dumpdonald |
						   +------------*/

static int
isGraphAttr(char *name)
{
    return !(strcmp(name, "nSegment") &&
	     strcmp(name, "f<i>") &&
	     strcmp(name, "x<i>") &&
	     strcmp(name, "node") &&
	     strcmp(name, "segment") &&
	     strcmp(name, "viewport"));
}

static void
indent(int level)
{
    while (level--)
	Tcl_EvalEC(interp, ".donald.t.text insert end {  }");
}

static void
insert_nl(void)
{
    Tcl_EvalEC(interp, ".donald.t.text insert end {\n}");
}

static char *lastmaster;
static char *lastvp;
static int  lasttype, neednl;

static void
dumpDonaldDeclaration(int level, symbol * symtab, int isGraph)
{
    symbol     *sym;
    extern char *map_token_to_name(intptr_t);

    for (sym = symtab; sym != SymEnd; sym = sym->next) {
	if (isGraph && isGraphAttr(sym->name))
	    continue;
	if (*sym->viewport == '\0')
	    sym->viewport = strdup("DoNaLD");
	if ((lastvp == 0 || strcmp(lastvp, sym->viewport)) &&
	    !(lastvp == 0 && strcmp("DoNaLD", sym->viewport) == 0)) {
	    lasttype = 0;
	    if (neednl) {
		insert_nl();
		indent(level);
	    }
	    Tcl_VarEval(interp, ".donald.t.text insert end {viewport ",
			sym->viewport, "\n} viewport", 0);
	    neednl = 0;
	    lastvp = sym->viewport;
	}
	if (lasttype == sym->type && sym->type != GRAPH) {
	    Tcl_VarEval(interp, ".donald.t.text insert end {, ",
			sym->name, "}", 0);
	    neednl = 1;
	} else {
	    if (neednl) {
		insert_nl();
		neednl = 0;
	    }
	    indent(level);
	    Tcl_VarEval(interp, ".donald.t.text insert end {",
		   map_token_to_name(sym->type), " ", sym->name, "}", 0);
	    neednl = 1;
	    lasttype = sym->type;
	}
	if (sym->type == OPENSHAPE || sym->type == GRAPH) {
	    insert_nl();
	    indent(level);
	    neednl = 0;
	    lasttype = 0;
	    Tcl_VarEval(interp, ".donald.t.text insert end \"within ",
			sym->name, " {\\n\"", 0);
	    dumpDonaldDeclaration(level + 1, sym->derived, sym->type == GRAPH);
	    if (neednl)
		insert_nl();
	    indent(level);
	    Tcl_EvalEC(interp, ".donald.t.text insert end \"}\\n\"");
	    neednl = 0;
	    lasttype = 0;
	}
    }
}

static void
dumpDonaldDefinition(int level, symbol * symtab)
{
    symbol     *sym;
    extern char *dumpbuf;
    extern void initbuf(int);
    extern void dumpdtree(tree);

    for (sym = symtab; sym != SymEnd; sym = sym->next) {
	if (lastmaster == 0 || strcmp(lastmaster, sym->master)) {
	    Tcl_VarEval(interp, ".donald.t.text insert end {AGENT ",
			sym->master, "\n} master", 0);
	    lastmaster = sym->master;
	}
	if (sym->type == OPENSHAPE || sym->type == GRAPH) {
	    indent(level);
	    Tcl_VarEval(interp, ".donald.t.text insert end \"within ",
			sym->name, " {\\n\"", 0);
	    dumpDonaldDefinition(level + 1, sym->derived);
	    indent(level);
	    Tcl_EvalEC(interp, ".donald.t.text insert end \"}\\n\"");
	} else if (sym->defn.t) {
	    indent(level);
	    Tcl_VarEval(interp, ".donald.t.text insert end {",
			sym->name, " = }", 0);
	    initbuf(2048);	/* initialize dumpbuf */
	    dumpdtree(sym->defn.t);
	    Tcl_VarEval(interp, ".donald.t.text insert end \"",
			dumpbuf, "\"", 0);
	    insert_nl();
	}
    }
}

int
dumpdonald(void)
{
    Tcl_EvalEC(interp, "cleanup donald");
    Tcl_EvalEC(interp, ".donald.t.text config -state normal");
    /* dump type declarations */
    lastvp = 0;
    lasttype = 0;
    neednl = 0;
    dumpDonaldDeclaration(0, GlobalSymbols, 0);
    if (neednl)
	insert_nl();
    insert_nl();
    /* dump variable definitions */
    lastmaster = 0;
    dumpDonaldDefinition(0, GlobalSymbols);
    Tcl_EvalEC(interp, ".donald.t.text mark set insert 1.0");
    Tcl_EvalEC(interp, ".donald.t.text see insert");
    Tcl_EvalEC(interp, ".donald.t.text config -state disabled");
    return TCL_OK;
}
