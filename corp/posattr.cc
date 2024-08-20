//  Copyright (c) 1999-2021  Pavel Rychly, Milos Jakubicek

#include "lexicon.hh"
#include "revidx.hh"
#include "text.hh"
#include "regexplex.hh"
#include "posattr.hh"
#include "pauniq.hh"
#include "dynattr.hh"
#include "regexopt.hh"

using namespace std;

//-------------------- GenPosAttr<RevClass,TextClass> --------------------

template <class RevClass, class TextClass>
class GenPosAttr : public PosAttr
{
public:
    class IDIter: public IDIterator {
        typename TextClass::const_iterator it;
    public:
        IDIter (const typename TextClass::const_iterator &i): it(i) {}
        virtual int next() {return it.next();}
    };
    class TextIter: public TextIterator {
        typename TextClass::const_iterator it;
        lexicon *lex;
    public:
        TextIter (const typename TextClass::const_iterator &i, 
                   lexicon *l): it(i), lex(l) {}
        virtual const char *next() {return lex->id2str (it.next());}
    };
    lexicon *lex;
    TextClass txt;
    RevClass rev;
    PosAttr *regex;
    
    GenPosAttr (const string &path, const string &n, const string &locale, 
                const string &encoding, NumOfPos text_size=0)
        :PosAttr (path, n, locale, encoding), lex (new_lexicon (path)),
         txt (path, text_size), rev (path, txt.size()), regex (NULL)
    {
        try {
            DynFun *fun = createDynFun ("", "internal", "lowercase"); // lowercase = dummy here
            regex = createDynAttr ("index", path + ".regex", n + ".regex", fun,
                                   this, locale, false);
        } catch (FileAccessError&) {errno = 0;}
    }
    virtual ~GenPosAttr ()
        {delete regex; delete lex;}

    virtual int id_range () {return lex->size();}
    virtual const char* id2str (int id) {return lex->id2str (id);}
    virtual int str2id (const char *str) {return lex->str2id (str);}
    virtual int pos2id (Position pos) {return txt.pos2id (pos);}
    virtual const char* pos2str (Position pos) 
        {return lex->id2str (txt.pos2id (pos));}
    virtual IDIterator *posat (Position pos) {return new IDIter (txt.at (pos));}
    virtual IDPosIterator *idposat (Position pos)
        {return new IDPosIterator (new IDIter (txt.at (pos)), size());}
    virtual TextIterator *textat (Position pos) 
        {return new TextIter (txt.at (pos), lex);}
    virtual FastStream *id2poss (int id) {return rev.id2poss (id);}
    virtual FastStream *compare2poss (const char *pat, int cmp, bool ignorecase) 
        {return ::compare2poss (rev, lex, pat, cmp, ignorecase);}
    virtual FastStream *regexp2poss (const char *pat, bool ignorecase) {
        if (regex) {
            FastStream *fs = optimize_regex (regex, pat, encoding);
            return ::regexp2poss (rev, lex, pat, locale, encoding, ignorecase, fs);
        }
        return ::regexp2poss (rev, lex, pat, locale, encoding, ignorecase);
    }
    virtual Generator<int> *regexp2ids (const char *pat, bool ignorecase, const char *filter_pat) {
        if (regex) {
            FastStream *fs = optimize_regex (regex, pat, encoding);
            return ::regexp2ids (lex, pat, locale, encoding, ignorecase, filter_pat, fs);
        }
        return ::regexp2ids (lex, pat, locale, encoding, ignorecase, filter_pat);
    }
    virtual IdStrGenerator *regexp2strids (const char *pat, bool ignorecase, const char *filter_pat = NULL) {
        if (regex) {
            FastStream *fs = optimize_regex (regex, pat, encoding);
            return ::regexp2strids (lex, pat, locale, encoding, ignorecase, filter_pat, fs);
        }
        return ::regexp2strids (lex, pat, locale, encoding, ignorecase, filter_pat);
    }
    virtual NumOfPos freq (int id) {return rev.count (id);}
    virtual NumOfPos size() {return txt.size();}
    virtual IdStrGenerator *dump_str() {return lex->pref2strids("");}
    virtual Frequency* get_stat(const char *frqtype) {
        if (!strcmp(frqtype, "frq"))
            return new RevCntFreq<RevClass> (this, &rev);
        return WordList::get_stat(frqtype);
    }
    virtual FreqIter* iter_freqs () { return rev.iter_freqs (); }
};




//-------------------- GenPosAttr instances --------------------

typedef GenPosAttr<map_delta_revidx, giga_delta_text<MapBinFile<uint8_t> > >
        MD_MGD_PosAttr;
typedef GenPosAttr<map_delta_revidx, map_delta_text>
        MD_MD_PosAttr;
typedef GenPosAttr<map_delta_revidx, map_delta_text>
        FD_MD_PosAttr;
typedef GenPosAttr<map_delta_revidx, map_delta_text>
        FD_FD_PosAttr;
typedef GenPosAttr<map_map_delta_revidx, map_delta_text>
        FFD_FD_PosAttr;
typedef GenPosAttr<map_delta_revidx, map_big_delta_text>
        FD_FBD_PosAttr;
typedef GenPosAttr<map_delta_revidx, giga_delta_text<MapBinFile<uint8_t> > >
        FD_FGD_PosAttr;
typedef GenPosAttr<map_map_delta_revidx, map_map_giga_delta_text> NoMem_PosAttr;
typedef GenPosAttr<map_delta_revidx, int_text> MD_MI_PosAttr;
typedef GenPosAttr<map_delta_revidx, int_text> FD_MI_PosAttr;

PosAttr *createPosAttr (string &typecode, const string &path, const string &n,
                        const string &locale, const string &encoding, 
                        NumOfPos text_size)
{
#define TEST_CODE(X) if (typecode == #X) \
    return new X##_PosAttr (path, n, locale, encoding, text_size)

    if (typecode == "default")
        return new MD_MD_PosAttr (path, n, locale, encoding, text_size);
    else if (typecode == "UNIQUE")
        return createUniqPosAttr (path, n, locale, encoding, text_size);
    else TEST_CODE (MD_MGD);
    else TEST_CODE (MD_MD);
    else TEST_CODE (FD_MD);
    else TEST_CODE (FD_FD);
    else TEST_CODE (FFD_FD);
    else TEST_CODE (FD_FBD);
    else TEST_CODE (FD_FGD);
    else TEST_CODE (NoMem);
    else TEST_CODE (MD_MI);
    else TEST_CODE (FD_MI);
    else throw AttrNotFound ("Uknown type: " + typecode + ", " + path);

#undef TEST_CODE
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:

