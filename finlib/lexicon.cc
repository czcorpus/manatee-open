// Copyright (c) 1999-2021  Pavel Rychly, Milos Jakubicek

#include "lexicon.hh"
#include "binfile.hh"
#if defined(LEXICON_USE_FSA3)
extern "C" {
#include <fsa3/fsa_op.h>
}
#endif

#include <stdexcept>
using namespace std;

int plain_strcmp (const char *a, const char *b)
{
     while (*a && *a == *b) {
	  a++; b++;
     }
     return *a - *b;
}

int prefstrcmp (const char *pref, const char *str)
{
     while (*pref && *pref == *str) {
	  pref++; str++;
     }
     return *pref ? *pref - *str : 0;
}

typedef uint32_t lexpos;
typedef typename MapBinFile<lexpos>::const_iterator Iterator;

class IdsIterator : public Generator<int> {
    Iterator it;
    int ids_count;
public:
    IdsIterator (Iterator i, int c): it(i), ids_count (c) {}
    int next() {
        if (ids_count) {
            ids_count--; return *it++;
        } else
            return *it;
    }
    bool end() {return ids_count == 0;}
    const int size() const {return ids_count;}
};

#if defined(LEXICON_USE_FSA3)
class FSAPrefixGenerator: public IdStrGenerator {
protected:
    fsa *aut;
    MapBinFile<lexpos> *lsrtf;
    bool finished;
    search_data data;
    int currid, count;
    string prefix;
public:
    // default (empty) p means to generate all strings
    FSAPrefixGenerator (fsa* a, MapBinFile<lexpos> *l, const char *p=""):
        aut(a), lsrtf(l), finished(false), prefix(p) {
        int error;
        if (prefix.size()) { // dump words prefixed by p
            fsa_res r = find_number (aut, (const unsigned char*) prefix.c_str());
            if (r.pref_beg < 0) {
                error = fsa_dump_search_init(aut, &data, NULL);
                *(data.candidate) = '\0';
                currid = -1;
                count = 0;
                finished = true;
                return;
            }
            error = fsa_dump_search_init(aut, &data, r.node);
            if (r.pref_end < 0) // p is not a word but is a prefix
                finished = !fsa_dump_get_word(aut, &data);
            else
                *(data.candidate) = '\0'; // p is a word
            count = abs(r.pref_end) - r.pref_beg + 1;
            currid = r.pref_beg;
        } else { // dump whole automaton
            error = fsa_dump_search_init(aut, &data, NULL);
            if (aut->epsilon)
                *(data.candidate) = '\0';
            else
                finished = !fsa_dump_get_word(aut, &data);
            count = aut->count;
            currid = 0;
        }
        if (error)
            throw runtime_error ("Error initializing prefix search in FSA");
    }
    virtual void next() {
        if (count == 1) // just a single item
            finished = true;
        if (!finished) {
            finished = !fsa_dump_get_word(aut, &data);
            currid++;
        }
    }
    virtual int getId() {return (lsrtf ? (*lsrtf)[currid] : currid);}
    virtual string getStr() {return prefix + (char*) data.candidate;}
    virtual bool end() {return finished;}
    virtual const int size() const {return count;}
    virtual ~FSAPrefixGenerator() {fsa_dump_search_close(&data);}
};

inline int str2id (fsa *f, MapBinFile<lexpos> *lsrtf, const char *str)
{
    fsa_res r = find_number (f, (const unsigned char*) str);
    if (r.pref_end < 0)
        return -1;
    if (lsrtf)
        return (*lsrtf)[r.pref_beg];
    return r.pref_beg;
}

inline Generator<int> *pref2ids (fsa *lexa, MapBinFile<lexpos> *lsrtf, const char *str)
{
    fsa_res r = find_number (lexa, (const unsigned char*) str);
    if (r.pref_beg < 0)
        return new EmptyGenerator<int>();
    if (lsrtf)
        return new IdsIterator(lsrtf->at(r.pref_beg), abs(r.pref_end) - r.pref_beg + 1);
    else
        return new SequenceGenerator<int> (r.pref_beg, abs(r.pref_end) + 1);
}

#endif

// --------------------- gen_map_lexicon ------------------------

class gen_map_lexicon : public lexicon
{
protected:
public:
    gen_map_lexicon (const string &filename);
    ~gen_map_lexicon();
    virtual const int size() const {return lidxf.size();}
    virtual const char *id2str (int id) {
        if (id < 0)
            return "";
        off_t lex_off = lidxf [id];
        if (!lovff)
            return lexf.at (lex_off);
        for (int i = 0; i < lovff->size(); i++) {
            if (id < (signed) (*lovff)[i])
                break;
            lex_off += numeric_limits<lexpos>::max();
            lex_off += 1;
        }
        return lexf.at (lex_off);
    }
    virtual int str2id (const char *str);
    virtual Generator<int> *pref2ids (const char *str);
    virtual IdStrGenerator *pref2strids (const char *str);
protected:
    int srtidx2id (int sort_index_pos) {return (*lsrtf) [sort_index_pos];}
    MapBinFile<char> lexf;
    MapBinFile<lexpos> lidxf;
    MapBinFile<lexpos> *lovff;
    MapBinFile<lexpos> *lsrtf;
#if defined(LEXICON_USE_FSA3)
    fsa lexa;
    MapBinFile<lexpos> *lisrtf;
#endif
};

gen_map_lexicon::gen_map_lexicon (const string &filename)
    : lexf (filename + ".lex"), lidxf (filename + ".lex.idx"),
      lovff (NULL), lsrtf (new MapBinFile<lexpos>(filename + ".lex.srt"))
{
    try { lovff = new MapBinFile<lexpos> (filename + ".lex.ovf"); }
    catch (FileAccessError&) {}
#if defined(LEXICON_USE_FSA3)
    if (fsa_init (&lexa, (filename + ".fsa").c_str()))
        lisrtf = NULL;
    else
        lisrtf = new MapBinFile<lexpos> (filename + ".lex.isrt");
#endif
}

gen_map_lexicon::~gen_map_lexicon()
{
#if defined(LEXICON_USE_FSA3)
    if (lisrtf)
        fsa_destroy (&lexa);
#endif
    delete lovff;
    delete lsrtf;
#if defined(LEXICON_USE_FSA3)
    delete lisrtf;
#endif
}

int gen_map_lexicon::str2id (const char *str)
{
#if defined(LEXICON_USE_FSA3)
    if (lisrtf)
        return ::str2id (&lexa, lsrtf, str);
#endif
    int minid = -1;
    int maxid = size();
    int m, c, id;
    while (minid < maxid -1) {
        m = (minid + maxid) /2;
        c = strcmp (id2str (id = (*lsrtf) [m]), str);
        if (c == 0) {
            return id;
        } else if (c < 0) {
            minid = m;
        } else {
            maxid = m;
        }
    }
    return -1;
}

Generator<int>*
gen_map_lexicon::pref2ids (const char *str)
{
    if (!strlen(str))
        return new IdsIterator (lsrtf->at (0), size());
#if defined(LEXICON_USE_FSA3)
    if (lisrtf)
        return ::pref2ids (&lexa, lsrtf, str);
#endif
    int minid = -1;
    int maxid = size();
    int m, c, beg, end;
    int len = strlen (str);
    // pokud je size() == -1 (asi by nemelo nastat), musime
    //    inicializovat beg, end
    beg = 0;
    end = -1;
    while (minid < maxid -1) {
        m = (minid + maxid) /2;
        c = strncmp (str, id2str ((*lsrtf) [m]), len);
        if (c == 0) {
            beg = end = m;
            break;
        } else if (c > 0) {
            minid = m;
        } else {
            maxid = m;
        }
    }
    if (minid >= maxid -1)
        return new IdsIterator (lsrtf->at (0), 0);

    /* najdeme spodni hranici */
    while (minid < beg -1) {
        m = (minid + beg) /2;
        if (strncmp (str, id2str ((*lsrtf) [m]), len) == 0) {
            beg = m;
        } else {
            minid = m;
        }
    }
    /* najdeme horni hranici */
    while (end < maxid -1) {
        m = (end + maxid) /2;
        if (strncmp (str, id2str ((*lsrtf) [m]), len) == 0) {
            end = m;
        } else {
            maxid = m;
        }
    }
    return new IdsIterator (lsrtf->at (beg), end - beg +1);
}

IdStrGenerator*
gen_map_lexicon::pref2strids (const char *str)
{
#if defined(LEXICON_USE_FSA3)
    if (lisrtf)
        return new FSAPrefixGenerator(&lexa, lsrtf, str);
#endif
    return new AddStr<lexicon>(pref2ids(str), this);
}

// --------------------- fsa_lexicon ------------------------

#if defined(LEXICON_USE_FSA3)

class fsa_lexicon : public lexicon {
    fsa lexa;
    MapBinFile<lexpos> *lsrtf, *lisrtf;
public:
    fsa_lexicon (const string &path)  {
        if (fsa_init (&lexa, (path + ".fsa").c_str()))
            throw FileAccessError (path, "failed to open FSA lexicon");
        try {
            lsrtf = new MapBinFile<lexpos> (path + ".lex.srt");
        } catch (FileAccessError&) {lsrtf = NULL; errno = 0;}
        try {
            lisrtf = new MapBinFile<lexpos> (path + ".lex.isrt");
        } catch (FileAccessError&) {lisrtf = NULL; errno = 0;}
    }
    virtual ~fsa_lexicon() {fsa_destroy (&lexa); delete lsrtf; delete lisrtf;}
    virtual const int size() const {return lexa.count;}
    virtual const char* id2str (int id) {
        if (id < 0)
            return "";
        if (lisrtf)
            id = (*lisrtf)[id];
        return (const char*) find_word (&lexa, id);
    }
    virtual int str2id (const char *str) {return ::str2id (&lexa, lsrtf, str);}
    virtual Generator<int> *pref2ids (const char *str) {return ::pref2ids (&lexa, lsrtf, str);}
    virtual IdStrGenerator *pref2strids (const char *str) {return new FSAPrefixGenerator(&lexa, lsrtf, str);}
};

#endif

// --------------------- factory function ---------------------

lexicon *new_lexicon (const string &path)
{
    try {
        return new gen_map_lexicon (path);
    } catch (FileAccessError&) {
#if defined(LEXICON_USE_FSA3)
        return new fsa_lexicon (path);
#else
        throw;
#endif
    }
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
