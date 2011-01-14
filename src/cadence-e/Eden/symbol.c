/*
 * $Id: symbol.c,v 1.11 2001/07/27 17:56:37 cssbz Exp $
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

static char rcsid[] = "$Id: symbol.c,v 1.11 2001/07/27 17:56:37 cssbz Exp $";

/**********************************************
 *	SYMBOL TABLE: INSTALL AND LOOK-UP     *
 **********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h> // [Richard] : For strlen() etc...

#include "../../../../../config.h"
#include "eden.h"
#include "yacc.h"
#include "symptr.q.c"		/* some of the code is used by other
				   modules */
#include "hash.h"
#include "notation.h"

#ifdef DISTRIB
#include "agency.q.c"
#endif /* DISTRIB */

#include "emalloc.h"
#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

#ifdef DISTRIB
void  add_agent_Q(agent_QUEUE *, char *);
agent_ATOM  search_agent_Q(agent_QUEUE *, char *);
void  delete_agent_Q(agent_QUEUE *, char *, char *);
#endif /* DISTRIB */

/*--------------------------------------------------------------------
			SYMBOL HASH TABLE
--------------------------------------------------------------------*/
/* HASHSIZE is defined in "hash.h": must be power of 2 minus 1 */
int
hashindex(char *s)
{
    int         i = 0;

    while (*s)
	i ^= *s++;
    return i & HASHSIZE;
}

symptr      hashtable[HASHSIZE + 1] = {(symptr) 0};	/* init to nulls */

symptr
lookup(char *s, int context)
{				/* FIND SYMBOL OF NAME S */
    /* 0 = NOT FOUND, ELSE = SYMBOL POINTER */
    symptr      sp;

    for (sp = hashtable[hashindex(s)]; sp != (symptr) 0; sp = sp->next)
	if ((sp->context == context) && (strcmp(sp->name, s) == 0))
	    return sp;
    return 0;			/* 0 ==> not found */
}

symptr
install(char *s, int context, int st, int t, Int i)
 /* INTERNAL: INSTALL A SYMBOL IN SYMBOL TABLE */
 /* s  = symbol name */
 /* st = symbol type */
 /* t  = initial data type */
 /* i  = initial data value */
{
    extern symptr hashtable[];
    int         indx;
    symptr      sp;
    extern      topEntryStack(void);
    extern char *topMasterStack(void);
    extern char agentName[128];
    extern notationType currentNotation;
    extern int builtin_ft_check(char *);
    extern int append_agentName;
    extern int append_NoAgentName;
#ifdef DISTRIB
    extern Int *EveryOneAllowed;
    extern char *everyone;
#endif /* DISTRIB */

    sp = (symptr) emalloc(sizeof(symbol));
    sp->name = emalloc(strlen(s) + 1);	/* +1 for '\0' */
    strcpy(sp->name, s);
    sp->stype = st;
    sp->inst = (Inst *) 0;
    sp->nauto = 0;
    sp->changed = TRUE;
    sp->text = (char *) 0;
    sp->context = context; /* DOSTE object [Nick] */
    sp->d.type = t;
    if (t == UNDEF) {
      sp->d.u.sym = sp; /* put a reference to the symbol in the UNDEF
                           Datum so that name details etc can be found
                           in the event of an error (in eval(), say)
                           [Ash] */
    } else {
      sp->d.u.i = i;
    }
    CLEAN_Q(&sp->targets);
    CLEAN_Q(&sp->sources);

#ifdef DISTRIB
    CLEAN_Q(&sp->OracleOf);
    CLEAN_Q(&sp->HandleOf);
    CLEAN_Q(&sp->StateOf);
    /* if (*agentName!=0 && builtin_ft_check(s) ==0 && currentNotation != LSD
        && append_NoAgentName > 0 && append_agentName > 0 )   {
       add_agent_Q(&sp->HandleOf, agentName);
       add_agent_Q(&sp->OracleOf, agentName);
    } depends on design decision. if agent owns default rights?  --sun */
     /*add_agent_Q(&sp->HandleOf, "wmb");
    delete_agent_Q(&sp->HandleOf, "sun");
    add_agent_Q(&sp->HandleOf, "wmb"); -- for test  */
#endif /* DISTRIB */

    sp->Qloc = 0;
    sp->marked = 0;
    sp->entry = topEntryStack();
    sp->master = topMasterStack();

#ifdef DISTRIB
    if (streq(sp->name, "EveryOneAllowed") || *EveryOneAllowed) {
      if (streq(topMasterStack(), "system") && !streq(sp->name, "screen")
          && !streq(sp->name, "DoNaLDdefaultWin") &&
          !streq(sp->name, "DoNaLD")) {
       add_agent_Q(&sp->HandleOf, "SYSTEM");
       add_agent_Q(&sp->OracleOf, "SYSTEM");
       add_agent_Q(&sp->StateOf, "SYSTEM");
      } else {
       add_agent_Q(&sp->HandleOf, everyone);
       add_agent_Q(&sp->OracleOf, everyone);
       add_agent_Q(&sp->StateOf, everyone);
      }
    }
#endif /* DISTRIB */

    sp->next = hashtable[indx = hashindex(s)];	/* put at front of list */
    hashtable[indx] = sp;
    return sp;
}

#ifdef DISTRIB
/* Add a new global variable to the symbol table, with an initially
   undefined value.  This is needed so that other modules (ie LSD)
   don't need to know the value of UNDEF (#including yacc.h leads to
   conflicts with YYSTYPE). [by Ash] */
symptr installGlobalUndef(char *name)
{
  return install(name, VAR, UNDEF, 0);
}

void
add_agent_Q(agent_QUEUE * AQ, char *name)
{
  agent_ATOM AA;
  extern char *everyone;

  if(AA = search_agent_Q(AQ, everyone))
        DELETE_agent_ATOM(AQ, AA);
  if (!(AA = search_agent_Q(AQ, name))) {
     AA = emalloc(sizeof(agent_QUEUE));
     AA->obj.name = emalloc(strlen(name)+1);
     strcpy(AA->obj.name, name);
     AA->prev = (agent_QUEUE *) 0;
     AA->next = (agent_QUEUE *) 0;
     INSERT_Q(AQ, AA);
  }
}

agent_ATOM
search_agent_Q(agent_QUEUE * AQ, char *name)
{
    agent_ATOM  Q;

    FOREACH(Q, AQ) {
        if (Q->obj.name == (char *) 0) break;
        if (streq(Q->obj.name, name))
           return Q ;
    }
    return (agent_ATOM) 0;
}

void
delete_agent_Q(agent_QUEUE * AQ, char *name, char *ObsName)
{
    agent_ATOM  Q;
    symptr sp;
    extern char *everyone;


    Q=search_agent_Q(AQ, name);
    if (Q) DELETE_agent_ATOM(AQ, Q);

    if (sp = lookup(ObsName)) {
       if (Q_EMPTY(AQ) && search_agent_Q(&sp->StateOf, everyone))
          add_agent_Q(AQ, everyone);
    }
}
#endif /* DISTRIB */


/*--------------------------------------------------------------------
			LOCAL VARIABLE LOOK UP
--------------------------------------------------------------------*/


struct entry {
    char       *name;
    short       level;
    short       num;
};

typedef struct entry entry;

#define	streq(X,Y)		(strcmp(X,Y)==0)
#define EQUAL_entry(A,B)	streq(A.name,B.name)
#define FREE_entry(A)		if(A.name)free(A.name)

#include "entry.q.c"

static entry_QUEUE LocalVarList = EMPTYQUEUE(LocalVarList);

/*
 * look up the position of local variable
 * 0 = not in LocalVarList
 */
entry_ATOM
search_local(char *name)
{
    entry_ATOM  A;

    FOREACH(A, &LocalVarList) {
	if (A->obj.name == (char *) 0)
	    break;
	/* printf("search %s %s %i %i\n", A->obj.name, name,
           A->obj.level, A->obj.num); */
	if (streq(A->obj.name, name))	/* match */
	    return A;
    }
    return (entry_ATOM) 0;
}

/*
 *	add a local variable to the list
 *	return the entry of the list
 */
entry_ATOM
add_local_variable(char *name)
{
    entry_ATOM       E;
    entry_ATOM  F;

    F = FRONT(&LocalVarList);
    E = emalloc(sizeof(entry_QUEUE));
    E->obj.name = emalloc(strlen(name) + 1);
    strcpy(E->obj.name, name);
    E->obj.level = indef;
    if (inpara) {
	E->obj.num = F->obj.num - 1;
    } else {			/* inauto */
	if (F->obj.num < 0)
	    E->obj.num = 1;
	else
	    E->obj.num = F->obj.num + 1;
    }
    E->prev = (entry_QUEUE *) 0;
    E->next = (entry_QUEUE *) 0;
    INSERT_Q(&LocalVarList, E);
    return FRONT(&LocalVarList);
}

/*
 *	insert a level marker
 */
void
insert_level_marker(int level)
{
    entry       E;

    E.name = (char *) 0;
    E.level = level;
    E.num = 0;
    INSERT_entry_Q(&LocalVarList, E);
}

/*
 *	initialist the LocalVarList
 */
void
init_LocalVarList(void)
{
    CLEAN_Q(&LocalVarList);
    insert_level_marker(0);
}

/* delete all entry not smaller than the level */
void
delete_local_level(int level)
{
    entry_ATOM  E;
    entry_QUEUE *Q = &LocalVarList;

    for (E = FRONT(Q); E->obj.level >= level; E = FRONT(Q))
	DELETE_FRONT_entry(Q);
}


//																<<<<<<<<<<-------------- TODO: Should this be overloaded ?
void
printlocal(void)
{
    entry_ATOM  E;

    FOREACH(E, &LocalVarList)
	printf("%s %d %d\n", E->obj.name ? E->obj.name : "*",
	       E->obj.level, E->obj.num);
}

int
local_declare(char *name)
{
    entry_ATOM  E;

    if (search_local(name))
	error2("redeclare local variable:", name);
    E = add_local_variable(name);
    return E->obj.num;
}

int
lookup_local(char *name)
{
    entry_ATOM  E;

    if ((E = search_local(name))) {
		if (E->obj.level != indef)
		    error2("local variable is not in this level:", name);
		return E->obj.num;
    } else
    	return 0;
}

symptr_QUEUE break_q = EMPTYQUEUE(break_q);
symptr_QUEUE cont_q = EMPTYQUEUE(cont_q);

void
dispatch(Inst * p, symptr_QUEUE * Q)
{
    APPEND_symptr_Q(Q, (symptr) p);
}

void
patch(Inst * mark, Inst * p, symptr_QUEUE * Q)
{
    symptr_ATOM A;
    Inst       *ip;

    while ((A = LAST(Q))) {
	ip = (Inst *) A->obj;
	DELETE_symptr_ATOM(Q, A);
	if (ip == mark)
	    return;
	ip[1] = (Inst) (p - (ip + 2));
    }
    /* should not get here */
    error("compiler error while patching code");
}
