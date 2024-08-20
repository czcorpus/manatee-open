// Copyright (c) 1999-2021  Pavel Rychly, Milos Jakubicek

#ifndef POSATTR_HH
#define POSATTR_HH

#include "excep.hh"
#include "wordlist.hh"
#include "fsop.hh"
#include <exception>

using namespace std;

class CorpInfo;

class IDIterator
{
public:
    virtual int next() =0;
    virtual ~IDIterator() {}
};

class TextIterator
{
public:
    virtual const char *next() =0;
    virtual ~TextIterator() {}
};

class IDPosIterator {
    IDIterator *ids;
    Position size;
    Position curr_pos = 0;
    int curr_id;
public:
    IDPosIterator (IDIterator *it, Position s):
        ids (it), size (s), curr_id (it->next()) {}
    IDPosIterator(): ids (NULL) {}
    virtual ~IDPosIterator() {delete ids;}
    virtual void next() {curr_pos++; curr_id = ids->next();}
    virtual Position peek_pos() {return curr_pos;}
    virtual NumOfPos get_delta() {return 0;}
    virtual int peek_id() {return curr_id;}
    virtual bool end() {return curr_pos >= size;}
};

class DummyIDIter: public IDIterator {
public:
    DummyIDIter () {}
    virtual int next() {return 0;}
};

class DummyTextIter: public TextIterator {
const string s;
public:
    DummyTextIter (const string &s): s(s) {}
    virtual const char *next() {return s.c_str();}
};

class DummyIDPosIter: public IDPosIterator {
    FastStream *poss;
public:
    DummyIDPosIter (FastStream *fs): poss(fs) {}
    virtual ~DummyIDPosIter() {delete poss;}
    virtual void next() {poss->next();}
    virtual Position peek_pos() {return poss->peek();}
    virtual NumOfPos get_delta() {return 0;}
    virtual int peek_id() {return 0;}
    virtual bool end() {return poss->peek() >= poss->final();}
};

class PosAttr : public WordList
{
    Frequency *frq = NULL;
public:
    PosAttr (const string &path, const string &n,
             const string &loc="", const string &enc="", const string &fpath=""):
            WordList (path, fpath, n, loc, enc) {}
    virtual ~PosAttr () {delete frq;}
    virtual int pos2id (Position pos) =0;
    virtual const char* pos2str (Position pos) =0;
    virtual IDIterator *posat (Position pos) =0;
    virtual TextIterator *textat (Position pos) =0;
    virtual FastStream *regexp2poss (const char *pat, bool ignorecase) =0;
    virtual FastStream *compare2poss (const char *pat, int cmp, bool ignorecase) =0;
    virtual NumOfPos freq (int id) {
        if (frq)
            return frq->freq(id);
        frq = get_stat("frq");
        return frq->freq(id);
    };
    virtual NumOfPos size() =0;
    class GenFreqIter : public FreqIter {
        PosAttr *attr;
        int32_t id;
    public:
        GenFreqIter (PosAttr *attr) : attr (attr), id (0) {};
        virtual int64_t next() { if (id >= attr->id_range ()) return -1; return attr->freq (id++); };
    };

    virtual FreqIter* iter_freqs() { return new GenFreqIter (this); };
};

const char *locale2c_str (const string &locale);


PosAttr *createPosAttr (string &typecode, const string &path,
                        const string &n, const string &locale,
                        const string &encoding, NumOfPos text_size=0);
PosAttr *createSubCorpPosAttr (PosAttr *pa, const string& subpath,
                               bool complement);

#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:

