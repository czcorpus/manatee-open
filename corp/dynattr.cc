//  Copyright (c) 2000-2021  Pavel Rychly, Milos Jakubicek

#include "fsop.hh"
#include "lexicon.hh"
#include "revidx.hh"
#include "regexplex.hh"
#include "dynattr.hh"
#include "regexopt.hh"
#include "posattr.hh"
#include <cstdlib>

using namespace std;

template <class FreqClass=MapBinFile<uint32_t>,
          class FloatFreqClass=MapBinFile<float> >
class DynAttr: public PosAttr {
public:
    class TextIter: public TextIterator {
        TextIterator *it;
        DynFun *fun;
    public:
        TextIter (TextIterator *i, DynFun *f): it(i), fun(f) {}
        virtual const char *next() {return (*fun)(it->next());}
        virtual ~TextIter() {delete it;}
    };
    PosAttr *rattr;
    DynFun *fun;
    bool ownedByCorpus;
    DynAttr (DynFun *fn, PosAttr *attr, const string &attrpath = "", 
             const string &n = "", const string &loc="", const string &enc="",
             bool ownedByCorpus = true)
        : PosAttr (attrpath, n, loc == "" ? attr->locale : loc, 
                   attr->encoding), 
          rattr (attr), fun(fn), ownedByCorpus(ownedByCorpus) {}
    virtual ~DynAttr () {
        delete fun;
        if (!ownedByCorpus) {
            delete rattr;
        }
    }
    virtual int id_range () {return 0;}
    virtual const char* id2str (int id) {return "";}
    virtual int str2id(const char *str) {return 0;}
    virtual int pos2id (Position pos) {return 0;}
    virtual const char* pos2str (Position pos) 
        {return (*fun) (rattr->pos2str (pos));}
    virtual IDIterator * posat(Position pos) 
        {return new DummyIDIter ();}
    virtual IDPosIterator * idposat(Position pos)
        {return new IDPosIterator (new DummyIDIter (), 0);}
    virtual TextIterator * textat(Position pos) 
        {return new TextIter (rattr->textat (pos), fun);}
    virtual FastStream *id2poss (int id)
        {return new EmptyStream();}
    virtual FastStream *regexp2poss (const char *pat, bool)
        {return new EmptyStream();}
    virtual FastStream *compare2poss (const char *pat, int, bool)
        {return new EmptyStream();}
    virtual Generator<int> *regexp2ids (const char *pat, bool ignorecase, const char *filter_pat)
        {return new EmptyGenerator<int>();}
    virtual IdStrGenerator *regexp2strids (const char *pat, bool ignorecase, const char *filter_pat = NULL)
        {return new EmptyIdStrGenerator();}
    virtual NumOfPos size () {return rattr->size();}
    virtual IdStrGenerator *dump_str() {NOTIMPLEMENTED}
};

class DynAttr_withLex: public DynAttr<> {
protected:
    bool transquery;
public:
    class IDIter: public IDIterator {
        TextIterator *ti;
        IDIterator *ii;
        DynAttr_withLex &at;
        Position pos, final;
    public:
        IDIter (DynAttr_withLex &a, Position p):
            ti(NULL), ii(NULL), at(a), pos(p), final(a.rattr->size()) {
            if (a.ridx)
                ii = a.rattr->posat (p);
            else
                ti = a.rattr->textat (p);
        }
        virtual int next() {
            if (pos >= 0 && pos++ < final) {
                if (at.ridx)
                    return (*(at.ridx))[ii->next()];
                else
                    return at.lex->str2id ((*at.fun) (ti->next()));
            } else
               return -1;
        }
        virtual ~IDIter() {delete ti; delete ii;}
    };
    class TextIter: public TextIterator {
        IDIterator *it;
        DynAttr_withLex &at;
    public:
        TextIter (IDIterator *i, DynAttr_withLex &a): it(i), at(a) {}
        virtual const char *next() {return at.lex->id2str((*(at.ridx))[it->next()]);}
        virtual ~TextIter() {delete it;}
    };
    lexicon *lex;
    MapBinFile<lexpos> *ridx;

    DynAttr_withLex (DynFun *fn, PosAttr *attr, const string &attrpath,
                     const string &n, const string &loc="", bool transq=false,
                     bool ownedByCorpus = true)
        : DynAttr<> (fn, attr, attrpath, n, loc, "", ownedByCorpus),
          transquery (transq), lex (new_lexicon(attrpath)), ridx (NULL)
    {
        try {
            ridx = new MapBinFile<lexpos> (attrpath + ".lex.ridx");
        } catch (FileAccessError&) {errno = 0;}
    }
    ~DynAttr_withLex() {delete lex; delete ridx;}
    virtual int id_range () {return lex->size();}
    virtual const char* id2str (int id) {return lex->id2str (id);}
    virtual int str2id(const char *str) 
        {return lex->str2id (transquery ? (*fun)(str) : str);}
    virtual int pos2id (Position pos) 
        {return ridx ? (*ridx)[rattr->pos2id (pos)]
                     : lex->str2id ((*fun) (rattr->pos2str (pos)));}
    virtual const char* pos2str (Position pos) 
        {return ridx ? lex->id2str((*ridx)[rattr->pos2id (pos)])
                     : (*fun) (rattr->pos2str (pos));}
    virtual IDIterator * posat (Position pos) 
        {return new IDIter (*this, pos);}
    virtual IDPosIterator * idposat (Position pos)
        {return new IDPosIterator (new IDIter (*this, pos), size());}
    virtual TextIterator * textat(Position pos)
        {return ridx ? (TextIterator*) new TextIter (rattr->posat (pos), *this)
           : (TextIterator*) new DynAttr<>::TextIter (rattr->textat (pos), fun);}
    virtual Generator<int> *regexp2ids (const char *pat, bool ignorecase, const char *filter_pat)
        {
            if (regex) {
                FastStream *fs = optimize_regex (regex, pat, encoding);
                return ::regexp2ids (lex, pat, locale, encoding, ignorecase,
                                     filter_pat, fs);
            }
            return ::regexp2ids (lex, pat, locale, encoding, ignorecase, filter_pat);
        }
    virtual IdStrGenerator *regexp2strids (const char *pat, bool ignorecase, const char *filter_pat)
        {
            if (regex) {
                FastStream *fs = optimize_regex (regex, pat, encoding);
                return ::regexp2strids (lex, pat, locale, encoding, ignorecase,
                                     filter_pat, fs);
            }
            return ::regexp2strids (lex, pat, locale, encoding, ignorecase, filter_pat);
        }
    virtual IdStrGenerator *dump_str() {return lex->pref2strids("");}
};

class DynAttr_withIndex: public DynAttr_withLex {
    class DynFrequency: public Frequency {
        Frequency *rattr_freq = NULL;
        map_delta_revidx *rev = NULL;
    public:
        DynFrequency (WordList *w, const char *n, WordList *rattr, map_delta_revidx *r)
            : rattr_freq (rattr->get_stat(n)), rev (r) { }
        ~DynFrequency() {delete rattr_freq;}
        double freq(int id) {
            if (id < 0) return 0;
            NumOfPos count = 0;
            FastStream *fs = rev->id2poss (id);
            while (fs->peek() < fs->final())
                count += rattr_freq->freq (fs->next());
            delete fs;
            return count;
        }
    };
protected:
    FastStream *ID_list2poss (FastStream *ids);
public:
    map_delta_revidx rev;
    DynAttr_withIndex (DynFun *fn, PosAttr *attr, const string &attrpath,
                       const string &n, const string &loc="", 
                       bool transq=false, bool ownedByCorpus = true)
        : DynAttr_withLex (fn, attr, attrpath, n, loc, transq, ownedByCorpus),
          rev (attrpath, rattr->id_range()) {}
    virtual ~DynAttr_withIndex() {}
    virtual FastStream *id2poss (int id)
        {return ID_list2poss (rev.id2poss (id));}
    virtual FastStream *dynid2srcids (int id) {return rev.id2poss (id);}
    virtual FastStream *regexp2poss (const char *pat, bool ignorecase)
    {
        regexp_pattern regpat (pat, locale, encoding, ignorecase);
        if (regpat.any())
            return new SequenceStream (0, size() - 1, size());
        FastStream *fs = NULL;
        if (regex)
            fs = optimize_regex (regex, pat, encoding);
        return ID_list2poss (::regexp2poss (rev, lex,
                                            transquery ? (*fun)(pat) : pat,
                                            locale, encoding, ignorecase, fs));
    }
    virtual FastStream *compare2poss (const char *pat, int cmp, bool ignorecase)
    {
        return ID_list2poss (::compare2poss (rev, lex,
                                             transquery ? (*fun)(pat) : pat,
                                             cmp, ignorecase));
    }
    virtual Frequency* get_stat(const char *frqtype) {
        if (!strcmp(frqtype, "frq")) {
            try {
                return WordList::get_stat("frq");
            } catch (FileAccessError&) {
                errno = 0;
            }
            try {
                return WordList::get_stat("freq:l");
            } catch (FileAccessError&) {
                errno = 0;
            }
            return new DynFrequency(this, frqtype, rattr, &rev);
        }
        if (!strcmp(frqtype, "norm:l")) {
            try {
                return WordList::get_stat("norm:l");
            } catch (FileAccessError&) {
                errno = 0;
            }
            return new DynFrequency(this, frqtype, rattr, &rev);
        } else
            return WordList::get_stat(frqtype);
    }
};

FastStream *DynAttr_withIndex::ID_list2poss (FastStream *ids)
{
    std::vector<FastStream*> *fsv = new std::vector<FastStream*>;
    fsv->reserve(10);
    while (ids->peek() < ids->final()) {
        int id = ids->next();
        FastStream *s = rattr->id2poss (id);
        fsv->push_back(s);
    }
    delete ids;
    return QOrVNode::create (fsv);
}

PosAttr *createDynAttr (const string &type, const string &apath, 
                        const string &n, DynFun *fun, PosAttr *from, 
                        const string &locale, bool trasquery,
                        bool ownedByCorpus)
{
    if (type == "default" || type == "plain")
        return new DynAttr<> (fun, from, apath, n, locale, "", ownedByCorpus);
    else if (type == "lexicon")
        return new DynAttr_withLex (fun, from, apath, n, locale, trasquery,
                                    ownedByCorpus);
    else if (type == "index" || type == "freq")
        return new DynAttr_withIndex (fun, from, apath, n, locale, trasquery,
                                      ownedByCorpus);
    else
        throw AttrNotFound ("Dynamic (" + type + "):" + apath);
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:

