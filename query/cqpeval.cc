/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Substitute the type names.  */
#define YYSTYPE         CQLSTYPE
/* Substitute the variable and function names.  */
#define yyparse         cqlparse
#define yylex           cqllex
#define yyerror         cqlerror
#define yydebug         cqldebug
#define yynerrs         cqlnerrs
#define yylval          cqllval
#define yychar          cqlchar

/* First part of user prologue.  */
#line 4 "cqpeval.y"


#include "config.hh"
#include "finlib/fstream.hh"
#include "frsop.hh"
#include "corpus.hh"
#include "parsop.hh"
#include "utf8.hh"
#include "cqpeval.hh"

#include <iostream>

using namespace std;

string errMsg;

string query;
static ssize_t pos = -1;  // position after the current lexer token
static ssize_t startpos = -1;  // position before the current lexer token
static bool onepos = false;

extern "C" {
     int yyparse(void);
     int yylex(void);
     void yyerror(char const* s) {
          stringstream ss;
          ss << s << " near position " << utf8pos(query.c_str(), startpos);
          errMsg = ss.str();
     }
     int cqlwrap() {
          return 1;
     }
}

Corpus* currCorp, *alCorp, *tmpCorp;
Corpus* defaultCorp;
Structure* currStruc;
Position lastPosition;

PosAttr* getAttr(string name) {
     return currCorp->get_attr(name);
}

PosAttr* getAttr(const char* name) {
     string s(name);
     return getAttr(s);
}

char* unescapeString(const char* s, size_t len) {
     char* unescaped = new char[len+1];
     size_t i, j;
     for (i = 0, j = 0; i < len; i++, j++) {
          if (s[i] == '\\' && i + 1 < len && (s[i + 1] == '"' || s[i + 1] == '\\'))
               i++;
          unescaped[j] = s[i];
     }
     unescaped[j] = '\0';
     return unescaped;
}

const int WORD_LESS_OR_EQUALS = -1;
const int WORD_GREATER_OR_EQUALS = 1;
const bool ignoreCase = false;

struct globOp {
     FilterCondFreq::Op op;
     int neg;
};

struct rep {
     int min, max;
};

struct evalResult {
     RangeStream* r;
     FastStream* f;
     int fcount;
     RangeStream* toRangeStream() {
          if (f != NULL) {
               return new Pos2Range(f, 0, fcount);
          }
          return r;
     }
     evalResult complement() {
          RangeStream* r = toRangeStream();
          evalResult ret;
          ret.f = NULL;
          ret.fcount = 0;
          ret.r = new RQoutsideNode(r, defaultCorp->size());
          return ret;
     }
};

evalResult createEvalResult(FastStream* f, int count=1) {
     evalResult ret;
     ret.r = NULL;
     ret.f = f;
     ret.fcount = count;
     return ret;
}

evalResult createEvalResult(RangeStream* r) {
     evalResult ret;
     ret.r = r;
     ret.f = NULL;
     ret.fcount = 0;
     return ret;
}

evalResult concat(evalResult e1, evalResult e2) {
     RangeStream* r1 = e1.r;
     RangeStream* r2 = e2.r;
     if (r1 != NULL || r2 != NULL) {
          bool sortBeg = false;
          if (r1 == NULL) {
               r1 = e1.toRangeStream();
          } else {
               r1 = new RQSortEnd(r1);
               sortBeg = true;
          }
          if (r2 == NULL) {
               r2 = e2.toRangeStream();
          }
          RangeStream* c = RQConcatNodeUnsort(r1, r2);
          if (sortBeg) {
               c = new RQSortBeg(c);
          }
          return createEvalResult(c);
     }
     FastStream* c = new QAndNode(e1.f, new QMoveNode(e2.f, -1*e1.fcount));
     return createEvalResult(c, e1.fcount + e2.fcount);
}

evalResult operationOr(evalResult e1, evalResult e2) {
     RangeStream* r1 = e1.r;
     RangeStream* r2 = e2.r;
     if (r1 != NULL || r2 != NULL || e1.fcount != e2.fcount) {
          if (r1 == NULL) {
               r1 = e1.toRangeStream();
          }
          if (r2 == NULL) {
               r2 = e2.toRangeStream();
          }
          return createEvalResult(new RQUnionNode(r1, r2));
     }
     return createEvalResult(new QOrNode(e1.f, e2.f), e1.fcount);
}


FastStream* concatFs(FastStream* f1, FastStream* f2) {
     return new QAndNode(f1, new QMoveNode(f2, -1));
}

evalResult queryResult;
stringstream seek;

#define DEL(X) delete X; X=NULL;
#define DELA(X) delete[] X; X=NULL;
#define TRY try {
#define CATCH } catch (exception &e) {yyerror(e.what()); YYABORT;}
#define CATCH1(X) } catch (exception &e) {DELA(X); yyerror(e.what()); YYABORT;}
#define CATCH2(X,Y) } catch (exception &e) {DELA(X); DELA(Y); yyerror(e.what()); YYABORT;}

#line 243 "cqpeval.cc"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif


/* Debug traces.  */
#ifndef CQLDEBUG
# if defined YYDEBUG
#if YYDEBUG
#   define CQLDEBUG 1
#  else
#   define CQLDEBUG 0
#  endif
# else /* ! defined YYDEBUG */
#  define CQLDEBUG 0
# endif /* ! defined YYDEBUG */
#endif  /* ! defined CQLDEBUG */
#if CQLDEBUG
extern int cqldebug;
#endif

/* Token kinds.  */
#ifndef CQLTOKENTYPE
# define CQLTOKENTYPE
  enum cqltokentype
  {
    CQLEMPTY = -2,
    CQLEOF = 0,                    /* "end of file"  */
    CQLerror = 256,                /* error  */
    CQLUNDEF = 257,                /* "invalid token"  */
    START_QUERY = 258,             /* START_QUERY  */
    START_ONEPOS = 259,            /* START_ONEPOS  */
    WORD = 260,                    /* WORD  */
    REGEXP = 261,                  /* REGEXP  */
    LBRACKET = 262,                /* LBRACKET  */
    RBRACKET = 263,                /* RBRACKET  */
    LPAREN = 264,                  /* LPAREN  */
    RPAREN = 265,                  /* RPAREN  */
    LBRACE = 266,                  /* LBRACE  */
    RBRACE = 267,                  /* RBRACE  */
    NOT = 268,                     /* NOT  */
    EQ = 269,                      /* EQ  */
    NEQ = 270,                     /* NEQ  */
    LEQ = 271,                     /* LEQ  */
    GEQ = 272,                     /* GEQ  */
    LSTRUCT = 273,                 /* LSTRUCT  */
    RSTRUCT = 274,                 /* RSTRUCT  */
    BIN_AND = 275,                 /* BIN_AND  */
    BIN_OR = 276,                  /* BIN_OR  */
    STAR = 277,                    /* STAR  */
    PLUS = 278,                    /* PLUS  */
    QUEST = 279,                   /* QUEST  */
    SLASH = 280,                   /* SLASH  */
    POSNUM = 281,                  /* POSNUM  */
    COMMA = 282,                   /* COMMA  */
    COLON = 283,                   /* COLON  */
    DOT = 284,                     /* DOT  */
    TEQ = 285,                     /* TEQ  */
    NNUMBER = 286,                 /* NNUMBER  */
    NUMBER = 287,                  /* NUMBER  */
    KW_MEET = 288,                 /* KW_MEET  */
    KW_UNION = 289,                /* KW_UNION  */
    KW_WITHIN = 290,               /* KW_WITHIN  */
    KW_CONTAINING = 291,           /* KW_CONTAINING  */
    KW_WS = 292,                   /* KW_WS  */
    KW_SWAP = 293,                 /* KW_SWAP  */
    KW_CCOLL = 294,                /* KW_CCOLL  */
    KW_FREQ = 295,                 /* KW_FREQ  */
    KW_TERM = 296,                 /* KW_TERM  */
    BISON_OLD_ERR = 297            /* BISON_OLD_ERR  */
  };
  typedef enum cqltokentype cqltoken_kind_t;
#endif

/* Value type.  */
#if ! defined CQLSTYPE && ! defined CQLSTYPE_IS_DECLARED
union CQLSTYPE
{
#line 168 "cqpeval.y"

     long long num;
     const char* str;
     evalResult evalRes;
     FastStream* fs;
     RangeStream* rs;
     rep repData;
     globOp globop;
     int i;
     FilterCond *fc;

#line 352 "cqpeval.cc"

};
typedef union CQLSTYPE CQLSTYPE;
# define CQLSTYPE_IS_TRIVIAL 1
# define CQLSTYPE_IS_DECLARED 1
#endif


extern CQLSTYPE cqllval;


int cqlparse (void);



/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_START_QUERY = 3,                /* START_QUERY  */
  YYSYMBOL_START_ONEPOS = 4,               /* START_ONEPOS  */
  YYSYMBOL_WORD = 5,                       /* WORD  */
  YYSYMBOL_REGEXP = 6,                     /* REGEXP  */
  YYSYMBOL_LBRACKET = 7,                   /* LBRACKET  */
  YYSYMBOL_RBRACKET = 8,                   /* RBRACKET  */
  YYSYMBOL_LPAREN = 9,                     /* LPAREN  */
  YYSYMBOL_RPAREN = 10,                    /* RPAREN  */
  YYSYMBOL_LBRACE = 11,                    /* LBRACE  */
  YYSYMBOL_RBRACE = 12,                    /* RBRACE  */
  YYSYMBOL_NOT = 13,                       /* NOT  */
  YYSYMBOL_EQ = 14,                        /* EQ  */
  YYSYMBOL_NEQ = 15,                       /* NEQ  */
  YYSYMBOL_LEQ = 16,                       /* LEQ  */
  YYSYMBOL_GEQ = 17,                       /* GEQ  */
  YYSYMBOL_LSTRUCT = 18,                   /* LSTRUCT  */
  YYSYMBOL_RSTRUCT = 19,                   /* RSTRUCT  */
  YYSYMBOL_BIN_AND = 20,                   /* BIN_AND  */
  YYSYMBOL_BIN_OR = 21,                    /* BIN_OR  */
  YYSYMBOL_STAR = 22,                      /* STAR  */
  YYSYMBOL_PLUS = 23,                      /* PLUS  */
  YYSYMBOL_QUEST = 24,                     /* QUEST  */
  YYSYMBOL_SLASH = 25,                     /* SLASH  */
  YYSYMBOL_POSNUM = 26,                    /* POSNUM  */
  YYSYMBOL_COMMA = 27,                     /* COMMA  */
  YYSYMBOL_COLON = 28,                     /* COLON  */
  YYSYMBOL_DOT = 29,                       /* DOT  */
  YYSYMBOL_TEQ = 30,                       /* TEQ  */
  YYSYMBOL_NNUMBER = 31,                   /* NNUMBER  */
  YYSYMBOL_NUMBER = 32,                    /* NUMBER  */
  YYSYMBOL_KW_MEET = 33,                   /* KW_MEET  */
  YYSYMBOL_KW_UNION = 34,                  /* KW_UNION  */
  YYSYMBOL_KW_WITHIN = 35,                 /* KW_WITHIN  */
  YYSYMBOL_KW_CONTAINING = 36,             /* KW_CONTAINING  */
  YYSYMBOL_KW_WS = 37,                     /* KW_WS  */
  YYSYMBOL_KW_SWAP = 38,                   /* KW_SWAP  */
  YYSYMBOL_KW_CCOLL = 39,                  /* KW_CCOLL  */
  YYSYMBOL_KW_FREQ = 40,                   /* KW_FREQ  */
  YYSYMBOL_KW_TERM = 41,                   /* KW_TERM  */
  YYSYMBOL_BISON_OLD_ERR = 42,             /* BISON_OLD_ERR  */
  YYSYMBOL_YYACCEPT = 43,                  /* $accept  */
  YYSYMBOL_list = 44,                      /* list  */
  YYSYMBOL_query = 45,                     /* query  */
  YYSYMBOL_globCond = 46,                  /* globCond  */
  YYSYMBOL_condExpr = 47,                  /* condExpr  */
  YYSYMBOL_condConj = 48,                  /* condConj  */
  YYSYMBOL_condAtom = 49,                  /* condAtom  */
  YYSYMBOL_globCondCmp = 50,               /* globCondCmp  */
  YYSYMBOL_within_containing = 51,         /* within_containing  */
  YYSYMBOL_within_containing_sufix = 52,   /* within_containing_sufix  */
  YYSYMBOL_within_number = 53,             /* within_number  */
  YYSYMBOL_aligned_part_labels = 54,       /* aligned_part_labels  */
  YYSYMBOL_55_1 = 55,                      /* $@1  */
  YYSYMBOL_aligned_part_no_labels = 56,    /* aligned_part_no_labels  */
  YYSYMBOL_57_2 = 57,                      /* $@2  */
  YYSYMBOL_sequence = 58,                  /* sequence  */
  YYSYMBOL_seq = 59,                       /* seq  */
  YYSYMBOL_repetition = 60,                /* repetition  */
  YYSYMBOL_atomQuery = 61,                 /* atomQuery  */
  YYSYMBOL_repOpt = 62,                    /* repOpt  */
  YYSYMBOL_structure = 63,                 /* structure  */
  YYSYMBOL_64_3 = 64,                      /* $@3  */
  YYSYMBOL_position = 65,                  /* position  */
  YYSYMBOL_oneposition = 66,               /* oneposition  */
  YYSYMBOL_thesaurus_default = 67,         /* thesaurus_default  */
  YYSYMBOL_muPart = 68,                    /* muPart  */
  YYSYMBOL_meetOp = 69,                    /* meetOp  */
  YYSYMBOL_integer = 70,                   /* integer  */
  YYSYMBOL_unionOp = 71,                   /* unionOp  */
  YYSYMBOL_attvallist = 72,                /* attvallist  */
  YYSYMBOL_attvaland = 73,                 /* attvaland  */
  YYSYMBOL_word = 74,                      /* word  */
  YYSYMBOL_wsseekpart = 75,                /* wsseekpart  */
  YYSYMBOL_wsseek = 76,                    /* wsseek  */
  YYSYMBOL_77_4 = 77,                      /* $@4  */
  YYSYMBOL_attval = 78                     /* attval  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

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


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
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

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if 1

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
#endif /* 1 */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined CQLSTYPE_IS_TRIVIAL && CQLSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
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
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  40
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   362

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  43
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  36
/* YYNRULES -- Number of rules.  */
#define YYNRULES  121
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  237

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   297


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
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
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42
};

#if CQLDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   211,   211,   212,   215,   216,   217,   220,   225,   226,
     229,   230,   233,   234,   235,   240,   245,   251,   252,   253,
     254,   255,   256,   257,   258,   261,   263,   265,   267,   269,
     271,   273,   276,   278,   280,   284,   285,   286,   289,   295,
     295,   309,   309,   323,   324,   325,   326,   329,   330,   333,
     334,   350,   354,   357,   364,   365,   366,   369,   370,   371,
     372,   373,   374,   375,   378,   379,   379,   383,   384,   387,
     388,   389,   390,   391,   397,   406,   415,   424,   436,   437,
     440,   441,   459,   460,   463,   466,   467,   470,   471,   473,
     473,   473,   473,   473,   473,   474,   474,   474,   474,   478,
     479,   482,   483,   483,   486,   491,   497,   503,   510,   515,
     521,   526,   532,   535,   538,   541,   544,   556,   571,   582,
     588,   595
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if 1
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "START_QUERY",
  "START_ONEPOS", "WORD", "REGEXP", "LBRACKET", "RBRACKET", "LPAREN",
  "RPAREN", "LBRACE", "RBRACE", "NOT", "EQ", "NEQ", "LEQ", "GEQ",
  "LSTRUCT", "RSTRUCT", "BIN_AND", "BIN_OR", "STAR", "PLUS", "QUEST",
  "SLASH", "POSNUM", "COMMA", "COLON", "DOT", "TEQ", "NNUMBER", "NUMBER",
  "KW_MEET", "KW_UNION", "KW_WITHIN", "KW_CONTAINING", "KW_WS", "KW_SWAP",
  "KW_CCOLL", "KW_FREQ", "KW_TERM", "BISON_OLD_ERR", "$accept", "list",
  "query", "globCond", "condExpr", "condConj", "condAtom", "globCondCmp",
  "within_containing", "within_containing_sufix", "within_number",
  "aligned_part_labels", "$@1", "aligned_part_no_labels", "$@2",
  "sequence", "seq", "repetition", "atomQuery", "repOpt", "structure",
  "$@3", "position", "oneposition", "thesaurus_default", "muPart",
  "meetOp", "integer", "unionOp", "attvallist", "attvaland", "word",
  "wsseekpart", "wsseek", "$@4", "attval", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-110)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-65)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     117,   310,   237,    32,  -110,    39,   109,   323,   259,    36,
      31,    28,  -110,  -110,   141,   323,  -110,   134,  -110,  -110,
    -110,  -110,  -110,   237,   237,     3,  -110,  -110,  -110,  -110,
      62,    77,    80,  -110,    84,  -110,    24,   104,   272,  -110,
    -110,  -110,    13,     8,     8,    28,   102,    10,   128,   130,
     323,   274,  -110,  -110,  -110,  -110,    92,   100,  -110,     7,
     138,    78,   164,    96,   142,    14,   318,  -110,   136,  -110,
    -110,  -110,  -110,    71,  -110,   115,    18,   132,   137,   160,
     237,   237,   188,    22,    93,   165,   179,  -110,   175,     8,
       8,  -110,  -110,  -110,  -110,   169,  -110,   173,   237,  -110,
     195,  -110,  -110,   184,   184,   221,    31,  -110,  -110,  -110,
     186,   166,   221,  -110,  -110,    14,   204,   200,   206,   217,
     225,  -110,   323,   323,   220,     6,  -110,  -110,   222,   238,
     239,   241,   270,   104,  -110,   263,   275,  -110,   276,  -110,
     277,  -110,  -110,   209,  -110,  -110,  -110,    24,  -110,   328,
    -110,  -110,   262,  -110,  -110,  -110,  -110,  -110,  -110,   144,
      14,   274,   269,    14,    14,   323,   279,  -110,    17,   296,
     271,   237,   273,  -110,  -110,  -110,  -110,  -110,  -110,  -110,
     209,  -110,   310,  -110,   149,   229,   289,   225,  -110,  -110,
    -110,   292,   293,  -110,  -110,    60,   174,   294,  -110,   310,
     186,  -110,   290,   301,   274,  -110,   300,  -110,  -110,  -110,
     237,   186,   297,   309,   329,   333,   271,   177,   274,   274,
     218,  -110,  -110,  -110,  -110,  -110,   231,  -110,  -110,  -110,
    -110,  -110,  -110,   312,  -110,  -110,  -110
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,     0,     0,     0,    69,     0,     0,     0,     0,     0,
       0,     2,     5,     6,     4,    43,    47,    49,    54,    67,
      71,    72,    89,     0,     0,     0,    90,    91,    92,    93,
      95,    97,    98,    94,    96,   121,     3,    85,     0,    87,
       1,    73,     0,     0,     0,     0,     6,     4,     0,     0,
      44,     0,    95,    97,    98,    96,     0,    65,    75,     0,
       0,     0,     0,     0,     0,     0,     0,    48,     0,    57,
      58,    59,    50,     0,   114,   112,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    70,     0,     0,
       0,    56,    55,    78,    79,     0,    51,     0,     0,    77,
       0,    74,    68,     0,     0,     0,    38,    25,    35,    29,
      37,     0,     0,    27,    32,     0,     0,     0,     0,     7,
       9,    11,     0,    45,     0,     0,   115,   113,     0,     0,
       0,     0,     0,    86,    88,     0,     0,   104,     0,   105,
       0,   108,   110,    80,    84,    53,    52,    66,    76,     0,
      26,    31,     0,    28,    34,    36,    30,    39,    33,     0,
       0,     0,     0,     0,     0,    46,     0,    60,     0,     0,
     100,     0,     0,   118,   109,   111,   106,   107,    83,    82,
       0,    41,     0,    12,     0,     0,     0,     8,    10,    62,
      63,     0,     0,    99,   101,     0,     0,     0,    81,     0,
      40,    13,     0,     0,     0,    61,     0,   116,   102,   119,
       0,    42,     0,     0,     0,     0,   100,     0,     0,     0,
       0,   117,   103,   120,    14,    15,     0,    17,    18,    19,
      20,    21,    23,     0,    24,    22,    16
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -110,  -110,   339,  -110,  -109,   189,   183,  -110,   343,   -42,
     -96,   287,  -110,   -14,  -110,     4,     1,   -13,  -110,  -110,
     303,  -110,   -32,   295,     2,  -110,  -110,   181,  -110,    -4,
     282,    -8,   143,  -110,  -110,   -21
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     3,    11,    12,   119,   120,   121,   233,    13,   107,
     108,   109,   182,   151,   199,   110,    15,    16,    17,    72,
      56,    98,    18,    19,    20,    21,    48,   180,    49,    36,
      37,    38,   194,   195,   216,    39
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      57,    42,    67,    74,    35,    14,   159,    35,    50,   155,
      47,    89,    90,    99,     4,     5,   155,    88,   167,    73,
      92,    87,   113,   115,   128,    35,    35,   116,   137,   190,
      65,    66,    40,   168,    80,    75,   138,    67,     9,   100,
      10,    62,    58,    57,    22,    80,   117,    41,    23,   191,
     129,   184,    24,   155,   118,   111,   111,   143,   144,    61,
     134,   150,   153,    63,    64,    25,    59,   123,    60,     9,
     207,    76,    26,    27,    28,    29,    30,    31,    32,    33,
      34,   126,    35,    35,     4,     5,    77,    88,   208,    78,
     154,   156,    80,    79,   147,   152,   152,   152,   158,   139,
      35,    22,     4,     5,   152,     6,    50,   140,     9,   105,
      67,    96,    91,    50,     8,     4,     5,    97,     6,   -64,
       1,     2,     7,   165,    81,   -64,     9,     8,   106,    26,
      27,    28,    29,    52,    53,    54,    33,    55,    93,     9,
      94,    10,    43,    44,   101,    68,   127,    22,     4,     5,
      50,     6,    67,   185,   183,   112,    69,    70,    71,   201,
       8,    65,    66,   124,   130,   163,   132,   196,   125,   131,
     163,   141,     9,    35,   106,    26,    27,    28,    29,    52,
      53,    54,    33,    55,   209,   142,   200,   223,   145,    22,
       4,     5,   146,     6,   157,    80,   214,   149,    80,   103,
     104,   148,     8,   211,   135,   136,   217,    66,    43,    44,
     224,   225,    35,   160,     9,   162,   106,    26,    27,    28,
      29,    52,    53,    54,    33,    55,    22,     4,     5,   161,
       6,   226,   227,   228,   229,   230,   231,   232,   163,     8,
     178,   179,    22,   202,   203,   164,    23,   234,   235,   169,
      24,     9,   166,   106,    26,    27,    28,    29,    52,    53,
      54,    33,    55,    25,    22,   170,   171,     9,   172,   174,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    22,
     173,   175,   176,   177,    51,    82,    83,    84,    85,    86,
     181,   189,    26,    27,    28,    29,    52,    53,    54,    33,
      55,   186,   192,   193,   205,   197,   215,    26,    27,    28,
      29,    52,    53,    54,    33,    55,     4,     5,   204,     6,
     206,   210,   212,     7,     4,     5,   218,     6,     8,     4,
       5,   122,     6,   213,     4,     5,     8,     6,   219,   220,
       9,     8,    10,   221,   236,    45,     8,   188,     9,    46,
      10,   114,   187,     9,    95,    10,   102,     0,     9,   222,
     106,   198,   133
};

static const yytype_int16 yycheck[] =
{
       8,     5,    15,    24,     2,     1,   115,     5,     7,   105,
       6,    43,    44,     6,     6,     7,   112,     9,    12,    23,
      10,     8,    64,     9,     6,    23,    24,    13,     6,    12,
      20,    21,     0,    27,    21,    32,    14,    50,    30,    32,
      32,    13,     6,    51,     5,    21,    32,     8,     9,    32,
      32,   160,    13,   149,    40,    63,    64,    89,    90,    28,
      81,   103,   104,    35,    36,    26,    30,    66,    32,    30,
      10,     9,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    10,    80,    81,     6,     7,     9,     9,    28,     9,
     104,   105,    21,     9,    98,   103,   104,   105,   112,     6,
      98,     5,     6,     7,   112,     9,   105,    14,    30,    13,
     123,    19,    10,   112,    18,     6,     7,    25,     9,    19,
       3,     4,    13,   122,    20,    25,    30,    18,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    10,    30,
      10,    32,    33,    34,     6,    11,    31,     5,     6,     7,
     149,     9,   165,   161,    10,    13,    22,    23,    24,    10,
      18,    20,    21,    27,    32,    21,     6,   171,    32,    32,
      21,     6,    30,   171,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    10,     6,   182,    10,    19,     5,
       6,     7,    19,     9,    28,    21,   204,    13,    21,    35,
      36,     6,    18,   199,    16,    17,   210,    21,    33,    34,
     218,   219,   210,     9,    30,     9,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,     5,     6,     7,    29,
       9,    13,    14,    15,    16,    17,    18,    19,    21,    18,
      31,    32,     5,    14,    15,    20,     9,    16,    17,    27,
      13,    30,    32,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    26,     5,    27,    27,    30,    27,     6,
      33,    34,    35,    36,    37,    38,    39,    40,    41,     5,
      10,     6,     6,     6,    25,    13,    14,    15,    16,    17,
      28,    12,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    32,     6,    32,    12,    32,     6,    33,    34,    35,
      36,    37,    38,    39,    40,    41,     6,     7,    29,     9,
      27,    27,    32,    13,     6,     7,    29,     9,    18,     6,
       7,    13,     9,    32,     6,     7,    18,     9,    29,    10,
      30,    18,    32,    10,    32,     6,    18,   164,    30,     6,
      32,    64,   163,    30,    51,    32,    61,    -1,    30,   216,
      32,   180,    80
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,    44,     6,     7,     9,    13,    18,    30,
      32,    45,    46,    51,    58,    59,    60,    61,    65,    66,
      67,    68,     5,     9,    13,    26,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    67,    72,    73,    74,    78,
       0,     8,    72,    33,    34,    45,    51,    58,    69,    71,
      59,    25,    37,    38,    39,    41,    63,    74,     6,    30,
      32,    28,    13,    35,    36,    20,    21,    60,    11,    22,
      23,    24,    62,    72,    78,    32,     9,     9,     9,     9,
      21,    20,    13,    14,    15,    16,    17,     8,     9,    65,
      65,    10,    10,    10,    10,    63,    19,    25,    64,     6,
      32,     6,    66,    35,    36,    13,    32,    52,    53,    54,
      58,    74,    13,    52,    54,     9,    13,    32,    40,    47,
      48,    49,    13,    59,    27,    32,    10,    31,     6,    32,
      32,    32,     6,    73,    78,    16,    17,     6,    14,     6,
      14,     6,     6,    65,    65,    19,    19,    72,     6,    13,
      52,    56,    74,    52,    56,    53,    56,    28,    56,    47,
       9,    29,     9,    21,    20,    59,    32,    12,    27,    27,
      27,    27,    27,    10,     6,     6,     6,     6,    31,    32,
      70,    28,    55,    10,    47,    74,    32,    48,    49,    12,
      12,    32,     6,    32,    75,    76,    72,    32,    70,    57,
      58,    10,    14,    15,    29,    12,    27,    10,    28,    10,
      27,    58,    32,    32,    74,     6,    77,    72,    29,    29,
      10,    10,    75,    10,    74,    74,    13,    14,    15,    16,
      17,    18,    19,    50,    16,    17,    32
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    43,    44,    44,    45,    45,    45,    46,    47,    47,
      48,    48,    49,    49,    49,    49,    49,    50,    50,    50,
      50,    50,    50,    50,    50,    51,    51,    51,    51,    51,
      51,    51,    51,    51,    51,    52,    52,    52,    53,    55,
      54,    57,    56,    58,    58,    58,    58,    59,    59,    60,
      60,    60,    60,    60,    61,    61,    61,    62,    62,    62,
      62,    62,    62,    62,    63,    64,    63,    65,    65,    66,
      66,    66,    66,    66,    67,    67,    67,    67,    68,    68,
      69,    69,    70,    70,    71,    72,    72,    73,    73,    74,
      74,    74,    74,    74,    74,    74,    74,    74,    74,    75,
      75,    76,    77,    76,    78,    78,    78,    78,    78,    78,
      78,    78,    78,    78,    78,    78,    78,    78,    78,    78,
      78,    78
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     2,     1,     1,     1,     3,     3,     1,
       3,     1,     3,     4,     7,     7,     8,     1,     1,     1,
       1,     1,     2,     1,     2,     3,     4,     3,     4,     3,
       4,     4,     3,     4,     4,     1,     2,     1,     1,     0,
       4,     0,     4,     1,     2,     3,     4,     1,     2,     1,
       2,     3,     4,     4,     1,     3,     3,     1,     1,     1,
       3,     5,     4,     4,     1,     0,     3,     1,     3,     1,
       3,     1,     1,     2,     3,     2,     4,     3,     3,     3,
       3,     5,     1,     1,     3,     1,     3,     1,     3,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       0,     1,     0,     4,     3,     3,     4,     4,     3,     4,
       3,     4,     2,     3,     2,     3,     6,     8,     4,     6,
       8,     1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = CQLEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == CQLEMPTY)                                        \
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

/* Backward compatibility with an undocumented macro.
   Use CQLerror or CQLUNDEF. */
#define YYERRCODE CQLUNDEF


/* Enable debugging if requested.  */
#if CQLDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
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
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
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
#else /* !CQLDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !CQLDEBUG */


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


/* Context of a parse error.  */
typedef struct
{
  yy_state_t *yyssp;
  yysymbol_kind_t yytoken;
} yypcontext_t;

/* Put in YYARG at most YYARGN of the expected tokens given the
   current YYCTX, and return the number of tokens stored in YYARG.  If
   YYARG is null, return the number of expected tokens (guaranteed to
   be less than YYNTOKENS).  Return YYENOMEM on memory exhaustion.
   Return 0 if there are more than YYARGN expected tokens, yet fill
   YYARG up to YYARGN. */
static int
yypcontext_expected_tokens (const yypcontext_t *yyctx,
                            yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  int yyn = yypact[+*yyctx->yyssp];
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
        if (yycheck[yyx + yyn] == yyx && yyx != YYSYMBOL_YYerror
            && !yytable_value_is_error (yytable[yyx + yyn]))
          {
            if (!yyarg)
              ++yycount;
            else if (yycount == yyargn)
              return 0;
            else
              yyarg[yycount++] = YY_CAST (yysymbol_kind_t, yyx);
          }
    }
  if (yyarg && yycount == 0 && 0 < yyargn)
    yyarg[0] = YYSYMBOL_YYEMPTY;
  return yycount;
}




#ifndef yystrlen
# if defined __GLIBC__ && defined _STRING_H
#  define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
# else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
# endif
#endif

#ifndef yystpcpy
# if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#  define yystpcpy stpcpy
# else
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
# endif
#endif

#ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
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
            else
              goto append;

          append:
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

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
}
#endif


static int
yy_syntax_error_arguments (const yypcontext_t *yyctx,
                           yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
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
  if (yyctx->yytoken != YYSYMBOL_YYEMPTY)
    {
      int yyn;
      if (yyarg)
        yyarg[yycount] = yyctx->yytoken;
      ++yycount;
      yyn = yypcontext_expected_tokens (yyctx,
                                        yyarg ? yyarg + 1 : yyarg, yyargn - 1);
      if (yyn == YYENOMEM)
        return YYENOMEM;
      else
        yycount += yyn;
    }
  return yycount;
}

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return -1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return YYENOMEM if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                const yypcontext_t *yyctx)
{
  enum { YYARGS_MAX = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  yysymbol_kind_t yyarg[YYARGS_MAX];
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* Actual size of YYARG. */
  int yycount = yy_syntax_error_arguments (yyctx, yyarg, YYARGS_MAX);
  if (yycount == YYENOMEM)
    return YYENOMEM;

  switch (yycount)
    {
#define YYCASE_(N, S)                       \
      case N:                               \
        yyformat = S;                       \
        break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
    }

  /* Compute error message size.  Don't count the "%s"s, but reserve
     room for the terminator.  */
  yysize = yystrlen (yyformat) - 2 * yycount + 1;
  {
    int yyi;
    for (yyi = 0; yyi < yycount; ++yyi)
      {
        YYPTRDIFF_T yysize1
          = yysize + yytnamerr (YY_NULLPTR, yytname[yyarg[yyi]]);
        if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
          yysize = yysize1;
        else
          return YYENOMEM;
      }
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return -1;
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
          yyp += yytnamerr (yyp, yytname[yyarg[yyi++]]);
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  switch (yykind)
    {
    case YYSYMBOL_WORD: /* WORD  */
#line 204 "cqpeval.y"
            { DELA(((*yyvaluep).str)); }
#line 1601 "cqpeval.cc"
        break;

    case YYSYMBOL_REGEXP: /* REGEXP  */
#line 204 "cqpeval.y"
            { DELA(((*yyvaluep).str)); }
#line 1607 "cqpeval.cc"
        break;

    case YYSYMBOL_KW_MEET: /* KW_MEET  */
#line 204 "cqpeval.y"
            { DELA(((*yyvaluep).str)); }
#line 1613 "cqpeval.cc"
        break;

    case YYSYMBOL_KW_UNION: /* KW_UNION  */
#line 204 "cqpeval.y"
            { DELA(((*yyvaluep).str)); }
#line 1619 "cqpeval.cc"
        break;

    case YYSYMBOL_KW_WITHIN: /* KW_WITHIN  */
#line 204 "cqpeval.y"
            { DELA(((*yyvaluep).str)); }
#line 1625 "cqpeval.cc"
        break;

    case YYSYMBOL_KW_CONTAINING: /* KW_CONTAINING  */
#line 204 "cqpeval.y"
            { DELA(((*yyvaluep).str)); }
#line 1631 "cqpeval.cc"
        break;

    case YYSYMBOL_KW_WS: /* KW_WS  */
#line 204 "cqpeval.y"
            { DELA(((*yyvaluep).str)); }
#line 1637 "cqpeval.cc"
        break;

    case YYSYMBOL_KW_SWAP: /* KW_SWAP  */
#line 204 "cqpeval.y"
            { DELA(((*yyvaluep).str)); }
#line 1643 "cqpeval.cc"
        break;

    case YYSYMBOL_KW_CCOLL: /* KW_CCOLL  */
#line 204 "cqpeval.y"
            { DELA(((*yyvaluep).str)); }
#line 1649 "cqpeval.cc"
        break;

    case YYSYMBOL_KW_FREQ: /* KW_FREQ  */
#line 204 "cqpeval.y"
            { DELA(((*yyvaluep).str)); }
#line 1655 "cqpeval.cc"
        break;

    case YYSYMBOL_KW_TERM: /* KW_TERM  */
#line 204 "cqpeval.y"
            { DELA(((*yyvaluep).str)); }
#line 1661 "cqpeval.cc"
        break;

    case YYSYMBOL_query: /* query  */
#line 206 "cqpeval.y"
            { delete ((*yyvaluep).evalRes).toRangeStream(); }
#line 1667 "cqpeval.cc"
        break;

    case YYSYMBOL_globCond: /* globCond  */
#line 205 "cqpeval.y"
            { DEL(((*yyvaluep).rs)); }
#line 1673 "cqpeval.cc"
        break;

    case YYSYMBOL_condExpr: /* condExpr  */
#line 205 "cqpeval.y"
            { DEL(((*yyvaluep).fc)); }
#line 1679 "cqpeval.cc"
        break;

    case YYSYMBOL_condConj: /* condConj  */
#line 205 "cqpeval.y"
            { DEL(((*yyvaluep).fc)); }
#line 1685 "cqpeval.cc"
        break;

    case YYSYMBOL_condAtom: /* condAtom  */
#line 205 "cqpeval.y"
            { DEL(((*yyvaluep).fc)); }
#line 1691 "cqpeval.cc"
        break;

    case YYSYMBOL_within_containing: /* within_containing  */
#line 205 "cqpeval.y"
            { DEL(((*yyvaluep).rs)); }
#line 1697 "cqpeval.cc"
        break;

    case YYSYMBOL_within_containing_sufix: /* within_containing_sufix  */
#line 205 "cqpeval.y"
            { DEL(((*yyvaluep).rs)); }
#line 1703 "cqpeval.cc"
        break;

    case YYSYMBOL_within_number: /* within_number  */
#line 205 "cqpeval.y"
            { DEL(((*yyvaluep).rs)); }
#line 1709 "cqpeval.cc"
        break;

    case YYSYMBOL_aligned_part_labels: /* aligned_part_labels  */
#line 205 "cqpeval.y"
            { DEL(((*yyvaluep).rs)); }
#line 1715 "cqpeval.cc"
        break;

    case YYSYMBOL_aligned_part_no_labels: /* aligned_part_no_labels  */
#line 205 "cqpeval.y"
            { DEL(((*yyvaluep).rs)); }
#line 1721 "cqpeval.cc"
        break;

    case YYSYMBOL_sequence: /* sequence  */
#line 206 "cqpeval.y"
            { delete ((*yyvaluep).evalRes).toRangeStream(); }
#line 1727 "cqpeval.cc"
        break;

    case YYSYMBOL_seq: /* seq  */
#line 206 "cqpeval.y"
            { delete ((*yyvaluep).evalRes).toRangeStream(); }
#line 1733 "cqpeval.cc"
        break;

    case YYSYMBOL_repetition: /* repetition  */
#line 206 "cqpeval.y"
            { delete ((*yyvaluep).evalRes).toRangeStream(); }
#line 1739 "cqpeval.cc"
        break;

    case YYSYMBOL_atomQuery: /* atomQuery  */
#line 206 "cqpeval.y"
            { delete ((*yyvaluep).evalRes).toRangeStream(); }
#line 1745 "cqpeval.cc"
        break;

    case YYSYMBOL_structure: /* structure  */
#line 205 "cqpeval.y"
            { DEL(((*yyvaluep).rs)); }
#line 1751 "cqpeval.cc"
        break;

    case YYSYMBOL_position: /* position  */
#line 205 "cqpeval.y"
            { DEL(((*yyvaluep).fs)); }
#line 1757 "cqpeval.cc"
        break;

    case YYSYMBOL_oneposition: /* oneposition  */
#line 205 "cqpeval.y"
            { DEL(((*yyvaluep).fs)); }
#line 1763 "cqpeval.cc"
        break;

    case YYSYMBOL_thesaurus_default: /* thesaurus_default  */
#line 205 "cqpeval.y"
            { DEL(((*yyvaluep).fs)); }
#line 1769 "cqpeval.cc"
        break;

    case YYSYMBOL_muPart: /* muPart  */
#line 205 "cqpeval.y"
            { DEL(((*yyvaluep).fs)); }
#line 1775 "cqpeval.cc"
        break;

    case YYSYMBOL_meetOp: /* meetOp  */
#line 205 "cqpeval.y"
            { DEL(((*yyvaluep).fs)); }
#line 1781 "cqpeval.cc"
        break;

    case YYSYMBOL_unionOp: /* unionOp  */
#line 205 "cqpeval.y"
            { DEL(((*yyvaluep).fs)); }
#line 1787 "cqpeval.cc"
        break;

    case YYSYMBOL_attvallist: /* attvallist  */
#line 205 "cqpeval.y"
            { DEL(((*yyvaluep).fs)); }
#line 1793 "cqpeval.cc"
        break;

    case YYSYMBOL_attvaland: /* attvaland  */
#line 205 "cqpeval.y"
            { DEL(((*yyvaluep).fs)); }
#line 1799 "cqpeval.cc"
        break;

    case YYSYMBOL_word: /* word  */
#line 204 "cqpeval.y"
            { DELA(((*yyvaluep).str)); }
#line 1805 "cqpeval.cc"
        break;

    case YYSYMBOL_attval: /* attval  */
#line 205 "cqpeval.y"
            { DEL(((*yyvaluep).fs)); }
#line 1811 "cqpeval.cc"
        break;

      default:
        break;
    }
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
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
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = CQLEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


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

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == CQLEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= CQLEOF)
    {
      yychar = CQLEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == CQLerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = CQLUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
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
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = CQLEMPTY;
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
| yyreduce -- do a reduction.  |
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
  case 2: /* list: START_QUERY query  */
#line 211 "cqpeval.y"
                       { queryResult = (yyvsp[0].evalRes); }
#line 2084 "cqpeval.cc"
    break;

  case 3: /* list: START_ONEPOS attvallist  */
#line 212 "cqpeval.y"
                             { queryResult = createEvalResult((yyvsp[0].fs)); }
#line 2090 "cqpeval.cc"
    break;

  case 4: /* query: sequence  */
#line 215 "cqpeval.y"
              { (yyval.evalRes) = (yyvsp[0].evalRes); }
#line 2096 "cqpeval.cc"
    break;

  case 5: /* query: globCond  */
#line 216 "cqpeval.y"
              { (yyval.evalRes) = createEvalResult((yyvsp[0].rs)); }
#line 2102 "cqpeval.cc"
    break;

  case 6: /* query: within_containing  */
#line 217 "cqpeval.y"
                       { (yyval.evalRes) = createEvalResult((yyvsp[0].rs)); }
#line 2108 "cqpeval.cc"
    break;

  case 7: /* globCond: sequence BIN_AND condExpr  */
#line 220 "cqpeval.y"
                               {
          (yyval.rs) = new RQFilterCond((yyvsp[-2].evalRes).toRangeStream(), (yyvsp[0].fc));
     }
#line 2116 "cqpeval.cc"
    break;

  case 8: /* condExpr: condExpr BIN_OR condConj  */
#line 225 "cqpeval.y"
                              { (yyval.fc) = new FilterCondOr((yyvsp[-2].fc), (yyvsp[0].fc)); }
#line 2122 "cqpeval.cc"
    break;

  case 9: /* condExpr: condConj  */
#line 226 "cqpeval.y"
              { (yyval.fc) = (yyvsp[0].fc); }
#line 2128 "cqpeval.cc"
    break;

  case 10: /* condConj: condConj BIN_AND condAtom  */
#line 229 "cqpeval.y"
                               { (yyval.fc) = new FilterCondAnd((yyvsp[-2].fc), (yyvsp[0].fc)); }
#line 2134 "cqpeval.cc"
    break;

  case 11: /* condConj: condAtom  */
#line 230 "cqpeval.y"
              { (yyval.fc) = (yyvsp[0].fc); }
#line 2140 "cqpeval.cc"
    break;

  case 12: /* condAtom: LPAREN condExpr RPAREN  */
#line 233 "cqpeval.y"
                            { (yyval.fc) = (yyvsp[-1].fc); }
#line 2146 "cqpeval.cc"
    break;

  case 13: /* condAtom: NOT LPAREN condExpr RPAREN  */
#line 234 "cqpeval.y"
                                { (yyval.fc) = new FilterCondNot((yyvsp[-1].fc)); }
#line 2152 "cqpeval.cc"
    break;

  case 14: /* condAtom: NUMBER DOT word EQ NUMBER DOT word  */
#line 235 "cqpeval.y"
                                        {TRY
         (yyval.fc) = new FilterCondVal(currCorp, getAttr((yyvsp[-4].str)), getAttr((yyvsp[0].str)),
              (yyvsp[-6].num), (yyvsp[-2].num), FilterCondVal::F_EQ);
         DELA((yyvsp[-4].str)); DELA((yyvsp[0].str));
     CATCH2((yyvsp[-4].str),(yyvsp[0].str))}
#line 2162 "cqpeval.cc"
    break;

  case 15: /* condAtom: NUMBER DOT word NEQ NUMBER DOT word  */
#line 240 "cqpeval.y"
                                         {TRY
         (yyval.fc) = new FilterCondVal(currCorp, getAttr((yyvsp[-4].str)), getAttr((yyvsp[0].str)),
               (yyvsp[-6].num), (yyvsp[-2].num), FilterCondVal::F_NEQ);
         DELA((yyvsp[-4].str)); DELA((yyvsp[0].str));
     CATCH2((yyvsp[-4].str),(yyvsp[0].str))}
#line 2172 "cqpeval.cc"
    break;

  case 16: /* condAtom: KW_FREQ LPAREN NUMBER DOT word RPAREN globCondCmp NUMBER  */
#line 245 "cqpeval.y"
                                                              {TRY
         (yyval.fc) = new FilterCondFreq(getAttr((yyvsp[-3].str)), (yyvsp[-5].num), (yyvsp[0].num), (yyvsp[-1].globop).op, (yyvsp[-1].globop).neg);
         DELA((yyvsp[-7].str)); DELA((yyvsp[-3].str));
     CATCH2((yyvsp[-7].str),(yyvsp[-3].str))}
#line 2181 "cqpeval.cc"
    break;

  case 17: /* globCondCmp: EQ  */
#line 251 "cqpeval.y"
        { (yyval.globop).op = FilterCondFreq::F_EQ; (yyval.globop).neg = 0; }
#line 2187 "cqpeval.cc"
    break;

  case 18: /* globCondCmp: NEQ  */
#line 252 "cqpeval.y"
         { (yyval.globop).op = FilterCondFreq::F_EQ; (yyval.globop).neg = 1; }
#line 2193 "cqpeval.cc"
    break;

  case 19: /* globCondCmp: LEQ  */
#line 253 "cqpeval.y"
         { (yyval.globop).op = FilterCondFreq::F_LEQ; (yyval.globop).neg = 0; }
#line 2199 "cqpeval.cc"
    break;

  case 20: /* globCondCmp: GEQ  */
#line 254 "cqpeval.y"
         { (yyval.globop).op = FilterCondFreq::F_GEQ; (yyval.globop).neg = 0; }
#line 2205 "cqpeval.cc"
    break;

  case 21: /* globCondCmp: LSTRUCT  */
#line 255 "cqpeval.y"
             { (yyval.globop).op = FilterCondFreq::F_GEQ; (yyval.globop).neg = 1; }
#line 2211 "cqpeval.cc"
    break;

  case 22: /* globCondCmp: NOT GEQ  */
#line 256 "cqpeval.y"
             { (yyval.globop).op = FilterCondFreq::F_GEQ; (yyval.globop).neg = 1; }
#line 2217 "cqpeval.cc"
    break;

  case 23: /* globCondCmp: RSTRUCT  */
#line 257 "cqpeval.y"
             { (yyval.globop).op = FilterCondFreq::F_LEQ; (yyval.globop).neg = 1; }
#line 2223 "cqpeval.cc"
    break;

  case 24: /* globCondCmp: NOT LEQ  */
#line 258 "cqpeval.y"
             { (yyval.globop).op = FilterCondFreq::F_LEQ; (yyval.globop).neg = 1; }
#line 2229 "cqpeval.cc"
    break;

  case 25: /* within_containing: query KW_WITHIN within_containing_sufix  */
#line 262 "cqpeval.y"
     {TRY (yyval.rs) = new RQinNode((yyvsp[-2].evalRes).toRangeStream(), (yyvsp[0].rs)); DELA((yyvsp[-1].str)); CATCH1((yyvsp[-1].str))}
#line 2235 "cqpeval.cc"
    break;

  case 26: /* within_containing: query NOT KW_WITHIN within_containing_sufix  */
#line 264 "cqpeval.y"
     {TRY (yyval.rs) = new RQnotInNode((yyvsp[-3].evalRes).toRangeStream(), (yyvsp[0].rs)); DELA((yyvsp[-1].str)); CATCH1((yyvsp[-1].str))}
#line 2241 "cqpeval.cc"
    break;

  case 27: /* within_containing: query KW_CONTAINING within_containing_sufix  */
#line 266 "cqpeval.y"
     {TRY (yyval.rs) = new RQcontainNode((yyvsp[-2].evalRes).toRangeStream(), (yyvsp[0].rs)); DELA((yyvsp[-1].str)); CATCH1((yyvsp[-1].str))}
#line 2247 "cqpeval.cc"
    break;

  case 28: /* within_containing: query NOT KW_CONTAINING within_containing_sufix  */
#line 268 "cqpeval.y"
     {TRY (yyval.rs) = new RQnotContainNode((yyvsp[-3].evalRes).toRangeStream(), (yyvsp[0].rs)); DELA((yyvsp[-1].str)); CATCH1((yyvsp[-1].str))}
#line 2253 "cqpeval.cc"
    break;

  case 29: /* within_containing: query KW_WITHIN aligned_part_labels  */
#line 270 "cqpeval.y"
     {TRY (yyval.rs) = new RQinNode((yyvsp[-2].evalRes).toRangeStream(), (yyvsp[0].rs)); DELA((yyvsp[-1].str)); CATCH1((yyvsp[-1].str))}
#line 2259 "cqpeval.cc"
    break;

  case 30: /* within_containing: query KW_WITHIN NOT aligned_part_no_labels  */
#line 272 "cqpeval.y"
     {TRY (yyval.rs) = new RQnotInNode((yyvsp[-3].evalRes).toRangeStream(), (yyvsp[0].rs)); DELA((yyvsp[-2].str)); CATCH1((yyvsp[-2].str))}
#line 2265 "cqpeval.cc"
    break;

  case 31: /* within_containing: query NOT KW_WITHIN aligned_part_no_labels  */
#line 274 "cqpeval.y"
     {TRY (yyval.rs) = new RQnotInNode((yyvsp[-3].evalRes).toRangeStream(), (yyvsp[0].rs)); DELA((yyvsp[-1].str)); CATCH1((yyvsp[-1].str))}
#line 2271 "cqpeval.cc"
    break;

  case 32: /* within_containing: query KW_CONTAINING aligned_part_labels  */
#line 277 "cqpeval.y"
     {TRY (yyval.rs) = new RQcontainNode((yyvsp[-2].evalRes).toRangeStream(), (yyvsp[0].rs)); DELA((yyvsp[-1].str)); CATCH1((yyvsp[-1].str))}
#line 2277 "cqpeval.cc"
    break;

  case 33: /* within_containing: query KW_CONTAINING NOT aligned_part_no_labels  */
#line 279 "cqpeval.y"
     {TRY (yyval.rs) = new RQnotContainNode((yyvsp[-3].evalRes).toRangeStream(), (yyvsp[0].rs)); DELA((yyvsp[-2].str)); CATCH1((yyvsp[-2].str))}
#line 2283 "cqpeval.cc"
    break;

  case 34: /* within_containing: query NOT KW_CONTAINING aligned_part_no_labels  */
#line 281 "cqpeval.y"
     {TRY (yyval.rs) = new RQnotContainNode((yyvsp[-3].evalRes).toRangeStream(), (yyvsp[0].rs)); DELA((yyvsp[-1].str)); CATCH1((yyvsp[-1].str))}
#line 2289 "cqpeval.cc"
    break;

  case 35: /* within_containing_sufix: within_number  */
#line 284 "cqpeval.y"
                   { (yyval.rs) = (yyvsp[0].rs); }
#line 2295 "cqpeval.cc"
    break;

  case 36: /* within_containing_sufix: NOT within_number  */
#line 285 "cqpeval.y"
                       { (yyval.rs) = new RQoutsideNode((yyvsp[0].rs), defaultCorp->size()); }
#line 2301 "cqpeval.cc"
    break;

  case 37: /* within_containing_sufix: sequence  */
#line 286 "cqpeval.y"
              { (yyval.rs) = (yyvsp[0].evalRes).toRangeStream(); }
#line 2307 "cqpeval.cc"
    break;

  case 38: /* within_number: NUMBER  */
#line 289 "cqpeval.y"
            {
          FastStream* f = new SequenceStream(0, lastPosition, defaultCorp->size());
          (yyval.rs) = new RQRepeatFSNode(f, (yyvsp[0].num), (yyvsp[0].num));
     }
#line 2316 "cqpeval.cc"
    break;

  case 39: /* $@1: %empty  */
#line 295 "cqpeval.y"
                {TRY
          tmpCorp = currCorp;
          alCorp = currCorp->get_aligned((yyvsp[-1].str));
          alCorp->set_default_attr(currCorp->get_default_attr()->name);
          currCorp = alCorp;
          DELA((yyvsp[-1].str));
          CATCH1((yyvsp[-1].str));
     }
#line 2329 "cqpeval.cc"
    break;

  case 40: /* aligned_part_labels: word COLON $@1 sequence  */
#line 302 "cqpeval.y"
                {
          currCorp = tmpCorp;
          (yyval.rs) = currCorp->map_aligned(alCorp, (yyvsp[0].evalRes).toRangeStream(), true);
     }
#line 2338 "cqpeval.cc"
    break;

  case 41: /* $@2: %empty  */
#line 309 "cqpeval.y"
                {TRY
          tmpCorp = currCorp;
          alCorp = currCorp->get_aligned((yyvsp[-1].str));
          alCorp->set_default_attr(currCorp->get_default_attr()->name);
          currCorp = alCorp;
          DELA((yyvsp[-1].str));
          CATCH1((yyvsp[-1].str));
     }
#line 2351 "cqpeval.cc"
    break;

  case 42: /* aligned_part_no_labels: word COLON $@2 sequence  */
#line 316 "cqpeval.y"
                {
          currCorp = tmpCorp;
          (yyval.rs) = currCorp->map_aligned(alCorp, (yyvsp[0].evalRes).toRangeStream(), false);
     }
#line 2360 "cqpeval.cc"
    break;

  case 43: /* sequence: seq  */
#line 323 "cqpeval.y"
         { (yyval.evalRes) = (yyvsp[0].evalRes); }
#line 2366 "cqpeval.cc"
    break;

  case 44: /* sequence: NOT seq  */
#line 324 "cqpeval.y"
             { (yyval.evalRes) = (yyvsp[0].evalRes).complement(); }
#line 2372 "cqpeval.cc"
    break;

  case 45: /* sequence: sequence BIN_OR seq  */
#line 325 "cqpeval.y"
                         { (yyval.evalRes) = operationOr((yyvsp[-2].evalRes), (yyvsp[0].evalRes)); }
#line 2378 "cqpeval.cc"
    break;

  case 46: /* sequence: sequence BIN_OR NOT seq  */
#line 326 "cqpeval.y"
                             { (yyval.evalRes) = operationOr((yyvsp[-3].evalRes), (yyvsp[0].evalRes).complement()); }
#line 2384 "cqpeval.cc"
    break;

  case 47: /* seq: repetition  */
#line 329 "cqpeval.y"
                { (yyval.evalRes) = (yyvsp[0].evalRes); }
#line 2390 "cqpeval.cc"
    break;

  case 48: /* seq: seq repetition  */
#line 330 "cqpeval.y"
                    { (yyval.evalRes) = concat((yyvsp[-1].evalRes), (yyvsp[0].evalRes)); }
#line 2396 "cqpeval.cc"
    break;

  case 49: /* repetition: atomQuery  */
#line 333 "cqpeval.y"
               { (yyval.evalRes) = (yyvsp[0].evalRes); }
#line 2402 "cqpeval.cc"
    break;

  case 50: /* repetition: atomQuery repOpt  */
#line 334 "cqpeval.y"
                      {
          RangeStream* r = NULL;
          if ((yyvsp[0].repData).min == 0 && (yyvsp[0].repData).max == 1) {
               if ((yyvsp[-1].evalRes).r == NULL) {
                    r = (yyvsp[-1].evalRes).toRangeStream();
               } else {
                    r = (yyvsp[-1].evalRes).r;
               }
               r = new RQOptionalNode(r);
          } else if ((yyvsp[-1].evalRes).r == NULL && (yyvsp[-1].evalRes).fcount == 1) {
               r = new RQRepeatFSNode((yyvsp[-1].evalRes).f, (yyvsp[0].repData).min, (yyvsp[0].repData).max);
          } else {
               r = new RQRepeatNode((yyvsp[-1].evalRes).toRangeStream(), (yyvsp[0].repData).min, (yyvsp[0].repData).max);
          }
          (yyval.evalRes) = createEvalResult(r);
     }
#line 2423 "cqpeval.cc"
    break;

  case 51: /* repetition: LSTRUCT structure RSTRUCT  */
#line 350 "cqpeval.y"
                               {
          FastStream* begs = new BegsOfRStream((yyvsp[-1].rs));
          (yyval.evalRes) = createEvalResult(new Pos2Range(begs, 0, 0));
     }
#line 2432 "cqpeval.cc"
    break;

  case 52: /* repetition: LSTRUCT structure SLASH RSTRUCT  */
#line 354 "cqpeval.y"
                                     {
          (yyval.evalRes) = createEvalResult((yyvsp[-2].rs));
     }
#line 2440 "cqpeval.cc"
    break;

  case 53: /* repetition: LSTRUCT SLASH structure RSTRUCT  */
#line 357 "cqpeval.y"
                                     {
          FastStream* ends = new EndsOfRStream((yyvsp[-1].rs));
          (yyval.evalRes) = createEvalResult(new Pos2Range(ends, 0, 0));
     }
#line 2449 "cqpeval.cc"
    break;

  case 54: /* atomQuery: position  */
#line 364 "cqpeval.y"
              { (yyval.evalRes) = createEvalResult((yyvsp[0].fs)); }
#line 2455 "cqpeval.cc"
    break;

  case 55: /* atomQuery: LPAREN sequence RPAREN  */
#line 365 "cqpeval.y"
                            { (yyval.evalRes) = (yyvsp[-1].evalRes); }
#line 2461 "cqpeval.cc"
    break;

  case 56: /* atomQuery: LPAREN within_containing RPAREN  */
#line 366 "cqpeval.y"
                                     { (yyval.evalRes) = createEvalResult((yyvsp[-1].rs)); }
#line 2467 "cqpeval.cc"
    break;

  case 57: /* repOpt: STAR  */
#line 369 "cqpeval.y"
          { (yyval.repData).min = 0; (yyval.repData).max = -1; }
#line 2473 "cqpeval.cc"
    break;

  case 58: /* repOpt: PLUS  */
#line 370 "cqpeval.y"
          { (yyval.repData).min = 1; (yyval.repData).max = -1; }
#line 2479 "cqpeval.cc"
    break;

  case 59: /* repOpt: QUEST  */
#line 371 "cqpeval.y"
           { (yyval.repData).min = 0; (yyval.repData).max = 1; }
#line 2485 "cqpeval.cc"
    break;

  case 60: /* repOpt: LBRACE NUMBER RBRACE  */
#line 372 "cqpeval.y"
                          { (yyval.repData).min = (yyvsp[-1].num); (yyval.repData).max = (yyvsp[-1].num); }
#line 2491 "cqpeval.cc"
    break;

  case 61: /* repOpt: LBRACE NUMBER COMMA NUMBER RBRACE  */
#line 373 "cqpeval.y"
                                       { (yyval.repData).min = (yyvsp[-3].num); (yyval.repData).max = (yyvsp[-1].num); }
#line 2497 "cqpeval.cc"
    break;

  case 62: /* repOpt: LBRACE COMMA NUMBER RBRACE  */
#line 374 "cqpeval.y"
                                { (yyval.repData).min = 0; (yyval.repData).max = (yyvsp[-1].num); }
#line 2503 "cqpeval.cc"
    break;

  case 63: /* repOpt: LBRACE NUMBER COMMA RBRACE  */
#line 375 "cqpeval.y"
                                { (yyval.repData).min = (yyvsp[-2].num); (yyval.repData).max = -1; }
#line 2509 "cqpeval.cc"
    break;

  case 64: /* structure: word  */
#line 378 "cqpeval.y"
          {TRY (yyval.rs) = currCorp->get_struct((yyvsp[0].str))->rng->whole(); DELA((yyvsp[0].str)); CATCH1((yyvsp[0].str))}
#line 2515 "cqpeval.cc"
    break;

  case 65: /* $@3: %empty  */
#line 379 "cqpeval.y"
          {TRY currStruc = currCorp->get_struct((yyvsp[0].str)); currCorp = currStruc; DELA((yyvsp[0].str)); CATCH1((yyvsp[0].str))}
#line 2521 "cqpeval.cc"
    break;

  case 66: /* structure: word $@3 attvallist  */
#line 380 "cqpeval.y"
                { (yyval.rs) = currStruc->rng->part((yyvsp[0].fs)); currCorp = defaultCorp; }
#line 2527 "cqpeval.cc"
    break;

  case 67: /* position: oneposition  */
#line 383 "cqpeval.y"
                 { (yyval.fs) = (yyvsp[0].fs); }
#line 2533 "cqpeval.cc"
    break;

  case 68: /* position: NUMBER COLON oneposition  */
#line 384 "cqpeval.y"
                              { (yyval.fs) = new AddLabel((yyvsp[0].fs), (yyvsp[-2].num)); }
#line 2539 "cqpeval.cc"
    break;

  case 69: /* oneposition: REGEXP  */
#line 387 "cqpeval.y"
            { (yyval.fs) = currCorp->get_default_attr()->regexp2poss((yyvsp[0].str), ignoreCase); DELA((yyvsp[0].str)); }
#line 2545 "cqpeval.cc"
    break;

  case 70: /* oneposition: LBRACKET attvallist RBRACKET  */
#line 388 "cqpeval.y"
                                  { (yyval.fs) = (yyvsp[-1].fs); }
#line 2551 "cqpeval.cc"
    break;

  case 71: /* oneposition: thesaurus_default  */
#line 389 "cqpeval.y"
                       { (yyval.fs) = (yyvsp[0].fs); }
#line 2557 "cqpeval.cc"
    break;

  case 72: /* oneposition: muPart  */
#line 390 "cqpeval.y"
            { (yyval.fs) = (yyvsp[0].fs); }
#line 2563 "cqpeval.cc"
    break;

  case 73: /* oneposition: LBRACKET RBRACKET  */
#line 391 "cqpeval.y"
                       {
           (yyval.fs) = new SequenceStream(0, lastPosition, defaultCorp->size());
     }
#line 2571 "cqpeval.cc"
    break;

  case 74: /* thesaurus_default: TEQ NUMBER REGEXP  */
#line 397 "cqpeval.y"
                       {
          #ifdef SKETCH_ENGINE
          (yyval.fs) = id2thesposs(currCorp, (yyvsp[0].str), (yyvsp[-1].num), true);
          DELA((yyvsp[0].str));
          #else
          yyerror("thesaurus not available in NoSketchEngine");
          YYABORT;
          #endif
     }
#line 2585 "cqpeval.cc"
    break;

  case 75: /* thesaurus_default: TEQ REGEXP  */
#line 406 "cqpeval.y"
                {
          #ifdef SKETCH_ENGINE
          (yyval.fs) = id2thesposs(currCorp, (yyvsp[0].str), 0, true);
          DELA((yyvsp[0].str));
          #else
          yyerror("thesaurus not available in NoSketchEngine");
          YYABORT;
          #endif
     }
#line 2599 "cqpeval.cc"
    break;

  case 76: /* thesaurus_default: TEQ TEQ NUMBER REGEXP  */
#line 415 "cqpeval.y"
                           {
          #ifdef SKETCH_ENGINE
          (yyval.fs) = id2thesposs(currCorp, (yyvsp[0].str), (yyvsp[-1].num), false);
          DELA((yyvsp[0].str));
          #else
          yyerror("thesaurus not available in NoSketchEngine");
          YYABORT;
          #endif
     }
#line 2613 "cqpeval.cc"
    break;

  case 77: /* thesaurus_default: TEQ TEQ REGEXP  */
#line 424 "cqpeval.y"
                    {
          #ifdef SKETCH_ENGINE
          (yyval.fs) = id2thesposs(currCorp, (yyvsp[0].str), 0, false);
          DELA((yyvsp[0].str));
          #else
          yyerror("thesaurus not available in NoSketchEngine");
          YYABORT;
          #endif
     }
#line 2627 "cqpeval.cc"
    break;

  case 78: /* muPart: LPAREN meetOp RPAREN  */
#line 436 "cqpeval.y"
                          { (yyval.fs) = (yyvsp[-1].fs); }
#line 2633 "cqpeval.cc"
    break;

  case 79: /* muPart: LPAREN unionOp RPAREN  */
#line 437 "cqpeval.y"
                           { (yyval.fs) = (yyvsp[-1].fs); }
#line 2639 "cqpeval.cc"
    break;

  case 80: /* meetOp: KW_MEET position position  */
#line 440 "cqpeval.y"
                               { (yyval.fs) = concatFs((yyvsp[-1].fs), (yyvsp[0].fs)); DELA((yyvsp[-2].str));}
#line 2645 "cqpeval.cc"
    break;

  case 81: /* meetOp: KW_MEET position position integer integer  */
#line 441 "cqpeval.y"
                                               {
          int num1 = (yyvsp[-1].num);
          int num2 = (yyvsp[0].num);

          if (num1 == num2) {
               (yyval.fs) = new QAndNode((yyvsp[-3].fs), new QMoveNode ((yyvsp[-2].fs), -num1));
          } else if (num1 > num2) {
               (yyval.fs) = new EmptyStream();
          } else {
               RangeStream* r1 = new Pos2Range((yyvsp[-3].fs), 0, 1);
               RangeStream* r2 = new Pos2Range((yyvsp[-2].fs), -num2, -num1+1);
               (yyval.fs) = new BegsOfRStream(new RQinNode(r1, r2));
          }
          DELA((yyvsp[-4].str));
     }
#line 2665 "cqpeval.cc"
    break;

  case 82: /* integer: NUMBER  */
#line 459 "cqpeval.y"
            { (yyval.num) = (yyvsp[0].num); }
#line 2671 "cqpeval.cc"
    break;

  case 83: /* integer: NNUMBER  */
#line 460 "cqpeval.y"
             { (yyval.num) = (yyvsp[0].num); }
#line 2677 "cqpeval.cc"
    break;

  case 84: /* unionOp: KW_UNION position position  */
#line 463 "cqpeval.y"
                                { (yyval.fs) = new QOrNode((yyvsp[-1].fs), (yyvsp[0].fs)); DELA((yyvsp[-2].str));}
#line 2683 "cqpeval.cc"
    break;

  case 85: /* attvallist: attvaland  */
#line 466 "cqpeval.y"
               { (yyval.fs) = (yyvsp[0].fs); }
#line 2689 "cqpeval.cc"
    break;

  case 86: /* attvallist: attvallist BIN_OR attvaland  */
#line 467 "cqpeval.y"
                                 { (yyval.fs) = new QOrNode((yyvsp[-2].fs), (yyvsp[0].fs)); }
#line 2695 "cqpeval.cc"
    break;

  case 87: /* attvaland: attval  */
#line 470 "cqpeval.y"
            { (yyval.fs) = (yyvsp[0].fs); }
#line 2701 "cqpeval.cc"
    break;

  case 88: /* attvaland: attvaland BIN_AND attval  */
#line 471 "cqpeval.y"
                              { (yyval.fs) = new QAndNode((yyvsp[-2].fs), (yyvsp[0].fs)); }
#line 2707 "cqpeval.cc"
    break;

  case 99: /* wsseekpart: NUMBER  */
#line 478 "cqpeval.y"
            { seek << (yyvsp[0].num); }
#line 2713 "cqpeval.cc"
    break;

  case 100: /* wsseekpart: %empty  */
#line 479 "cqpeval.y"
     { /* empty */ }
#line 2719 "cqpeval.cc"
    break;

  case 102: /* $@4: %empty  */
#line 483 "cqpeval.y"
                  { seek << ":"; }
#line 2725 "cqpeval.cc"
    break;

  case 104: /* attval: word EQ REGEXP  */
#line 486 "cqpeval.y"
                    {TRY
          (yyval.fs) = getAttr((yyvsp[-2].str))->regexp2poss((yyvsp[0].str), ignoreCase);
          DELA((yyvsp[-2].str));
          DELA((yyvsp[0].str));
     CATCH2((yyvsp[-2].str),(yyvsp[0].str))}
#line 2735 "cqpeval.cc"
    break;

  case 105: /* attval: word NEQ REGEXP  */
#line 491 "cqpeval.y"
                     {TRY
          FastStream* f = getAttr((yyvsp[-2].str))->regexp2poss((yyvsp[0].str), ignoreCase);
          (yyval.fs) = new QNotNode(f, defaultCorp->size());
          DELA((yyvsp[-2].str));
          DELA((yyvsp[0].str));
     CATCH2((yyvsp[-2].str),(yyvsp[0].str))}
#line 2746 "cqpeval.cc"
    break;

  case 106: /* attval: word EQ EQ REGEXP  */
#line 497 "cqpeval.y"
                       {TRY
          PosAttr *pa = getAttr((yyvsp[-3].str));
          (yyval.fs) = pa->id2poss((pa->str2id((yyvsp[0].str))));
          DELA((yyvsp[-3].str));
          DELA((yyvsp[0].str));
     CATCH2((yyvsp[-3].str),(yyvsp[0].str))}
#line 2757 "cqpeval.cc"
    break;

  case 107: /* attval: word NEQ EQ REGEXP  */
#line 503 "cqpeval.y"
                        {TRY
          PosAttr *pa = getAttr((yyvsp[-3].str));
          FastStream *f = pa->id2poss((pa->str2id((yyvsp[0].str))));
          (yyval.fs) = new QNotNode(f, defaultCorp->size());
          DELA((yyvsp[-3].str));
          DELA((yyvsp[0].str));
     CATCH2((yyvsp[-3].str),(yyvsp[0].str))}
#line 2769 "cqpeval.cc"
    break;

  case 108: /* attval: word LEQ REGEXP  */
#line 510 "cqpeval.y"
                     {TRY
          (yyval.fs) = getAttr((yyvsp[-2].str))->compare2poss((yyvsp[0].str), WORD_LESS_OR_EQUALS, ignoreCase);
          DELA((yyvsp[-2].str));
          DELA((yyvsp[0].str));
     CATCH2((yyvsp[-2].str),(yyvsp[0].str))}
#line 2779 "cqpeval.cc"
    break;

  case 109: /* attval: word NOT LEQ REGEXP  */
#line 515 "cqpeval.y"
                         {TRY
          FastStream* f = getAttr((yyvsp[-3].str))->compare2poss((yyvsp[0].str), WORD_LESS_OR_EQUALS, ignoreCase);
          (yyval.fs) = new QNotNode(f, defaultCorp->size());
          DELA((yyvsp[-3].str));
          DELA((yyvsp[0].str));
     CATCH2((yyvsp[-3].str),(yyvsp[0].str))}
#line 2790 "cqpeval.cc"
    break;

  case 110: /* attval: word GEQ REGEXP  */
#line 521 "cqpeval.y"
                     {TRY
          (yyval.fs) = getAttr((yyvsp[-2].str))->compare2poss((yyvsp[0].str), WORD_GREATER_OR_EQUALS, ignoreCase);
          DELA((yyvsp[-2].str));
          DELA((yyvsp[0].str));
     CATCH2((yyvsp[-2].str),(yyvsp[0].str))}
#line 2800 "cqpeval.cc"
    break;

  case 111: /* attval: word NOT GEQ REGEXP  */
#line 526 "cqpeval.y"
                         {TRY
          FastStream* f = getAttr((yyvsp[-3].str))->compare2poss((yyvsp[0].str), WORD_GREATER_OR_EQUALS, ignoreCase);
          (yyval.fs) = new QNotNode(f, defaultCorp->size());
          DELA((yyvsp[-3].str));
          DELA((yyvsp[0].str));
     CATCH2((yyvsp[-3].str),(yyvsp[0].str))}
#line 2811 "cqpeval.cc"
    break;

  case 112: /* attval: POSNUM NUMBER  */
#line 532 "cqpeval.y"
                   {
          (yyval.fs) = new SequenceStream((yyvsp[0].num), (yyvsp[0].num), defaultCorp->size());
     }
#line 2819 "cqpeval.cc"
    break;

  case 113: /* attval: POSNUM NUMBER NNUMBER  */
#line 535 "cqpeval.y"
                           {
          (yyval.fs) = new SequenceStream((yyvsp[-1].num), -1*(yyvsp[0].num), defaultCorp->size());
     }
#line 2827 "cqpeval.cc"
    break;

  case 114: /* attval: NOT attval  */
#line 538 "cqpeval.y"
                {
          (yyval.fs) = new QNotNode((yyvsp[0].fs), defaultCorp->size());
     }
#line 2835 "cqpeval.cc"
    break;

  case 115: /* attval: LPAREN attvallist RPAREN  */
#line 541 "cqpeval.y"
                              { 
          (yyval.fs) = (yyvsp[-1].fs);
     }
#line 2843 "cqpeval.cc"
    break;

  case 116: /* attval: KW_WS LPAREN NUMBER COMMA wsseek RPAREN  */
#line 544 "cqpeval.y"
                                             {
          #ifdef SKETCH_ENGINE
          int level = (yyvsp[-3].num);
          (yyval.fs) = getWMapPoss(currCorp->get_conf("WSBASE"), level, seek.str().c_str());
          seek.str(string());
          seek.clear();
          #else
          yyerror("word sketches not available in NoSketchEngine");
          YYABORT;
          #endif
          DELA((yyvsp[-5].str));
     }
#line 2860 "cqpeval.cc"
    break;

  case 117: /* attval: KW_WS LPAREN REGEXP COMMA REGEXP COMMA REGEXP RPAREN  */
#line 556 "cqpeval.y"
                                                          {
          #ifdef SKETCH_ENGINE
          const char* gr = (yyvsp[-3].str);
          const char *pat1 = (const char *) (yyvsp[-5].str);
          const char *pat2 = (const char *) (yyvsp[-1].str);
          (yyval.fs) = getWMapPossMatchingRegexp(currCorp->get_conf("WSBASE"), gr, pat1, pat2);
          #else
          yyerror("word sketches not available in NoSketchEngine");
          YYABORT;
          #endif
          DELA((yyvsp[-7].str));
          DELA((yyvsp[-5].str));
          DELA((yyvsp[-3].str));
          DELA((yyvsp[-1].str));
     }
#line 2880 "cqpeval.cc"
    break;

  case 118: /* attval: KW_TERM LPAREN REGEXP RPAREN  */
#line 571 "cqpeval.y"
                                  {
          #ifdef SKETCH_ENGINE
          const char *term = (const char *) (yyvsp[-1].str);
          (yyval.fs) = getTermPossMatchingRegexp(currCorp->get_conf("TERMBASE"), term);
          #else
          yyerror("terms not available in NoSketchEngine");
          YYABORT;
          #endif
          DELA((yyvsp[-3].str));
          DELA((yyvsp[-1].str));
     }
#line 2896 "cqpeval.cc"
    break;

  case 119: /* attval: KW_SWAP LPAREN NUMBER COMMA attvallist RPAREN  */
#line 582 "cqpeval.y"
                                                   {
          FastStream* src = (yyvsp[-1].fs);
          int coll = (yyvsp[-3].num);
          (yyval.fs) = new SwapKwicColl(src, coll);
          DELA((yyvsp[-5].str));
     }
#line 2907 "cqpeval.cc"
    break;

  case 120: /* attval: KW_CCOLL LPAREN NUMBER COMMA NUMBER COMMA attvallist RPAREN  */
#line 588 "cqpeval.y"
                                                                 {
          FastStream* src = (yyvsp[-1].fs);
          int from = (yyvsp[-5].num);
          int to = (yyvsp[-3].num);
          (yyval.fs) = new ChangeLabel(src, from, to);
          DELA((yyvsp[-7].str));
     }
#line 2919 "cqpeval.cc"
    break;

  case 121: /* attval: thesaurus_default  */
#line 595 "cqpeval.y"
                       {
          (yyval.fs) = (yyvsp[0].fs);
     }
#line 2927 "cqpeval.cc"
    break;


#line 2931 "cqpeval.cc"

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
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == CQLEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      {
        yypcontext_t yyctx
          = {yyssp, yytoken};
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == -1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *,
                             YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (yymsg)
              {
                yysyntax_error_status
                  = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
                yymsgp = yymsg;
              }
            else
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = YYENOMEM;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == YYENOMEM)
          YYNOMEM;
      }
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= CQLEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == CQLEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = CQLEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

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

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
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
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != CQLEMPTY)
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
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
  return yyresult;
}

#line 599 "cqpeval.y"


#ifdef BISON_OLD
#define CQLerror BISON_OLD_ERR
#endif

char cqlCurrChar() {
     return query[pos];
}

char cqlNextChar() {
     if (pos+1 == (ssize_t) query.size()) {
          return 0;
     }
     return query[pos+1];
}

char* readRegexp() {
     // read until another apostrophe (without backslash) is reached
     int from = pos;
     int slashes = 0;
     while (pos < (ssize_t) query.size() && (cqlCurrChar() != '"' || slashes % 2 == 1)) {
          if(cqlCurrChar() == '\\') {
               slashes++;
          } else {
               slashes = 0;
          }
          pos++;
     }
     int size = pos-from;
     pos++;
     return unescapeString(&(query[from]), size);
}

bool cqlIsAlpha(char c) {
     return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

bool cqlIsNumber(char c) {
     return '0' <= c && c <= '9';
}

void skipSpaces() {
     while (pos < (ssize_t) query.size() && (cqlCurrChar() == ' ' || cqlCurrChar() == '\t')) {
          pos++;
     }
}

long long readNumber() {
     int from = pos++;
     while (pos < (ssize_t) query.size() && cqlIsNumber(cqlCurrChar())) {
          pos++;
     }
     string numStr = query.substr(from, pos-from);
     return atoll(numStr.c_str());
}

string readWord() {
     int from = pos++;
     while (pos < (ssize_t) query.size() && (cqlIsAlpha(cqlCurrChar()) ||
        cqlCurrChar() == '_' || cqlCurrChar() == '@' || cqlIsNumber(cqlCurrChar()))) {
          pos++;
     }
     return query.substr(from, pos-from);
}

int yylex() {
     if (pos < 0) {
          pos = 0;
          if (onepos) return START_ONEPOS;
          else return START_QUERY;
     }

     startpos = pos;

     if (pos >= (ssize_t) query.size()) {
          return 0;
     }
     
     bool negativeNumber = (cqlCurrChar() == '-' && cqlIsNumber(cqlNextChar()));
     if (negativeNumber || cqlIsNumber(cqlCurrChar())) {
          if (negativeNumber) {
               pos++;
          }
          yylval.num = readNumber();
          if (negativeNumber) {
               yylval.num *= -1;
               return NNUMBER;
          }
          return NUMBER;
     }
     
     // word
     if (cqlIsAlpha(cqlCurrChar()) || cqlCurrChar() == '_') {
          string word = readWord();
          char* str = new char[word.size()+1];
          word.copy(str, word.size());
          str[word.size()] = '\0';
          yylval.str = str;
          if (word == "f")
               return KW_FREQ;
          if (word == "meet")
               return KW_MEET;
          if (word == "union")
               return KW_UNION;
          if (word == "within")
               return KW_WITHIN;
          if (word == "containing")
               return KW_CONTAINING;
          if (word == "ws")
               return KW_WS;
          if (word == "term")
               return KW_TERM;
          if (word == "swap")
               return KW_SWAP;
          if (word == "ccoll")
               return KW_CCOLL;
          return WORD;
     }

     char c = cqlCurrChar();
     char next = cqlNextChar();
     pos++;

     switch (c) {
          case '"':
          {
               char* s = readRegexp();
               yylval.str = s;
               return REGEXP;
          }
          case '\t': // skip
          case ' ':
               skipSpaces();
               return yylex();
          case '[':
               return LBRACKET;
          case ']':
               return RBRACKET;
          case '=':
               return EQ;
          case '<':
               if (next == '=') {
                    pos++;
                    return LEQ;
               }
               return LSTRUCT;
          case '>':
               if (next == '=') {
                    pos++;
                    return GEQ;
               }
               return RSTRUCT;
          case '&':
               return BIN_AND;
          case '|':
               return BIN_OR;
          case '!':
               if (next == '=') {
                    pos++;
                    return NEQ;
               }
               return NOT;
          case '(':
               return LPAREN;
          case ')':
               return RPAREN;
          case '{':
               return LBRACE;
          case '}':
               return RBRACE;
          case '*':
               return STAR;
          case '+':
               return PLUS;
          case '?':
               return QUEST;
          case '/':
               return SLASH;
          case '#':
               return POSNUM;
          case ',':
               return COMMA;
          case ':':
               return COLON;
          case '.':
               return DOT;
          case '~':
               return TEQ;
          case ';':
               if (pos == (ssize_t) query.size()) {
                    return 0;
               }
               break;
     }

     std::ostringstream ss;
     ss << "unexpected character";
     if(c>=32 && c<=127) ss << " " << c;
     ss << " at position " << utf8pos(query.c_str(), pos);
     yyerror(ss.str().c_str());
     return CQLerror;
}

evalResult eval(const char* cqpQuery, Corpus *corp, bool eval_onepos) {
     if (!corp)
          throw EvalQueryException ("Internal Error: eval_cqpquery(corp==NULL)");
     if (!cqpQuery)
          throw EvalQueryException ("Internal Error: eval_cqpquery(query==NULL)");
     if (!*cqpQuery)
          throw EvalQueryException ("Internal Error: eval_cqpquery(query[0]==NULL)");

     defaultCorp = corp;
     currCorp = defaultCorp;
     lastPosition = defaultCorp->size()-1;
     query = cqpQuery;
     pos = -1;
     startpos = -1;
     onepos = eval_onepos;

     if (yyparse()) {
          throw EvalQueryException(errMsg);
     }
     return queryResult;
}

RangeStream *eval_cqpquery (const char *query, Corpus *corp) {
     evalResult res = eval(query, corp, false);
     return res.toRangeStream();
}

FastStream *eval_cqponepos (const char *query, Corpus *corp) {
     evalResult res = eval(query, corp, true);
     if (res.f == NULL) {
          throw EvalQueryException("Not one pos query");
     }
     return res.f;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
