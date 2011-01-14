/*
 * $Id: agentList.c,v 1.7 2001/07/27 16:36:46 cssbz Exp $
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

static char rcsid[] = "$Id: agentList.c,v 1.7 2001/07/27 16:36:46 cssbz Exp $";

#include <stdio.h>
#include <string.h>

#include "../config.h"
#include "../Eden/eden.h"
#include "../Eden/agency.q.h"
#include "Obs.q.h"
#include "LSDagent.q.h"
#include "parser.h"
#include <tk.h>

#include "../Eden/emalloc.h"
#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

extern Tcl_Interp *interp;
extern void LSD_err(char *);
extern symptr installGlobalUndef(char *);
char *LSDagentName = 0;

int         dumpLSD(ClientData, Tcl_Interp *, int, char *[]);

static LSDagent_QUEUE  LSDagentList = EMPTYQUEUE(LSDagentList);
LSDagent_ATOM  current_LSDagent;

void        declare_LSDagent(char *);
static LSDagent_ATOM     search_LSDagent_Q(LSDagent_QUEUE *, char *);
static LSDagent_ATOM     add_LSDagent_Q(LSDagent_QUEUE *, char *);

static Obs_ATOM    search_ObsQUEUE(Obs_QUEUE *, char *);
void        add_ObsQUEUE(char *);
void        add_Obs_Q(char *, int, char *);
void        declare_ObsQUEUE(int);
void        delete_agency(int);
void        init_LSDagentList(void);
void        remove_ObsQUEUE(char *);
void        remove_Obs_Q(char *, int, char *);
static int  LSDtype=0;



static LSDagent_ATOM
search_LSDagent_Q(LSDagent_QUEUE *LQ, char *name)
{
    LSDagent_ATOM  L;

    FOREACH(L, LQ) {
	if (L->obj.name == (char *) 0)
	    break;
	if (streq(L->obj.name, name))	/* match */
	    return L;
    }
    return (LSDagent_ATOM) 0;
}

static LSDagent_ATOM
add_LSDagent_Q(LSDagent_QUEUE * LQ, char *name)
{
  LSDagent  L;
  LSDagent_ATOM  LA;

  if (!(LA = search_LSDagent_Q(LQ, name))) {
     LA =  emalloc(sizeof(LSDagent_QUEUE));
     LA->obj.name = emalloc(strlen(name) + 1);
     strcpy(LA->obj.name, name);
     CLEAN_Q(&LA->obj.oracle);
     CLEAN_Q(&LA->obj.handle);
     CLEAN_Q(&LA->obj.state);
     LA->prev = (LSDagent_QUEUE *) 0;
     LA->next = (LSDagent_QUEUE *) 0;
     INSERT_Q(LQ, LA);
     return LA;
  } else {
     return LA;
  }

}


void
declare_LSDagent(char * name)
{
     current_LSDagent = add_LSDagent_Q(&LSDagentList, name);
     LSDagentName = current_LSDagent->obj.name;
}

static Obs_ATOM
search_ObsQUEUE(Obs_QUEUE * OQ, char *name)
{
    Obs_ATOM  OA;

    for(OA=FRONT(OQ);OA;OA=NEXT(OQ,OA)) {
        if (OA->obj.name == (char *) 0) break;
        if (streq(OA->obj.name, name)) {
           return OA ;
        }
    }
    return (Obs_ATOM) 0;
}

void
add_ObsQUEUE(char *name)
{
  extern symptr install(char *, int, int, Int);
  extern symptr lookup(char *);
  extern void add_agent_Q(agent_QUEUE *, char *);
  symptr  sp;
  LSDagent_ATOM  LA = current_LSDagent;

  add_Obs_Q(LA->obj.name, LSDtype, name);
     if ((sp = lookup(name)) == 0)  {     /* must be global */
        sp = installGlobalUndef(name);
     }
     switch(LSDtype) {
       case ORACLE: add_agent_Q(&sp->OracleOf, LA->obj.name); break;
       case HANDLE: add_agent_Q(&sp->HandleOf, LA->obj.name); break;
       case STATE: add_agent_Q(&sp->StateOf, LA->obj.name); break;
       default: LSD_err("undefined LSD type");
     }
}

void
add_Obs_Q(char *LAname, int type, char *name)
{
  Obs_ATOM   OA;
  Obs_QUEUE  *OQ;
  LSDagent_ATOM  LA;

  if (!(LA = search_LSDagent_Q(&LSDagentList, LAname)))
      LA = add_LSDagent_Q(&LSDagentList, LAname);

   switch (type) {
     case  1:    /* integer 1 2 3 come from addAgency() in builtin.c --sun */
     case  ORACLE:
             OQ = &LA->obj.oracle; break;
     case 2:
     case  HANDLE:
             OQ = &LA->obj.handle; break;
     case 3:
     case  STATE:
             OQ = &LA->obj.state; break;
     default: LSD_err("LSD type error ");
   }
  if (!search_ObsQUEUE(OQ, name)) {
     OA = emalloc(sizeof(Obs_QUEUE));
     OA->obj.name = emalloc(strlen(name)+1);
     strcpy(OA->obj.name, name);
     OA->next = (Obs_QUEUE *) 0;
     OA->prev = (Obs_QUEUE *) 0;
     INSERT_Q(OQ, OA);
  }
}

void
remove_ObsQUEUE(char *name)
{
  extern symptr lookup(char *);
  symptr  sp;
  extern void delete_agent_Q(agent_QUEUE *, char *, char *);
  LSDagent_ATOM LA = current_LSDagent;

  remove_Obs_Q(LA->obj.name, LSDtype, name);
  if (sp = lookup(name)) {
     switch(LSDtype) {
       case ORACLE: delete_agent_Q(&sp->OracleOf, LA->obj.name, name); break;
       case HANDLE: delete_agent_Q(&sp->HandleOf, LA->obj.name, name); break;
       case STATE: delete_agent_Q(&sp->StateOf, LA->obj.name, name); break;
       default: LSD_err("undefined LSD type");
     }
  } else { LSD_err("no such observable"); }
}


void
remove_Obs_Q(char *LAname, int type, char *name)
{
  Obs_ATOM   OA;
  Obs_QUEUE  *OQ;
  LSDagent_ATOM  LA;

  if (!(LA = search_LSDagent_Q(&LSDagentList, LAname)))
      LSD_err(" no such a LSD agent");
   switch (type) {
     case  1:          /* integer 1 2 3 come from removeAgency() in builtin.c --sun */
     case  ORACLE:
             OQ = &LA->obj.oracle; break;
     case  2:
     case  HANDLE:
             OQ = &LA->obj.handle; break;
     case  3:
     case  STATE:
             OQ = &LA->obj.state; break;
     default: LSD_err("LSD type error ");
   }
   if (OA = search_ObsQUEUE(OQ, name)) {
     OA->prev->next = OA->next;
     OA->next->prev = OA->prev;
     if (OA->obj.name) free(OA->obj.name);
     free(OA);
   } else
     LSD_err("no such observable");
}


void
declare_ObsQUEUE(int type)
{
   LSDagent_ATOM    LA = current_LSDagent;
   Obs_QUEUE         *OQ;

   if (!LA) LSD_err("undeclare LSDagent");
   LSDtype = type;
}

void
init_LSDagentList(void)
{
   CLEAN_Q(&LSDagentList);
}

static void
printObs(Obs_QUEUE *OQ)
{
    Obs_ATOM   OA;
    int     firstObs = 1;
    FOREACH(OA, OQ) {
       if (OA->obj.name == (char *) 0) LSD_err("no Observable name ");
       if (!firstObs)
           Tcl_EvalEC(interp, ".lsd.t.text insert end {, }");
       else firstObs = 0;
       Tcl_VarEval(interp, ".lsd.t.text insert end {", OA->obj.name, "}", 0);
    }
    Tcl_EvalEC(interp, ".lsd.t.text insert end {\n}");
}

int
dumpLSD(ClientData clientData, Tcl_Interp * interp, int argc, char *argv[])
{

   LSDagent_QUEUE   *LQ = &LSDagentList;
   LSDagent_ATOM    LA;

   if (Q_EMPTY(LQ)) return;
   Tcl_EvalEC(interp, "cleanup lsd");
   Tcl_EvalEC(interp, ".lsd.t.text config -state normal");
   FOREACH(LA, LQ) {
      if (LA->obj.name == (char *) 0) LSD_err("no LSDagentName");
        else  Tcl_VarEval(interp, ".lsd.t.text insert end {agent ",
	         LA->obj.name, "\n}", 0);
      if (!Q_EMPTY(&LA->obj.oracle)) {
	 Tcl_EvalEC(interp, ".lsd.t.text insert end {   oracle }");
         printObs(&LA->obj.oracle);
      }
      if (!Q_EMPTY(&LA->obj.handle)) {
	 Tcl_EvalEC(interp, ".lsd.t.text insert end {   handle }");
         printObs(&LA->obj.handle);
      }
      if (!Q_EMPTY(&LA->obj.state)) {
	 Tcl_EvalEC(interp, ".lsd.t.text insert end {   state }");
         printObs(&LA->obj.state);
      }
      Tcl_EvalEC(interp, ".lsd.t.text insert end {\n}");
   }
   Tcl_EvalEC(interp, ".lsd.t.text mark set insert 1.0");
   Tcl_EvalEC(interp, ".lsd.t.text see insert");
   Tcl_EvalEC(interp, ".lsd.t.text config -state disabled");
   return TCL_OK;
}

