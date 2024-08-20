// Copyright (c) 1999-2021  Pavel Rychly, Milos Jakubicek

#ifndef LEXICON_HH
#define LEXICON_HH

#include "generator.hh"
#include <limits>
#include <string>
#include <inttypes.h>

typedef uint32_t lexpos;

class lexicon {
protected:
    lexicon () {}
public:
    virtual ~lexicon() {}
    virtual const int size() const = 0;
    virtual const char* id2str (int id) = 0;
    virtual int str2id (const char *str) = 0;
    virtual Generator<int> *pref2ids (const char *str) = 0;
    virtual IdStrGenerator *pref2strids (const char *str) = 0;
};

lexicon *new_lexicon (const std::string &path);

class Ids2Str : public Generator<std::string> {
    Generator<int> *gen;
    lexicon *lex;
public:
    Ids2Str (Generator <int> *g, lexicon *l) : gen(g), lex(l) {}
    virtual ~Ids2Str() {delete gen;}
    virtual std::string next() {return lex->id2str(gen->next());}
    virtual bool end() {return gen->end();}
    virtual const int size() const {return gen->size();}
};

template <class HasId2Str>
class AddStr : public IdStrGenerator {
    Generator<int> *gen;
    HasId2Str *i2s;
    int currid;
    bool finished;
public:
    AddStr (Generator <int> *g, HasId2Str *l) : gen(g), i2s(l) {
        finished = false;
        if (gen->end())
            finished = true;
        else
            currid = gen->next();
    }
    virtual ~AddStr() {delete gen;}
    virtual void next() {
        if (gen->end())
            finished = true;
        else
            currid = gen->next();
    }
    virtual int getId() {return currid;}
    virtual string getStr() {return i2s->id2str(currid);}
    virtual bool end() {return finished;}
    virtual const int size() const {return gen->size();}
};

#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
