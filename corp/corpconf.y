%define parse.error verbose

%{
#include <fstream>
#include <iostream>
#include <cstring>
#include <cerrno>
#include "corpconf.hh"
#include "config.hh"
#include "sys/stat.h"

using namespace std;

CorpInfo* result;

string s;
static size_t pos;
static size_t lineno;
string fname;
string corpConfErrMsg;

const int errorCtxLen = 30; // length of printed context

string errorLeftCtx() {
    size_t ctxStart = 0;
    if (pos > errorCtxLen) {
        // print from the beginning of line or just previous `errorCtxLen` chars
        ctxStart = pos - errorCtxLen;
        size_t lineEnd = s.find('\n', ctxStart);
        if (lineEnd != string::npos) {
            size_t lineStart = lineEnd+1;
            if (lineStart < pos)
                ctxStart = lineStart;
        }
    }
    auto u = s.substr(ctxStart, pos-ctxStart);
    return u;
}

string errorRightCtx() {
    size_t ctxLen = s.size() - pos;
    if (pos+errorCtxLen < s.size()) {
        // print the line from the end or just previous `errorCtxLen` chars
        ctxLen = errorCtxLen;
        size_t lineEnd = s.find('\n', pos + 1);
        if (lineEnd != string::npos && lineEnd <= pos + errorCtxLen)
            ctxLen = lineEnd - pos;
    }
    return s.substr(pos, ctxLen);
}

void setErrorMsg(const char *s) {
    stringstream ss;
    ss << "error '" << s << "' while parsing corp conf " << fname
        << " on line " << lineno << ": " << endl;
    ss << errorLeftCtx() << "<*>" << errorRightCtx();
    corpConfErrMsg = ss.str();
}

extern "C" {
     int yyparse(void);
     int yylex(void);
     void yyerror(const char* s) {
         setErrorMsg(s);
     }
     int corpconfwrap() {
        return 1;
     }
}
%}

%union{
    CorpInfo* ci;
    const char* val;
}

%token KW_ATTRIBUTE KW_STRUCTURE KW_PROCESS KW_AT
%token LBRACE RBRACE PATH ATTR STR NL
%token BISON_OLD_ERR

%type <val> value PATH ATTR STR
%type <ci> statement outerBlock

%destructor { delete $$; } <ci>
%destructor { delete[] $$; } <val>

%start config

%%

config:
    {
        result = new CorpInfo();
    }
    block
;

block:
    /* empty */
|   line
|   line NL block
|   NL block
;

statement:
    KW_ATTRIBUTE value
    {
        $$ = new CorpInfo();
        result->attrs.push_back (std::pair<std::string,CorpInfo*>($2, $$));
        delete [] $2;
    }
|   KW_STRUCTURE value
    {
        $$ = new CorpInfo();
        result->structs.push_back (std::pair<std::string,CorpInfo*>($2, $$));
        delete [] $2;
    }
|   KW_PROCESS value
    {
        $$ = new CorpInfo();
        result->procs.push_back (std::pair<std::string,CorpInfo*>($2, $$));
        delete [] $2;
    }
;

outerBlock: // block before nested block: outerBlock { block }
    statement
    {
        $$ = result;
        result = $1;
    }
;

line:
    statement
|   outerBlock LBRACE NL block RBRACE
    {
        result = $1;
    }
|   ATTR value
    {
        result->opts[$1] = $2;
        delete [] $1;
        delete [] $2;
    }
|   error // consume error
;

value:
    PATH
    {
        $$ = $1;
    }
|   STR
    {
        $$ = $1;
    }
|   KW_AT PATH
    {
        char* c = new char[strlen($2)+2];
        c[0] = '@';
        c[1] = '\0';
        strcat(c, $2);
        delete [] $2;
        $$ = c;//"@" + $2;
    }
;

%%

char corpconfCurrChar() {
     return s[pos];
}

bool corpconfIsAlpha(char c) {
     return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

bool corpconfIsNumber(char c) {
     return '0' <= c && c <= '9';
}

bool tryToReadKeyword(string keyword) {
     size_t end = pos + keyword.size();
     if (end < s.size() && s.substr(pos, keyword.size()) == keyword && !corpconfIsAlpha(s[end])) {
          pos = end;
          return true;
     }
     return false;
}

char* copyStr(string str) {
    char* cstr = new char[str.size()+1];
    str.copy(cstr, str.size());
    cstr[str.size()] = '\0';
    return cstr;
}

#ifdef BISON_OLD
#define YYerror BISON_OLD_ERR
#endif
int yylex() {
    if (pos == s.size()) {
        return 0;
    }
    // skip white spaces
    while (corpconfCurrChar() == ' ' || corpconfCurrChar() == '\t') {
        pos++;
        if (pos == s.size()) {
            return yylex();
        }
    }
    // keywords
    if (tryToReadKeyword("ATTRIBUTE"))
        return KW_ATTRIBUTE;
    if (tryToReadKeyword("STRUCTURE"))
        return KW_STRUCTURE;
    if (tryToReadKeyword("PROCESS"))
        return KW_PROCESS;
    if (tryToReadKeyword("AT"))
        return KW_AT;

    char c = corpconfCurrChar();
    int start = pos++;

    // skip comment
    if (c == '#') {
        while (pos < s.size() && corpconfCurrChar() != '\n'){
            pos++;
        }
        return yylex();
    }

    if (c == '\n' || c == '\r') {
        lineno += 1;
        return NL;
    }
    if (c == '{')
        return LBRACE;
    if (c == '}')
        return RBRACE;

    // attr ('A'..'Z') ('A'..'Z'|'0'..'9')*
    if (c >= 'A' && c <= 'Z') {
        for (; pos < s.size() && (corpconfIsAlpha(corpconfCurrChar()) || corpconfIsNumber(corpconfCurrChar())); pos++){
            // empty
        }
        yylval.val = copyStr(s.substr(start, pos-start));
        return ATTR;
    }

    // path
    // ('a'..'z'|'0'..'9'|'/'|'.') ('a'..'z'|'A'..'Z'|'0'..'9'|'/'|'.'|'-'|'_')*
    if ((c >= 'a' && c <= 'z') || corpconfIsNumber(c) || c == '/' || c == '.') {
        for (; pos < s.size(); pos++) {
            c = s[pos];
            if (!corpconfIsAlpha(c) && !corpconfIsNumber(c) && c != '/' && c != '.' && c != '-' && c != '_')
                break;
        }
        yylval.val = copyStr(s.substr(start, pos-start));
        return PATH;
    }

    // str ('"' (~('"'|'\n'))* '"' | '\'' (~('\''|'\n'))* '\'')
    // str = string between quotes
    bool simple = (c == '\'');
    bool doubled = (c == '"');
    if (simple || doubled) {
        char apostrophe = c;
        int strBeg = pos;
        while(true) {
            if (pos == s.size() || s[pos] == '\n') {
                setErrorMsg("wrong format of PATH!");
                return YYerror;
            }
            if (s[pos] == apostrophe)
                break;
            pos++;
        }
        yylval.val = copyStr(s.substr(strBeg, pos-strBeg));
        pos++;
        return STR;
    }

    setErrorMsg("Unexpected end of input!");
    return YYerror;
}

string getFileContent(const char *filename) {
    ifstream in(filename, ios::in | ios::binary);
    if (in) {
        std::string contents;
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
        return(contents);
    }
    throw(errno);
}

#ifndef MANATEE_REGISTRY_STR
#define MANATEE_REGISTRY_STR "/corpora/registry"
#endif

CorpInfo *loadCorpInfoDefaults (const std::string &corp_name_or_path) {
    return loadCorpInfo(corp_name_or_path, false);
}

CorpInfo *loadCorpInfoNoDefaults (const std::string &corp_name_or_path) {
    return loadCorpInfo(corp_name_or_path, true);
}

CorpInfo *loadCorpInfo (const std::string &corp_name_or_path, bool no_defaults) {
    string configPath;
    ifstream input;
    struct stat dirstat;
    const char *registry = MANATEE_REGISTRY_STR;
    if (getenv ("MANATEE_REGISTRY") != NULL) {
        registry = getenv ("MANATEE_REGISTRY");
    }
    if (corp_name_or_path[0] != '.' && corp_name_or_path[0] != '/' && strlen(registry) > 0) {
        // find first config in ':' separated list of directories
        istringstream rs (registry);
        string regdir;
        while (getline (rs, regdir, ':')) {
            configPath = regdir + "/" + corp_name_or_path;
            if (stat(configPath.c_str(), &dirstat) != -1) {
                if ((dirstat.st_mode & S_IFMT) == S_IFDIR) {
                    continue; // skip directories since they succeed to be "opened" and !fail()
                }
            }
            input.open (configPath.c_str());
            if (!input.fail()) {
                break;
            }
        }
    } else {
        configPath = corp_name_or_path;
        if (stat(configPath.c_str(), &dirstat) != -1) {
            if ((dirstat.st_mode & S_IFMT) != S_IFDIR) {
                input.open (configPath.c_str());
            }
        }
    }
    if (input.fail() || !input.is_open()) {
        throw CorpInfoNotFound (configPath);
    }
    input.close();

    pos = 0;
    lineno = 1;
    s = getFileContent(configPath.c_str());
    fname = configPath;

    int ret = yyparse();
    if (ret != 0) {
        delete result;
        throw runtime_error(corpConfErrMsg);
    }

    result->conffile = configPath;
    if (!no_defaults) {
        result->set_defaults(result, CorpInfo::Corpus_type);
    }
    return result;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
