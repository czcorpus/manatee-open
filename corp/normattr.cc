//  Copyright (c) 2016-2020  Milos Jakubicek

#include "posattr.hh"
#include "revidx.hh"

class NormPosAttr : public PosAttr
{
    PosAttr *srcattr, *normattr;
    map_delta_revidx rev;
public:
    NormPosAttr (PosAttr *src, PosAttr *norm)
        : PosAttr (src->attr_path, src->name, src->locale, src->encoding),
          srcattr (src), normattr (norm), rev (attr_path + '@' + norm->name,
                                               norm->id_range())
    {
    }
    virtual ~NormPosAttr () {}

    virtual int id_range () {return srcattr->id_range();}
    virtual const char* id2str (int id) {return srcattr->id2str (id);}
    virtual int str2id(const char *str) {return srcattr->str2id (str);}
    virtual int pos2id (Position pos) {return srcattr->pos2id (pos);}
    virtual const char* pos2str (Position pos) {return srcattr->pos2str (pos);}
    virtual IDIterator *posat (Position pos) {return srcattr->posat (pos);}
    virtual IDPosIterator *idposat (Position pos) {return srcattr->idposat (pos);}
    virtual TextIterator *textat (Position pos) {return srcattr->textat (pos);}
    virtual FastStream *dynid2srcids (int id) {return rev.id2poss (id);}
    virtual Generator<int> *regexp2ids (const char *pat, bool ignorecase,
                                        const char *filter_pat)
        {return srcattr->regexp2ids (pat, ignorecase, filter_pat);}
    virtual IdStrGenerator *regexp2strids (const char *pat, bool ignorecase, const char *filter_pat)
        {return srcattr->regexp2strids (pat, ignorecase, filter_pat);}
    virtual NumOfPos size () {return srcattr->size();}
    virtual FastStream *id2poss (int id) {
        FastStream *fs = rev.id2poss (id);
        std::vector<FastStream*> *fsv = new std::vector<FastStream*>;
        while (fs->peek() < fs->final()) {
            int id = fs->next();
            FastStream *s = normattr->id2poss (id);
            fsv->push_back(s);
        }
        delete fs;
        return QOrVNode::create (fsv);
    }
    virtual FastStream *regexp2poss (const char *pat, bool ignorecase) {
        Generator<int> *gen = srcattr->regexp2ids (pat, ignorecase);
        std::vector<FastStream*> *fsv = new std::vector<FastStream*>;
        fsv->reserve (100);
        while (!gen->end())
            fsv->push_back (id2poss(gen->next()));
        delete gen;
        return QOrVNode::create (fsv);
    }
    virtual FastStream *compare2poss (const char *pat, int cmp, bool ignorecase)
        {NOTIMPLEMENTED}
    virtual IdStrGenerator *dump_str() {return srcattr->dump_str();}
    virtual Frequency* get_stat (const char *frqtype) {return srcattr->get_stat(frqtype);}
};

PosAttr *createNormPosAttr (PosAttr *src, PosAttr *norm) {
    return new NormPosAttr (src, norm);
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
