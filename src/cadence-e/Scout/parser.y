%{
/*
 * $Id: parser.y,v 1.16 2001/08/02 16:26:11 cssbz Exp $
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

static char rcsid[] = "$Id: parser.y,v 1.16 2001/08/02 16:26:11 cssbz Exp $";

#include <stdio.h>
#include <string.h>

#include "../../../../../config.h"
#include "symbol.h"
#include "tree.h"
#include "../Eden/error.h"

#include "../EX/script.h"

#include "../Eden/emalloc.h"
#ifdef HAVE_DMALLOC
#include <dmalloc.h>
#endif

#define YYERROR_VERBOSE 1

extern Script *st_script;

/* function prototypes */
static WinStruct subst(WinStruct, WinField);
void        initDefaultWindow(void);
void        scout_err(char *);
void        yyerror(char *);
extern void printdef(tree *, FILE *);
extern void printsym(symbol *, FILE *);
int useOldTree = 0;

extern char scoutErrorStr[];
WinStruct   defaultWindow;

void
initDefaultWindow(void)
{
    defaultWindow.type = 0;
    defaultWindow.frame = 0;
    defaultWindow.string = 0;
    defaultWindow.box = 0;
    defaultWindow.pict = 0;
    defaultWindow.xmin = 0;
    defaultWindow.ymin = 0;
    defaultWindow.xmax = 0;
    defaultWindow.ymax = 0;
    defaultWindow.font = 0;
    defaultWindow.bgcolor = 0;
    defaultWindow.fgcolor = 0;
    defaultWindow.bdcolor = 0;
    defaultWindow.border = 0;
    defaultWindow.bdtype = 0;
    defaultWindow.align = -1;
    defaultWindow.sensitive = 0;
}

static      WinStruct
subst(WinStruct w, WinField f)
{
    switch (f.change) {
	case TYPE:
	w.type = f.f.type;
	break;
    case FRAME:
	if (w.frame)
	    freetree(w.frame);
	w.frame = f.f.frame;
	break;
    case STRING:
	if (w.string)
	    freetree(w.string);
	w.string = f.f.string;
	break;
    case BOX:
	if (w.box)
	    freetree(w.box);
	w.box = f.f.box;
	break;
    case PICT:
	if (w.pict)
	    freetree(w.pict);
	w.pict = f.f.pict;
	break;
    case XMIN:
	if (w.xmin)
	    freetree(w.xmin);
	w.xmin = f.f.xmin;
	break;
    case YMIN:
	if (w.ymin)
	    freetree(w.ymin);
	w.ymin = f.f.ymin;
	break;
    case XMAX:
	if (w.xmax)
	    freetree(w.xmax);
	w.xmax = f.f.xmax;
	break;
    case YMAX:
	if (w.ymax)
	    freetree(w.ymax);
	w.ymax = f.f.ymax;
	break;
    case FONT:
	if (w.font)
	    freetree(w.font);
	w.font = f.f.font;
	break;
    case BG:
	if (w.bgcolor)
	    freetree(w.bgcolor);
	w.bgcolor = f.f.bgcolor;
	break;
    case FG:
	if (w.fgcolor)
	    freetree(w.fgcolor);
	w.fgcolor = f.f.fgcolor;
	break;
    case BDCOLOR:
	if (w.bdcolor)
	    freetree(w.bdcolor);
	w.bdcolor = f.f.bdcolor;
	break;
    case BORDER:
	w.border = f.f.border;
	break;
    case BDTYPE:
	if (w.bdtype)
	    freetree(w.bdtype);
	w.bdtype = f.f.bdtype;
	break;
    case ALIGN:
	w.align = f.f.align;
	break;
    case SENSITIVE:
	w.sensitive = f.f.sensitive;
	break;
    };
    return w;
}
%}
%union {
	char		*s;	/* character string */
	double		 d;	/* double value */
	int		 i;	/* integer value */
	symbol		*v;	/* variables */
	tree		*t;	/* parse tree */
	WinStruct	 w;	/* window */
	WinField	 f;	/* window attributes */
}

%token <s> STR
%token <d> NUMBER ROW COLUMN
%token <i> CONTENT JUST SWITCH INTEGERHONEST
%token STRING INTEGER POINT BOX FRAME WINDOW DISPLAY IMAGE
%token TYPE PICT XMIN YMIN XMAX YMAX FONT BG FG BDCOLOR BORDER ALIGN SENSITIVE
%token BDTYPE
%token ALL
%token <v> STRVAR INTVAR PTVAR BOXVAR FRAMEVAR WINVAR DISPVAR UNKNOWN IMGVAR
%token FORMPT FORMBOX TEXTBOX FORMFRAME FORMWIN FORMDISP
%token SUBSTR STRCAT CONCAT STRLEN TOSTRING
%token DOTFRAME DOTSTR DOTBOX
%token DOTTYPE DOTPICT DOTXMIN DOTYMIN DOTXMAX DOTYMAX DOTFONT
%token DOTBG DOTFG DOTBDCOLOR DOTBORDER DOTBDTYPE DOTALIGN DOTSENSITIVE
%token DOTNE DOTNW DOTSE DOTSW DOTN DOTE DOTS DOTW
%token BOXSHIFT BOXINTERSECT BOXCENTRE BOXENCLOSING BOXREDUCE
%token APPEND DELETE IMGFUNC
%token IF THEN ELSE ENDIF
%token ImageFile ImageScale
%token ERROR

%type <v> var_name
%type <t> definition var_list unknown
%type <t> str_exp int_exp point_exp box_exp
%type <t> frame_exp win_exp display_exp image_exp
%type <t> box_list win_list
%type <w> win_field_list
%type <f> win_field

%nonassoc '='
%left OR
%left AND
%left EQ NE
%left LT LE GT GE
%left '+' '-'
%left '*' '/' '%'
%left UMINUS
%left DOTR DOTC
%left '.'
%left CONCAT
%left '&'

%expect 0

%%
program :
	  statement	
	| program statement
	;

statement :
	  ';'
	| declaration
	| definition	{ if ($1) define($1); st_script->ready = 1; }
	| enquiry
	| error ';'	{ yyerrok; }
	;

declaration :
	  STRING var_list ';'
		{
		  if (!declare(STRVAR, $2)) {	/* declaration error */
		    freetree($2);
		    scout_err(scoutErrorStr);
		  }
		}
	| INTEGER var_list ';'
		{
		  if (!declare(INTVAR, $2)) {	/* declaration error */
		    freetree($2);
		    scout_err(scoutErrorStr);
		  }
		}
	| POINT var_list ';'
		{
		  if (!declare(PTVAR, $2)) {	/* declaration error */
		    freetree($2);
		    scout_err(scoutErrorStr);
		  }
		}
	| BOX var_list ';'
		{
		  if (!declare(BOXVAR, $2)) {	/* declaration error */
		    freetree($2);
		    scout_err(scoutErrorStr);
		  }
		}
	| FRAME var_list ';'
		{
		  if (!declare(FRAMEVAR, $2)) {	/* declaration error */
		    freetree($2);
		    scout_err(scoutErrorStr);
		  }
		}
	| WINDOW var_list ';'
		{
		  if (!declare(WINVAR, $2)) {	/* declaration error */
		    freetree($2);
		    scout_err(scoutErrorStr);
		  }
		}
	| DISPLAY var_list ';'
		{
		  if (!declare(DISPVAR, $2)) {	/* declaration error */
		    freetree($2);
		    scout_err(scoutErrorStr);
		  }
		}
	| IMAGE var_list ';'
		{
		  if (!declare(IMGVAR, $2)) {	/* declaration error */
		    freetree($2);
		    scout_err(scoutErrorStr);
		  }
		}
	;

definition :
	  INTVAR '=' int_exp ';'		{ $$ = def_tree($1, $3); }
	| INTEGER unknown '=' int_exp ';'	{
		if (declare(INTVAR, $2))
			$$ = def_tree($2->l.v, $4);
		else {
			freetree($2);
			freetree($4);
			scout_err(scoutErrorStr);
		}
	  }
	| STRVAR '=' str_exp ';'		{ $$ = def_tree($1, $3); }
	| STRING unknown '=' str_exp ';'	{
		if (declare(STRVAR, $2))
			$$ = def_tree($2->l.v, $4);
		else {
			freetree($2);
			freetree($4);
			scout_err(scoutErrorStr);
		}
	  }
	| PTVAR '=' point_exp ';'		{ $$ = def_tree($1, $3); }
	| POINT unknown '=' point_exp ';'	{
		if (declare(PTVAR, $2))
			$$ = def_tree($2->l.v, $4);
		else {
			freetree($2);
			freetree($4);
			scout_err(scoutErrorStr);
		}
	  }
	| BOXVAR '=' box_exp ';'		{ $$ = def_tree($1, $3); }
	| BOX unknown '=' box_exp ';'		{
		if (declare(BOXVAR, $2))
			$$ = def_tree($2->l.v, $4);
		else {
			freetree($2);
			freetree($4);
			scout_err(scoutErrorStr);
		}
	  }
	| FRAMEVAR '=' frame_exp ';'		{ $$ = def_tree($1, $3); }
	| FRAME unknown '=' frame_exp ';'	{
		if (declare(FRAMEVAR, $2))
			$$ = def_tree($2->l.v, $4);
		else {
			freetree($2);
			freetree($4);
			scout_err(scoutErrorStr);
		}
	  }
	| WINVAR '=' win_exp ';'		{ $$ = def_tree($1, $3); }
	| WINDOW unknown '=' win_exp ';'	{
		if (declare(WINVAR, $2))
			$$ = def_tree($2->l.v, $4);
		else {
			freetree($2);
			freetree($4);
			scout_err(scoutErrorStr);
		}
	  }
	| DISPVAR '=' display_exp ';'		{ $$ = def_tree($1, $3); }
	| DISPVAR '&' '=' DISPVAR ';'           {
	        if ($1) {
	              if ($1->def) {
	                useOldTree = 1;
	                $$ = def_tree($1, tree3($4->def, '&', $1->def)); 
	              } else {
	                $$ = def_tree($1, $4->def);
	              }
	        } else {
	               freetree($1);
	               freetree($4);
	               scout_err(scoutErrorStr);
	        } 
	  }
	| DISPLAY unknown '=' display_exp ';'	{
		if (declare(DISPVAR, $2))
			$$ = def_tree($2->l.v, $4);
		else {
			freetree($2);
			freetree($4);
			scout_err(scoutErrorStr);
		}
	  }
	| IMGVAR '=' image_exp ';'		{ $$ = def_tree($1, $3); }
	| IMAGE unknown '=' image_exp ';'	{
		if (declare(IMGVAR, $2))
			$$ = def_tree($2->l.v, $4);
		else {
			freetree($2);
			freetree($4);
			scout_err(scoutErrorStr);
		}
	  }
	;

var_list :
	  unknown
	| var_list ',' unknown	{ $$ = tree3($1, ',', $3);	}
	;

unknown :
	  STRVAR		{ $$ = sym_tree($1, STRVAR);	}
	| INTVAR		{ $$ = sym_tree($1, INTVAR);	}
	| PTVAR			{ $$ = sym_tree($1, PTVAR);	}
	| BOXVAR		{ $$ = sym_tree($1, BOXVAR);	}
	| FRAMEVAR		{ $$ = sym_tree($1, FRAMEVAR);	}
	| WINVAR		{ $$ = sym_tree($1, WINVAR);	}
	| DISPVAR		{ $$ = sym_tree($1, DISPVAR);	}
	| UNKNOWN		{ $$ = sym_tree($1, UNKNOWN);	}
	| IMGVAR		{ $$ = sym_tree($1, IMGVAR);	}
	;

enquiry :
	  '?' ALL ';'		{ listsym(0);	}
	| '?' var_name ';'	{ listsym($2->name);	}
	;

var_name :
	  STRVAR
	| INTVAR
	| PTVAR
	| BOXVAR
	| FRAMEVAR
	| WINVAR
	| DISPVAR
	| UNKNOWN
	| IMGVAR
	;

str_exp :
	  STR			{ $$ = str_tree($1);		}
	| STRVAR		{ $$ = sym_tree($1, STRVAR);	}
	| str_exp CONCAT str_exp	{ $$ = tree3($1, CONCAT, $3);	}
	| STRCAT '(' str_exp ',' str_exp ')'
		{ $$ = tree3($3, STRCAT, $5);	}
	| SUBSTR '(' str_exp ',' int_exp ',' int_exp ')'
		{ $$ = tree3($3, SUBSTR, tree3($5, 0, $7));	}
	| TOSTRING '(' int_exp ')'	{ $$ = tree2($3, TOSTRING);	}
	| win_exp DOTSTR	{ $$ = tree2($1, DOTSTR);	}
	| win_exp DOTPICT	{ $$ = tree2($1, DOTPICT);	}
	| win_exp DOTBG		{ $$ = tree2($1, DOTBG);	}
	| win_exp DOTFG		{ $$ = tree2($1, DOTFG);	}
	| win_exp DOTBDCOLOR	{ $$ = tree2($1, DOTBDCOLOR);	}
	| win_exp DOTBDTYPE	{ $$ = tree2($1, DOTBDTYPE);	}
	| win_exp DOTFONT	{ $$ = tree2($1, DOTFONT);	}
	| IF int_exp THEN str_exp ELSE str_exp ENDIF
		{ $$ = tree3($2, IF, tree3($4, STRVAR, $6));	}
	;

int_exp	:
	  NUMBER		{ $$ = int_tree($1);		}
	| INTEGERHONEST         { $$ = inthonest_tree($1);      }
	| int_exp DOTR		{ $$ = tree2($1, ROW);		}
	| int_exp DOTC		{ $$ = tree2($1, COLUMN);	}
	| INTVAR		{ $$ = sym_tree($1, INTVAR);	}
	| int_exp '+' int_exp	{ $$ = tree3($1, '+', $3);	}
	| int_exp '-' int_exp	{ $$ = tree3($1, '-', $3);	}
	| int_exp '*' int_exp	{ $$ = tree3($1, '*', $3);	}
	| int_exp '/' int_exp	{ $$ = tree3($1, '/', $3);	}
	| int_exp '%' int_exp	{ $$ = tree3($1, '%', $3);	}
	| int_exp EQ int_exp	{ $$ = tree3($1, EQ, $3);	}
	| int_exp NE int_exp	{ $$ = tree3($1, NE, $3);	}
	| int_exp LT int_exp	{ $$ = tree3($1, LT, $3);	}
	| int_exp LE int_exp	{ $$ = tree3($1, LE, $3);	}
	| int_exp GT int_exp	{ $$ = tree3($1, GT, $3);	}
	| int_exp GE int_exp	{ $$ = tree3($1, GE, $3);	}
	| str_exp EQ str_exp	{ $$ = tree3($1, EQ, $3);	}
	| str_exp NE str_exp	{ $$ = tree3($1, NE, $3);	}
	| str_exp LT str_exp	{ $$ = tree3($1, LT, $3);	}
	| str_exp LE str_exp	{ $$ = tree3($1, LE, $3);	}
	| str_exp GT str_exp	{ $$ = tree3($1, GT, $3);	}
	| str_exp GE str_exp	{ $$ = tree3($1, GE, $3);	}
	| point_exp EQ point_exp	{ $$ = tree3($1, EQ, $3);	}
	| point_exp NE point_exp	{ $$ = tree3($1, NE, $3);	}
	| point_exp LT point_exp	{ $$ = tree3($1, LT, $3);	}
	| point_exp LE point_exp	{ $$ = tree3($1, LE, $3);	}
	| point_exp GT point_exp	{ $$ = tree3($1, GT, $3);	}
	| point_exp GE point_exp	{ $$ = tree3($1, GE, $3);	}
	| int_exp OR int_exp	{ $$ = tree3($1, OR, $3);	}
	| int_exp AND int_exp	{ $$ = tree3($1, AND, $3);	}
	| '-' int_exp	%prec UMINUS	{ $$ = tree2($2, UMINUS);	}
	| '(' int_exp ')'	{ $$ = tree2($2, '(');	}
	| STRLEN '(' str_exp ')'	{ $$ = tree2($3, STRLEN);	}
	| point_exp '.' int_exp	{ $$ = tree3($1, '.', $3);	}
	| win_exp DOTXMIN	{ $$ = tree2($1, DOTXMIN);	}
	| win_exp DOTYMIN	{ $$ = tree2($1, DOTYMIN);	}
	| win_exp DOTXMAX	{ $$ = tree2($1, DOTXMAX);	}
	| win_exp DOTYMAX	{ $$ = tree2($1, DOTYMAX);	}
	| IF int_exp THEN int_exp ELSE int_exp ENDIF
		{ $$ = tree3($2, IF, tree3($4, INTVAR, $6));	}
	;

point_exp :
	  PTVAR			{ $$ = sym_tree($1, PTVAR);	}
	| '{' int_exp ',' int_exp '}'	{ $$ = tree3($2, FORMPT, $4);	}
	| point_exp '+' point_exp	{ $$ = tree3($1, '+', $3);	}
	| point_exp '-' point_exp	{ $$ = tree3($1, '-', $3);	}
	| box_exp DOTNE		{ $$ = tree2($1, DOTNE);	}
	| box_exp DOTNW		{ $$ = tree2($1, DOTNW);	}
	| box_exp DOTSE		{ $$ = tree2($1, DOTSE);	}
	| box_exp DOTSW		{ $$ = tree2($1, DOTSW);	}
	| box_exp DOTN		{ $$ = tree2($1, DOTN);	}
	| box_exp DOTE		{ $$ = tree2($1, DOTE);	}
	| box_exp DOTS		{ $$ = tree2($1, DOTS);	}
	| box_exp DOTW		{ $$ = tree2($1, DOTW);	}
	| IF int_exp THEN point_exp ELSE point_exp ENDIF
		{ $$ = tree3($2, IF, tree3($4, PTVAR, $6));	}
	;

box_exp :
	  BOXVAR	{ $$ = sym_tree($1, BOXVAR);	}
	| '[' point_exp ',' point_exp ']'
			{ $$ = tree3($2, FORMBOX, $4);	}
	| '[' point_exp ',' int_exp ',' int_exp ']'
			{ $$ = tree3($2, TEXTBOX, tree3($4, 0, $6)); }
	| frame_exp '.' int_exp
			{ $$ = tree3($1, '.', $3); }
	| win_exp DOTBOX
			{ $$ = tree2($1, DOTBOX);	}
	| BOXSHIFT '(' box_exp ',' int_exp ',' int_exp ')'
			{ $$ = tree3($3, BOXSHIFT, tree3($5, 0, $7));	}
	| BOXINTERSECT '(' box_exp ',' box_exp ')'
			{ $$ = tree3($3, BOXINTERSECT, $5);	}
	| BOXCENTRE '(' box_exp ',' box_exp ')'
			{ $$ = tree3($3, BOXCENTRE, $5);	}
	| BOXENCLOSING '(' box_exp ',' box_exp ')'
			{ $$ = tree3($3, BOXENCLOSING, $5);	}
	| BOXREDUCE '(' box_exp ',' box_exp ')'
			{ $$ = tree3($3, BOXREDUCE, $5);	}
	| IF int_exp THEN box_exp ELSE box_exp ENDIF
		{ $$ = tree3($2, IF, tree3($4, BOXVAR, $6));	}
	;

box_list :
	  box_exp
	| box_list ',' box_exp	{ $$ = tree3($1, ',', $3);	}
	;

frame_exp :
	  FRAMEVAR		{ $$ = sym_tree($1, FRAMEVAR);	}
	| '(' box_list ')'	{ $$ = tree2($2, FORMFRAME);	}
	| win_exp DOTFRAME	{ $$ = tree2($1, DOTFRAME);	}
	| APPEND '(' frame_exp ',' int_exp ',' box_exp ')'
			{ $$ = tree3($3, APPEND, tree3($5, 0, $7));	}
	| DELETE '(' frame_exp ',' int_exp ')'
			{ $$ = tree3($3, DELETE, $5);	}
	| frame_exp '&' frame_exp
			{ $$ = tree3($1, '&', $3);	}
	| IF int_exp THEN frame_exp ELSE frame_exp ENDIF
		{ $$ = tree3($2, IF, tree3($4, FRAMEVAR, $6));	}
	;

win_exp :
	  WINVAR			{ $$ = sym_tree($1, WINVAR);	}
	| '{' win_field_list '}'	{ $$ = win_tree($2);		}
	| display_exp '.' int_exp	{ $$ = tree3($1, '.', $3);	}
	| IF int_exp THEN win_exp ELSE win_exp ENDIF
		{ $$ = tree3($2, IF, tree3($4, WINVAR, $6));	}
	;

win_field_list : 
	  win_field			{ initDefaultWindow();
					  $$ = subst(defaultWindow, $1);}
	| win_field_list win_field	{ $$ = subst($1, $2);		}
	| win_field_list ',' win_field	{ $$ = subst($1, $3);		}
	;

win_field :
	  TYPE ':' CONTENT	{ $$.f.type = $3;	$$.change = TYPE;   }
	| FRAME ':' frame_exp	{ $$.f.frame = $3;	$$.change = FRAME;  }
	| STRING ':' str_exp	{ $$.f.string = $3;	$$.change = STRING; }
	| BOX ':' box_exp	{ $$.f.box = $3;	$$.change = BOX;    }
	| PICT ':' str_exp	{ $$.f.pict = $3;	$$.change = PICT;   }
	| XMIN ':' int_exp	{ $$.f.xmin = $3;	$$.change = XMIN;   }
	| YMIN ':' int_exp	{ $$.f.ymin = $3;	$$.change = YMIN;   }
	| XMAX ':' int_exp	{ $$.f.xmax = $3;	$$.change = XMAX;   }
	| YMAX ':' int_exp	{ $$.f.ymax = $3;	$$.change = YMAX;   }
	| FONT ':' str_exp	{ $$.f.font = $3;	$$.change = FONT;   }
	| BG ':' str_exp	{ $$.f.bgcolor = $3;	$$.change = BG;     }
	| FG ':' str_exp	{ $$.f.fgcolor = $3;	$$.change = FG;     }
	| BDCOLOR ':' str_exp	{ $$.f.bdcolor = $3;	$$.change = BDCOLOR; }
	| BORDER ':' int_exp	{ $$.f.border = $3;	$$.change = BORDER; }
	| BDTYPE ':' str_exp	{ $$.f.bdtype = $3;	$$.change = BDTYPE; }
	| ALIGN ':' JUST	{ $$.f.align = $3;	$$.change = ALIGN;  }
/*	| SENSITIVE ':' SWITCH	{ $$.f.sensitive = $3;	$$.change = SENSITIVE; } */
	| SENSITIVE ':' int_exp	{ $$.f.sensitive = $3;	$$.change = SENSITIVE; }
	;

win_list :
	  win_exp
	| win_list '/' win_exp	{ $$ = tree3($1, '>', $3);	}
	;

display_exp :
	  DISPVAR		{ $$ = sym_tree($1, DISPVAR);	}
	| LT win_list GT	{ $$ = tree2($2, FORMDISP);	}
	| APPEND '(' display_exp ',' int_exp ',' win_exp ')'
			{ $$ = tree3($3, APPEND, tree3($5, 0, $7));	}
	| DELETE '(' display_exp ',' int_exp ')'
			{ $$ = tree3($3, DELETE, $5);	}
	| display_exp '&' display_exp
			{ $$ = tree3($1, '&', $3);	}
	| IF int_exp THEN display_exp ELSE display_exp ENDIF
		{ $$ = tree3($2, IF, tree3($4, DISPVAR, $6));	}
	;

image_exp :
	  IMGVAR		{ $$ = sym_tree($1, IMGVAR);	}
	| ImageFile '(' str_exp ',' str_exp ')'
		{ $$ = tree3(strdup("ImageFile"), IMGFUNC,
			tree3($3, STRVAR, tree3($5, STRVAR, tree3(0, 0, 0)))); }
	| ImageScale '(' image_exp ',' int_exp ','int_exp ')'
		{ $$ = tree3(strdup("ImageScale"), IMGFUNC,
			tree3($3, IMGVAR, tree3($5, INTVAR,
				tree3($7, INTVAR, tree3(0, 0, 0))))); }
	;
%%

#include <tk.h>
extern Tcl_Interp *interp;

void
scout_err(char *s)
{
    extern void yyrestart(void);
    extern int  yy_parse_init;

    yyrestart();		/* reset lexical analyzer */
    yy_parse_init = 1;		/* reset bison */

    deleteScript(st_script);

    errorf("SCOUT: %s", s);
}

void
yyerror(char *s)
{
#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "yyerror: %s\n", s);
#endif
    scout_err(s);
}
