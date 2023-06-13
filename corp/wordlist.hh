// Copyright (c) 2016-2021  Milos Jakubicek

#ifndef WORDLIST_HH
#define WORDLIST_HH

class PosAttr;

#include "dynattr.hh"
#include "regexopt.hh"
#include <finlib/binfile.hh>
#include <finlib/generator.hh>
#include <finlib/fstream.hh>
#include <finlib/revidx.hh>
#include <finlib/regexplex.hh>
#include <finlib/lexicon.hh>
extern "C" {
#include <fsa3/fsa_op.h>
}
#include <unordered_map>
#include <unordered_set>

using namespace std;
const char *locale2c_str (const string &locale);
const char *encoding2c_str (const string &encoding);

class IDPosIterator;
class Frequency;

Frequency *get_stat (const char *frqpath, const char *frqtype);

class WordList
{
    unordered_map<int, unordered_set<int>> *multivalue_to_parts = NULL;  // maps
public:
    const string attr_path, freq_path, name;
    const char *locale, *encoding;
    WordList *regex = NULL;
    WordList (const string &path, const string &freqpath, const string &name,
              const string &loc, const string &enc) :
              attr_path (path), freq_path (freqpath.size() ? freqpath : path), name (name),
              locale (locale2c_str(loc)), encoding (encoding2c_str(enc)) {}
    virtual ~WordList() {delete multivalue_to_parts; delete regex;}
    virtual int id_range () =0;
    virtual const char* id2str (int id) =0;
    virtual int str2id (const char *str) =0;
    virtual Generator<int> *regexp2ids (const char *pat, bool ignorecase, const char *filter_pat = NULL) =0;
    virtual IdStrGenerator *regexp2strids (const char *pat, bool ignorecase, const char *filter_pat = NULL) =0;
    virtual FastStream *dynid2srcids (int id) {NOTIMPLEMENTED}
    virtual FastStream *id2poss (int id) =0;
    virtual IDPosIterator *idposat (Position pos) =0;
    virtual IdStrGenerator* dump_str() =0;
    virtual Frequency *get_stat (const char *frqtype);
    const char *get_freqpath () {return freq_path.c_str();}
    void prepare_multivalue_map (const char *multisep);
    void expand_multivalue_id(int id, function<void(int)> cb);
};

class Frequency {
public:
    virtual ~Frequency() {}
    virtual double freq (int id) =0;
};

class ComplementFrequency : public Frequency {
    Frequency *whole, *part;
public:
    ComplementFrequency (Frequency *whole, Frequency *part) : whole(whole), part(part) {}
    virtual ~ComplementFrequency () { delete whole; delete part; }
    virtual double freq (int id) {
        return std::max(0., whole->freq(id) - part->freq(id));
    }
};

template <class RevClass>
class RevCntFreq : public Frequency {
    RevClass *rev;
public:
    RevCntFreq (WordList *w, RevClass *r): rev(r) {}
    double freq (int id) {return (double) rev->count(id);}
};

class WordListWithLex : public WordList
{
protected:
    lexicon *lex;
public:
    WordListWithLex (const string &path, const string &freqpath,
                     const string &name, const string &loc,
                     const string &enc);
    virtual ~WordListWithLex();
    virtual int id_range ();
    virtual const char* id2str (int id);
    virtual int str2id (const char *str);
    virtual Generator<int> *regexp2ids (const char *pat, bool ignorecase,
                                        const char *filter_pat = NULL);
    virtual IdStrGenerator *regexp2strids (const char *pat, bool ignorecase, const char *filter_pat = NULL);
    virtual FastStream *id2poss (int id) =0;
    virtual IDPosIterator *idposat (Position pos) =0;
    virtual IdStrGenerator* dump_str();
};

class WordListLeftJoin
{
    search_data data;
    fsa wl1, wl2;
    bool finished;
    MapBinFile <uint32_t> *map1, *map2;
public:
    WordListLeftJoin (const string& path1, const string& path2);
    ~WordListLeftJoin ();
    bool end() {return finished;}
    void next (string &str, int &id1, int &id2);
};

WordList *get_wordlist (const string &path, const string &freqpath,
                        const string &name, const string &loc,
                        const string &enc,
                        bool subcorp = false, WordList *complement_wordlist = 0);
#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
