/*
 * $Id: oper.h,v 1.7 2001/12/06 22:26:23 cssbz Exp $
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

                /*----- operator code -----*/

#define OP_INF          0
#define OP_ASSIGN       1
#define OP_PLUS         2
#define OP_MINUS        3
#define OP_MULT         4
#define OP_MOD          5
#define OP_DIV          6
#define OP_UMINUS       7
#define OP_TILDE        8
#define OP_DOTX         9
#define OP_DOTY         10
#define OP_DOT1         11
#define OP_DOT2         12
#define OP_DOTRAD       13
#define OP_DOTARG       14
#define OP_CART         15
#define OP_POLAR        16
#define OP_LINE         17
#define OP_ROT          18
#define OP_SCALE        19
#define OP_SLASH        20
#define OP_ID           21
#define OP_INUMBER      22
#define OP_COMMA        23
#define OP_PAREN        24
#define OP_IF           25
#define OP_VECT_ADD     26
#define OP_VECT_SUB     27
#define OP_SCALAR_MULT  28
#define OP_SCALAR_DIV   29
#define OP_SCALAR_MOD   30
#define OP_COS          31
#define OP_SIN          32
#define OP_RNUMBER      33
#define OP_INTERSECT    34
#define OP_PARALLEL     35
#define OP_PERPEND      36
#define OP_CIRCLE       37
#define OP_AND          38
#define OP_OR           39
#define OP_NOT          40
#define OP_BOOL         41
#define OP_PT_BETWN_PTS 42
#define OP_COLINEAR     43
#define OP_INTERSECTS   44
#define OP_SEPARATES    45
#define OP_INCLUDES     46
#define OP_DISTLARGER   47
#define OP_DISTSMALLER  48
#define OP_LABEL        49
#define OP_MONITOR      50
#define OP_IMPOSE       51
#define OP_CSTRING      52
#define OP_DISTANCE     53
#define OP_MIDPOINT     54
#define OP_EQ_EQ	55
#define OP_NOT_EQ	56
#define OP_GT		57
#define OP_GT_EQ	58
#define OP_LT		59
#define OP_LT_EQ	60
#define OP_SQRT		61
#define OP_TAN		62
#define OP_ASIN		63
#define OP_ACOS		64
#define OP_ATAN		65
#define OP_LOG		66
#define OP_EXP		67
#define OP_TRUNC	68
#define OP_FLOAT	69
#define OP_INCIDENT	70
#define OP_TRANS	71
#define OP_ELLIPSE      72
#define OP_RANDOM       73
#define OP_ARC		74
#define OP_PI		75
#define OP_SLASH_SLASH	76
#define OP_ITOS		77
#define OP_RTOS		78
#define OP_I		79
#define OP_XI		80
#define OP_FI		81
#define OP_I_1		82
#define OP_XI_1		83
#define OP_FI_1		84
#define OP_GSPECLIST	85
#define OP_GSPEC	86
#define OP_GLOBAL	87
#define OP_FUNC		88
#define OP_EDEN		89
#define OP_IMGFUNC	90
#define OP_RECTANGLE    91
#define OP_REFLECT      92
#define OP_SCALEXY	93

#define UNARY           -1
#define BINARY          -2
#define TRIARY          -3
#define QUADARY         -4
#define INFIX           -5
#define POSTFIX         -6
#define CONSTANT        -7

struct trans {
        int     order;
        char   *op_str;
} ;

#define TRANSLAT  \
{                                                          \
        /* OP_INF    */ { CONSTANT,     "[ INF ]"       }, \
        /* OP_ASSIGN */ { INFIX,        " = "           }, \
        /* OP_PLUS   */ { INFIX,        " + "           }, \
        /* OP_MINUS  */ { INFIX,        " - "           }, \
        /* OP_MULT   */ { INFIX,        " * "           }, \
        /* OP_MOD    */ { INFIX,        " % "           }, \
        /* OP_DIV    */ { INFIX,        " / "           }, \
        /* OP_UMINUS */ { UNARY,        "- "            }, \
        /* OP_TILDE  */ { UNARY,        0               }, \
        /* OP_DOTX   */ { UNARY,        "dotx("         }, \
        /* OP_DOTY   */ { UNARY,        "doty("         }, \
        /* OP_DOT1   */ { UNARY,        "dot1("         }, \
        /* OP_DOT2   */ { UNARY,        "dot2("         }, \
        /* OP_DOTRAD */ { UNARY,        "dotrad("       }, \
        /* OP_DOTARG */ { UNARY,        "dotarg("       }, \
        /* OP_CART   */ { BINARY,       "cart("         }, \
        /* OP_POLAR  */ { BINARY,       "polar("        }, \
        /* OP_LINE   */ { BINARY,       "line("         }, \
        /* OP_ROT    */ { TRIARY,       "rot("          }, \
        /* OP_SCALE  */ { BINARY,       "scale("        }, \
        /* OP_SLASH  */ { OP_SLASH,     ""              }, \
        /* OP_ID     */ { OP_ID,	0               }, \
        /* OP_INUMBER*/ { OP_INUMBER ,	0               }, \
        /* OP_COMMA  */ { INFIX,        ", "            }, \
        /* OP_PAREN  */ { UNARY,        "("             }, \
        /* OP_IF     */ { OP_IF,	0               }, \
      /* OP_VECT_ADD */ { BINARY,       "vector_add("   }, \
      /* OP_VECT_SUB */ { BINARY,       "vector_sub("   }, \
   /* OP_SCALAR_MULT */ { BINARY,       "scalar_mult("  }, \
    /* OP_SCALAR_DIV */ { BINARY,       "scalar_div("   }, \
    /* OP_SCALAR_MOD */ { BINARY,       "scalar_mod("   }, \
        /* OP_COSINE */ { UNARY,        "cos("          }, \
        /* OP_SINE   */ { UNARY,        "sin("          }, \
       /* OP_RNUMBER */ { OP_RNUMBER,	0               }, \
     /* OP_INTERSECT */ { BINARY,       "intersect("    }, \
      /* OP_PARALLEL */ { QUADARY,      "parallel("     }, \
       /* OP_PERPEND */ { BINARY,       "perpend("      }, \
        /* OP_CIRCLE */ { BINARY,       "circle("       }, \
           /* OP_AND */ { INFIX,        " && "          }, \
            /* OP_OR */ { INFIX,        " || "          }, \
           /* OP_NOT */ { UNARY,        "! "            }, \
          /* OP_BOOL */ { OP_BOOL,	0               }, \
  /* OP_PT_BETWN_PTS */ { TRIARY,       "between("      }, \
      /* OP_COLINEAR */ { TRIARY,       "colinear("     }, \
    /* OP_INTERSECTS */ { BINARY,       "intersects("   }, \
     /* OP_SEPARATES */ { TRIARY,       "separates("    }, \
      /* OP_INCLUDES */ { BINARY,       "includes("     }, \
    /* OP_DISTLARGER */ { TRIARY,       "distlarger("   }, \
   /* OP_DISTSMALLER */ { TRIARY,       "distsmaller("  }, \
         /* OP_LABEL */ { BINARY,       "label("        }, \
       /* OP_MONITOR */ { OP_MONITOR,	0               }, \
        /* OP_IMPOSE */ { OP_IMPOSE,	0               }, \
       /* OP_CSTRING */ { OP_CSTRING,	0               }, \
       /*OP_DISTANCE */ { BINARY,       "dist("         }, \
      /* OP_MIDPOINT */ { UNARY,        "midpoint("     }, \
      /* OP_EQ_EQ */	{ INFIX,	" == "		}, \
      /* OP_NOT_EQ */	{ INFIX,	" != "		}, \
      /* OP_GT */	{ INFIX,	" > "		}, \
      /* OP_GT_EQ */	{ INFIX,	" >= "		}, \
      /* OP_LT */	{ INFIX,	" < "		}, \
      /* OP_LT_EQ */	{ INFIX,	" <= "		}, \
      /* OP_SQRT */	{ UNARY,	"sqrt("		}, \
      /* OP_TAN */	{ UNARY,	"tan("		}, \
      /* OP_ASIN */	{ UNARY,	"asin("		}, \
      /* OP_ACOS */	{ UNARY,	"acos("		}, \
      /* OP_ATAN */	{ UNARY,	"atan("		}, \
      /* OP_LOG */	{ UNARY,	"log("		}, \
      /* OP_EXP */	{ UNARY,	"exp("		}, \
      /* OP_TRUNC */	{ UNARY,	"int("		}, \
      /* OP_FLOAT */	{ UNARY,	"float("	}, \
      /* OP_INCIDENT */ { BINARY,	"incident("	}, \
      /* OP_TRANS */	{ TRIARY,	"trans("	}, \
      /* OP_ELLIPSE */  { TRIARY,       "ellipse("      }, \
      /* OP_RANDOM */   { UNARY,        "DD_random("    }, \
      /* OP_ARC */	{ TRIARY,       "arc("		}, \
      /* OP_PI */	{ CONSTANT,	"PI"		}, \
   /* OP_SLASH_SLASH */	{ INFIX,	" // "		}, \
      /* OP_ITOS */	{ UNARY,	"str("		}, \
      /* OP_RTOS */	{ BINARY,	"rtos("		}, \
      /* OP_I */	{ CONSTANT,	"?1"		}, \
      /* OP_XI */	{ CONSTANT,	"?2"		}, \
      /* OP_FI */	{ CONSTANT,	"?3"		}, \
      /* OP_I_1 */	{ CONSTANT,	"?4"		}, \
      /* OP_XI_1 */	{ CONSTANT,	"?5"		}, \
      /* OP_FI_1 */	{ CONSTANT,	"?6"		}, \
      /* OP_GSPECLIST */{ OP_GSPECLIST,	0		}, \
      /* OP_GSPEC */	{ OP_GSPEC,	0		}, \
       /* OP_GLOBAL  */ { OP_GLOBAL,	""              }, \
       /* OP_FUNC  */	{ OP_FUNC,	0               }, \
       /* OP_EDEN  */	{ OP_EDEN,	0               }, \
      /* OP_IMGFUNC  */	{ OP_IMGFUNC,	0               }, \
     /* OP_RECTANGLE */ { BINARY,       "rectangle("    }, \
     /* OP_REFLECT */   { BINARY,       "reflect("      }, \
     /* OP_SCALEXY */   { TRIARY,       "scalexy("      } \
}

#define ORIGINAL  \
{                                                          \
        /* OP_INF    */ { OP_INF,       "inf"       }, \
        /* OP_ASSIGN */ { INFIX,        " = "           }, \
        /* OP_PLUS   */ { INFIX,        " + "           }, \
        /* OP_MINUS  */ { INFIX,        " - "           }, \
        /* OP_MULT   */ { INFIX,        " * "           }, \
        /* OP_MOD    */ { INFIX,        " mod "         }, \
        /* OP_DIV    */ { INFIX,        " div "         }, \
        /* OP_UMINUS */ { UNARY,        "-"             }, \
        /* OP_TILDE  */ { UNARY,        "~"             }, \
        /* OP_DOTX   */ { POSTFIX,      ".x"            }, \
        /* OP_DOTY   */ { POSTFIX,      ".y"            }, \
        /* OP_DOT1   */ { POSTFIX,      ".1"            }, \
        /* OP_DOT2   */ { POSTFIX,      ".2"            }, \
        /* OP_DOTRAD */ { POSTFIX,      ".rad"          }, \
        /* OP_DOTARG */ { POSTFIX,      ".arg"          }, \
        /* OP_CART   */ { OP_CART,      0         }, \
        /* OP_POLAR  */ { OP_POLAR,     0        }, \
        /* OP_LINE   */ { OP_LINE,      0         }, \
        /* OP_ROT    */ { TRIARY,       "rot("          }, \
        /* OP_SCALE  */ { BINARY,       "scale("        }, \
        /* OP_SLASH  */ { OP_SLASH,     "/"             }, \
        /* OP_ID     */ { OP_ID,	0               }, \
        /* OP_INUMBER*/ { OP_INUMBER ,	0               }, \
        /* OP_COMMA  */ { INFIX,        ", "            }, \
        /* OP_PAREN  */ { UNARY,        "("             }, \
        /* OP_IF     */ { OP_IF,	0               }, \
      /* OP_VECT_ADD */ { INFIX,       " + "   }, \
      /* OP_VECT_SUB */ { INFIX,       " - "   }, \
   /* OP_SCALAR_MULT */ { INFIX,       " * "  }, \
    /* OP_SCALAR_DIV */ { INFIX,       " div "   }, \
    /* OP_SCALAR_MOD */ { BINARY,       " mod "   }, \
        /* OP_COSINE */ { UNARY,        "cos("          }, \
        /* OP_SINE   */ { UNARY,        "sin("          }, \
       /* OP_RNUMBER */ { OP_RNUMBER,	0               }, \
     /* OP_INTERSECT */ { BINARY,       "intersect("    }, \
      /* OP_PARALLEL */ { QUADARY,      "parallel("     }, \
       /* OP_PERPEND */ { BINARY,       "perpend("      }, \
        /* OP_CIRCLE */ { BINARY,       "circle("       }, \
           /* OP_AND */ { INFIX,        " && "          }, \
            /* OP_OR */ { INFIX,        " || "          }, \
           /* OP_NOT */ { UNARY,        "!"             }, \
          /* OP_BOOL */ { OP_BOOL,	0               }, \
  /* OP_PT_BETWN_PTS */ { TRIARY,       "pt_betwn_pts("      }, \
      /* OP_COLINEAR */ { TRIARY,       "colinear("     }, \
    /* OP_INTERSECTS */ { BINARY,       "intersects("   }, \
     /* OP_SEPARATES */ { TRIARY,       "separates("    }, \
      /* OP_INCLUDES */ { BINARY,       "includes("     }, \
    /* OP_DISTLARGER */ { TRIARY,       "distlarger("   }, \
   /* OP_DISTSMALLER */ { TRIARY,       "distsmaller("  }, \
         /* OP_LABEL */ { BINARY,       "label("        }, \
       /* OP_MONITOR */ { OP_MONITOR,	0               }, \
        /* OP_IMPOSE */ { OP_IMPOSE,	0               }, \
       /* OP_CSTRING */ { OP_CSTRING,	0               }, \
       /*OP_DISTANCE */ { BINARY,       "dist("         }, \
      /* OP_MIDPOINT */ { UNARY,        "midpoint("     }, \
      /* OP_EQ_EQ */	{ INFIX,	" == "		}, \
      /* OP_NOT_EQ */	{ INFIX,	" != "		}, \
      /* OP_GT */	{ INFIX,	" > "		}, \
      /* OP_GT_EQ */	{ INFIX,	" >= "		}, \
      /* OP_LT */	{ INFIX,	" < "		}, \
      /* OP_LT_EQ */	{ INFIX,	" <= "		}, \
      /* OP_SQRT */	{ UNARY,	"sqrt("		}, \
      /* OP_TAN */	{ UNARY,	"tan("		}, \
      /* OP_ASIN */	{ UNARY,	"asin("		}, \
      /* OP_ACOS */	{ UNARY,	"acos("		}, \
      /* OP_ATAN */	{ UNARY,	"atan("		}, \
      /* OP_LOG */	{ UNARY,	"log("		}, \
      /* OP_EXP */	{ UNARY,	"exp("		}, \
      /* OP_TRUNC */	{ UNARY,	"trunc("	}, \
      /* OP_FLOAT */	{ UNARY,	"float("	}, \
      /* OP_INCIDENT */ { BINARY,	"incident("	}, \
      /* OP_TRANS */	{ TRIARY,	"trans("	}, \
      /* OP_ELLIPSE */  { TRIARY,       "ellipse("      }, \
      /* OP_RANDOM */   { UNARY,        "rand("		}, \
      /* OP_ARC */	{ OP_ARC,	0		}, \
      /* OP_PI */	{ CONSTANT,	"pi"		}, \
   /* OP_SLASH_SLASH */	{ INFIX,	" // "		}, \
      /* OP_ITOS */	{ UNARY,	"itos("		}, \
      /* OP_RTOS */	{ BINARY,	"rtos("		}, \
      /* OP_I */	{ CONSTANT,	"<i>"		}, \
      /* OP_XI */	{ CONSTANT,	"x<i>"		}, \
      /* OP_FI */	{ CONSTANT,	"f<i>"		}, \
      /* OP_I_1 */	{ CONSTANT,	"<i-1>"		}, \
      /* OP_XI_1 */	{ CONSTANT,	"x<i-1>"	}, \
      /* OP_FI_1 */	{ CONSTANT,	"f<i-1>"	}, \
      /* OP_GSPECLIST */{ OP_GSPECLIST,	";\n\t"		}, \
      /* OP_GSPEC */	{ OP_GSPEC,	" : "		}, \
       /* OP_GLOBAL  */ { OP_GLOBAL,	""              }, \
       /* OP_FUNC  */	{ OP_FUNC,	0               }, \
       /* OP_EDEN  */	{ OP_EDEN,	0               }, \
      /* OP_IMGFUNC  */	{ OP_IMGFUNC,	0               }, \
     /* OP_RECTANGLE */ { BINARY,       "rectangle("    }, \
      /* OP_REFLECT */  { BINARY,       "reflect("      }, \
     /* OP_SCALEXY */   { TRIARY,       "scalexy("      } \
}
