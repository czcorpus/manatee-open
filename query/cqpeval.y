%define api.prefix {cql}
%define parse.error verbose

%{

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
%}

%union {
     long long num;
     const char* str;
     evalResult evalRes;
     FastStream* fs;
     RangeStream* rs;
     rep repData;
     globOp globop;
     int i;
     FilterCond *fc;
}

%type <fs> attval attvaland attvallist thesaurus_default oneposition position muPart meetOp unionOp
%type <rs> within_containing within_containing_sufix within_number aligned_part_labels aligned_part_no_labels structure globCond
%type <num> integer
%type <globop> globCondCmp
%type <repData> repOpt
%type <evalRes> sequence seq atomQuery repetition query
%type <fc> condExpr condConj condAtom

%type <num> NUMBER NNUMBER
%type <str> word //WORD REGEXP KW_MEET KW_UNION KW_WITHIN KW_CONTAINING KW_WS KW_SWAP KW_CCOLL KW_FREQ KW_TERM

%start list

%token START_QUERY START_ONEPOS
%token <str> WORD REGEXP
%token LBRACKET RBRACKET LPAREN RPAREN LBRACE RBRACE
%token NOT EQ NEQ LEQ GEQ
%token LSTRUCT RSTRUCT
%token BIN_AND BIN_OR
%token STAR PLUS QUEST SLASH POSNUM COMMA COLON DOT TEQ
%token NNUMBER NUMBER
%token <str> KW_MEET KW_UNION KW_WITHIN KW_CONTAINING KW_WS KW_SWAP KW_CCOLL KW_FREQ KW_TERM
%token BISON_OLD_ERR

%destructor { DELA($$); } <str>
%destructor { DEL($$); } <rs> <fs> <fc>
%destructor { delete $$.toRangeStream(); } <evalRes>

%%

list :
     START_QUERY query { queryResult = $2; }
|    START_ONEPOS attvallist { queryResult = createEvalResult($2); };

query:
     sequence { $$ = $1; }
|    globCond { $$ = createEvalResult($1); }
|    within_containing { $$ = createEvalResult($1); };

globCond:
     sequence BIN_AND condExpr {
          $$ = new RQFilterCond($1.toRangeStream(), $3);
     };

condExpr:
     condExpr BIN_OR condConj { $$ = new FilterCondOr($1, $3); }
|    condConj { $$ = $1; };

condConj:
     condConj BIN_AND condAtom { $$ = new FilterCondAnd($1, $3); }
|    condAtom { $$ = $1; };

condAtom:
     LPAREN condExpr RPAREN { $$ = $2; }
|    NOT LPAREN condExpr RPAREN { $$ = new FilterCondNot($3); }
|    NUMBER DOT word EQ NUMBER DOT word {TRY
         $$ = new FilterCondVal(currCorp, getAttr($3), getAttr($7),
              $1, $5, FilterCondVal::F_EQ);
         DELA($3); DELA($7);
     CATCH2($3,$7)}
|    NUMBER DOT word NEQ NUMBER DOT word {TRY
         $$ = new FilterCondVal(currCorp, getAttr($3), getAttr($7),
               $1, $5, FilterCondVal::F_NEQ);
         DELA($3); DELA($7);
     CATCH2($3,$7)}
|    KW_FREQ LPAREN NUMBER DOT word RPAREN globCondCmp NUMBER {TRY
         $$ = new FilterCondFreq(getAttr($5), $3, $8, $7.op, $7.neg);
         DELA($1); DELA($5);
     CATCH2($1,$5)};

globCondCmp:
     EQ { $$.op = FilterCondFreq::F_EQ; $$.neg = 0; }
|    NEQ { $$.op = FilterCondFreq::F_EQ; $$.neg = 1; }
|    LEQ { $$.op = FilterCondFreq::F_LEQ; $$.neg = 0; }
|    GEQ { $$.op = FilterCondFreq::F_GEQ; $$.neg = 0; }
|    LSTRUCT { $$.op = FilterCondFreq::F_GEQ; $$.neg = 1; }
|    NOT GEQ { $$.op = FilterCondFreq::F_GEQ; $$.neg = 1; }
|    RSTRUCT { $$.op = FilterCondFreq::F_LEQ; $$.neg = 1; }
|    NOT LEQ { $$.op = FilterCondFreq::F_LEQ; $$.neg = 1; };

within_containing:
     query KW_WITHIN within_containing_sufix
     {TRY $$ = new RQinNode($1.toRangeStream(), $3); DELA($2); CATCH1($2)}
|    query NOT KW_WITHIN within_containing_sufix
     {TRY $$ = new RQnotInNode($1.toRangeStream(), $4); DELA($3); CATCH1($3)}
|    query KW_CONTAINING within_containing_sufix
     {TRY $$ = new RQcontainNode($1.toRangeStream(), $3); DELA($2); CATCH1($2)}
|    query NOT KW_CONTAINING within_containing_sufix
     {TRY $$ = new RQnotContainNode($1.toRangeStream(), $4); DELA($3); CATCH1($3)}
|    query KW_WITHIN aligned_part_labels
     {TRY $$ = new RQinNode($1.toRangeStream(), $3); DELA($2); CATCH1($2)}
|    query KW_WITHIN NOT aligned_part_no_labels
     {TRY $$ = new RQnotInNode($1.toRangeStream(), $4); DELA($2); CATCH1($2)}
|    query NOT KW_WITHIN aligned_part_no_labels
     {TRY $$ = new RQnotInNode($1.toRangeStream(), $4); DELA($3); CATCH1($3)}

|    query KW_CONTAINING aligned_part_labels
     {TRY $$ = new RQcontainNode($1.toRangeStream(), $3); DELA($2); CATCH1($2)}
|    query KW_CONTAINING NOT aligned_part_no_labels
     {TRY $$ = new RQnotContainNode($1.toRangeStream(), $4); DELA($2); CATCH1($2)}
|    query NOT KW_CONTAINING aligned_part_no_labels
     {TRY $$ = new RQnotContainNode($1.toRangeStream(), $4); DELA($3); CATCH1($3)}

within_containing_sufix:
     within_number { $$ = $1; }
|    NOT within_number { $$ = new RQoutsideNode($2, defaultCorp->size()); }
|    sequence { $$ = $1.toRangeStream(); };

within_number:
     NUMBER {
          FastStream* f = new SequenceStream(0, lastPosition, defaultCorp->size());
          $$ = new RQRepeatFSNode(f, $1, $1);
     };

aligned_part_labels:
     word COLON {TRY
          tmpCorp = currCorp;
          alCorp = currCorp->get_aligned($1);
          alCorp->set_default_attr(currCorp->get_default_attr()->name);
          currCorp = alCorp;
          DELA($1);
          CATCH1($1);
     } sequence {
          currCorp = tmpCorp;
          $$ = currCorp->map_aligned(alCorp, $4.toRangeStream(), true);
     }
;

aligned_part_no_labels:
     word COLON {TRY
          tmpCorp = currCorp;
          alCorp = currCorp->get_aligned($1);
          alCorp->set_default_attr(currCorp->get_default_attr()->name);
          currCorp = alCorp;
          DELA($1);
          CATCH1($1);
     } sequence {
          currCorp = tmpCorp;
          $$ = currCorp->map_aligned(alCorp, $4.toRangeStream(), false);
     }
;

sequence:
     seq { $$ = $1; }
|    NOT seq { $$ = $2.complement(); }
|    sequence BIN_OR seq { $$ = operationOr($1, $3); }
|    sequence BIN_OR NOT seq { $$ = operationOr($1, $4.complement()); };

seq:
     repetition { $$ = $1; }
|    seq repetition { $$ = concat($1, $2); };

repetition:
     atomQuery { $$ = $1; }
|    atomQuery repOpt {
          RangeStream* r = NULL;
          if ($2.min == 0 && $2.max == 1) {
               if ($1.r == NULL) {
                    r = $1.toRangeStream();
               } else {
                    r = $1.r;
               }
               r = new RQOptionalNode(r);
          } else if ($1.r == NULL && $1.fcount == 1) {
               r = new RQRepeatFSNode($1.f, $2.min, $2.max);
          } else {
               r = new RQRepeatNode($1.toRangeStream(), $2.min, $2.max);
          }
          $$ = createEvalResult(r);
     }
|    LSTRUCT structure RSTRUCT {
          FastStream* begs = new BegsOfRStream($2);
          $$ = createEvalResult(new Pos2Range(begs, 0, 0));
     }
|    LSTRUCT structure SLASH RSTRUCT {
          $$ = createEvalResult($2);
     }
|    LSTRUCT SLASH structure RSTRUCT {
          FastStream* ends = new EndsOfRStream($3);
          $$ = createEvalResult(new Pos2Range(ends, 0, 0));
     }
;

atomQuery:
     position { $$ = createEvalResult($1); }
|    LPAREN sequence RPAREN { $$ = $2; }
|    LPAREN within_containing RPAREN { $$ = createEvalResult($2); };

repOpt:
     STAR { $$.min = 0; $$.max = -1; }
|    PLUS { $$.min = 1; $$.max = -1; }
|    QUEST { $$.min = 0; $$.max = 1; }
|    LBRACE NUMBER RBRACE { $$.min = $2; $$.max = $2; }
|    LBRACE NUMBER COMMA NUMBER RBRACE { $$.min = $2; $$.max = $4; }
|    LBRACE COMMA NUMBER RBRACE { $$.min = 0; $$.max = $3; }
|    LBRACE NUMBER COMMA RBRACE { $$.min = $2; $$.max = -1; };

structure:
     word {TRY $$ = currCorp->get_struct($1)->rng->whole(); DELA($1); CATCH1($1)}
|    word {TRY currStruc = currCorp->get_struct($1); currCorp = currStruc; DELA($1); CATCH1($1)}
     attvallist { $$ = currStruc->rng->part($3); currCorp = defaultCorp; };

position:
     oneposition { $$ = $1; }
|    NUMBER COLON oneposition { $$ = new AddLabel($3, $1); };

oneposition:
     REGEXP { $$ = currCorp->get_default_attr()->regexp2poss($1, ignoreCase); DELA($1); }
|    LBRACKET attvallist RBRACKET { $$ = $2; }
|    thesaurus_default { $$ = $1; }
|    muPart { $$ = $1; }
|    LBRACKET RBRACKET {
           $$ = new SequenceStream(0, lastPosition, defaultCorp->size());
     }
;

thesaurus_default:
     TEQ NUMBER REGEXP {
          #ifdef SKETCH_ENGINE
          $$ = id2thesposs(currCorp, $3, $2, true);
          DELA($3);
          #else
          yyerror("thesaurus not available in NoSketchEngine");
          YYABORT;
          #endif
     }
|    TEQ REGEXP {
          #ifdef SKETCH_ENGINE
          $$ = id2thesposs(currCorp, $2, 0, true);
          DELA($2);
          #else
          yyerror("thesaurus not available in NoSketchEngine");
          YYABORT;
          #endif
     }
|    TEQ TEQ NUMBER REGEXP {
          #ifdef SKETCH_ENGINE
          $$ = id2thesposs(currCorp, $4, $3, false);
          DELA($4);
          #else
          yyerror("thesaurus not available in NoSketchEngine");
          YYABORT;
          #endif
     }
|    TEQ TEQ REGEXP {
          #ifdef SKETCH_ENGINE
          $$ = id2thesposs(currCorp, $3, 0, false);
          DELA($3);
          #else
          yyerror("thesaurus not available in NoSketchEngine");
          YYABORT;
          #endif
     }
;

muPart:
     LPAREN meetOp RPAREN { $$ = $2; }
|    LPAREN unionOp RPAREN { $$ = $2; };

meetOp:
     KW_MEET position position { $$ = concatFs($2, $3); DELA($1);}
|    KW_MEET position position integer integer {
          int num1 = $4;
          int num2 = $5;

          if (num1 == num2) {
               $$ = new QAndNode($2, new QMoveNode ($3, -num1));
          } else if (num1 > num2) {
               $$ = new EmptyStream();
          } else {
               RangeStream* r1 = new Pos2Range($2, 0, 1);
               RangeStream* r2 = new Pos2Range($3, -num2, -num1+1);
               $$ = new BegsOfRStream(new RQinNode(r1, r2));
          }
          DELA($1);
     }
;

integer:
     NUMBER { $$ = $1; }
|    NNUMBER { $$ = $1; };

unionOp:
     KW_UNION position position { $$ = new QOrNode($2, $3); DELA($1);};

attvallist:
     attvaland { $$ = $1; }
|    attvallist BIN_OR attvaland { $$ = new QOrNode($1, $3); };

attvaland:
     attval { $$ = $1; }
|    attvaland BIN_AND attval { $$ = new QAndNode($1, $3); };

word: WORD | KW_MEET | KW_UNION | KW_WITHIN | KW_CONTAINING | KW_FREQ
           | KW_WS | KW_TERM | KW_SWAP | KW_CCOLL
;

wsseekpart:
     NUMBER { seek << $1; }
|    { /* empty */ };

wsseek:
     wsseekpart
|    wsseek COLON { seek << ":"; } wsseekpart;

attval:
     word EQ REGEXP {TRY
          $$ = getAttr($1)->regexp2poss($3, ignoreCase);
          DELA($1);
          DELA($3);
     CATCH2($1,$3)}
|    word NEQ REGEXP {TRY
          FastStream* f = getAttr($1)->regexp2poss($3, ignoreCase);
          $$ = new QNotNode(f, defaultCorp->size());
          DELA($1);
          DELA($3);
     CATCH2($1,$3)}
|    word EQ EQ REGEXP {TRY
          PosAttr *pa = getAttr($1);
          $$ = pa->id2poss((pa->str2id($4)));
          DELA($1);
          DELA($4);
     CATCH2($1,$4)}
|    word NEQ EQ REGEXP {TRY
          PosAttr *pa = getAttr($1);
          FastStream *f = pa->id2poss((pa->str2id($4)));
          $$ = new QNotNode(f, defaultCorp->size());
          DELA($1);
          DELA($4);
     CATCH2($1,$4)}
|    word LEQ REGEXP {TRY
          $$ = getAttr($1)->compare2poss($3, WORD_LESS_OR_EQUALS, ignoreCase);
          DELA($1);
          DELA($3);
     CATCH2($1,$3)}
|    word NOT LEQ REGEXP {TRY
          FastStream* f = getAttr($1)->compare2poss($4, WORD_LESS_OR_EQUALS, ignoreCase);
          $$ = new QNotNode(f, defaultCorp->size());
          DELA($1);
          DELA($4);
     CATCH2($1,$4)}
|    word GEQ REGEXP {TRY
          $$ = getAttr($1)->compare2poss($3, WORD_GREATER_OR_EQUALS, ignoreCase);
          DELA($1);
          DELA($3);
     CATCH2($1,$3)}
|    word NOT GEQ REGEXP {TRY
          FastStream* f = getAttr($1)->compare2poss($4, WORD_GREATER_OR_EQUALS, ignoreCase);
          $$ = new QNotNode(f, defaultCorp->size());
          DELA($1);
          DELA($4);
     CATCH2($1,$4)}
|    POSNUM NUMBER {
          $$ = new SequenceStream($2, $2, defaultCorp->size());
     }
|    POSNUM NUMBER NNUMBER {
          $$ = new SequenceStream($2, -1*$3, defaultCorp->size());
     }
|    NOT attval {
          $$ = new QNotNode($2, defaultCorp->size());
     }
|    LPAREN attvallist RPAREN { 
          $$ = $2;
     }
|    KW_WS LPAREN NUMBER COMMA wsseek RPAREN {
          #ifdef SKETCH_ENGINE
          int level = $3;
          $$ = getWMapPoss(currCorp->get_conf("WSBASE"), level, seek.str().c_str());
          seek.str(string());
          seek.clear();
          #else
          yyerror("word sketches not available in NoSketchEngine");
          YYABORT;
          #endif
          DELA($1);
     }
|    KW_WS LPAREN REGEXP COMMA REGEXP COMMA REGEXP RPAREN {
          #ifdef SKETCH_ENGINE
          const char* gr = $5;
          const char *pat1 = (const char *) $3;
          const char *pat2 = (const char *) $7;
          $$ = getWMapPossMatchingRegexp(currCorp->get_conf("WSBASE"), gr, pat1, pat2);
          #else
          yyerror("word sketches not available in NoSketchEngine");
          YYABORT;
          #endif
          DELA($1);
          DELA($3);
          DELA($5);
          DELA($7);
     }
|    KW_TERM LPAREN REGEXP RPAREN {
          #ifdef SKETCH_ENGINE
          const char *term = (const char *) $3;
          $$ = getTermPossMatchingRegexp(currCorp->get_conf("TERMBASE"), term);
          #else
          yyerror("terms not available in NoSketchEngine");
          YYABORT;
          #endif
          DELA($1);
          DELA($3);
     }
|    KW_SWAP LPAREN NUMBER COMMA attvallist RPAREN {
          FastStream* src = $5;
          int coll = $3;
          $$ = new SwapKwicColl(src, coll);
          DELA($1);
     }
|    KW_CCOLL LPAREN NUMBER COMMA NUMBER COMMA attvallist RPAREN {
          FastStream* src = $7;
          int from = $3;
          int to = $5;
          $$ = new ChangeLabel(src, from, to);
          DELA($1);
     }
|    thesaurus_default {
          $$ = $1;
     };

%%

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
