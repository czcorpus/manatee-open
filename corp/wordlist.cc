// Copyright (c) 2016-2020  Milos Jakubicek

#include "wordlist.hh"
#include "revit.hh"
#include <set>

inline void ensuresize (char *&ptr, int &allocsize, int needed,
                        const char *where)
{
    if (allocsize >= needed) return;
    char *p = (char *)realloc (ptr, needed);
    if (!p)
        throw MemoryAllocationError (where);
    allocsize = needed;
    ptr = p;
}

static set<string> locale_strings;

const char *locale2c_str (const string &locale)
{
    return (*locale_strings.insert (locale).first).c_str();
}

static set<string> encoding_strings;

const char *encoding2c_str (const string &encoding)
{
    return (*encoding_strings.insert (encoding).first).c_str();
}

template <class Format>
class CustomFrequency : public Frequency {
    string name;
    MapBinFile<Format> freqfile;
public:
    CustomFrequency(const char *freqpath, const char *n):
        name (n), freqfile (string(freqpath) + "." + name) {}
    double freq (int id) {return freqfile[id];}
};

Frequency* get_stat (const char *freqpath, const char *frqtype) {
    if (!strcmp(frqtype, "frq")) {
        try {
            return new CustomFrequency<uint64_t>(freqpath, "frq64");
        } catch (FileAccessError&) {
            errno = 0;
            return new CustomFrequency<uint32_t>(freqpath, "frq");
        }
    } else if (!strcmp(frqtype, "arf"))
        return new CustomFrequency<float>(freqpath, "arf");
    else if (!strcmp(frqtype, "aldf"))
        return new CustomFrequency<float>(freqpath, "aldf");
    else if (!strcmp(frqtype, "docf"))
        return new CustomFrequency<uint32_t>(freqpath, "docf");
    else { // custom frequency
        const char *format = NULL;
        const char *pos = strchr(frqtype, ':');
        if (!pos || *(pos+1) == '\0')
            throw invalid_argument("No format specified for custom frequency");
        format = pos+1;
        const string ftype(frqtype, pos-frqtype);
        if (*format == 'd')
            return new CustomFrequency<uint32_t>(freqpath, ftype.c_str());
        if (*format == 'l')
            return new CustomFrequency<int64_t>(freqpath, ftype.c_str());
        if (*format == 'f')
            return new CustomFrequency<float>(freqpath, ftype.c_str());
        throw invalid_argument("Invalid format specified for custom frequency");
    }
}

Frequency* WordList::get_stat (const char *frqtype) {
    return ::get_stat(this->get_freqpath(), frqtype);
}

void WordList::prepare_multivalue_map(const char *multisep) {
    if (multivalue_to_parts)
        return;
    multivalue_to_parts = new unordered_map<int, unordered_set<int>>();
    for(int oid = 0; oid < id_range(); oid++) {
        const char *full_value = id2str(oid);
        unordered_set<int> &new_ids = (*multivalue_to_parts)[oid] = unordered_set<int>();
        if(!multisep || *multisep == 0) {  // zero-length MULTISEP, attribute value is split into bytes
            if(strlen(full_value) > 1)
                for(size_t i = 0; i < strlen(full_value) - 1; i++) {
                    const char new_val[] = {full_value[i], 0};
                    new_ids.insert(str2id(new_val));
                }
        } else {  // MULTISEP is a single byte delimiter
            static int value_len = 0;
            static char *value = NULL;
            ensuresize (value, value_len, strlen (full_value) +1, "store_multival");
            value[0] = 0;
            strncat (value, full_value, value_len -1);
            for (char *s = strtok (value, multisep); s; s = strtok (NULL, multisep))
                new_ids.insert(str2id(s));
        }
    }
}

void WordList::expand_multivalue_id(int id, function<void(int)> cb) {
    if (!multivalue_to_parts)
        return cb(id);
    cb(id);
    for (auto it = (*multivalue_to_parts)[id].cbegin(); it != (*multivalue_to_parts)[id].cend(); ++it)
        cb(*it);
}

// --------------------- WordListWithLex ---------------------------

#define WLWL WordListWithLex

WLWL::WLWL (const string &path, const string &freqpath,
            const string &name, const string &loc, const string &enc)
    : WordList (path, freqpath, name, loc, enc), lex (new_lexicon (path))
{
    const string fpath = freqpath.empty() ? path : freqpath;
    try {
        regex = createDynAttr ("index", path + ".regex", name + ".regex",
                               NULL, (PosAttr*) this, locale, false);
    } catch (FileAccessError&) {regex = NULL; errno = 0;}
}
WLWL::~WLWL()
{
    delete lex;
}
int WLWL::id_range () {return lex->size();}
const char* WLWL::id2str (int id) {return lex->id2str (id);}
int WLWL::str2id (const char *str) {return lex->str2id (str);}
Generator<int> *WLWL::regexp2ids (const char *pat, bool ignorecase,
                                  const char *filter_pat)
{
    if (regex) {
        FastStream *fs = optimize_regex (regex, pat, encoding);
        return ::regexp2ids (lex, pat, locale, encoding, ignorecase, filter_pat, fs);
    }
    return ::regexp2ids (lex, pat, locale, encoding, ignorecase, filter_pat);
}
IdStrGenerator *WLWL::regexp2strids (const char *pat, bool ignorecase, const char *filter_pat)
{
    if (regex) {
        FastStream *fs = optimize_regex (regex, pat, encoding);
        return ::regexp2strids (lex, pat, locale, encoding, ignorecase, filter_pat, fs);
    }
    return ::regexp2strids (lex, pat, locale, encoding, ignorecase, filter_pat);
}
IdStrGenerator* WLWL::dump_str() {return lex->pref2strids("");}

// --------------------- WordListWithLexAndRev ---------------------

class WordListWithLexAndRev : public WordListWithLex
{
protected:
    map_colldelta_revidx rev;
    bool is_subcorp;
    WordList *complement_wordlist;
public:
    WordListWithLexAndRev (const string &path, const string &freqpath,
                           const string &name, const string &loc,
                           const string &enc, bool subcorp, WordList *complement_wordlist)
        : WordListWithLex (path, freqpath, name, loc, enc), rev (path), is_subcorp(subcorp), complement_wordlist(complement_wordlist) {}
    virtual FastStream *id2poss (int id) {return rev.id2poss (id);}
    virtual IDPosIterator *idposat (Position pos) {
        return new IDPosIteratorFromRevs<WordList> (this, pos);
    }
    virtual Frequency* get_stat(const char *frqtype) {
        if (!is_subcorp && !strcmp(frqtype, "frq"))
            return new RevCntFreq<map_colldelta_revidx> (this, &rev);
        Frequency *frq = WordList::get_stat(frqtype);
        if (complement_wordlist)
            frq = new ComplementFrequency(complement_wordlist->get_stat(frqtype), frq);
        return frq;
    }
    virtual ~WordListWithLexAndRev() {
        if (complement_wordlist) delete complement_wordlist;
    }
};

WordList *get_wordlist (const string &path, const string &freqpath,
                        const string &name, const string &loc,
                        const string &enc, bool sc, WordList *complement_wordlist)
{
    return new WordListWithLexAndRev (path, freqpath, name, loc, enc, sc, complement_wordlist);
}

// --------------------- WordListLeftJoin ----------------------

WordListLeftJoin::WordListLeftJoin (const string& path1, const string& path2)
    : finished (false), map1 (NULL), map2 (NULL)
{
    int error;
    if ((error = fsa_init(&wl1, (path1 + ".fsa").c_str())))
        throw FileAccessError (path1, "could not open");
    if ((error = fsa_init(&wl2, (path2 + ".fsa").c_str())))
        throw FileAccessError (path2, "could not open");
    if (!wl1.start || !wl2.start) {
        finished = true;
        return;
    }
    try {
        map1 = new MapBinFile<uint32_t>(path1 + ".lex.srt");
    } catch (FileAccessError&) {errno = 0;}
    try {
        map2 = new MapBinFile<uint32_t>(path2 + ".lex.srt");
    } catch (FileAccessError&) {errno = 0;}
    if ((error = fsa_li_search_init(&wl1, &wl2, &data)))
        throw FileAccessError (path1 + "||" + path2,
                               "could not initialize intersect");
    if (wl1.epsilon) {
        data.candidate[0] = '\0';
        data.word_no1 = 0;
        if (wl2.epsilon)
            data.word_no2 = 0;
        else
            data.word_no2 = -1;
    } else
        finished = !fsa_li_get_word(&wl1, &wl2, &data);
}

WordListLeftJoin::~WordListLeftJoin ()
{
    fsa_li_search_close(&data);
    fsa_destroy(&wl1);
    fsa_destroy(&wl2);
    delete map1;
    delete map2;
}

void WordListLeftJoin::next (string &str, int &id1, int &id2)
{
    if (finished)
        return;
    str.assign ((const char*)data.candidate);
    id1 = data.word_no1;
    id2 = data.word_no2;
    if (map1)
        id1 = (*map1)[id1];
    if (map2 && id2 != -1)
        id2 = (*map2)[id2];
    finished = !fsa_li_get_word(&wl1, &wl2, &data);
}


// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
