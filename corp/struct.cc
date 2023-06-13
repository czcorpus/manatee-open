//  Copyright (c) 1999-2021  Pavel Rychly, Milos Jakubicek

#include "corpus.hh"
using namespace std;

Structure::Structure (CorpInfo *i, const string &path, const string &n, Corpus *c)
    : Corpus (i, Struct_type), 
      corpus (c), rng (create_ranges(path + ".rng", i->opts["TYPE"])), name (n),
    endtagstring ("</" + name + '>')
{
}

Structure::Structure (CorpInfo *i, const string &n, VirtualCorpus *vc)
    : Corpus (i, Struct_type), corpus (NULL), rng (setup_virtstructrng (vc, n)),
      name (n), endtagstring ("</" + n + '>')
{
    virt = virtcorp2virtstruc (vc, n);
}

class StructNums: public FastStream {
protected:
    ranges *rng;
    RangeStream *src;
    Position currnum;
    Position blockend;
    NumOfPos finval;
public:
    StructNums (ranges *rngi, RangeStream *srci)
        : rng(rngi), src(srci), currnum(-1), finval(rng->size()) {find(0);}
    virtual ~StructNums() {delete src;}
    virtual Position peek() {return currnum;}
    virtual Position next() {
        Position ret = currnum;
        if (currnum < finval && ++currnum > blockend)
            find (currnum);
        return ret;
    }
    virtual Position find (Position pos) {
        if (currnum == finval || src->end() || pos >= rng->size()) {
            currnum = finval;
            blockend = finval;
        } else {
            src->find_beg(rng->beg_at(pos));
            Position s = rng->num_next_pos(src->peek_beg());
            if (currnum < s)
                currnum = s;
            blockend = rng->num_next_pos(src->peek_end() -1);
        }
        return currnum;
    }
    virtual NumOfPos rest_min() {return src->rest_min();}
    virtual NumOfPos rest_max() {return src->rest_max();}
    virtual Position final() {return finval;}
    virtual void add_labels(Labels &lab) {src->add_labels (lab);}
};

FastStream *Structure::part_nums (RangeStream *r) {return new StructNums(rng, r);}

class StructPosAttr : public PosAttr
{
    Structure *st;
    PosAttr *pa;
    Position lastrng;
    bool nested;
    char multisep;
    NumOfPos corpsize;
    Position locate_rng (Position pos) {
        if (!nested && // no cache for nested structures
            st->rng->beg_at (lastrng) <= pos && st->rng->end_at (lastrng) > pos)
            return lastrng;
        else {
            Position n = st->rng->num_at_pos (pos);
            if (n >= 0)
                lastrng = n;
            return n;
        }
    }
public:
    class IDIter : public IDIterator {
        RangeStream *rs;
        PosAttr *pa;
        Position pos;
        Position cur_beg, cur_end, rs_pos, rs_id;
    public:
        IDIter(Structure *st, PosAttr *pa, Position from)
            : rs(st->rng->whole()), pa(pa), pos(from),
              cur_beg(rs->peek_beg()), cur_end(rs->peek_end()), rs_pos(0) {
            if(CorpInfo::str2bool(st->get_conf("NESTED"))) NOTIMPLEMENTED;
        }
        virtual ~IDIter() {delete rs;}
        virtual int next() {
            int ret = -1;
            if(rs->end()) return -1;

            while(pos >= rs->peek_end()) {
                rs_pos++;
                if(!rs->next()) return -1;
            }

            if(pos >= rs->peek_beg()) ret = rs_pos;
            pos++;
            if(ret > -1) return pa->pos2id(ret);
            else return -1;
        }
    };

    StructPosAttr (Structure *st, PosAttr *pa, NumOfPos corpsize,
                   const std::string &fpath)
        : PosAttr (pa->attr_path, (st->name + '.' + pa->name), pa->locale, pa->encoding, fpath),
          st (st), pa(pa), lastrng (0),
          nested (CorpInfo::str2bool(st->get_conf("NESTED"))),
          multisep (st->conf->find_attr(pa->name)["MULTISEP"][0]),
          corpsize(corpsize)
    {}
    virtual ~StructPosAttr () {}
    virtual int id_range () {return pa->id_range();}
    virtual const char* id2str (int id) {return pa->id2str (id);}
    virtual int str2id(const char *str) {return pa->str2id (str);}
    virtual int pos2id (Position pos) {
        Position n = locate_rng (pos);
        return (n == -1) ? -1 : pa->pos2id (n);
    }
    virtual const char* pos2str (Position pos) {
        Position n = locate_rng (pos);
        if (n == -1)
            return "";
        if (!nested || !st->rng->nesting_at(n))
            return pa->pos2str (n);
        while (st->rng->nesting_at(n))
            n--;
        static string ret;
        ret.clear();
        while (n < st->rng->size()
               && (st->rng->nesting_at(n) || pos >= st->rng->beg_at(n))) {
            if (pos >= st->rng->beg_at(n) && pos < st->rng->end_at(n)) {
                ret += pa->pos2str (n);
                ret += multisep;
            }
            n++;
        }
        if (!ret.empty())
            ret.replace (ret.length() -1, 1, "");
        return ret.c_str();
    }
    virtual IDIterator *posat (Position pos) {return new IDIter(st, pa, pos);}
    virtual IDPosIterator *idposat (Position pos)
        {return new IDPosIterator(new IDIter(st, pa, pos), size());}
    virtual TextIterator *textat (Position pos) {NOTIMPLEMENTED}
    virtual FastStream *id2poss (int id) {NOTIMPLEMENTED}
    virtual FastStream *dynid2srcids (int id) {return pa->dynid2srcids(id);}
    virtual FastStream *regexp2poss (const char *pat, bool) {NOTIMPLEMENTED}
    virtual FastStream *compare2poss (const char *pat, int, bool) {NOTIMPLEMENTED}
    virtual Generator<int> *regexp2ids (const char *pat, bool ignorecase, const char *filter_pat)
        {return pa->regexp2ids (pat,ignorecase,filter_pat);}
    virtual IdStrGenerator *regexp2strids (const char *pat, bool ignorecase, const char *filter_pat = NULL)
        {return pa->regexp2strids (pat,ignorecase,filter_pat);}
    virtual NumOfPos size () {return corpsize;}
    virtual IdStrGenerator *dump_str() {return pa->dump_str();}
    virtual Frequency* get_stat (const char *frqtype) {
        if (freq_path == pa->attr_path)
            return pa->get_stat(frqtype);
        else return WordList::get_stat(frqtype);
    }
};



PosAttr* Corpus::get_struct_pos_attr (const std::string &strname, 
                                      const std::string &attname)
{
    Structure* s = get_struct (strname);
    PosAttr* a = s->get_attr (attname);
    std::string fpath = "";
    if (!conf->opts ["SUBCPATH"].empty()) {
        string path = conf->opts ["PATH"];
        size_t slash = path.rfind('/');
        if (slash != string::npos && slash != path.length() - 1)
            fpath += path.substr (slash + 1);
        fpath = conf->opts ["SUBCPATH"] + strname + "." + attname;
    }
    StructPosAttr *spa = new StructPosAttr (s, a, size(), fpath);
    attrs.push_back (pair<string,PosAttr*> (strname + "." + attname, spa));
    return spa;
}


/*
XXX nesting musi byt ulozeno v .rng a tady se musi predavat
    pro nesting > 0 tam musi byt i final (max end)
RangeStream *Structure::whole()
{
    return new whole_range (&rng);
}


RangeStream *Structure::part(FastStream *filter)
{
    return new part_range (&rng, filter);
}

*/

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:

