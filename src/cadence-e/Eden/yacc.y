%{
/*
 * $Id: yacc.y,v 1.19 2002/07/10 19:26:05 cssbz Exp $
 */

/* This is from the Autoconf manual. Note the pragma directive is
   indented so that pre-ANSI C compilers will ignore it. [Ash] */
/* AIX requires this to be the first thing in the file */
#ifdef __GNUC__
# define alloca __builtin_alloca
#else
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
 #pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#endif

static char rcsid[] = "$Id: yacc.y,v 1.19 2002/07/10 19:26:05 cssbz Exp $";

#include "../config.h"
#include <stdlib.h>

#include "eden.h"

#include "emalloc.h"
#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

#include "error.h"

#define YYERROR_VERBOSE 1
#define WANT_SETYYPARSEINIT

#define code2(c1,c2)    	code(c1); code(c2)
#define code3(c1,c2,c3) 	code(c1); code(c2); code(c3)
#define fromto(F,T)		(F)[1] = (Inst)((T)-((F)+2))
extern char *textptr, textcode[];
extern symptr_QUEUE break_q, cont_q;
extern void patch(Inst *, Inst *, symptr_QUEUE *);
extern void dispatch(Inst *, symptr_QUEUE *);

#define dispatch_break()	dispatch(progp, &break_q); code2(jmp, 0)
#define dispatch_continue()	dispatch(progp, &cont_q); code2(jmp, 0)
#define patch_break(mark,p)	patch(mark, p, &break_q)
#define patch_continue(mark,p)	patch(mark, p, &cont_q)
#define rts			(Inst) 0

typedef struct {
    Inst       *location;
    Datum      *dp;
}           Garbage;

typedef struct {
    Inst       *start;
    Inst       *end;
}           UsefulGarbage;

static int  MAXGARBAGE = 0;
static int  MAXUSABLE = 0;
static Garbage *garbage;
static UsefulGarbage *usable;
static int  nGarbage = 0;
static int  nUsable = 0;
static int  garbageLevel = 0;
extern int  inEVAL;
extern int *autocalc;
extern int *eden_backticks_dependency_hack;

/* Garbage seems to be a structure describing which information on the
   stack can be thrown away. But see also the Usable structure :) */
void
markGarbage(location, dp)
    Inst       *location;
    Datum      *dp;
{
    if (MAXGARBAGE == 0) {
	garbage = (Garbage *) emalloc(sizeof(Garbage) * 64);
	MAXGARBAGE = 64;
    }
    if (nGarbage == MAXGARBAGE) {
	MAXGARBAGE += 64;
	garbage = (Garbage *) erealloc(garbage, sizeof(Garbage) * MAXGARBAGE);
    }
    garbage[nGarbage].location = location;
    garbage[nGarbage].dp = dp;
    nGarbage++;
}

/* Usable describes information on the stack that must be kept. Usable
   overrides Garbage - see Garbage above :) */
void
unmarkGarbage(start, end)
    Inst       *start;
    Inst       *end;
{
    if (MAXUSABLE == 0) {
	usable = (UsefulGarbage *) emalloc(sizeof(UsefulGarbage) * 4);
	MAXUSABLE = 4;
    }
    if (nUsable == MAXUSABLE) {
	MAXUSABLE += 4;
	usable = (UsefulGarbage *) erealloc(usable, sizeof(UsefulGarbage) *
					    MAXUSABLE);
    }
    usable[nUsable].start = start;
    usable[nUsable].end = end;
    nUsable++;
}

void
incGarbageLevel(void)
{
    garbageLevel++;
}

void
decGarbageLevel(void)
{
    --garbageLevel;
}

void
clearGarbage(void)
{
  int         i, j, save;

  if (--garbageLevel == 0) {
    for (i = 0; i < nGarbage; i++) {
      save = 0; /* nonzero ('true') if we should keep this information */
      for (j = 0; j < nUsable; j++) {
	if (garbage[i].location >= usable[j].start &&
	    garbage[i].location <= usable[j].end) {
	  /* this garbage is within this usable section, so we should
	   keep it */
	  save = 1;
	  break;
	}
      }
      if (!save) {
	/* don't save the info - free it up */
	freedatum(*garbage[i].dp);
	free(garbage[i].dp);
      }
    }
    nGarbage = nUsable = 0;
  }
}

void
defnonly(char *s)
{				/* warn if illegal definition */
    if (!indef)
	error2(s, "used outside definition");
}

extern void addentry(Datum *, Inst *);
extern Inst *code(/* Inst */);
extern Inst *code_related_by(symptr);
extern Inst *code_related_by_runtimelhs();
extern Inst *code_definition(int, symptr, Inst *, Inst *, Int, char *);
extern Inst *code_definition_runtimelhs(int, Inst *, Inst *, Int, char *);
extern Inst *code_eval(Inst *, Inst *);
extern void execute(Inst *); /* can be used to execute pc at end with (rts) */
extern void codeswitch(struct t *);
extern void insert_level_marker(int), delete_local_level(int);
extern void push_text(char *, int);

%}
%union {
	Datum      *dp;		/* constants */
	symptr      sym;	/* symbol table ptr */
	Inst       *inst;	/* machine instruction */
	Int         narg;	/* number of arguments */
	Inst        fun;	/* binop */
	struct t   *sw;		/* switch */
	char       *tbegin;	/* beginning of text */
}

%token  <sym>   UNDEF
%token  <sym>   REAL
%token  <sym>   INTEGER
%token  <sym>  	MYCHAR
%token  <sym>  	STRING
%token  <sym>   LIST
%token  <dp>    CONSTANT
%token  <sym>   VAR
%token  <sym>   FORMULA
%token  <sym>   BLTIN LIB LIB64 RLIB
%token  <sym>   FUNC PROC PMAC
%token  <sym>  	FUNCTION
%token  <sym>   PROCEDURE
%token  <sym>   PROCMACRO

%token  <sym>   AUTO PARA
%token  <narg>  LOCAL
%token  <sym>   BREAK CONTINUE
%token  <sym>   SWITCH CASE DEFAULT
%token  <sym>   DO FOR WHILE IF ELSE
%token  <sym>   SHIFT APPEND INSERT DELETE RETURN
%token  <narg>  ARG

%type   <sym>   id identifier
%type   <inst>  lvalue primary secondary
%type   <inst>  lazy_and lazy_or colon
%type   <inst>  expr
%type   <inst>  expr1 expr2 end_expr2 expr3 end_expr3
%type	<sw>	switch
%type   <inst>  cases
%type   <inst>  stmt compound
%type   <inst>  asgn
%type   <inst>  stmtlist
%type   <inst>  then else begin
%type	<narg>	action def_end
%type	<tbegin>is def_begin evaluate
%type   <inst>  defn declare_formula declare_action declare_relation
%type   <narg>  declare_local local_list
%type   <narg>  refer_opt id_list id_list_opt
%type   <narg>  arglist argument_list
%nonassoc IS TILDE_GT
%left   ','
%right  '=' PLUS_EQ MINUS_EQ
%right  '?'
%left   OR LAZY_OR
%left   AND LAZY_AND
%left   BITAND BITOR
%left   EQ_EQ NOT_EQ
%left   '>' GT_EQ '<' LT_EQ
%left   '+' '-' SLASH_SLASH
%left   '*' '/' '%'
%nonassoc  NEGATE NOT '!' PLUS_PLUS MINUS_MINUS '#' '&' ASTERISK EVAL
%expect 6
%%
program:     /* nothing */
	| stmt			{ code2(freeheap, rts);
				  textptr = textcode;
				  return 1; }
	| error			{ yyerrok; }
	;

lvalue:
	  id			{ $$ = code2(addr, $1); }
	| LOCAL			{ defnonly("local variable");
				  if (inEVAL)
				    error("can't reference local variable within eval()");
				  $$ = code2(localaddr, $1);
				}
	| '$'			{ defnonly("$");
				  if (inEVAL)
				    error("can't reference local $ variable within eval()");
				  $$ = code2(localaddr, (Inst)0);
				}
	| ARG			{ defnonly("$");
				  if (inEVAL)
				    error("can't reference local $ variable within eval()");
				  $$ = code2(localaddr, (Inst)0);
				  code3(pushint, (Inst)$1, indexcalc);
				}
	| lvalue '[' expr ']'		{ code(indexcalc); }
	| '*' primary    %prec ASTERISK	{ $$ = $2; }
	| '*' '&' lvalue		{ $$ = $3; }
	| '`' expr '`' 			{ $$ = $2; code(lookup_address);
		/* The below is a hack by Patrick which evaluates expr
                   at parse time.  It seems to be necessary in some
                   situations (eg the project timetable).  It was
                   added in yacc.y 1.8 (6th Sept 1999, release 1.0),
                   removed again in yacc.y 1.15 (27th July 2001,
                   release 1.13), and made optional in 1.18 (release
                   1.38).  [Ash] */
					  if (informula
					      && !inEVAL
					      && *autocalc
					      && *eden_backticks_dependency_hack) {
					    code(rts); execute($2);
					    progp--; /* remove rts */
					  }
	                                }
	| '(' lvalue ')'		{ $$ = $2; }
	;

/* these didn't work as attempts at adding concatopt... */
/*	| lvalue '=' primary SLASH_SLASH expr */
/*	| lvalue '=' id { $$=code2(addr, $3); } SLASH_SLASH expr */
/*	| lvalue '=' lvalue { code(getvalue); } SLASH_SLASH expr */
/*	| lvalue '=' simplelvalue SLASH_SLASH expr */

asgn:	  lvalue '=' expr		{ code(assign); }
	| lvalue PLUS_EQ expr	%prec '='
					{ code(inc_asgn); }
	| lvalue MINUS_EQ expr	%prec '='
					{ code(dec_asgn); }
	| lvalue '=' lvalue { code(getvalue); } SLASH_SLASH expr    %prec '='
					{ if ($1[1] == $3[1]) {
					    // l = l // expr: optimise
					    code(concatopt);
					  } else {
					    // l = notl // expr:
					    // treat normally
					    code(concat);
					    code(assign);
					  }
					}
	| PLUS_PLUS lvalue		{ code(pre_inc); $$ = $2; }
	| lvalue PLUS_PLUS		{ code(post_inc); }
	| MINUS_MINUS lvalue		{ code(pre_dec); $$ = $2; }
	| lvalue MINUS_MINUS		{ code(post_dec); }
	;

primary:
	  lvalue			{ code(getvalue); }
	| secondary
	| '(' expr ')'			{ $$ = $2; }
	;

secondary:
	  primary '(' arglist ')'
				{ code3(makelist, $3, eval); }
	| secondary '[' expr ']'
				{ code(sel); }
	;

stmt:
	  defn
	| expr ';'		{ code2(popd, freeheap); }
	| RETURN ';'		{ defnonly("return");
				  $$ = code2(pushUNDEF, rts); }
	| RETURN expr ';'	{ defnonly("return");
				  $$ = $2; code(rts); }
	| SHIFT ';'		{ defnonly("shift");
				  $$ = code3(localaddr, 0, shift);
				}
	| SHIFT lvalue ';'	{ $$ = $2; code(shift); }
	| APPEND lvalue ',' expr ';'
				{ $$ = $2; code2(append, freeheap); }
	| INSERT lvalue ',' expr ',' expr ';'
				{ $$ = $2; code2(insert, freeheap); }
	| DELETE lvalue ',' expr ';'
				{ $$ = $2; code2(delete, freeheap); }
	| CASE CONSTANT ':'	{ if (inswitch)
				    addentry($2, progp);
				  else
				    error("'case' used outside switch");
				  $$ = progp;
				}
	| DEFAULT ':'		{ if (inswitch)
				    addentry(0, progp);
				  else
				    error("'default' used outside switch");
				  $$ = progp;
				}
	| BREAK ';'		{ if (!inloop && !inswitch)
				    error("'break' used outside loop/switch");
				  $$ = progp;
				  dispatch_break();
				}
	| CONTINUE ';'			{ if (!inloop)
					    error("'continue' used outside loop");
					  $$ = progp;
					  dispatch_continue();
					}
	| switch expr cases stmt	{ dispatch_break();
					  fromto($3, progp);
					  code(switchcode);
					  codeswitch($1);
					  patch_break($2, progp);
					  $$ = $2;
					  --inswitch;
					}

	| for
	  expr1
	  expr2 end_expr2
	  expr3	end_expr3
	  stmt	         	{ Inst * p = code(jmp);
				  code($5 - (p + 2));
				  fromto($4, $7);
				  if ($3 != $4 - 2)
				    fromto($4-2, progp);
				  fromto($6, $3);
				  patch_continue($2, $5);
				  patch_break($2, progp);
				  $$ = $2;
				  --inloop;
				}

	| do stmt WHILE '(' expr ')' ';'
				{ Inst * p = code(jpt);
				  code($2 - (p + 2));
				  patch_continue($2, $5);
				  patch_break($2, progp);
				  $$ = $2;
				  --inloop; }

	| while '(' expr ')' then stmt
				{ Inst * p = code(jmp);
				  code($3 - (p + 2));
				  fromto($5, progp);
				  patch_continue($3, $3);
				  patch_break($3, progp);
				  $$ = $3;
				  --inloop; }

	| IF '(' expr ')' then stmt
				{ fromto($5, progp);
				  $$ = $3; }

	| IF '(' expr ')' then stmt
	  else stmt		{ fromto($5, $8);
				  fromto($7, progp);
				  $$ = $3; }

	| compound
	| ';'			{ $$ = progp; }
	;

while:    WHILE			{ inloop++;
				  dispatch(progp, &cont_q);
				  dispatch(progp, &break_q); }
	;

do:       DO			{ inloop++;
				  dispatch(progp, &cont_q);
				  dispatch(progp, &break_q); }
	;

for:      FOR '('		{ inloop++;
				  dispatch(progp, &cont_q);
				  dispatch(progp, &break_q); }
	;

expr1:    ';'			{ $$ = progp; }
	| expr ';'		{ code(popd); }
	;

expr2:    ';'			{ $$ = code2(jmp, 0); }
	| expr ';'		{ code2(jpnt, 0); }
	;

end_expr2: /* nothing */	{ $$ = code2(jmp, 0); }
	;

expr3:    ')'			{ $$ = code2(jmp, 0); }
	| expr ')'		{ code3(popd, jmp, 0); }
	;

end_expr3: /* nothing */	{ $$ = code2(jmp, 0); }
	;

then:	  /* nothing */		{ $$ = code2(jpnt, 0); }
	;

else:	 ELSE			{ $$ = code2(jmp, 0); }
	;

compound: '{' stmtlist '}'	{ $$ = $2; }
	;

stmtlist: /* nothing */		{ /* A loop with empty contents still
				     needs to prevent the heap from
				     overflowing [Ash] */
				  /* $$ = progp; */
				  $$ = code(freeheap);
				}
	| stmtlist stmt         { /*fprintf(stderr, "rd inprocmacro=%d\n", inprocmacro);*/
				  if (inprocmacro) code(eager); }
	;

switch:   SWITCH '('		{ $$ = entry_ptr;
				  inswitch++;
				  dispatch(progp, &break_q);
				}
	;

cases:	  ')'			{ $$ = code2(jmp, 0); }
	;

begin:    /* nothing */		{ $$ = progp; }
	;

id:       identifier		{ if (informula && !inEVAL) addID($1); }
	| BLTIN
	| LIB
	| LIB64
	| RLIB
	;

identifier:
	  VAR
	| FORMULA
	| FUNCTION
	| PROCEDURE
	| PROCMACRO
	;

expr:
	  CONSTANT			{ markGarbage(progp, $1);
					  $$ = code2(constpush, (Inst)$1); }
	| asgn		{ if (informula)
			    error("assignment used inside formula");
			}
	| primary	/* already coded in primary */
	| '&' lvalue    %prec NEGATE	{ $$ = $2; }
	| '[' begin arglist ']'		{ code2(makelist, (Inst)$3); $$ = $2; }
	| expr '+' expr			{ code(add); }
	| expr '-' expr			{ code(sub); }
	| expr '*' expr			{ code(mul); }
	| expr '/' expr			{ code(divide); }
	| expr '%' expr			{ code(mod); }
	| expr SLASH_SLASH expr		{ code(concat); }
	| expr '>' expr			{ code(gt); }
	| expr GT_EQ expr		{ code(ge); }
	| expr '<' expr			{ code(lt); }
	| expr LT_EQ expr		{ code(le); }
	| expr EQ_EQ expr		{ code(eq); }
	| expr NOT_EQ expr		{ code(ne); }
	| '?'				{ code(noupdate); }
	  lvalue	%prec NEGATE	{ $$ = $3;
					  code(query);
					  code(resetupdate);
					}
	| '-' expr      %prec NEGATE	{ $$ = $2; code(negate); }
	| expr '#'			{ code(listsize); }
	| '!' expr			{ $$ = $2; code(lazy_not); }
	| NOT expr			{ $$ = $2; code(not); }
	| expr '?' then expr colon expr { fromto($3, $6);
					  fromto($5, progp);
					}
	| expr AND expr			{ code(and); }
	| expr OR expr			{ code(or); }
	| expr lazy_and expr	%prec LAZY_AND
					{ fromto($2, progp);
					  code(cnv_2_bool);
					}
	| expr lazy_or expr	%prec LAZY_OR
					{ fromto($2, progp);
					  code(cnv_2_bool);
					}
	| expr BITAND expr		{ code(bitand); }
	| expr BITOR expr		{ code(bitor); }
	| evaluate '(' expr ')'             { code(rts);
	                                 unmarkGarbage($3, progp);
	                                 $$ = code_eval($3, progp); 
	                                 inEVAL = 0;
	                                }
	;
evaluate:   EVAL                            { inEVAL = 1; }

colon:	  ':'				{ $$ = code2(jmp, 0); }
	;

lazy_and: LAZY_AND			{ code(ddup);
					  $$ = code2(jpnt, 0);
					  code(popd);
					}
	;

lazy_or:  LAZY_OR			{ code(ddup);
					  $$ = code2(jpnf, 0);
					  code(popd);
					}
	;

defn:     declare_formula
	| declare_action
	| declare_relation
	;

declare_relation:
	  identifier tilde_gt '[' id_list_opt ']' ';'
					{ $$ = code_related_by($1); }
	/* related_by_runtimelhs by [Ash] */
	| '`' expr '`'			{ $$ = $2; code(lookup_address); }
	  tilde_gt '[' id_list_opt ']' ';'
					{ $$ = code_related_by_runtimelhs(); }
	;

tilde_gt:
	TILDE_GT			{ addID((symptr) 0); }
	;

declare_formula:
	  identifier is expr ';'	{ informula = 0;
					  code(rts);
					  unmarkGarbage($3, progp);
					  $$ = code_definition(
					    FORMULA, $1, $3, progp, 0, $2);
					}
	| '`' expr '`'                  { code(lookup_address); }  /* --sun */
	  is expr ';'                   {  informula = 0;
	                                   code(rts);
	                                   unmarkGarbage($6, progp);
	                                   $$ = code_definition_runtimelhs(
	                                    FORMULA, $6, progp, 0, $5); 
	                                } 
	;

is:	  IS				{ $$ = textptr;
					  informula = 1;
					  addID((symptr) 0);
					}
	;

declare_action:
	  action identifier		{ inprocmacro = ($1 == 2);
					  /*fprintf(stderr, "inprocmacro=%d\n",
					            inprocmacro);*/
	                                  addID((symptr) 0); }
	  refer_opt
	  def_begin
		declare_para
		declare_local
		stmtlist
	  def_end			{ code2(pushUNDEF, rts);
					  unmarkGarbage($8, progp);
					  $$ = code_definition(
						($1 ? (($1 == 1) ? FUNCTION :
						                  PROCMACRO) :
						     PROCEDURE),
						$2, $8, progp, $7, $5
					       );
					  delete_local_level($9);
					  inprocmacro = 0;
					}
	;

action:   FUNC				{ $$ = 1; }
	| PROC				{ $$ = 0; }
	| PMAC                          { $$ = 2; }
	;

def_begin:
	  '{'				{
					  static char c = '{';
					  if (indef)
					    $$ = textptr - 1;
					  else {
					    $$ = textptr;
					    push_text(&c,1);
					  }
					  insert_level_marker(++indef);
					}
	;

def_end:
	  '}' 				{ $$ = indef--; }
	;

/* optional refer_opt: ie nothing, or a refer [Ash] */
refer_opt:
	  /* nothing */			{ $$ = 0; }
	| ':' id_list			{ $$ = $2; }
	;

/* optional id_list: ie nothing, or an id_list [Ash] */
id_list_opt:
	  /* nothing */			{ $$ = 0; }
	| id_list
	;

id_list:
	  identifier			{ $$ = 1; addID($1); }
	| id_list ',' identifier	{ $$ = $1 + 1; addID($3); }
	;

declare_para:	/* nothing */
	| PARA 				{ inpara = 1; }
	  local_list ';'		{ inpara = 0; }
	;

declare_local:
	  /* nothing */			{ $$ = 0; }
	| declare_local
	  AUTO				{ inauto = 1; }
	  local_list ';'		{ inauto = 0; $$ = $1 + $4; }
	;

local_list:
	  LOCAL				{ $$ = 1; }
	| local_list ',' LOCAL		{ $$ = $1 + 1; }
	;

arglist:  /* nothing */			{ $$ = 0; }
	| argument_list
	;

argument_list:
	  expr				{ $$ = 1; }
	| argument_list ',' expr	{ $$ = $1 + 1; }
	;
%%
    /*----------------------- end of grammar --------------------------*/
