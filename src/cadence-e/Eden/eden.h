/*
 * $Id: eden.h,v 1.13 2001/07/27 17:27:16 cssbz Exp $
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

#ifdef NO_CHECK_CIRCULAR
extern int  NCC;
#endif /* NO_CHECK_CIRCULAR */

#ifdef DEBUG
extern int  Debug;
#endif /* DEBUG */

#ifndef FALSE
#define FALSE  (0)
#endif

#ifndef TRUE
#define TRUE   (1)
#endif

#include <inttypes.h>

typedef intptr_t Int;		/* pointer compatible integer */
typedef void (*Inst) ();
typedef struct symbol *symptr;
typedef struct Datum Datum;
typedef union uDatum uDatum;

#include "symptr.q.h"

#ifdef DISTRIB
/* For the three item agency list [Ash, with Patrick] */
#include "agency.q.h"
#endif /* DISTRIB */

typedef struct symbol {
    char       *name;		/* variable name */
    short       stype;		/* symbol type */
    Inst       *inst;		/* instruction entry */
    union {
    unsigned    nauto;		/* no. of local variables */
    unsigned    doste;		/* Is DOSTE listening to this [Nick] */
    };
    char       *text;		/* original text of definition, if any */
    struct Datum {
	short       type;	/* data type */
	union uDatum {
	    double      r;
	    Int         i;	/* integer/char */
	    char       *s;	/* string */
	    Datum      *a;	/* list */
	    symptr      sym;	/* symbol pointer */
	    struct {
		Int         x;
		Int         y;
	    }           v;
	}           u;
    }           d;

    int context;

    symptr_QUEUE sources, targets;
    symptr_ATOM Qloc;		/* location in action_queue /
				   formula_queue */
    char        marked:1;	/* flag */
    char        changed:1;	/* flag */
    char        entry:6;	/* 0 = INTERNAL, 1 = EDEN, 2 = DoNaLD, 3 =
				   Scout, 4 = ARCA */
    char       *master;		/* who change this symbol */

#ifdef DISTRIB
    agent_QUEUE OracleOf, HandleOf, StateOf;
                                /* agency of all agents in this
                                    symbol(observable) --sun */
#endif /* DISTRIB */

    symptr      next;		/* to link to another */
}           symbol;

/* bylist & tolist: for backward compatibility */
#define bylist targets
#define tolist sources

symptr      install(char *, int, int, int, Int);
symptr      lookup(char *, int);

#include "error.h"

extern Datum UndefDatum;
extern Inst *progp, *progbase, prog[];

extern union compiler_flags {
    struct {
	unsigned    define_level:8;
	unsigned    loop_level:8;
	unsigned    switch_level:8;
	unsigned    formula:1;
	unsigned    local_declare:1;
	unsigned    arg_declare:1;
        unsigned    procmacro:1;
    }           s;
    int         all;		/* an int is 32 bit */
}           compiler_flag;

extern struct t {
    Datum      *dp;
    Inst       *ip;
}          *entry_ptr, entry_tbl[];

#define reset_entry_tbl() (entry_ptr=entry_tbl)
#define reset_compiler_flags()	(compiler_flag.all = 0)

#define	indef		compiler_flag.s.define_level
#define informula	compiler_flag.s.formula
#define inauto		compiler_flag.s.local_declare
#define inpara		compiler_flag.s.arg_declare
#define	inloop		compiler_flag.s.loop_level
#define	inswitch	compiler_flag.s.switch_level
#define inprocmacro     compiler_flag.s.procmacro

extern Datum newdatum(Datum);
extern Datum newhdat(Datum);
extern void freedatum(Datum);

#ifndef DOSTE
#define INCLUDE 'H'
#include "inst.h"
#undef INCLUDE
#endif

/*----- typecheck.h -----*/
#define ADDRESSMASK	0xF000
#define SYMBOL		0x1000
#define LOCALV		0x2000
#define POINTERTYPE	0x0FFF
#define DPTR		0x0001
#define CPTR		0x0002
#define isundef(d)	(d.type == UNDEF)
#define ischar(d)	(d.type == MYCHAR)
#define isstr(d)	(d.type == STRING)
#define isint(d)	(d.type == INTEGER)
#define isnum(d)	(d.type==REAL||d.type==INTEGER||d.type==MYCHAR)
#define is_address(d)	(d.type & ADDRESSMASK)
#define is_symbol(d)	(is_address(d)==SYMBOL)
#define is_local(d)	(is_address(d)==LOCALV)
#define address_type(d)	(d.type & POINTERTYPE)
#define dptr(d)		((Datum*)d.u.v.x)
#define cptr(d)		((char *)d.u.s)
#define symbol_of(d)	((symptr)d.u.v.y)
#define local(d)	(d.u.v.y)

/*----- code.h -----*/
#define	NSTACK 1024
extern Datum stack[];		/* the stack */
extern Datum *stackp;		/* next free spot on stack */

#define top_of_stack		stackp[-1]	/* top of stack */

#ifdef DEBUG
#define reset_stack() ((Debug&1 ? debugMessage("DATSTK reset_stack\n") \
			: 0), \
		       stackp=stack)
#else /* not DEBUG */
#define reset_stack()		(stackp=stack)
#endif

extern Datum stack_underflow_err(void);
extern void stack_overflow_err(void);

#if defined(DEBUG) && !defined(WEDEN_ENABLED)
#include <stdio.h>
	void push(Datum);
	Datum pop(void);
#else /* not DEBUG and WebEDEN on or off */
#define push(d)	\
	if (stackp>=&stack[NSTACK]) stack_overflow_err(); else *stackp++ = d
#define pop()	\
	(stackp==stack? stack_underflow_err() : *--stackp)
#endif /* DEBUG */

#define	NPROG 	5000

extern Inst prog[];		/* the machine */
extern Inst *progp;		/* next free spot for code generation */
extern Inst *pc;		/* program counter during execurtion */

typedef struct Frame {		/* proc/func call stack frame */
    symptr      sp;		/* symbol table entry */
    Inst       *retpc;		/* where to resume after return */
    Datum      *stackp;		/* the stack to be resume */
    char       *hptr;		/* heap pointer */
    char       *master;		/* master of the call */
}           Frame;

#define NFRAME	100
extern Frame frame[];
extern Frame *fp;		/* frame pointer */

#define reset_frames()		(fp=frame)

/* INPUT DEVICE TYPES */
#define STRING_DEV	0
#define FILE_DEV	1

/*----- refer -----*/
#define NID 100
extern symptr_QUEUE IDlist;
extern void addID(symptr);

/*----- heap -----*/
#define HEAPSIZE		0x200000  /* change from 0x100000 for
                                             classroom --sun */
extern char *hptr, heap[];
extern char *getheap(int);
extern void freeheap(void);

/*----- eval -----*/
extern void reset_eval(void);
extern void mark_changed(symptr);
extern void change(symptr, int);
