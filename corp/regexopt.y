%{
//  Copyright (c) 2016-2020 Radoslav Rabara, Ondrej Herman, Milos Jakubicek
%}

%define api.prefix {regexopt}
%define parse.error verbose
%define parse.trace

%{

#include "config.hh"
#include "corpus.hh"
#include <cstdio>
#include <cstring>
#include "dynattr.hh"
#include "corp/utf8.hh"
#include <finlib/fsop.hh>
#include <finlib/seek.hh>
#include <iostream>
#include "wordlist.hh"
#include "regexopt.hh"
#include "regexoptwalker.cc"

string pattern;
size_t pos;

extern "C" {
     int yyparse(void);
     int yylex(void);
     void yyerror(const char *msg) {
		ostringstream oss;
		oss << "at position " << utf8pos(pattern.c_str(), pos) << ": " << msg;
        throw RegexOptException(oss.str());
     }
     int yywrap() {
        return 1;
     }

}

Node *root;

%}


%union {
	const char *str;
	Node *node;
}

%token ENUM ESC NOMETA BACKREF SPECIAL
%token ONEANDMORE ZEROANDMORE MORETHANONE
%token QUEST STAR PLUS CARET DOLLAR
%token DOT OR LPAREN RPAREN LBRACE RBRACE LBRACKET RBRACKET

%type<node> regex nested regAlt regPart regTerm

%type<str> reStr ENUM ESC NOMETA

%start program

%%


program:
	regex
	{
		root = $1;
		if (root->first == root->last) {
			root = root->first;
		}
	}
;

regex:
	regAlt
	{
		$$ = Node::createOr($1);
	}
|	regex OR regAlt
	{
		$$ = $1;
		$$->addChild($3);
	}
;

regAlt:
	regPart
	{
		$$ = Node::createAnd($1);
	}
|	regAlt regPart
	{
		$$ = $1;
		$$->addChild($2);
	}
;

regPart:
	regTerm
	{
		$$ = $1;
	}
|	regTerm MORETHANONE
	{
		$$ = Node::createTwoAndMore($1);
	}
|	regTerm repetOne
	{
		$$ = Node::createOneAndMore($1);
	}
|	regTerm repetZero
	{
		$$ = Node::createSeparator();
	}
;

regTerm:
	LPAREN nested RPAREN
	{
		if ($2->first == $2->last) {
		    $$ = $2->first;
		} else {
		    $$ = $2;
		}
	}
|	LPAREN RPAREN
	{
		$$ = Node::createSeparator();
	}
|	ENUM
	{
		$$ = Node::createStr($1, true);
	}
|	reStr
	{
		$$ = Node::createStr($1, false);
	}
|	separator
	{
		$$ = Node::createSeparator();
	}
;

reStr:
	NOMETA
	{
		$$ = $1;
	}
|	ESC
	{
		$$ = $1;
	}
;

separator:
	DOT
|	CARET
|	DOLLAR
|	BACKREF
|	SPECIAL
;

repetOne:
	PLUS
|	ONEANDMORE
;

repetZero:
	QUEST
|	STAR
|	ZEROANDMORE
;

nested:
	regAlt
	{
		$$ = Node::createOr($1);
	}
|	nested OR regAlt
	{
		$$ = $1;
		$$->addChild($3);
	}
;

%%
extern int yydebug;
FastStream *optimize_regex (WordList *attr, const char *regexPattern, const char *encoding) {
	//TODO encoding is ignored (backward compatibility with ANTLR-based parser)
	if (regexPattern == NULL || regexPattern[0] == '\0') {
    	return new EmptyStream();
    }
	if (strstr(regexPattern, "(?") || utf8_with_supp_plane (regexPattern)
        || strstr(regexPattern, "\\p") || strstr(regexPattern, "\\x") || getenv ("MANATEE_NOREGEXOPT") != NULL)
        return NULL;
    pattern = regexPattern;
    pos = 0;
    // yydebug = 1;  // set to trace parser state
    yyparse();
    FastStream *f = TreeWalker::walk(attr, root);;
    delete root;
    return f;
}

char currChar() {
     return pattern[pos];
}

bool isNumber(char c) {
	return c >= '0' && c <= '9';
}

bool tryToReadCharClass() {
	if (pos+1 < pattern.size() && pattern[pos+1] == ':') {
		const int n = 12;
		string charClasses[n] = {"[:alnum:]", "[:alpha:]", "[:blank:]", "[:cntrl:]", "[:digit:]", "[:graph:]", "[:lower:]", "[:print:]", "[:punct:]", "[:space:]", "[:upper:]", "[:xdigit:]"};
		size_t remainingSize = pattern.size()-pos;
		for(int i = 0; i < n; i++) {
			string charClass = charClasses[i];
			if (charClass.size() <= remainingSize && pattern.compare(pos, charClass.size(), charClass) == 0) {
				pos += charClass.size();
				return true;
			}
		}
	}
	return false;
}

int parseNumber() {
	if (!isNumber(currChar())) {
		throw RegexOptException("invalid number");
	}
    int from = pos++;
    while (pos < pattern.size() && isNumber(currChar())) {
         pos++;
    }
    string numStr = pattern.substr(from, pos-from);
    return atoi(numStr.c_str());
}

inline bool is_utf8char_begin (const unsigned char c)
{
    return (c & 0xc0) != 0x80;
}

int yylex() {
	if (pos == pattern.size()) {
		return 0;
	}
	int from = pos;
	char c = pattern[pos++];
	switch (c) {
	case '(':
		return LPAREN;
	case ')':
		return RPAREN;
	case '[':
		// check if it is ENUM: LBRACKET charClass+ RBRACKET
		// fragment charClass: ( (NOMETA|DOT) '-' (NOMETA|DOT)
		// 			| ALNUM | ALPHA | BLANK | CNTRL | DIGIT | GRAPH | LOWER
		// 			| PRINT | PUNCT | SPACE | UPPER | XDIGIT
		// 			| (NOMETA|DOT) )
		if (pos < pattern.size() && currChar() != ']') {
			while (pos < pattern.size()) {
				if (!tryToReadCharClass()) {
					if (currChar() == '\\' && pos+1 < pattern.size() && pattern[pos+1] == ']') {
						pos += 2;
					}
					if (currChar() != ']') {
						pos++;
					} else {
						pos++;
						int size = pos-from;
					    char* str = (char*) malloc(sizeof(char)*(size+1));
					    memcpy(str, &pattern[from], size);
					    str[size] = '\0';
						yylval.str = str;
						return ENUM;
					}
				}
			}
		}
		pos = from+1;
		return LBRACKET;
	case ']':
		return RBRACKET;
	case '{':
		// check if it is ZEROANDMORE: '{0} | {0,}' | '{0,' ('0'..'9')+ '}' | '{,' ('0'..'9')+ '}';
		// or ONEANDMORE:  '{1} | {1,}' | '{1,' ('0'..'9')+ '}';
		// or MORETHANONE:  '{' ('2'..'9')('0'..'9')* ','? '}' | '{' ('2'..'9')('0'..'9')*',' ('0'..'9')+ '}'|'{1' ('0'..'9')+ ','? '}' | '{1' ('0'..'9')+ ',' ('0'..'9')+ '}';
		if (pos+1 < pattern.size()) {
			bool zero = false, one = false, more = false;
			if (isNumber(currChar())) {
				int firstNum = parseNumber();
				switch (firstNum) {
					case 0:
						zero = true;
						break;
					case 1:
						one = true;
						break;
					default:
						more = true;
						break;
				}
			} else if (currChar() == ',') {
				zero = true;
			}
			if (zero || one || more) {
				if (currChar() == ',') {
					pos++;
					if (isNumber(currChar()))
						parseNumber();
				}
				if (currChar() == '}') {
					pos++;
					if (zero)
						return ZEROANDMORE;
					else if (one)
						return ONEANDMORE;
					else
						return MORETHANONE;
				}
			}
		}
		pos = from+1;
		return LBRACE;
	case '}':
		return RBRACE;
	case '|':
		return OR;
	case '*':
		return STAR;
	case '+':
		return PLUS;
	case '?':
		return QUEST;
	case '.':
		return DOT;
	case '^':
		return CARET;
	case '$':
		return DOLLAR;
	case '\\':
		if (pos < pattern.size()) {
			char next = currChar();
			pos++;
			if (isNumber(next)) {
				return BACKREF;
			}
			if (next == '*' || next == '+' || next == '?' || next == '{' || next == '}' ||
				next == '[' || next == ']' || next == '(' || next == ')' || next == '.' ||
				next == '|' || next == '\\' || next == '^' || next == '$') {
				char *escStr = new char[3];
				escStr[0] = c;
				escStr[1] = next;
				escStr[2] = '\0';
				yylval.str = escStr;
				return ESC;
			}
			return SPECIAL;
		}
		break;
	}
	char *noMetaStr = new char[7]();
	noMetaStr[0] = c;

	for(size_t i = 1; i < 7 && !is_utf8char_begin(currChar()); i++) {
		c = pattern[pos++];
		noMetaStr[i] = c;
	}

	yylval.str = noMetaStr;
	return NOMETA;
}
