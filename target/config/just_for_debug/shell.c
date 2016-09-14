/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.0.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 1 "shell.yacc" /* yacc.c:339  */

/* shell.yacc - grammar for VxWorks shell */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
07m,16oct01,jn   use symFindSymbol for symbol lookups
07l,27nov01,pch  Provide floating-point exclusion based on _WRS_NO_TGT_SHELL_FP
		 definition instead of testing a specific CPU type.
07k,23oct01,fmk  Do not call symFindByValue and print symbol name if symbol
                 value = -1 SPR 22254
07j,04sep98,cdp  apply 07i for all ARM CPUs with ARM_THUMB==TRUE.
07i,30jul97,cdp  for ARM7TDMI_T, force calls to be in Thumb state.
07h,31may96,ms   added in patch for SPR 4439.
07g,19mar95,dvs  removed tron references.
07f,02mar95,yao  removed floating point temporarily for PPC403.
07e,19mar95,dvs  removed tron references.
07d,13feb93,kdl  changed cplusLib.h to private/cplusLibP.h (SPR #1917).
07c,03sep92,wmd  modified addArg() to pass floats correcty for the i960.
07b,03sep92,rrr  reduced max function arguments from 16 to 12 (for i960).
07a,31aug92,kdl  fixed passing of more than 10 parameters during funcCall();
		 changed MAX_ARGS to MAX_SHELL_ARGS.
06z,19aug92,jmm  fixed problem with not recognizing <= (SPR 1517)
06y,01aug92,srh  added C++ demangling idiom to printSym.
                 added include of cplusLib.h.
06x,20jul92,jmm  added group parameter to symAdd call
06w,23jun92,kdl  increased max function arguments from 10 to 16.
06v,22jun92,jmm  backed out 6u change, now identical to gae's 21dec revision
06u,22jun92,jmm  added group parameter to symAdd
06t,21dec91,gae  more ANSI cleanups.
06s,19nov91,rrr  shut up some warnings.
06r,05oct91,rrr  changed strLib.h to string.h
06q,02jun91,del  added I960 parameter alignment fixes.
06p,10aug90,kdl  added forward declarations for functions returning VOID.
06o,10jul90,dnw  spr 738: removed checking of access (checkAccess, chkLvAccess)
		   Access checking did vxMemProbe with 4 byte read, which
		   caused problems with memory or devices that couldn't do
		   4 byte reads but could do other types of access.
		   Access checking was actually a throw-back to a time when
		   the shell couldn't recover from bus errors, but it can now.
		   So I just deleted the access checking.
		 lint clean-up, esp to allow VOID to be void one day.
06n,10dec89,jcf  symbol table type now a SYM_TYPE.
06m,09aug89,gae  fixed copyright notice.
06l,30jul89,gae  changed obsolete sysMemProbe to vxMemProbe. 
06k,07jul88,jcf  changed malloc to match new declaration.
06j,30may88,dnw  changed to v4 names.
06i,01apr88,gae  made it work with I/O system changes -- io{G,S}etGlobalStd().
06h,20feb88,dnw  lint
06g,14dec87,dnw  removed checking for odd byte address access.
		 changed printing format of doubles from "%f" to "%g". 
06f,18nov87,gae  made assignment to be of type specified by rhs.
06e,07nov87,gae  fixed undefined symbol bug.
		 fixed history bug by redirecting LED I/O.
06d,03nov87,ecs  documentation.
06c,28oct87,gae  got rid of string type.
06b,06oct87,gae  split off "execution" portion to shellExec.c.
		 changed to use conventional C type casting.
		 provided more info for invalid yacc operations.
		 allowed expressions to be function addresses.
06a,01jun87,gae  added interpretation of bytes, words, floats, doubles;
		   expressions can now be "typed" a la assembler, .[bwfdls].
		 fixed redirection bug with ">>" and "<<".
05i,16jul87,ecs  fixed newSym so that new symbols will be global.
05h,01apr87,gae  made assign() not print "new value" message (duplicated
		   normal "value" message for expressions.
05g,25apr87,gae  fixed bug in assign() that allowed memory corruption.
		 checked h() parameter for greater than or equal to zero.
		 improved redirection detection.
		 now parse assignments correctly as expressions.
05f,01apr87,ecs  added include of strLib.h.
05e,20jan87,jlf  documentation.
05d,14jan87,gae  got rid of unused curLineNum.  h() now has parameter, if
		   non-zero then resets history to that size.
05c,20dec86,dnw  changed to not get include files from default directories.
05b,18dec86,gae  made history initialization only happen on first start of
		   shell.  Added neStmt to fix empty stmt assignment bug.
05a,17dec86,gae	 use new shCmd() in execShell() to do Korn shell-like input.
04q,08dec86,dnw  changed shell.slex.c to shell_slex.c for VAX/VMS compatiblity.
	    jlf  fixed a couple bugs causing problems mainly on Heurikon port.
04p,24nov86,llk  deleted SYSTEM conditional compiles.
04o,08oct86,gae  added C assignment operators; allowed multiple assignment.
		 STRINGs are no longer temporary.  Added setShellPrompt().
04n,27jul86,llk  added standard error fd, setOrigErrFd.
04m,17jun86,rdc  changed memAllocates to mallocs.
04l,08apr86,dnw  added call to vxSetTaskBreakable to make shell unbreakable.
		 changed sstLib calls to symLib.
04k,02apr86,rdc  added routines setOrigInFd and setOrigOutFd.
04j,18jan86,dnw  removed resetting (flushing) for standard in/out upon restarts;
		   this is now done more appropriately by the shell restart
		   routine in dbgLib.
...deleted pre 86 history - see RCS
*/

/*
DESCRIPTION
This is the parser for the VxWorks shell, written in yacc.
It provides the basic programmer's interface to VxWorks.
It is a C expression interpreter, containing no built-in commands.  

SEE ALSO: "Shell"
*/

#include "vxWorks.h"
#include "sysSymTbl.h"
#include "errno.h"
#include "errnoLib.h"
#include "ioLib.h"
#include "taskLib.h"
#include "stdio.h"
#include "private/cplusLibP.h"

#undef YYSTYPE VALUE		/* type of parse stack */

#define	MAX_SHELL_LINE	128	/* max chars on line typed to shell */

#define MAX_SHELL_ARGS	30	/* max number of args on stack */
#define MAX_FUNC_ARGS	12	/* max number of args to any one function */
				/*  NOTE: The array indices in funcCall()
				 *        must agree with MAX_FUNC_ARGS!!
				 */

#define BIN_OP(op)	rvOp((getRv(&yypvt[-2], &tmpVal1)), op, \
			      getRv(&yypvt[-0], &tmpVal2))

#define	RV(value)	(getRv (&(value), &tmpVal2))
#define NULLVAL		(VALUE *) NULL

#define CHECK		if (semError) YYERROR
#define SET_ERROR	semError = TRUE


typedef enum		/* TYPE */
    {
    T_UNKNOWN,
    T_BYTE,
    T_WORD,
#ifndef	_WRS_NO_TGT_SHELL_FP
    T_INT,
    T_FLOAT,
    T_DOUBLE
#else	/* _WRS_NO_TGT_SHELL_FP */
    T_INT
#endif	/* _WRS_NO_TGT_SHELL_FP */
    } TYPE;

typedef enum		/* SIDE */
    {
    LHS,
    RHS,
    FHS			/* function: rhs -> lhs */
    } SIDE;

typedef struct		/* VALUE */
    {
    SIDE side;
    TYPE type;
    union
	{
	int *lv;	/* pointer to any of the below */

	char byte;
	short word;
	int rv;
	char *string;
#ifndef	_WRS_NO_TGT_SHELL_FP
	float fp;
	double dp;
#endif	/* _WRS_NO_TGT_SHELL_FP */
	} value;
    } VALUE;

IMPORT int redirInFd;
IMPORT int redirOutFd;

 BOOL semError;	/* TRUE = semantic error found */
 VALUE tmpVal1;	/* used by BIN_OP above for expression evaluation */
 VALUE tmpVal2;	/* used by BIN_OP above for expression evaluation */
 int argStack [MAX_SHELL_ARGS];	/* arguments to functions */
 int nArgs;	/* number of args currently on argStack */
 BOOL usymFlag;	/* TRUE = U_SYMBOL has been seen */
 VALUE usymVal;	/* value of U_SYMBOL which has been seen */
 BOOL spawnFlag;	/* TRUE if spawn is first parameter in argStack[] */


#line 255 "y.tab.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif


/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    NL = 0,
    T_SYMBOL = 258,
    D_SYMBOL = 259,
    U_SYMBOL = 260,
    NUMBER = 261,
    CHAR = 262,
    STRING = 263,
    FLOAT = 264,
    OR = 265,
    AND = 266,
    EQ = 267,
    NE = 268,
    GE = 269,
    LE = 270,
    INCR = 271,
    DECR = 272,
    ROT_LEFT = 273,
    ROT_RIGHT = 274,
    UMINUS = 275,
    PTR = 276,
    TYPECAST = 277,
    ENDFILE = 278,
    LEX_ERROR = 279,
    MULA = 280,
    DIVA = 281,
    MODA = 282,
    ADDA = 283,
    SUBA = 284,
    SHLA = 285,
    SHRA = 286,
    ANDA = 287,
    ORA = 288,
    XORA = 289,
    UNARY = 290
  };
#endif
/* Tokens.  */
#define NL 0
#define T_SYMBOL 258
#define D_SYMBOL 259
#define U_SYMBOL 260
#define NUMBER 261
#define CHAR 262
#define STRING 263
#define FLOAT 264
#define OR 265
#define AND 266
#define EQ 267
#define NE 268
#define GE 269
#define LE 270
#define INCR 271
#define DECR 272
#define ROT_LEFT 273
#define ROT_RIGHT 274
#define UMINUS 275
#define PTR 276
#define TYPECAST 277
#define ENDFILE 278
#define LEX_ERROR 279
#define MULA 280
#define DIVA 281
#define MODA 282
#define ADDA 283
#define SUBA 284
#define SHLA 285
#define SHRA 286
#define ANDA 287
#define ORA 288
#define XORA 289
#define UNARY 290

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef int YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);



/* Copy the second part of user declarations.  */

#line 375 "y.tab.c" /* yacc.c:358  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  30
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   546

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  57
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  8
/* YYNRULES -- Number of rules.  */
#define YYNRULES  63
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  117

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   290

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    53,     2,     2,     2,    47,    40,     2,
      50,    52,    45,    43,    56,    44,     2,    46,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    37,    51,
      42,    25,    41,    36,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    49,     2,    55,    39,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    38,     2,    54,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      48
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   216,   216,   217,   219,   220,   223,   224,   225,   227,
     228,   229,   230,   231,   233,   238,   243,   245,   246,   247,
     248,   250,   253,   256,   257,   258,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   270,   271,   272,
     273,   274,   276,   278,   283,   288,   289,   290,   291,   292,
     293,   294,   295,   296,   297,   298,   303,   299,   321,   322,
     325,   327,   331,   332
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "NL", "error", "$undefined", "T_SYMBOL", "D_SYMBOL", "U_SYMBOL",
  "NUMBER", "CHAR", "STRING", "FLOAT", "OR", "AND", "EQ", "NE", "GE", "LE",
  "INCR", "DECR", "ROT_LEFT", "ROT_RIGHT", "UMINUS", "PTR", "TYPECAST",
  "ENDFILE", "LEX_ERROR", "'='", "MULA", "DIVA", "MODA", "ADDA", "SUBA",
  "SHLA", "SHRA", "ANDA", "ORA", "XORA", "'?'", "':'", "'|'", "'^'", "'&'",
  "'>'", "'<'", "'+'", "'-'", "'*'", "'/'", "'%'", "UNARY", "'['", "'('",
  "';'", "')'", "'!'", "'~'", "']'", "','", "$accept", "line", "stmt",
  "expr", "$@1", "arglist", "neArglist", "typecast", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,    61,   280,   281,   282,   283,
     284,   285,   286,   287,   288,   289,    63,    58,   124,    94,
      38,    62,    60,    43,    45,    42,    47,    37,   290,    91,
      40,    59,    41,    33,   126,    93,    44
};
# endif

#define YYPACT_NINF -43

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-43)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      79,   -43,   -43,   -43,   -43,   -43,   -43,   -43,    79,    79,
      79,    79,    79,    59,    79,    79,     9,   -39,   254,    79,
     -12,    40,    40,    40,    40,    40,   -42,   170,    40,    40,
     -43,    79,    79,    79,    79,    79,    79,    79,   -43,   -43,
      79,    79,    79,    79,    79,    79,    79,    79,    79,    79,
      79,    79,    79,    79,    79,    79,    79,    79,    79,    79,
      79,    79,    79,    79,    79,    79,    79,   -43,    79,   -38,
     -43,   -43,   -43,   308,   347,   445,   445,   453,   453,   461,
     461,   -43,   295,   295,   295,   295,   295,   295,   295,   295,
     295,   295,   295,   213,   359,   398,   408,   453,   453,   496,
     496,    57,    57,    57,   124,   254,   -37,   -40,   295,   -35,
      79,   -43,   -43,    79,   -43,   295,   254
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       4,     7,     6,    56,    10,     9,     8,    11,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     2,     5,     0,
       0,    41,    42,    16,    17,    15,     0,     0,    18,    19,
       1,     4,     0,     0,     0,     0,     0,     0,    43,    44,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    58,    14,     0,     0,
      62,    12,     3,    34,    33,    35,    36,    37,    38,    29,
      28,    22,    55,    51,    52,    49,    45,    46,    53,    54,
      47,    48,    50,     0,    32,    31,    30,    39,    40,    23,
      24,    25,    26,    27,     0,    60,     0,    59,    57,     0,
       0,    21,    13,     0,    63,    20,    61
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -43,   -13,   -43,    -8,   -43,   -43,   -43,   -43
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,    16,    17,    18,    20,   106,   107,    19
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      21,    22,    23,    24,    25,    27,    28,    29,    69,    30,
      70,    67,    31,    68,   109,   112,   113,   114,    72,     0,
       0,     0,     0,     0,    73,    74,    75,    76,    77,    78,
       0,     0,    79,    80,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,     0,
     108,    42,     1,     2,     3,     4,     5,     6,     7,     0,
       0,     0,     0,    38,    39,     8,     9,     0,    42,     0,
       0,    26,     1,     2,     3,     4,     5,     6,     7,    65,
      66,     0,     0,     0,     0,     8,     9,     0,     0,    10,
       0,     0,   115,    11,    12,   116,    65,    66,     0,    13,
       0,     0,    14,    15,     0,     0,     0,     0,     0,    10,
       0,     0,     0,    11,    12,     0,     0,     0,     0,    13,
       0,     0,    14,    15,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,     0,    42,     0,     0,     0,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,     0,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,     0,    65,    66,     0,     0,     0,     0,   111,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
       0,    42,     0,     0,     0,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,     0,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,     0,    65,
      66,     0,    71,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,     0,    42,     0,     0,     0,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
     110,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,     0,    65,    66,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,     0,    42,     0,     0,     0,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,     0,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,     0,    65,    66,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,     0,    42,     0,     0,    33,
      34,    35,    36,    37,    38,    39,    40,    41,     0,    42,
       0,    54,     0,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,     0,    65,    66,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,     0,    65,    66,    34,
      35,    36,    37,    38,    39,    40,    41,     0,    42,     0,
       0,    34,    35,    36,    37,    38,    39,    40,    41,     0,
      42,     0,     0,     0,     0,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,     0,    65,    66,    56,    57,
      58,    59,    60,    61,    62,    63,    64,     0,    65,    66,
      34,    35,    36,    37,    38,    39,    40,    41,     0,    42,
      34,    35,    36,    37,    38,    39,    40,    41,     0,    42,
       0,     0,     0,     0,     0,     0,     0,     0,    57,    58,
      59,    60,    61,    62,    63,    64,     0,    65,    66,    58,
      59,    60,    61,    62,    63,    64,     0,    65,    66,    36,
      37,    38,    39,    40,    41,     0,    42,     0,     0,    38,
      39,    40,    41,     0,    42,     0,     0,    38,    39,     0,
       0,     0,    42,     0,     0,     0,    58,    59,    60,    61,
      62,    63,    64,     0,    65,    66,    60,    61,    62,    63,
      64,     0,    65,    66,    60,    61,    62,    63,    64,     0,
      65,    66,    38,    39,     0,     0,     0,    42,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    62,    63,    64,     0,    65,    66
};

static const yytype_int8 yycheck[] =
{
       8,     9,    10,    11,    12,    13,    14,    15,    50,     0,
      52,    19,    51,    25,    52,    52,    56,    52,    31,    -1,
      -1,    -1,    -1,    -1,    32,    33,    34,    35,    36,    37,
      -1,    -1,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    -1,
      68,    21,     3,     4,     5,     6,     7,     8,     9,    -1,
      -1,    -1,    -1,    16,    17,    16,    17,    -1,    21,    -1,
      -1,    22,     3,     4,     5,     6,     7,     8,     9,    49,
      50,    -1,    -1,    -1,    -1,    16,    17,    -1,    -1,    40,
      -1,    -1,   110,    44,    45,   113,    49,    50,    -1,    50,
      -1,    -1,    53,    54,    -1,    -1,    -1,    -1,    -1,    40,
      -1,    -1,    -1,    44,    45,    -1,    -1,    -1,    -1,    50,
      -1,    -1,    53,    54,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    -1,    21,    -1,    -1,    -1,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    -1,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    -1,    49,    50,    -1,    -1,    -1,    -1,    55,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      -1,    21,    -1,    -1,    -1,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    -1,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    -1,    49,
      50,    -1,    52,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    -1,    21,    -1,    -1,    -1,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    -1,    49,    50,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    -1,    21,    -1,    -1,    -1,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    -1,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    -1,    49,    50,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    -1,    21,    -1,    -1,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    -1,    21,
      -1,    36,    -1,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    -1,    49,    50,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    -1,    49,    50,    12,
      13,    14,    15,    16,    17,    18,    19,    -1,    21,    -1,
      -1,    12,    13,    14,    15,    16,    17,    18,    19,    -1,
      21,    -1,    -1,    -1,    -1,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    -1,    49,    50,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    -1,    49,    50,
      12,    13,    14,    15,    16,    17,    18,    19,    -1,    21,
      12,    13,    14,    15,    16,    17,    18,    19,    -1,    21,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    40,    41,
      42,    43,    44,    45,    46,    47,    -1,    49,    50,    41,
      42,    43,    44,    45,    46,    47,    -1,    49,    50,    14,
      15,    16,    17,    18,    19,    -1,    21,    -1,    -1,    16,
      17,    18,    19,    -1,    21,    -1,    -1,    16,    17,    -1,
      -1,    -1,    21,    -1,    -1,    -1,    41,    42,    43,    44,
      45,    46,    47,    -1,    49,    50,    43,    44,    45,    46,
      47,    -1,    49,    50,    43,    44,    45,    46,    47,    -1,
      49,    50,    16,    17,    -1,    -1,    -1,    21,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    45,    46,    47,    -1,    49,    50
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,     9,    16,    17,
      40,    44,    45,    50,    53,    54,    58,    59,    60,    64,
      61,    60,    60,    60,    60,    60,    22,    60,    60,    60,
       0,    51,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    21,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    49,    50,    60,    25,    50,
      52,    52,    58,    60,    60,    60,    60,    60,    60,    60,
      60,    60,    60,    60,    60,    60,    60,    60,    60,    60,
      60,    60,    60,    60,    60,    60,    60,    60,    60,    60,
      60,    60,    60,    60,    60,    60,    62,    63,    60,    52,
      37,    55,    52,    56,    52,    60,    60
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    57,    58,    58,    59,    59,    60,    60,    60,    60,
      60,    60,    60,    60,    60,    60,    60,    60,    60,    60,
      60,    60,    60,    60,    60,    60,    60,    60,    60,    60,
      60,    60,    60,    60,    60,    60,    60,    60,    60,    60,
      60,    60,    60,    60,    60,    60,    60,    60,    60,    60,
      60,    60,    60,    60,    60,    60,    61,    60,    62,    62,
      63,    63,    64,    64
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     3,     0,     1,     1,     1,     1,     1,
       1,     1,     3,     4,     2,     2,     2,     2,     2,     2,
       5,     4,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     2,     2,     2,     2,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     0,     4,     0,     1,
       1,     3,     3,     5
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yystacksize);

        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 5:
#line 220 "shell.yacc" /* yacc.c:1646  */
    { printValue (&(yyvsp[0])); CHECK; }
#line 1614 "y.tab.c" /* yacc.c:1646  */
    break;

  case 7:
#line 224 "shell.yacc" /* yacc.c:1646  */
    { (yyvsp[0]).side = RHS; setRv (&(yyval), &(yyvsp[0])); }
#line 1620 "y.tab.c" /* yacc.c:1646  */
    break;

  case 8:
#line 225 "shell.yacc" /* yacc.c:1646  */
    { (yyval) = (yyvsp[0]); (yyval).value.rv = newString((char*)(yyvsp[0]).value.rv);
			   CHECK; }
#line 1627 "y.tab.c" /* yacc.c:1646  */
    break;

  case 12:
#line 230 "shell.yacc" /* yacc.c:1646  */
    { (yyval) = (yyvsp[-1]); }
#line 1633 "y.tab.c" /* yacc.c:1646  */
    break;

  case 13:
#line 232 "shell.yacc" /* yacc.c:1646  */
    { (yyval) = funcCall (&(yyvsp[-3]), &(yyvsp[-1])); CHECK; }
#line 1639 "y.tab.c" /* yacc.c:1646  */
    break;

  case 14:
#line 234 "shell.yacc" /* yacc.c:1646  */
    { 
			typeConvert (&(yyvsp[0]), (yyvsp[-1]).type, (yyvsp[-1]).side); (yyval) = (yyvsp[0]);
			CHECK;
			}
#line 1648 "y.tab.c" /* yacc.c:1646  */
    break;

  case 15:
#line 238 "shell.yacc" /* yacc.c:1646  */
    { VALUE tmp;
					  (void)getRv (&(yyvsp[0]), &tmp);
					  setLv (&(yyval), &tmp);
					  CHECK;
					}
#line 1658 "y.tab.c" /* yacc.c:1646  */
    break;

  case 16:
#line 243 "shell.yacc" /* yacc.c:1646  */
    { (yyval).value.rv = (int)getLv (&(yyvsp[0]));
					  (yyval).type = T_INT; (yyval).side = RHS; }
#line 1665 "y.tab.c" /* yacc.c:1646  */
    break;

  case 17:
#line 245 "shell.yacc" /* yacc.c:1646  */
    { rvOp (RV((yyvsp[0])), UMINUS, NULLVAL); }
#line 1671 "y.tab.c" /* yacc.c:1646  */
    break;

  case 18:
#line 246 "shell.yacc" /* yacc.c:1646  */
    { rvOp (RV((yyvsp[0])), '!', NULLVAL); }
#line 1677 "y.tab.c" /* yacc.c:1646  */
    break;

  case 19:
#line 247 "shell.yacc" /* yacc.c:1646  */
    { rvOp (RV((yyvsp[0])), '~', NULLVAL); }
#line 1683 "y.tab.c" /* yacc.c:1646  */
    break;

  case 20:
#line 248 "shell.yacc" /* yacc.c:1646  */
    { setRv (&(yyval), RV((yyvsp[-4]))->value.rv ? &(yyvsp[-2])
								       : &(yyvsp[0])); }
#line 1690 "y.tab.c" /* yacc.c:1646  */
    break;

  case 21:
#line 250 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP ('+');
					  typeConvert (&(yyval), T_INT, RHS);
					  setLv (&(yyval), &(yyval)); }
#line 1698 "y.tab.c" /* yacc.c:1646  */
    break;

  case 22:
#line 253 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP ('+');
					  typeConvert (&(yyval), T_INT, RHS);
					  setLv (&(yyval), &(yyval)); }
#line 1706 "y.tab.c" /* yacc.c:1646  */
    break;

  case 23:
#line 256 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP ('+'); }
#line 1712 "y.tab.c" /* yacc.c:1646  */
    break;

  case 24:
#line 257 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP ('-'); }
#line 1718 "y.tab.c" /* yacc.c:1646  */
    break;

  case 25:
#line 258 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP ('*'); }
#line 1724 "y.tab.c" /* yacc.c:1646  */
    break;

  case 26:
#line 259 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP ('/'); }
#line 1730 "y.tab.c" /* yacc.c:1646  */
    break;

  case 27:
#line 260 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP ('%'); }
#line 1736 "y.tab.c" /* yacc.c:1646  */
    break;

  case 28:
#line 261 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP (ROT_RIGHT); }
#line 1742 "y.tab.c" /* yacc.c:1646  */
    break;

  case 29:
#line 262 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP (ROT_LEFT); }
#line 1748 "y.tab.c" /* yacc.c:1646  */
    break;

  case 30:
#line 263 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP ('&'); }
#line 1754 "y.tab.c" /* yacc.c:1646  */
    break;

  case 31:
#line 264 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP ('^'); }
#line 1760 "y.tab.c" /* yacc.c:1646  */
    break;

  case 32:
#line 265 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP ('|'); }
#line 1766 "y.tab.c" /* yacc.c:1646  */
    break;

  case 33:
#line 266 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP (AND); }
#line 1772 "y.tab.c" /* yacc.c:1646  */
    break;

  case 34:
#line 267 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP (OR); }
#line 1778 "y.tab.c" /* yacc.c:1646  */
    break;

  case 35:
#line 268 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP (EQ); }
#line 1784 "y.tab.c" /* yacc.c:1646  */
    break;

  case 36:
#line 269 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP (NE); }
#line 1790 "y.tab.c" /* yacc.c:1646  */
    break;

  case 37:
#line 270 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP (GE); }
#line 1796 "y.tab.c" /* yacc.c:1646  */
    break;

  case 38:
#line 271 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP (LE); }
#line 1802 "y.tab.c" /* yacc.c:1646  */
    break;

  case 39:
#line 272 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP ('>'); }
#line 1808 "y.tab.c" /* yacc.c:1646  */
    break;

  case 40:
#line 273 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP ('<'); }
#line 1814 "y.tab.c" /* yacc.c:1646  */
    break;

  case 41:
#line 274 "shell.yacc" /* yacc.c:1646  */
    { rvOp (RV((yyvsp[0])), INCR, NULLVAL);
						  assign (&(yyvsp[0]), &(yyval)); CHECK; }
#line 1821 "y.tab.c" /* yacc.c:1646  */
    break;

  case 42:
#line 276 "shell.yacc" /* yacc.c:1646  */
    { rvOp (RV((yyvsp[0])), DECR, NULLVAL);
						  assign (&(yyvsp[0]), &(yyval)); CHECK; }
#line 1828 "y.tab.c" /* yacc.c:1646  */
    break;

  case 43:
#line 278 "shell.yacc" /* yacc.c:1646  */
    { VALUE tmp;
						  tmp = (yyvsp[-1]);
						  rvOp (RV((yyvsp[-1])), INCR, NULLVAL);
						  assign (&(yyvsp[-1]), &(yyval)); CHECK;
						  (yyval) = tmp; }
#line 1838 "y.tab.c" /* yacc.c:1646  */
    break;

  case 44:
#line 283 "shell.yacc" /* yacc.c:1646  */
    { VALUE tmp;
						  tmp = (yyvsp[-1]);
						  rvOp (RV((yyvsp[-1])), DECR, NULLVAL);
						  assign (&(yyvsp[-1]), &(yyval)); CHECK;
						  (yyval) = tmp; }
#line 1848 "y.tab.c" /* yacc.c:1646  */
    break;

  case 45:
#line 288 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP (ADDA); assign (&(yyvsp[-2]), &(yyval)); CHECK;}
#line 1854 "y.tab.c" /* yacc.c:1646  */
    break;

  case 46:
#line 289 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP (SUBA); assign (&(yyvsp[-2]), &(yyval)); CHECK;}
#line 1860 "y.tab.c" /* yacc.c:1646  */
    break;

  case 47:
#line 290 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP (ANDA); assign (&(yyvsp[-2]), &(yyval)); CHECK;}
#line 1866 "y.tab.c" /* yacc.c:1646  */
    break;

  case 48:
#line 291 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP (ORA);  assign (&(yyvsp[-2]), &(yyval)); CHECK;}
#line 1872 "y.tab.c" /* yacc.c:1646  */
    break;

  case 49:
#line 292 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP (MODA); assign (&(yyvsp[-2]), &(yyval)); CHECK;}
#line 1878 "y.tab.c" /* yacc.c:1646  */
    break;

  case 50:
#line 293 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP (XORA); assign (&(yyvsp[-2]), &(yyval)); CHECK;}
#line 1884 "y.tab.c" /* yacc.c:1646  */
    break;

  case 51:
#line 294 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP (MULA); assign (&(yyvsp[-2]), &(yyval)); CHECK;}
#line 1890 "y.tab.c" /* yacc.c:1646  */
    break;

  case 52:
#line 295 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP (DIVA); assign (&(yyvsp[-2]), &(yyval)); CHECK;}
#line 1896 "y.tab.c" /* yacc.c:1646  */
    break;

  case 53:
#line 296 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP (SHLA); assign (&(yyvsp[-2]), &(yyval)); CHECK;}
#line 1902 "y.tab.c" /* yacc.c:1646  */
    break;

  case 54:
#line 297 "shell.yacc" /* yacc.c:1646  */
    { BIN_OP (SHRA); assign (&(yyvsp[-2]), &(yyval)); CHECK;}
#line 1908 "y.tab.c" /* yacc.c:1646  */
    break;

  case 55:
#line 298 "shell.yacc" /* yacc.c:1646  */
    { assign (&(yyvsp[-2]), &(yyvsp[0])); (yyval) = (yyvsp[-2]); }
#line 1914 "y.tab.c" /* yacc.c:1646  */
    break;

  case 56:
#line 303 "shell.yacc" /* yacc.c:1646  */
    { usymFlag = TRUE; usymVal = (yyvsp[0]); }
#line 1920 "y.tab.c" /* yacc.c:1646  */
    break;

  case 57:
#line 305 "shell.yacc" /* yacc.c:1646  */
    {
			if ((yyvsp[-3]).type != T_UNKNOWN)
			    {
			    printf ("typecast of lhs not allowed.\n");
			    YYERROR;
			    }
			else
			    {
			    (yyval) = newSym ((char *)(yyvsp[-3]).value.rv, (yyvsp[0]).type); CHECK;
			    assign (&(yyval), &(yyvsp[0])); CHECK;
			    }
			usymFlag = FALSE;
			}
#line 1938 "y.tab.c" /* yacc.c:1646  */
    break;

  case 58:
#line 321 "shell.yacc" /* yacc.c:1646  */
    { (yyval) = newArgList (); }
#line 1944 "y.tab.c" /* yacc.c:1646  */
    break;

  case 60:
#line 326 "shell.yacc" /* yacc.c:1646  */
    { (yyval) = newArgList (); addArg (&(yyval), &(yyvsp[0])); CHECK; }
#line 1950 "y.tab.c" /* yacc.c:1646  */
    break;

  case 61:
#line 328 "shell.yacc" /* yacc.c:1646  */
    { addArg (&(yyvsp[-2]), &(yyvsp[0])); CHECK; }
#line 1956 "y.tab.c" /* yacc.c:1646  */
    break;

  case 62:
#line 331 "shell.yacc" /* yacc.c:1646  */
    { (yyvsp[-1]).side = RHS; (yyval) = (yyvsp[-1]); }
#line 1962 "y.tab.c" /* yacc.c:1646  */
    break;

  case 63:
#line 332 "shell.yacc" /* yacc.c:1646  */
    { (yyvsp[-3]).side = FHS; (yyval) = (yyvsp[-3]); }
#line 1968 "y.tab.c" /* yacc.c:1646  */
    break;


#line 1972 "y.tab.c" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 335 "shell.yacc" /* yacc.c:1906  */


#include "a_out.h"
#include "ctype.h"
#include "stdlib.h"
#include "string.h"
#include "symLib.h"

#include "shell_slex_c"

/* forward declarations */

 int newString ();
 VALUE *getRv ();
 int *getLv ();
 VALUE evalExp ();
#ifndef	_WRS_NO_TGT_SHELL_FP
 void doubleToInts ();
#endif	/* _WRS_NO_TGT_SHELL_FP */
 void setRv ();
 void typeConvert ();
 BOOL checkLv ();
 BOOL checkRv ();

/*******************************************************************************
*
* yystart - initialize  variables
*
* NOMANUAL
*/

void yystart (line)
    char *line;

    {
    lexNewLine (line);
    semError = FALSE;
    usymFlag = FALSE;
    nArgs = 0;
    spawnFlag = FALSE;
    }
/*******************************************************************************
*
* yyerror - report error
*
* This routine is called by yacc when an error is detected.
*/

 void yyerror (string)
    char *string;

    {
    if (semError)	/* semantic errors have already been reported */
	return;

    /* print error depending on what look-ahead token is */

    switch (yychar)
	{
	case U_SYMBOL:	/* U_SYM not at beginning of line */
	    printf ("undefined symbol: %s\n", (char *) yylval.value.rv);
	    break;

	case LEX_ERROR:	     /* lex should have already reported the problem */
	    break;

	default:
	    if (usymFlag)    /* leading U_SYM was followed by invalid assign */
		printf ("undefined symbol: %s\n", (char *)usymVal.value.rv);
	    else
		printf ("%s\n", string);
	    break;
	}
    }
/*******************************************************************************
*
* rvOp - sets rhs of yyval to evaluated expression
*/

 void rvOp (pY1, op, pY2)
    VALUE *pY1;
    int op;
    VALUE *pY2;

    {
    VALUE yy;

    yy = evalExp (pY1, op, pY2);

    setRv (&yyval, &yy);
    }
/*******************************************************************************
*
* assign - make assignment of new value to a cell
*/

 void assign (pLv, pRv)
    FAST VALUE *pLv;	/* lhs to be assigned into */
    FAST VALUE *pRv;	/* rhs value */

    {
    VALUE val;

    /* verify that lv can be assigned to, then make the assignment */

    if (checkLv (pLv) && checkRv (pRv))
	{
	(void)getRv (pRv, &val);

	/* make value agree in type */

	pLv->type = pRv->type;

	typeConvert (&val, pLv->type, RHS);

	switch (pLv->type)
	    {
	    case T_BYTE:
		* (char *)getLv (pLv) = val.value.byte;
		break;

	    case T_WORD:
		* (short *)getLv (pLv) = val.value.word;
		break;

	    case T_INT:
		*getLv (pLv) = val.value.rv;
		break;

#ifndef	_WRS_NO_TGT_SHELL_FP
	    case T_FLOAT:
		* (float *)getLv (pLv) = val.value.fp;
		break;

	    case T_DOUBLE:
		* (double *)getLv (pLv) = val.value.dp;
		break;
#endif	/* _WRS_NO_TGT_SHELL_FP */

	    default:
		printf ("bad assignment.\n");
		SET_ERROR;
	    }
	}
    else
	{
	printf ("bad assignment.\n");
	SET_ERROR;
	}
    }
/*******************************************************************************
*
* newString - allocate and copy a string
*/

 int newString (string)
    char *string;

    {
    int length    = strlen (string) + 1;
    char *address = (char *) malloc ((unsigned) length);

    if (address == NULL)
	{
	printf ("not enough memory for new string.\n");
	SET_ERROR;
	}
    else
	bcopy (string, address, length);

    return ((int)address);
    }
/*******************************************************************************
*
* newSym - allocate a new symbol and add to symbol table
*/

 VALUE newSym (name, type)
    char *name;
    TYPE type;

    {
    VALUE value;
    void *address = (void *) malloc (sizeof (double));

    if (address == NULL)
	{
	printf ("not enough memory for new variable.\n");
	SET_ERROR;
	}

    else if (symAdd (sysSymTbl, name, (char *) address, (N_BSS | N_EXT),
                     symGroupDefault) != OK)
	{
	free ((char *) address);
	printf ("can't add '%s' to system symbol table - error = 0x%x.\n",
		name, errnoGet());
	SET_ERROR;
	}
    else
	{
	printf ("new symbol \"%s\" added to symbol table.\n", name);

	value.side	= LHS;
	value.type	= type;
	value.value.lv	= (int *) address;
	}

    return (value);
    }
/*******************************************************************************
*
* printSym - print symbolic value
*/

 void printSym (val, prefix, suffix)
    FAST int val;
    char *prefix;
    char *suffix;

    {
    void *    symVal;  /* symbol value      */
    SYMBOL_ID symId;   /* symbol identifier */
    char *    name;    /* symbol name       */
    char      demangled [MAX_SYS_SYM_LEN + 1];
    char *    nameToPrint;

    /* Only search for symbol value and print symbol name if value is not -1 */
    
        if ((val != -1) && 
	    (symFindSymbol (sysSymTbl, NULL, (void *)val, 
		   	    SYM_MASK_NONE, SYM_MASK_NONE, &symId) == OK) &&
	    (symNameGet (symId, &name) == OK) &&
	    (symValueGet (symId, &symVal) == OK) &&
	    (symVal != 0) && ((val - (int)symVal) < 0x1000))
	    {
	    printf (prefix);

	    nameToPrint = cplusDemangle(name, demangled, sizeof (demangled));

	    if (val == (int) symVal)
	        printf ("%s", nameToPrint);
	    else
	        printf ("%s + 0x%x", nameToPrint, val - (int) symVal);

	    printf (suffix);
	    }
    
    }
/*******************************************************************************
*
* newArgList - start a new argument list
*/

 VALUE newArgList ()
    {
    VALUE value;

    value.side	   = RHS;
    value.type	   = T_INT;
    value.value.rv = nArgs;

    return (value);
    }
/*******************************************************************************
*
* addArg - add an argument to an argument list
*/

 void addArg (pArgList, pNewArg)
    VALUE *pArgList;
    FAST VALUE *pNewArg;

    {
    VALUE val;
    int partA;
    int partB;
#if CPU_FAMILY==I960
    int nArgsSave;
#endif
#ifndef	_WRS_NO_TGT_SHELL_FP
    BOOL isfloat = pNewArg->type == T_FLOAT || pNewArg->type == T_DOUBLE;
#endif	/* _WRS_NO_TGT_SHELL_FP */
    SYMBOL_ID   symId;  /* symbol identifier           */
    SYM_TYPE    sType;  /* place to return symbol type */

#ifndef	_WRS_NO_TGT_SHELL_FP
    if (isfloat)
# if CPU_FAMILY!=I960
	nArgs++;	/* will need an extra arg slot */
# else /* CPU_FAMILY!=I960 */
	{
	nArgsSave = nArgs;
	if (spawnFlag)
	    {
	    if ((nArgs %2) == 0)
	  	nArgs++;
	    }
	else
	    {
	    nArgs += nArgs % 2;	/* conditionally borrow slot to double align */
	    nArgs++;		/* borrow second slot for double-word value  */
	    }
	}
# endif /* CPU_FAMILY!=I960 */
#endif	/* _WRS_NO_TGT_SHELL_FP */

    if (nArgs == MAX_SHELL_ARGS || 
        (nArgs - pArgList->value.rv) == MAX_FUNC_ARGS)
	{
#ifndef	_WRS_NO_TGT_SHELL_FP
	if (isfloat)
# if CPU_FAMILY!=I960
	    nArgs--;		/* return borrowed slot */
# else  /* CPU_FAMILY!=I960 */
	    nArgs = nArgsSave;	/* return borrowed slot(s) */
# endif /* CPU_FAMILY!=I960 */
#endif	/* _WRS_NO_TGT_SHELL_FP */
	printf ("too many arguments to functions.\n");
	SET_ERROR;
	}
    else
	{
	/* push arg value on top of arg stack */

	(void)getRv (pNewArg, &val);

#ifndef	_WRS_NO_TGT_SHELL_FP
	if (isfloat)
	    {
# if CPU_FAMILY==I960
	    if (spawnFlag == FALSE)
# endif /* CPU_FAMILY==I960 */
		nArgs--;	/* return borrowed slot */
	    
	    /* put float as integers on argStack */

	    doubleToInts (pNewArg->type == T_FLOAT ?
			  val.value.fp : val.value.dp,
			  &partA, &partB);

	    argStack[nArgs++] = partA;
	    argStack[nArgs++] = partB;
	    }
	else if (checkRv (&val))
#else	/* _WRS_NO_TGT_SHELL_FP */
	if (checkRv (&val))
#endif	/* _WRS_NO_TGT_SHELL_FP */
	    {
	    int rv;

	    switch (val.type)
		{
		case T_BYTE:
		    rv = val.value.byte;
		    break;

		case T_WORD:
		    rv = val.value.word;
		    break;

		case T_INT:
		    rv = val.value.rv;
		
		    /* 
		     * new symLib api - symbol name lengths are no
		     * longer limited 
		     */

		    if (symFindSymbol (sysSymTbl, NULL, (void *)rv, 
			   	       SYM_MASK_NONE, SYM_MASK_NONE, 
				       &symId) == OK)
		 	symTypeGet (symId, &sType);

		    if ((nArgs == 0) && (sType == (N_TEXT + N_EXT)))
			spawnFlag = TRUE;
		    break;

		default:
		    rv = 0;
		    printf ("addArg: bad type.\n");
		    SET_ERROR;
		}

	    argStack[nArgs++] = rv;
	    }
	}
    }
#ifndef	_WRS_NO_TGT_SHELL_FP
/*******************************************************************************
*
* doubleToInts - separate double into two integer parts
*/

 void doubleToInts (d, partA, partB)
    double d;
    int *partA;
    int *partB;

    {
    union 
	{
	struct
	    {
	    int a;
	    int b;
	    } part;
	double d;
	} val;

    val.d = d;

    *partA = val.part.a;
    *partB = val.part.b;
    }
#endif	/* _WRS_NO_TGT_SHELL_FP */
/*******************************************************************************
*
* funcCall - call a function
*/

 VALUE funcCall (pV, pArgList)
    VALUE *pV;
    VALUE *pArgList;

    {
    static int funcStatus;	/* status from function calls */
    int a [MAX_FUNC_ARGS];
    VALUE value;
    FAST int i;
    FAST int argNum;
    int oldInFd	 = ioGlobalStdGet (STD_IN);
    int oldOutFd = ioGlobalStdGet (STD_OUT);
    FUNCPTR pFunc = (pV->side == LHS) ? (FUNCPTR) (int)getLv (pV)
				      : (FUNCPTR) pV->value.rv;

#if ((CPU_FAMILY == ARM) && ARM_THUMB)
    pFunc = (FUNCPTR)((UINT32)pFunc | 1);	/* make it a Thumb call */
#endif

    /* get any specified args off stack, or pre-set all args to 0 */

    for (argNum = pArgList->value.rv, i = 0; i < MAX_FUNC_ARGS; argNum++, i++)
	{
	a [i] = (argNum < nArgs) ? argStack[argNum] : 0;
	}

    /* set standard in/out to redirection fds */

    if (redirInFd >= 0)
	ioGlobalStdSet (STD_IN, redirInFd);

    if (redirOutFd >= 0)
	ioGlobalStdSet (STD_OUT, redirOutFd);

    /* call function and save resulting status */

    errnoSet (funcStatus);

    value.side = RHS;
    value.type = pV->type;

    switch (pV->type)
	{
	case T_BYTE:
	case T_WORD:
	case T_INT:
	    {
	    /* NOTE: THE FOLLOWING ARRAY REFERENCES MUST AGREE WITH THE
	     *       MAX_FUNC_ARGS COUNT DEFINED ABOVE IN THIS FILE!
	     */
	    int rv = (* pFunc) (a[0], a[1], a[2], a[3], a[4], a[5], a[6],
				a[7], a[8], a[9], a[10], a[11]);

	    switch (pV->type)
		{
		case T_BYTE:
		    value.value.byte = (char) rv;
		    break;

		case T_WORD:
		    value.value.word = (short) rv;
		    break;

		case T_INT:
		    value.value.rv = rv;
		    break;
		default:
		    break;
		}

	    break;
	    }

#ifndef	_WRS_NO_TGT_SHELL_FP
	case T_FLOAT:
	    value.value.fp = (* (float (*)())pFunc) (a[0], a[1], a[2], a[3],
			a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11]);
	    break;

	case T_DOUBLE:
	    value.value.dp = (* (double (*)())pFunc) (a[0], a[1], a[2], a[3],
			a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11]);
	    break;
#endif	/* _WRS_NO_TGT_SHELL_FP */

	default:
	    printf ("funcCall: bad function type.\n");
	    SET_ERROR;
	}

    funcStatus = errnoGet ();

    /* restore original in/out fds */

    if (redirInFd >= 0)
	ioGlobalStdSet (STD_IN, oldInFd);

    if (redirOutFd >= 0)
	ioGlobalStdSet (STD_OUT, oldOutFd);

    /* arg stack back to previous level */

    nArgs = pArgList->value.rv;

    return (value);
    }
/*******************************************************************************
*
* checkLv - check that a value can be used as left value
*/

 BOOL checkLv (pValue)
    VALUE *pValue;

    {
    if (pValue->side != LHS)
	{
	printf ("invalid application of 'address of' operator.\n");
	SET_ERROR;
	return (FALSE);
	}

    return (TRUE);
    }
/*******************************************************************************
*
* checkRv - check that a value can be used as right value
*/

 BOOL checkRv (pValue)
    VALUE *pValue;

    {
    if (pValue->side == LHS)
	return (checkLv (pValue));

    return (TRUE);
    }
/*******************************************************************************
*
* getRv - get a value's right value 
*/

 VALUE *getRv (pValue, pRv)
    FAST VALUE *pValue;
    FAST VALUE *pRv;			/* where to put value */

    {
    if (pValue->side == RHS)
	*pRv = *pValue;
    else
	{
	pRv->side = RHS;
	pRv->type = pValue->type;

	switch (pValue->type)
	    {
	    case T_BYTE:
		pRv->value.byte = *(char *)pValue->value.lv;
		break;

	    case T_WORD:
		pRv->value.word = *(short *)pValue->value.lv;
		break;

	    case T_INT:
		pRv->value.rv = *pValue->value.lv;
		break;

#ifndef	_WRS_NO_TGT_SHELL_FP
	    case T_FLOAT:
		pRv->value.fp = *(float *)pValue->value.lv;
		break;

	    case T_DOUBLE:
		pRv->value.dp = *(double *)pValue->value.lv;
		break;
#endif	/* _WRS_NO_TGT_SHELL_FP */

	    default:
		printf ("getRv: invalid rhs.");
		SET_ERROR;
	    }
	}

    return (pRv);
    }
/*******************************************************************************
*
* getLv - get a value's left value (address)
*/

 int *getLv (pValue)
    VALUE *pValue;

    {
    return (checkLv (pValue) ? pValue->value.lv : 0);
    }
/*******************************************************************************
*
* setLv - set a lv
*/

 void setLv (pVal1, pVal2)
    FAST VALUE *pVal1;
    FAST VALUE *pVal2;

    {
    if (pVal2->side == LHS)
	{
	printf ("setLv: invalid lhs.\n");
	SET_ERROR;
	}

    if ((int)pVal2->type != (int)T_INT)
	{
	printf ("setLv: type conflict.\n");
	SET_ERROR;
	}

    pVal1->side     = LHS;
    pVal1->type     = pVal2->type;
    pVal1->value.lv = (int *)pVal2->value.rv;
    }
/*******************************************************************************
*
* setRv - set the rv
*/

 void setRv (pVal1, pVal2)
    FAST VALUE *pVal1;
    FAST VALUE *pVal2;

    {
    pVal1->side = RHS;
    pVal1->type = pVal2->type;

    switch (pVal2->type)
	{
	case T_BYTE:
	    pVal1->value.byte = (pVal2->side == LHS) ?
			    *(char *)pVal2->value.lv : pVal2->value.byte;
	case T_WORD:
	    pVal1->value.word = (pVal2->side == LHS) ?
			    *(short *)pVal2->value.lv : pVal2->value.word;

	case T_INT:
	    pVal1->value.rv = (pVal2->side == LHS) ?
			    *pVal2->value.lv : pVal2->value.rv;
	    break;

#ifndef	_WRS_NO_TGT_SHELL_FP
	case T_FLOAT:
	    pVal1->value.fp = (pVal2->side == LHS) ?
			    *(float *)pVal2->value.lv : pVal2->value.fp;
	    break;

	case T_DOUBLE:
	    pVal1->value.dp = (pVal2->side == LHS) ?
			    *(double *)pVal2->value.lv : pVal2->value.dp;
	    break;
#endif	/* _WRS_NO_TGT_SHELL_FP */

	default:
	    printf ("setRv: bad type.\n");
	    SET_ERROR;
	}
    }
/*******************************************************************************
*
* printLv - print left-hand side value
*
* "ssss + xxx = xxxx"
*/

 void printLv (pValue)
    VALUE *pValue;

    {
    FAST int *lv = getLv (pValue);

    printSym ((int) lv, "", " = ");

    printf ("0x%x", (UINT) lv);
    }
/*******************************************************************************
*
* printRv - print right-hand side value
*
* The format for integers is:
*
* "nnnn = xxxx = 'c' = ssss + nnn"
*                           ^ only if nn < LIMIT for some ssss
*                 ^ only if value is printable
*/

 void printRv (pValue)
    VALUE *pValue;

    {
    VALUE val;
    int rv;

    (void)getRv (pValue, &val);

    switch (pValue->type)
	{
	case T_BYTE:
	    rv = val.value.byte;
	    goto caseT_INT;

	case T_WORD:
	    rv = val.value.word;
	    goto caseT_INT;

	case T_INT:
	    rv = val.value.rv;
	    /* drop through */

	caseT_INT:
	    printf ("%d = 0x%x", rv, rv);
	    if (isascii (rv) && isprint (rv))
		printf (" = '%c'", rv);

	    printSym (rv, " = ", "");
	    break;

#ifndef	_WRS_NO_TGT_SHELL_FP
	case T_FLOAT:
	    printf ("%g", val.value.fp);
	    break;

	case T_DOUBLE:
	    printf ("%g", val.value.dp);
	    break;
#endif	/* _WRS_NO_TGT_SHELL_FP */

	default:
	    printf ("printRv: bad type.\n");
	    SET_ERROR;
	}
    }
/*******************************************************************************
*
* printValue - print out value
*/

 void printValue (pValue)
    FAST VALUE *pValue;

    {
    if (pValue->side == LHS)
	{
	if (checkLv (pValue) && checkRv (pValue))
	    {
	    printLv (pValue);
	    printf (": value = ");

	    printRv (pValue);
	    printf ("\n");
	    }
	else
	    {
	    printf ("invalid lhs.\n");
	    SET_ERROR;
	    }
	}
    else if (checkRv (pValue))
	{
	printf ("value = ");

	printRv (pValue);
	printf ("\n");
	}
    else
	{
	printf ("invalid rhs.\n");
	SET_ERROR;
	}
    }

/* TYPE SUPPORT */

 VALUE evalUnknown ();
 VALUE evalByte ();
 VALUE evalWord ();
 VALUE evalInt ();
 VALUE evalFloat ();
 VALUE evalDouble ();

typedef struct		/* EVAL_TYPE */
    {
    VALUE (*eval) ();
    } EVAL_TYPE;

 EVAL_TYPE evalType [] =
    {
    /*	eval		type		*/
    /*	---------------	--------------	*/
      { evalUnknown,	/* T_UNKNOWN*/	},
      { evalByte,	/* T_BYTE   */	},
      { evalWord,	/* T_WORD   */	},
      { evalInt,	/* T_INT    */	},
#ifndef	_WRS_NO_TGT_SHELL_FP
      { evalFloat,	/* T_FLOAT  */	},
      { evalDouble,	/* T_DOUBLE */	},
#endif	/* _WRS_NO_TGT_SHELL_FP */
    };

/*******************************************************************************
*
* evalExp - evaluate expression
*/

 VALUE evalExp (pValue1, op, pValue2)
    VALUE *pValue1;
    int op;
    VALUE *pValue2;

    {
    VALUE *p1 = pValue1;
    VALUE *p2 = pValue2;

    if (pValue2 == NULLVAL) /* unary expresions must set pValue2 to something */
	p2 = pValue2 = pValue1;

    /* make sure values have the same type */

    if ((int)p1->type > (int)p2->type)
	typeConvert (p2, p1->type, p1->side);
    else
	typeConvert (p1, p2->type, p2->side);

    return ((evalType[(int)pValue1->type].eval) (pValue1, op, pValue2));
    }
/*******************************************************************************
*
* evalUnknown - evaluate for unknown result
*
* ARGSUSED
*/

 VALUE evalUnknown (pValue1, op, pValue2)
    VALUE *pValue1;
    int op;
    VALUE *pValue2;

    {
    printf ("evalUnknown: bad evaluation.\n");

    SET_ERROR;

    return (*pValue1);	/* have to return something */
    }
/*******************************************************************************
*
* evalByte - evaluate for byte result
*/

 VALUE evalByte (pValue1, op, pValue2)
    VALUE *pValue1;
    int op;
    VALUE *pValue2;

    {
    VALUE *p1 = pValue1;
    VALUE *p2 = pValue2;
    VALUE result;

    /* evaluate as integers and then convert back */

    typeConvert (p1, T_INT, RHS);
    typeConvert (p2, T_INT, RHS);

    result = evalInt (p1, op, p2);

    typeConvert (&result, T_BYTE, RHS);

    return (result);
    }
/*******************************************************************************
*
* evalWord - evaluate for word result
*/

 VALUE evalWord (pValue1, op, pValue2)
    VALUE *pValue1;
    int op;
    VALUE *pValue2;

    {
    VALUE *p1 = pValue1;
    VALUE *p2 = pValue2;
    VALUE result;

    /* evaluate as integers and then convert back */

    typeConvert (p1, T_INT, RHS);
    typeConvert (p2, T_INT, RHS);

    result = evalInt (p1, op, p2);

    typeConvert (&result, T_WORD, RHS);

    return (result);
    }
/*******************************************************************************
*
* evalInt - evaluate for integer result
*/

 VALUE evalInt (pValue1, op, pValue2)
    VALUE *pValue1;
    int op;
    VALUE *pValue2;

    {
#define	OP_INT(op)	rv = e1 op e2; break
#define	OP_INT_U(op)	rv = op e1; break

    FAST int e1 = pValue1->value.rv;
    FAST int e2 = pValue2->value.rv;
    FAST int rv;
    VALUE result;

    switch (op)
	{
	case ADDA:
	case '+':
	    OP_INT(+);
	case SUBA:
	case '-':
	    OP_INT(-);
	case MULA:
	case '*':
	    OP_INT(*);
	case DIVA:
	case '/':
	    OP_INT(/);
	case '!':
	    OP_INT_U(!);
	case '~':
	    OP_INT_U(~);
	case MODA:
	case '%':
	    OP_INT(%);
	case ANDA:
	case '&':
	    OP_INT(&);
	case XORA:
	case '^':
	    OP_INT(^);
	case ORA:
	case '|':
	    OP_INT(|);
	case '<':
	    OP_INT(<);
	case '>':
	    OP_INT(>);
	case OR:
	    OP_INT(||);
	case AND:
	    OP_INT(&&);
	case EQ:
	    OP_INT(==);
	case NE:
	    OP_INT(!=);
	case GE:
	    OP_INT(>=);
	case LE:
	    OP_INT(<=);
	case INCR:
	    OP_INT_U(++);
	case DECR:
	    OP_INT_U(--);
	case SHLA:
	case ROT_LEFT:
	    OP_INT(<<);
	case SHRA:
	case ROT_RIGHT:
	    OP_INT(>>);
	case UMINUS:
	    OP_INT_U(-);
	default:
	    rv = 0;
	    printf ("operands have incompatible types.\n");
	    SET_ERROR;
	}

    result.side     = RHS;
    result.type     = pValue1->type;
    result.value.rv = rv;

    return (result);
    }
#ifndef	_WRS_NO_TGT_SHELL_FP
/*******************************************************************************
*
* evalFloat - evaluate for float result
*/

 VALUE evalFloat (pValue1, op, pValue2)
    VALUE *pValue1;
    int op;
    VALUE *pValue2;

    {
    VALUE *p1 = pValue1;
    VALUE *p2 = pValue2;
    VALUE result;

    /* evaluate as doubles and then convert back */

    typeConvert (p1, T_DOUBLE, RHS);
    typeConvert (p2, T_DOUBLE, RHS);

    result = evalDouble (p1, op, p2);

    typeConvert (&result, T_FLOAT, RHS);

    return (result);
    }
/*******************************************************************************
*
* evalDouble - evaluate for double result
*/

 VALUE evalDouble (pValue1, op, pValue2)
    VALUE *pValue1;
    int op;
    VALUE *pValue2;

    {
#define	OP_DOUBLE(op)	dp = e1 op e2; break
#define	OP_DOUBLE_U(op)	dp = op e1; break

    FAST double e1 = pValue1->value.dp;
    FAST double e2 = pValue2->value.dp;
    FAST double dp;
    VALUE result;

    switch (op)
	{
	case ADDA:
	case '+':
	    OP_DOUBLE(+);
	case SUBA:
	case '-':
	    OP_DOUBLE(-);
	case MULA:
	case '*':
	    OP_DOUBLE(*);
	case DIVA:
	case '/':
	    OP_DOUBLE(/);
	case '!':
	    OP_DOUBLE_U(!);

	case '<':
	    OP_DOUBLE(<);
	case '>':
	    OP_DOUBLE(>);
	case OR:
	    OP_DOUBLE(||);
	case AND:
	    OP_DOUBLE(&&);
	case EQ:
	    OP_DOUBLE(==);
	case NE:
	    OP_DOUBLE(!=);
	case GE:
	    OP_DOUBLE(>=);
	case LE:
	    OP_DOUBLE(<=);
	case INCR:
	    OP_DOUBLE_U(++);
	case DECR:
	    OP_DOUBLE_U(--);

	case UMINUS:
	    OP_DOUBLE_U(-);

	default:
	    dp = 0;
	    printf ("operands have incompatible types.\n");
	    SET_ERROR;
	}

    result.side     = RHS;
    result.type     = T_DOUBLE;
    result.value.dp = dp;

    return (result);
    }
#endif	/* _WRS_NO_TGT_SHELL_FP */

/* TYPE CONVERSION */

 void convUnknown ();
 void convByte ();
 void convWord ();
 void convInt ();
#ifndef	_WRS_NO_TGT_SHELL_FP
 void convFloat ();
 void convDouble ();
#endif	/* _WRS_NO_TGT_SHELL_FP */

typedef void (*VOID_FUNCPTR) ();	/* ptr to a function returning void */

 VOID_FUNCPTR convType [] =
    {
    /*  conversion	type	    */
    /*  ----------	----------- */
	convUnknown,	/* T_UNKNOWN*/
	convByte,	/* T_BYTE   */
	convWord,	/* T_WORD   */
	convInt,	/* T_INT    */
#ifndef	_WRS_NO_TGT_SHELL_FP
	convFloat,	/* T_FLOAT  */
	convDouble,	/* T_DOUBLE */
#endif	/* _WRS_NO_TGT_SHELL_FP */
    };

/*******************************************************************************
*
* typeConvert - change value to specified type
*/

 void typeConvert (pValue, type, side)
    FAST VALUE *pValue;
    TYPE type;
    SIDE side;

    {
    if (side == FHS)
	{
	pValue->side = RHS;
	pValue->type = type;
	}
    else if (side == RHS)
	{
	if (pValue->side == LHS)
	    pValue->type = type;
	else
	    (convType [(int) type]) (pValue);
	}
    else if (pValue->side == LHS)
	pValue->type = type;
    else
	{
	printf ("typeConvert: bad type.\n");
	SET_ERROR;
	}
    }
/*******************************************************************************
*
* convUnknown - convert value to unknown
*
* ARGSUSED
*/

 void convUnknown (pValue)
    VALUE *pValue;

    {
    printf ("convUnknown: bad type.\n");
    SET_ERROR;
    }
/*******************************************************************************
*
* convByte - convert value to byte
*/

 void convByte (pValue)
    FAST VALUE *pValue;

    {
    char value;

    if ((int)pValue->type > (int)T_BYTE)
	{
	convWord (pValue);
	value = pValue->value.word;
	pValue->value.byte = value;
	pValue->type = T_BYTE;
	}
    }
/*******************************************************************************
*
* convWord - convert value to word
*/

 void convWord (pValue)
    FAST VALUE *pValue;

    {
    short value;

    if ((int)pValue->type < (int)T_WORD)
	{
	value = pValue->value.byte;
	pValue->value.word = value;
	pValue->type = T_WORD;
	}
    else if ((int)pValue->type > (int)T_WORD)
	{
	convInt (pValue);
	value = pValue->value.rv;
	pValue->value.word = value;
	pValue->type = T_WORD;
	}
    }
/*******************************************************************************
*
* convInt - convert value to integer
*/

 void convInt (pValue)
    FAST VALUE *pValue;

    {
    int value;

    if ((int)pValue->type < (int)T_INT)
	{
	convWord (pValue);
	value = pValue->value.word;
	pValue->value.rv = value;
	pValue->type = T_INT;
	}
    else if ((int)pValue->type > (int)T_INT)
	{
#ifndef	_WRS_NO_TGT_SHELL_FP
	convFloat (pValue);
	value = pValue->value.fp;
	pValue->value.rv = value;
	pValue->type = T_INT;
#endif	/* _WRS_NO_TGT_SHELL_FP */
	}
    }
#ifndef	_WRS_NO_TGT_SHELL_FP
/*******************************************************************************
*
* convFloat - convert value to float
*/

 void convFloat (pValue)
    FAST VALUE *pValue;

    {
    float value;

    if ((int)pValue->type < (int)T_FLOAT)
	{
	convInt (pValue);
	value = pValue->value.rv;
	pValue->value.fp = value;
	pValue->type = T_FLOAT;
	}
    else if ((int)pValue->type > (int)T_FLOAT)
	{
	convDouble (pValue);
	value = pValue->value.dp;
	pValue->value.fp = value;
	pValue->type = T_FLOAT;
	}
    }
/*******************************************************************************
*
* convDouble - convert value to double
*/

 void convDouble (pValue)
    FAST VALUE *pValue;

    {
    double value;

    if ((int)pValue->type < (int)T_DOUBLE)
	{
	convFloat (pValue);

	value = pValue->value.fp;
	pValue->value.dp = value;
	pValue->type = T_DOUBLE;
	}
    }
#endif	/* _WRS_NO_TGT_SHELL_FP */
