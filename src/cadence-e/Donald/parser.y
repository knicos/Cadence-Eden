%{
/*
 * $Id: parser.y,v 1.16 2001/12/06 22:27:37 cssbz Exp $
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

static char rcsid[] = "$Id: parser.y,v 1.16 2001/12/06 22:27:37 cssbz Exp $";

#include "../../../../../config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tree.h"
#include "symbol.h"
#include "error.h"
#include "../Eden/error.h"
#include "oper.h"
#include "../EX/script.h"

#include "../Eden/emalloc.h"
#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

#define YYERROR_VERBOSE 1

#ifdef DEBUG
extern int Debug;
#define  display(a)     if (Debug & 16) fprintf(stderr, a)
#define  display2(a,i)  if (Debug & 16) fprintf(stderr, a, i)
#endif
#ifndef DEBUG
#define  display(a)     /* nothing */
#define  display2(a,i)  /* nothing */
#endif
#define ERROR -1

extern Script *dd_script;
extern int _changed;

extern void eden_declare(char *, int, char *);
extern void declare_openshape(void);
extern void Declare(int, tree);
extern void DeclareGraph(tree);
extern void Define(tree, tree);
extern void DefineFunc(int, tree);
extern void Delete(tree);
extern int reset_context(void);
extern void change_scope(tree);
extern void resume_scope(void);
extern int count_id(tree);
extern int count_expr(tree);
extern void freeTree(tree);
extern void yyerror(char *);
extern char *expr_to_donald_name(tree);
extern char *expr_to_eden_name(tree);
extern char *eden_full_name(char *);
static char *withinName;
static char temp[256];
extern int dd_appAgentName;

#ifdef DISTRIB
extern void propagateDonaldDef(char *, char *);   /*  for agency --sun */
extern int handle_check1(char *);
int  withinHandle=1;
#endif /* DISTRIB */
%}

%union {
	char   *s;      /* identifier, etc */
	void*     i;      /* integer value   */
	double  r;      /* floating point value */
	tree    t;      /* expression tree */
}

%token  <i>     INF PI
%token  	I XI FI I_1 XI_1 FI_1
%token  	IMAGE
%token  <s>     MYCHAR
%token  <r>     REAL
%token  <i>     INT BOOLEAN POINT LABEL GSPEC GSPECLIST ANY
%token  <i>     LINE CIRCLE ELLIPSE SHAPE OPENSHAPE ARC RECTANGLE
%token	<i>	GRAPH OF
%token	<i>	VIEWPORT
%token	<i>	DELETE
%token  <i>     WITHIN
%token  <i>     IF THEN ELSE
%token  <s>     ID IMGFUNC
%token  <s>     CSTRING
%token  <i>     BOOL
%token  <i>     INUMBER
%token  <r>     RNUMBER
%token  <i>     ITOS RTOS
%token  <i>     TRUNC FLOAT
%token  <i>     SQRT SIN COS TAN ASIN ACOS ATAN LOG EXP RANDOM
%token  <i>     ROT SCALE SCALEXY TRANS REFLECT
%token  <r>     INTERSECT
%token  <i>     PARALLEL PERPEND DISTANCE
%token  <i>     MIDPOINT PT_BETWN_PTS
%token  <i>     COLINEAR INTERSECTS SEPARATES INCLUDES INCIDENT
%token  <i>     DISTLARGER DISTSMALLER
/*
%token  <i>     DRAW
%token  <i>     MONITOR IMPOSE
%token  <i>     ALIAS AS
%token  <i>     FOR IN RANGE
*/
%token  <s>	QUERY

%type   <i>     type_name func1 func2 func3
%type   <t>     expr expr_list identifier local_identifier id local_id
%type   <t>     graph_spec graph_spec_list
/*
%type   <t>     within_id
%type   <t>     defn_monitor defn_impose
*/

%left		SEMICOLON
%left		COLON
%right		EQUALS
%left		COMMA
/*
%nonassoc	DOTDOT
*/
%left		ELSE
%left		OR
%left		AND
%right		NOT
%left		EQ_EQ NOT_EQ
%left		GT GT_EQ LT LT_EQ
%left		SLASH_SLASH
%nonassoc	ATSIGN
%left		PLUS MINUS
%left		STAR PERCENT DIV MOD
%left		TILDE TILDE_SLASH
%left		DOTX DOTY DOTARG DOTRAD
%left		DOT1 DOT2
%right		UMINUS
%left		SLASH
%nonassoc	LID
%nonassoc	AMPERSAND HASH VERTBAR NEWLINE
%nonassoc	LPAREN RPAREN LBRACK RBRACK
%nonassoc	LCURLY RCURLY LANGLE RANGLE
%expect 11

%%	      /*----- grammar -----*/

program_list:
	  /* nothing */
	| program_list stmt	{ dd_script->ready = 1; }
	| program_list error    { yyerrok; }
	;

stmt:
	  VIEWPORT ID		{ eden_declare($2, VIEWPORT, $2); }
	| declaration		{
				  if (_changed) { 
				    declare_openshape();
				    _changed = 0;
				  }
#ifdef DEBUG
				  if (Debug & 16) print_all_symbols();
#endif
				}
	| definition
	|			{ appendEden("%+eden\n", dd_script); }
	  evaluation		{ appendEden("%-eden\n", dd_script); }
	| within_clause         { 
#ifdef DISTRIB
	    withinHandle = 1;
#endif /* DISTRIB */
	}
	| delete		{
				  if (_changed) { 
				    declare_openshape();
				    _changed = 0;
				  }
				}
	| NEWLINE	       /* null statement */
/*
	| defn_monitor		{
				  if (_changed) { 
				    declare_openshape();
				    _changed = 0;
				  }
				}
	| defn_impose		{
				  if (_changed) {
				    declare_openshape();
				    _changed = 0;
				  }
				}
	| draw
	| alias
	| inside
*/
	;

delete:
	  DELETE expr_list NEWLINE	{ Delete($2); }
	;

/*
draw:     DRAW NEWLINE			{ draw_gfx(0); }
	| DRAW identifier NEWLINE	{ count_id($2);
					  draw_gfx($2);
					  free($2); }
	;
*/

within_id:
	  WITHIN identifier 
	     {
#ifdef DISTRIB
	       withinName = strdup(expr_to_eden_name($2)); /* within
                                                              door {}
                                                              - this
                                                              finds
                                                              'door' */
	       if (handle_check1(withinName)) {
		 /* The current agent is allowed to change the
                    contents of the variable 'withinName' */
		 dd_appAgentName--; /* we don't need to prepend the
                                      agent name any more */
		 withinHandle = 1;
		 sprintf(temp, "%%donald\nwithin %s {\n%%eden\n",
			 expr_to_donald_name($2));
		 propagateDonaldDef(withinName, temp);
		 change_scope($2); freeTree($2);
		 /*   propagateAgency1(withinName, temp);  */
	       } else { withinHandle = 0; }
#else
	       dd_appAgentName--;
	       change_scope($2); freeTree($2);
#endif /* DISTRIB */
	     }
	;

within_clause:
	  within_id stmt   {
#ifdef DISTRIB
	       if (withinHandle) {
		 resume_scope();
		 propagateDonaldDef(withinName, "%donald\n}\n%eden\n"); 
	       };
	       withinHandle = 1;
	       dd_appAgentName++;
#else
	       dd_appAgentName++;
	       resume_scope();
#endif
	  }
	                  
	| within_id NEWLINE LCURLY stmt_list RCURLY	{
#ifdef DISTRIB
	  if (withinHandle) {
	    resume_scope(); 
	    propagateDonaldDef(withinName, "%donald\n}\n%eden\n"); 
	  };
	  withinHandle = 1;
	  dd_appAgentName++;
#else
	  dd_appAgentName++;
	  resume_scope();
#endif
	}

	| within_id LCURLY stmt_list RCURLY	{
#ifdef DISTRIB
	  if (withinHandle) {
	    resume_scope(); 
	    propagateDonaldDef(withinName, "%donald\n}\n%eden\n"); 
	  }
	  withinHandle = 1;
	  dd_appAgentName++;
#else
	  dd_appAgentName++;
	  resume_scope();
#endif
	}
	;

stmt_list:
	  stmt				{ dd_script->ready = 1; }
	| stmt_list stmt		{ dd_script->ready = 1; }
	;

declaration:
	  type_name expr_list NEWLINE	{
#ifdef DISTRIB
	    if (withinHandle) {
	      Declare($1, $2);
	    } else {
	      freeTree($2);
	    }
#else
	    Declare($1, $2);
#endif /* DISTRIB */
	  }
	| GRAPH expr_list		{ DeclareGraph($2); }
	;

/*
defn_monitor:
	  MONITOR expr EQUALS
	      	expr COMMA expr COMMA expr NEWLINE
			{
			    int     no_id;
			    tree    mon_exp;

			    mon_exp = dtree3(OP_MONITOR, $6, $4, $8);
			    declare2(MONITOR, $2);
			    if ((no_id = count_id($2)) == ERROR)
				don_err(IdListExpect, 0);
			    if (no_id != count_expr(mon_exp))
				don_err(IdExprUnmatch, 0);
			    if (no_id > 1)
				appendEden("autocalc = OFF;\n", dd_script);
			    Define($2, mon_exp);
			    freeTree(mon_exp);
			    if (no_id > 1)
				appendEden("autocalc = ON;\n", dd_script);
			}
	;

defn_impose:
	    IMPOSE expr_list EQUALS expr NEWLINE
			{
			    int     no_id;
			    tree    imp_exp;

			    imp_exp = dtree1(OP_IMPOSE, $4);
			    declare2(IMPOSE, $2);
			    if ((no_id = count_id($2)) == ERROR)
				don_err(IdListExpect, 0);
			    if (no_id != count_expr(imp_exp))
				don_err(IdExprUnmatch, 0);
			    if (no_id > 1)
				appendEden("autocalc = OFF;\n", dd_script);
			    Define($2, imp_exp);
			    freeTree(imp_exp);
			    if (no_id > 1)
				appendEden("autocalc = ON;\n", dd_script);
			}
	;
*/

type_name:
	  INT			{ $$ = INT;		}
	| REAL			{ $$ = REAL;		}
	| MYCHAR			{ $$ = MYCHAR;		}
	| BOOLEAN		{ $$ = BOOLEAN;		}
	| POINT			{ $$ = POINT;		}
	| LINE			{ $$ = LINE;		}
	| ARC			{ $$ = ARC;		}
	| CIRCLE		{ $$ = CIRCLE;		}
	| ELLIPSE		{ $$ = ELLIPSE;		}
	| LABEL			{ $$ = LABEL;		}
	| SHAPE			{ $$ = SHAPE;		}
	| OPENSHAPE		{ $$ = OPENSHAPE;	}
	| IMAGE			{ $$ = IMAGE;		}
	| RECTANGLE             { $$ = RECTANGLE;       }
	;

definition:
	  expr_list EQUALS expr_list NEWLINE
                {
#ifdef DISTRIB
		  if (withinHandle) {
#endif /* DISTRIB */
		    int     no_id;
		    tree	t1 = $1;
		    tree	t3 = $3;

		    if ((no_id = count_id(t1)) == ERROR)
		      don_err(IdListExpect, 0);
		    if (no_id != count_expr(t3))
		      don_err(IdExprUnmatch, 0);
		    if (no_id > 1)
		      appendEden("autocalc = OFF;\n", dd_script);
		    if (t3->type == ANY)
		      t3->type = t1->type;
		    Define(t1, t3);
		    freeTree(t1);
		    freeTree(t3);
		    if (no_id > 1)
		      appendEden("autocalc = ON;\n", dd_script);
#ifdef DISTRIB
	          } else {
		    freeTree($1);
		    freeTree($3);
	          }
#endif /* DISTRIB */
		}
	| XI EQUALS expr NEWLINE
		{
			count_expr($3);
			DefineFunc(XI, $3);
			freeTree($3);
		}
	| FI EQUALS expr NEWLINE
		{
			count_expr($3);
			DefineFunc(FI, $3);
			freeTree($3);
		}
	;

evaluation:
	QUERY NEWLINE	   		{
					  appendEden($1, dd_script);
					  appendEden("\n", dd_script);
					  free($1);
					}
	| evaluation QUERY NEWLINE	{
					  appendEden($2, dd_script);
					  appendEden("\n", dd_script);
					  free($2);
					}
	;

expr_list:
	  expr
	| expr_list COMMA expr  { $$ = dtree2(OP_COMMA, $1, $3); }
/*
	| range
	| range COMMA expr_list { display("range,expr_list\n"); }
*/
	;


/*
alias:  ALIAS expr_list AS expr_list
	;

inside: ENTER expr DO stmt_list EXIT
	;

range:    iterator	      { display("iterator\n"); }
	| subrange	      { display("subrange\n"); }
	| expr LBRACK STAR RBRACK
	| expr LCURLY STAR RCURLY
	| expr SLASH STAR
	| expr HASH STAR
	| expr AMPERSAND STAR
	;

iterator:
	  LANGLE FOR type_name IN expr_list RANGE expr RANGLE
	;

subrange:
	  expr DOTDOT expr      { display("expr..expr "); }
	;

range_list:
	  type_name ID
	| range_list COMMA type_name ID IN range
	;
*/

expr:
	/* constants */
	  INF			{ $$ = dtree0(OP_INF); }
	| PI			{ $$ = dtree0(OP_PI);
				  $$->type = REAL;
				  Rvalue($$) = 3.141593;
				}
	| INUMBER		{ $$ = dtree0(OP_INUMBER);
				  $$->type = INT;
				  Ivalue($$) = $1;
				}

	| RNUMBER		{ $$ = dtree0(OP_RNUMBER);
				  $$->type = REAL;
				  Rvalue($$) = $1;
				  display2("rnumber(%lf) ", $1);
				}

	| CSTRING		{ $$ = dtree0(OP_CSTRING);
				  $$->type = MYCHAR;
				  Cvalue($$) = $1;
				  display2("cstring(%s) ", $1);
				}

	| BOOL			{ $$ = dtree0(OP_BOOL);
				  $$->type = BOOLEAN;
				  Bvalue($$) = $1;
				}
	/* graph related variables */
	| I			{ $$ = dtree0(OP_I); $$->type = INT; }
	| XI			{ $$ = dtree0(OP_XI); $$->type = REAL; }
	| FI			{ $$ = dtree0(OP_FI); $$->type = REAL; }
	| I_1			{ $$ = dtree0(OP_I_1); $$->type = INT; }
	| XI_1			{ $$ = dtree0(OP_XI_1); $$->type = REAL; }
	| FI_1			{ $$ = dtree0(OP_FI_1); $$->type = REAL; }
	/* graph spec */
	| LBRACK graph_spec_list RBRACK
				{ $$ = $2;	}
	| LBRACK NEWLINE graph_spec_list RBRACK
				{ $$ = $3;	}
	/* variable */
	| identifier	%prec LID
	/* eden variable */
	| ID NOT		{ $$ = dtree2(OP_EDEN, $1); }
	/* ( expr ) */
	| LPAREN expr RPAREN    { $$ = dtree1(OP_PAREN, $2); }
	/* constructors */
	| LCURLY expr COMMA expr RCURLY
				{  $$ = dtree2(OP_CART, $2, $4); }
	| LCURLY expr ATSIGN expr RCURLY
				{ $$ = dtree2(OP_POLAR, $2, $4); }
	| LBRACK expr COMMA expr RBRACK
				{ $$ = dtree2(OP_LINE, $2, $4); }
	| LBRACK expr COMMA expr COMMA expr RBRACK
				{ $$ = dtree3(OP_ARC, $2, $4, $6); }
	| CIRCLE LPAREN expr COMMA expr RPAREN
				{ $$ = dtree2(OP_CIRCLE, $3, $5); }
	| RECTANGLE LPAREN expr COMMA expr RPAREN
				{ $$ = dtree2(OP_RECTANGLE, $3, $5); }
	| ELLIPSE LPAREN expr COMMA expr COMMA expr RPAREN
				{ $$ = dtree3(OP_ELLIPSE, $3, $5, $7); }
	| LABEL LPAREN expr COMMA expr RPAREN
				{ $$ = dtree2(OP_LABEL, $3, $5); }
	/* arithmetic expressions */
	| expr PLUS expr	{ $$ = dtree2(OP_PLUS, $1, $3); }
	| expr MINUS expr       { $$ = dtree2(OP_MINUS, $1, $3); }
	| expr STAR expr	{ $$ = dtree2(OP_MULT, $1, $3); }
	| expr MOD expr		{ $$ = dtree2(OP_MOD, $1, $3); }
	| expr DIV expr		{ $$ = dtree2(OP_DIV, $1, $3); }
	| MINUS expr %prec UMINUS
				{ $$ = dtree1(OP_UMINUS, $2); }
	/* logic expressions */
/*
	| TILDE expr		{ $$ = dtree1(OP_TILDE, $2); }
*/
	| NOT expr		{ $$ = dtree1(OP_NOT, $2); }
	| expr AND expr		{ $$ = dtree2(OP_AND, $1, $3); }
	| expr OR expr		{ $$ = dtree2(OP_OR, $1, $3); }
	| expr EQ_EQ expr	{ $$ = dtree2(OP_EQ_EQ, $1, $3); }
	| expr NOT_EQ expr	{ $$ = dtree2(OP_NOT_EQ, $1, $3); }
	| expr LT expr		{ $$ = dtree2(OP_LT, $1, $3); }
	| expr LT_EQ expr	{ $$ = dtree2(OP_LT_EQ, $1, $3); }
	| expr GT expr		{ $$ = dtree2(OP_GT, $1, $3); }
	| expr GT_EQ expr	{ $$ = dtree2(OP_GT_EQ, $1, $3); }
	/* string expressions */
	| expr SLASH_SLASH expr	{ $$ = dtree2(OP_SLASH_SLASH, $1, $3); }
	| ITOS LPAREN expr RPAREN
				{ $$ = dtree1(OP_ITOS, $3); }
	| RTOS LPAREN expr COMMA expr RPAREN
				{ $$ = dtree2(OP_RTOS, $3, $5); }
	/* selectors */
	| expr DOTX		{ $$ = dtree1(OP_DOTX, $1); }
	| expr DOTY		{ $$ = dtree1(OP_DOTY, $1); }
	| expr DOT1		{ $$ = dtree1(OP_DOT1, $1); }
	| expr DOT2		{ $$ = dtree1(OP_DOT2, $1); }
	| expr DOTRAD		{ $$ = dtree1(OP_DOTRAD, $1); }
	| expr DOTARG		{ $$ = dtree1(OP_DOTARG, $1); }
	/* functions */
	| ID NOT LPAREN RPAREN
				{ $$ = dtree2(OP_FUNC, $1, 0); }
	| ID NOT LPAREN expr_list RPAREN
				{ $$ = dtree2(OP_FUNC, $1, $4); }
	| IMGFUNC LPAREN RPAREN
				{ $$ = dtree2(OP_IMGFUNC, $1, 0); }
	| IMGFUNC LPAREN expr_list RPAREN
				{ $$ = dtree2(OP_IMGFUNC, $1, $3); }
	| func1 LPAREN expr RPAREN
				{ $$ = dtree1($1, $3); }
	| func2 LPAREN expr COMMA expr RPAREN
				{ $$ = dtree2($1, $3, $5); }
	| func3 LPAREN expr COMMA expr COMMA expr RPAREN
				{ $$ = dtree3($1, $3, $5, $7); }
	| PARALLEL
		LPAREN expr COMMA expr COMMA expr COMMA expr RPAREN
				{ $$ = dtree4(OP_PARALLEL, $3, $5, $7, $9); }
	/* conditional expression */
	| IF expr THEN expr ELSE expr   %prec ELSE
				{ $$ = dtree3(OP_IF, $4, $2, $6); }
/*
	| VERTBAR expr VERTBAR
*/
	;

func1:
	  SQRT			{ $$ = OP_SQRT;		}
	| SIN			{ $$ = OP_SIN;		}
	| COS			{ $$ = OP_COS;		}
	| TAN			{ $$ = OP_TAN;		}
	| ASIN			{ $$ = OP_ASIN;		}
	| ACOS			{ $$ = OP_ACOS;		}
	| ATAN			{ $$ = OP_ATAN;		}
	| LOG			{ $$ = OP_LOG;		}
	| EXP			{ $$ = OP_EXP;		}
	| TRUNC			{ $$ = OP_TRUNC;	}
	| FLOAT			{ $$ = OP_FLOAT;	}
	| MIDPOINT		{ $$ = OP_MIDPOINT;	}
	| RANDOM		{ $$ = OP_RANDOM;	}
	;

func2:
	  INTERSECT		{ $$ = OP_INTERSECT;	}
	| PERPEND		{ $$ = OP_PERPEND;	}
	| DISTANCE		{ $$ = OP_DISTANCE;	}
	| SCALE			{ $$ = OP_SCALE;	}
	| INTERSECTS		{ $$ = OP_INTERSECTS;	}
	| INCLUDES		{ $$ = OP_INCLUDES;	}
	| INCIDENT		{ $$ = OP_INCIDENT;     }
	| REFLECT               { $$ = OP_REFLECT;      }
	;

func3:
	  ROT			{ $$ = OP_ROT;		}
	| TRANS			{ $$ = OP_TRANS;	}
	| SCALEXY		{ $$ = OP_SCALEXY;	}
	| PT_BETWN_PTS		{ $$ = OP_PT_BETWN_PTS;	}
	| COLINEAR		{ $$ = OP_COLINEAR;	}
	| SEPARATES		{ $$ = OP_SEPARATES;	}
	| DISTLARGER		{ $$ = OP_DISTLARGER;	}
	| DISTSMALLER		{ $$ = OP_DISTSMALLER;	}
	;

identifier:
	  local_identifier
	| SLASH local_identifier
				{ $$ = dtree2(OP_GLOBAL, $2); }
	;

local_identifier:
	  local_id SLASH local_identifier
				{ $$ = dtree2(OP_SLASH, $1, $3); }
	| TILDE_SLASH local_identifier
		{
		  $$ = dtree2(OP_SLASH, dtree1(OP_ID, 0), $2);
		  dd_appAgentName--;
		}
       	| local_id
	;

local_id:
	  id    %prec LID
	;

id:       ID		{ $$ = dtree1(OP_ID, $1); }
	;

graph_spec_list:
	  /* nothing */		{ $$ = dtree2(OP_GSPECLIST, 0, 0); }
	| graph_spec
	| graph_spec_list SEMICOLON graph_spec
				{ $$ = dtree2(OP_GSPECLIST, $1, $3); }
	| graph_spec_list SEMICOLON NEWLINE graph_spec
				{ $$ = dtree2(OP_GSPECLIST, $1, $4); }
	;

graph_spec:
	  type_name COLON		{ $$ = dtree2(OP_GSPEC, 0, (tree)$1); }
	| type_name COLON expr		{ $$ = dtree2(OP_GSPEC, $3, (tree)$1); }
	| type_name COLON NEWLINE	{ $$ = dtree2(OP_GSPEC, 0, (tree)$1); }
	| type_name COLON expr NEWLINE 	{ $$ = dtree2(OP_GSPEC, $3, (tree)$1); }
	;
%%

void
yyerror(char *s)
{
    don_err(Unclassified, s);
}

static char *errorstring[] = {ERRORSTRINGS};

/*---------------------------------------------------+---------+
						     | don_err |
						     +---------*/
#include <tk.h>
extern Tcl_Interp *interp;

void
don_err(enum errorcodes error_code, char *s)
{
    extern void yyrestart(void);
    extern int yy_parse_init;
    char errStr[120];
    Tcl_DString err, message;

    yyrestart();		/* reset lexical analyzer */
    yy_parse_init = 1;		/* reset bison */

    deleteScript(dd_script);

    sprintf(errStr, errorstring[(int) error_code], s);

    if (reset_context() > 0) {
      strcat(errStr, " (context was reset)");
    }

    errorf("DoNaLD: %s", errStr);
}

#ifdef DISTRIB
/* This is a sort of drop-in replacement for appendEden, but with
   distribution [Ash, with Sun] */
void
propagateDonaldDef(char *name, char *s)
{
    extern void appendEden(char *, Script *);
    
    appendEden("propagate(\"", dd_script);
    appendEden(name, dd_script);
    appendEden("\", \"", dd_script);
    appendEden(s, dd_script);
    appendEden("\");\n", dd_script);
}
#endif /* DISTRIB */
