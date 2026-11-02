#include "pabitset.hh"
#include "lexicon.hh"
#include "revidx.hh"
#include "text.hh"
#include "regexplex.hh"
#include <algorithm>
#include <memory>
#include <strings.h>

using namespace std;

//-------------------- BitsetPosAttr --------------------

class BitsetPosAttr : public PosAttr
{
    typedef map_list_text TextClass;
    typedef map_delta_revidx RevClass;

    lexicon *lex;
    TextClass txt;
    RevClass rev;
    const string sep;
    int empty_id;
    vector<int> ids;
    string strbuf;
    // reading position of the last random access, so that consecutive
    // positions (e.g. a full dump) do not seek into the segment again
    unique_ptr<TextClass::const_iterator> seq;
    Position seq_next;

    void read_set (Position pos, vector<int> &out) {
        if (!seq || pos != seq_next) {
            seq.reset (new TextClass::const_iterator (txt.at (pos)));
            seq_next = pos;
        }
        seq->next (out);
        seq_next++;
    }

    int representative (const vector<int> &set) {
        return set.empty() ? empty_id : set[0];
    }
    // values are joined in case-insensitive alphabetical order as in UD
    // FEATS (not in id order, which reflects the order of first occurrence),
    // copied as id2str() may reuse its buffer
    static bool value_less (const string &a, const string &b) {
        int c = strcasecmp (a.c_str(), b.c_str());
        return c ? c < 0 : a < b;
    }
    const char *join (const vector<int> &set, string &out) {
        vector<string> vals;
        vals.reserve (set.size());
        for (size_t i = 0; i < set.size(); i++)
            vals.push_back (lex->id2str (set[i]));
        sort (vals.begin(), vals.end(), value_less);
        out.clear();
        for (size_t i = 0; i < vals.size(); i++) {
            if (i)
                out += sep;
            out += vals[i];
        }
        return out.c_str();
    }

    class IDIter: public IDIterator {
        TextClass::const_iterator it;
        BitsetPosAttr *pa;
        vector<int> set;
    public:
        IDIter (const TextClass::const_iterator &i, BitsetPosAttr *pa)
            : it (i), pa (pa) {}
        virtual int next() {
            return it.next (set) ? pa->representative (set) : -1;
        }
    };
    class TextIter: public TextIterator {
        TextClass::const_iterator it;
        BitsetPosAttr *pa;
        vector<int> set;
        string buf;
    public:
        TextIter (const TextClass::const_iterator &i, BitsetPosAttr *pa)
            : it (i), pa (pa) {}
        virtual const char *next() {
            if (!it.next (set))
                return "";
            return pa->join (set, buf);
        }
    };
    // one (position, id) pair for every value; an empty set yields the id of
    // "" (as in the reverse index), or is skipped if "" is not in the lexicon
    class IDPosIter: public IDPosIterator {
        TextClass::const_iterator it;
        int empty_id;
        vector<int> set;
        size_t idx;
        Position pos;
        bool finished;
        void next_position() {
            idx = 0;
            do {
                if (!it.next (set)) {
                    finished = true;
                    return;
                }
                pos++;
                if (set.empty() && empty_id >= 0)
                    set.push_back (empty_id);
            } while (set.empty());
        }
    public:
        IDPosIter (const TextClass::const_iterator &i, int empty_id,
                   Position start)
            : it (i), empty_id (empty_id), idx (0), pos (start -1),
              finished (false)
            {next_position();}
        virtual void next() {
            if (!finished && ++idx >= set.size())
                next_position();
        }
        virtual Position peek_pos() {return pos;}
        virtual NumOfPos get_delta() {return 0;}
        virtual int peek_id() {return finished ? -1 : set[idx];}
        virtual bool end() {return finished;}
    };
public:
    BitsetPosAttr (const string &path, const string &n, const string &locale,
                   const string &encoding, const string &sep,
                   NumOfPos text_size=0)
        :PosAttr (path, n, locale, encoding), lex (new_lexicon (path)),
         txt (path, text_size), rev (path, txt.size()), sep (sep),
         empty_id (lex->str2id ("")), seq_next (0)
    {}
    virtual ~BitsetPosAttr () {delete lex;}

    virtual int id_range () {return lex->size();}
    virtual const char* id2str (int id) {return lex->id2str (id);}
    virtual int str2id (const char *str) {return lex->str2id (str);}
    virtual int pos2id (Position pos) {
        read_set (pos, ids);
        return representative (ids);
    }
    virtual void posset (Position pos, vector<int> &out) {
        read_set (pos, out);
    }
    virtual const char* pos2str (Position pos) {
        read_set (pos, ids);
        return join (ids, strbuf);
    }
    virtual IDIterator *posat (Position pos)
        {return new IDIter (txt.at (pos), this);}
    virtual IDPosIterator *idposat (Position pos)
        {return new IDPosIter (txt.at (pos), empty_id,
                               max (pos, Position (0)));}
    virtual TextIterator *textat (Position pos)
        {return new TextIter (txt.at (pos), this);}
    virtual FastStream *id2poss (int id) {return rev.id2poss (id);}
    virtual FastStream *compare2poss (const char *pat, int cmp, bool ignorecase)
        {return ::compare2poss (rev, lex, pat, cmp, ignorecase);}
    virtual FastStream *regexp2poss (const char *pat, bool ignorecase) {
        return ::regexp2poss (rev, lex, pat, locale, encoding, ignorecase);
    }
    virtual Generator<int> *regexp2ids (const char *pat, bool ignorecase,
                                        const char *filter_pat) {
        return ::regexp2ids (lex, pat, locale, encoding, ignorecase,
                             filter_pat);
    }
    virtual IdStrGenerator *regexp2strids (const char *pat, bool ignorecase,
                                           const char *filter_pat = NULL) {
        return ::regexp2strids (lex, pat, locale, encoding, ignorecase,
                                filter_pat);
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

PosAttr *createBitsetPosAttr (const string &path, const string &n,
                              const string &locale, const string &enc,
                              const string &sep, NumOfPos text_size)
{
    return new BitsetPosAttr (path, n, locale, enc, sep, text_size);
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
