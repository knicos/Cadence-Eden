typedef union {
	Datum      *dp;		/* constants */
	symptr      sym;	/* symbol table ptr */
	Inst       *inst;	/* machine instruction */
	Int         narg;	/* number of arguments */
	Inst        fun;	/* binop */
	struct t   *sw;		/* switch */
	char       *tbegin;	/* beginning of text */
} YYSTYPE;
#define	UNDEF	257
#define	REAL	258
#define	INTEGER	259
#define	MYCHAR	260
#define	STRING	261
#define	LIST	262
#define	CONSTANT	263
#define	VAR	264
#define	FORMULA	265
#define	BLTIN	266
#define	LIB	267
#define	LIB64	268
#define	RLIB	269
#define	FUNC	270
#define	PROC	271
#define	PMAC	272
#define	FUNCTION	273
#define	PROCEDURE	274
#define	PROCMACRO	275
#define	AUTO	276
#define	PARA	277
#define	LOCAL	278
#define	BREAK	279
#define	CONTINUE	280
#define	SWITCH	281
#define	CASE	282
#define	DEFAULT	283
#define	DO	284
#define	FOR	285
#define	WHILE	286
#define	IF	287
#define	ELSE	288
#define	SHIFT	289
#define	APPEND	290
#define	INSERT	291
#define	DELETE	292
#define	RETURN	293
#define	ARG	294
#define	IS	295
#define	TILDE_GT	296
#define	PLUS_EQ	297
#define	MINUS_EQ	298
#define	OR	299
#define	LAZY_OR	300
#define	AND	301
#define	LAZY_AND	302
#define	BITAND	303
#define	BITOR	304
#define	EQ_EQ	305
#define	NOT_EQ	306
#define	GT_EQ	307
#define	LT_EQ	308
#define	SLASH_SLASH	309
#define	NEGATE	310
#define	NOT	311
#define	PLUS_PLUS	312
#define	MINUS_MINUS	313
#define	ASTERISK	314
#define	EVAL	315


extern YYSTYPE yylval;
