/*
 * $Id: inst.h,v 1.9 2002/02/27 16:37:24 cssbz Exp $
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

/* This file defines C functions that can be used as Eden VM opcodes:
   ie function pointers to them can be coded into the prog array and
   they can be invoked by execute() or from somewhere else by a
   function pointer dereference.

   This file gets included from eden.h, when INCLUDE should be set to
   'H' for Header, and from code.c if DEBUG is defined, for use in the
   disAss function, when INCLUDE should be set to 'T' to Table.  [Ash] */

#if INCLUDE == 'H'
#define INSTFUNC(name)extern void name(void);
#elif INCLUDE == 'T'
#ifdef __STDC__
#define INSTFUNC(name){#name, name},
#else /* not __STDC__... */
#define INSTFUNC(name){"name", name},
#endif
#else
#error Wrong value for preprocessor INCLUDE
#endif

/* These opcodes are defined in machine.c */
INSTFUNC(add)
INSTFUNC(sub)
INSTFUNC(mul)
INSTFUNC(divide)
INSTFUNC(mod)
INSTFUNC(negate)
INSTFUNC(lazy_not)
INSTFUNC(not)
INSTFUNC(concat)
INSTFUNC(concatopt)
INSTFUNC(jmp)
INSTFUNC(jpt)
INSTFUNC(jpf)
INSTFUNC(jpnt)
INSTFUNC(jpnf)
INSTFUNC(and)
INSTFUNC(or)
INSTFUNC(ddup)
INSTFUNC(popd)
INSTFUNC(pushUNDEF)
INSTFUNC(pushint)
INSTFUNC(constpush)
INSTFUNC(cnv_2_bool)
INSTFUNC(gt)
INSTFUNC(lt)
INSTFUNC(ge)
INSTFUNC(le)
INSTFUNC(eq)
INSTFUNC(ne)
INSTFUNC(switchcode)
INSTFUNC(definition)
INSTFUNC(definition_runtimelhs)
INSTFUNC(assign)
INSTFUNC(inc_asgn)
INSTFUNC(dec_asgn)
INSTFUNC(pre_inc)
INSTFUNC(post_inc)
INSTFUNC(pre_dec)
INSTFUNC(post_dec)
INSTFUNC(noupdate)
INSTFUNC(resetupdate)
INSTFUNC(addr)
INSTFUNC(lookup_address)
INSTFUNC(localaddr)
INSTFUNC(indexcalc)
INSTFUNC(makelist)
INSTFUNC(getvalue)
INSTFUNC(sel)
INSTFUNC(listsize)
INSTFUNC(shift)
INSTFUNC(append)
INSTFUNC(insert)
INSTFUNC(delete)
INSTFUNC(query)
INSTFUNC(bitand)
INSTFUNC(bitor)
/* ... end machine.c */

/* this is defined in eval.c */
INSTFUNC(eager)

/* this is defined in heap.c */
INSTFUNC(freeheap)

/* these are defined in code.c */
INSTFUNC(eval)
INSTFUNC(related_by_code)
INSTFUNC(related_by_code_runtimelhs)

#undef INSTFUNC
