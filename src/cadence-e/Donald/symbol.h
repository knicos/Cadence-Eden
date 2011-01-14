/*
 * $Id: symbol.h,v 1.4 1999/11/16 21:20:40 ashley Rel1.10 $
 */

struct symbol {
    char       *name;		/* symbol name */
    char       *viewport;	/* symbol viewport */
    char       *master;		/* who change this symbol last */
    union {
	tree        t;		/* symbol definition */
	char       *f;		/* graph function name */
    }           defn;		/* symbol definition */
    int         type;		/* symbol type */
    struct symbol *derived;	/* sub-directory */
    struct symbol *next;	/* next symbol entry */
};

typedef struct symbol symbol;

extern symbol *SymbolTable;

#define UNDEFINED 0

#define SymEnd  (symbol*)0

extern char *topMasterStack();
