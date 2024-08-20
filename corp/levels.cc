//  Copyright (c) 2010-2023  Pavel Rychly, Milos Jakubicek
//  Multi Level Tokenization

#include "levels.hh"
#include "posattr.hh"
#include "frstream.hh"
#include "binfile.hh"
#include "bitio.hh"

using namespace std;


//==================== TokenLevel =========================
// one level data files
// provides streams for transforming from and to that level

class TokenLevel {
protected:
    struct IdxItem{
        int64_t orgp, newp, bits;
    };
    MapBinFile<IdxItem> idx;
    BinCachedFile<unsigned char> changes;
    Position levelsize;  // number of tokens on this level
    int nattrs; // number of attributes stored
public:
    TokenLevel (const string &filename):
        idx (filename + ".idx"), changes (filename),
        levelsize (idx[idx.size()-1].newp) // XXX, nattrs()
    {}

    friend class LevelPosAttr;

    class MLTS_FromFile: public MLTStream {
        TokenLevel *level;
        change_type_t ctype;
        NumOfPos csize;       // change size (#tokens in old)
        NumOfPos newsize;     // #tokens of change in new (n,1,1,m)
        vector<int> cvalue;   // concat values
        Position opos, npos;  // original position, new position
        int nextsegment;      // XXX staci int??
        typedef read_bits<BinCachedFile<unsigned char>::const_iterator> Bits;
        Bits *rb;
        void from_prevsegment () {
            int64_t seek = level->idx[nextsegment-1].bits;
            delete rb;
            rb = new Bits(level->changes.at(seek / 8), seek % 8);
            opos = level->idx[nextsegment-1].orgp;
            npos = level->idx[nextsegment-1].newp;
            csize = newsize = 0;
            if (nextsegment == level->idx.size())
                nextsegment = level->idx.size() -1;
            next();
        }
    public:
        MLTS_FromFile (TokenLevel *level) : level (level), rb (NULL) {
            reset();
        }
        MLTS_FromFile (TokenLevel *level, int segment)
            : level(level), csize(0), newsize(0), 
              opos(level->idx[segment].orgp), npos(level->idx[segment].newp), 
              nextsegment(segment+1),
              rb (new Bits(level->changes.at(level->idx[segment].bits / 8),
                  level->idx[segment].bits % 8)) {
            next();
        }
        ~MLTS_FromFile() {delete rb;}
        virtual change_type_t change_type() {return ctype;}
        virtual NumOfPos change_size() {return csize;}
        virtual NumOfPos change_newsize() {return newsize;}
        virtual int concat_value(int attrn) {return cvalue[attrn];}
        virtual Position orgpos() {return opos;}
        virtual Position newpos() {return npos;}
        virtual void reset() {
            csize = newsize = opos = npos = 0;
            nextsegment = 1;
            delete rb;
            rb = new Bits(level->changes.at(level->idx[0].bits / 8));
            next();
        }
        virtual bool end() {
            return npos >= level->levelsize;
        }
        virtual Position newfinal() {
            return level->levelsize;
        }
        virtual void next() {
            // read next change
            opos += csize;
            npos += newsize;
            if (npos >= level->levelsize)
                return;
            ctype = (MLTStream::change_type_t)rb->gamma();
            csize = rb->delta();
            switch (ctype) {
            case MLTStream::KEEP:
                newsize = csize;
                break;
            case MLTStream::CONCAT:
                newsize = 1;
                for (int a=0; a < level->nattrs; a++) {
                    int value = rb->gamma();
                    if (value > 0)
                        cvalue[a] = value -1;
                    else
                        cvalue[a] = - (rb->delta() -1);
                }
                break;
            case MLTStream::DELETE:
                newsize = 0;
                break;
            case MLTStream::INSERT:
                newsize = csize;
                csize = 0;
                break;
            case MLTStream::MORPH:
                newsize = rb->gamma();
                break;
            }
        }
        virtual Position find_org(Position pos) {
            if (opos >= pos)
                reset();
            if (level->idx[nextsegment].orgp <= pos) {
                // XXX binary search
                while (++nextsegment < level->idx.size()
                       && level->idx[nextsegment].orgp <= pos)
                    ; // increment only
                from_prevsegment();
            }
            while ((ctype == MLTStream::INSERT || opos + csize <= pos)
                   && npos < level->levelsize)
                next();
            return opos;
        }
        virtual Position find_new(Position pos) {
            if (npos >= pos)
                reset();
            if (level->idx[nextsegment].newp <= pos) {
                // XXX binary search
                while (++nextsegment < level->idx.size()
                       && level->idx[nextsegment].newp <= pos)
                    ; // increment only
                from_prevsegment();
            }
            while ((ctype == MLTStream::DELETE || npos + newsize <= pos)
                   && npos < level->levelsize)
                next();
            return npos;
        }
    };
protected:
    MLTStream *at_org (Position pos) {
        // XXX find segment directly
        MLTS_FromFile *s = new MLTS_FromFile(this);
        s->find_org(pos);
        return s;
    }
    MLTStream *at_new (Position pos) {
        // XXX find segment directly
        MLTS_FromFile *s = new MLTS_FromFile(this);
        s->find_new(pos);
        return s;
    }
public:
    MLTStream *full() {
        return new MLTS_FromFile(this);
    }
    PosAttr *level_attr(PosAttr *a);
    FastStream *tolevelfs(FastStream *fs);
    RangeStream *tobase (RangeStream *rs);
};

MLTStream *full_level (TokenLevel *level) {return level->full();}
TokenLevel *new_TokenLevel (const string &path) {return new TokenLevel (path);}
void delete_TokenLevel (TokenLevel *level) {delete level;}

class ToLevelFStream: public FastStream {
protected:
    TokenLevel *level;
    MLTStream *levels;
    FastStream *src;
    Position curr;

    void locate() {
        Position speek = src->peek();
        if (speek >= src->final() || levels->orgpos() >= src->final()) {
            curr = levels->newfinal();
            return;
        }
        Position changeend;
        levels->find_org(speek);
        while (!levels->end()) {
            switch (levels->change_type()) { 
            case MLTStream::KEEP:
                curr = speek - levels->orgpos() + levels->newpos();
                return;
            case MLTStream::DELETE:
                changeend = levels->orgpos() + levels->change_size();
                do {
                    src->next();
                    speek = src->peek();
                } while (speek < changeend && speek < src->final());
                if(speek >= src->final()) {
                    curr = levels->newfinal();
                    return;
                }
                levels->find_org(speek);
                break;
            case MLTStream::INSERT:
                levels->next();
                break;
            case MLTStream::MORPH:
                // MORPH: output all tokens from new range
                if (curr >= levels->newpos() + levels->change_newsize()) {
                    levels->next();
                    break;
                }
                if (curr < levels->newpos())
                    curr = levels->newpos();
                return;
            case MLTStream::CONCAT: // prevent warning
                break;
            }
        }
        curr = levels->newfinal();
    }
public:
    ToLevelFStream(MLTStream *ls, FastStream *src) 
        : level(NULL), levels(ls), src(src), curr(src->peek()) {locate();}
    ToLevelFStream(TokenLevel *lev, FastStream *src) 
        : level(lev), levels(lev->full()), src(src), curr(src->peek()) {
        locate();
    }
    virtual ~ToLevelFStream() {delete src; delete levels;}
    virtual void add_labels (Labels &lab) {src->add_labels(lab);}
    virtual Position peek() {return curr;}
    virtual Position next() {
        Position ret = curr;
        if (levels->change_type() != MLTStream::MORPH
            || ++curr >= levels->newpos() + levels->change_newsize())
            src->next();
        locate();
        return ret;
    }
    virtual Position find (Position pos) {
        if (pos > levels->newfinal())
            pos = levels->newfinal();
        levels->find_new (pos);
        if (levels->orgpos() < src->final()) {
            if (levels->change_type() == MLTStream::KEEP)
                src->find (pos - levels->newpos() + levels->orgpos());
            else
                src->find (levels->orgpos());
        }
        locate();
        if (curr < pos) // for the case when there are multiple hits in the same single segment
            curr = pos;
        return curr;
    }
    virtual NumOfPos rest_min() {return 0;}
    virtual NumOfPos rest_max() {return src->rest_max();}
    virtual Position final() {return levels->newfinal();}
};



FastStream *tolevelfs (TokenLevel *level, FastStream *src)
{
    return new ToLevelFStream (level, src);
}

//------------------- LevelPosAttr ------------------------

static const char *concat_from_ti (TextIterator *ti, int tokens, bool del=false)
{
    static string s = "";
    while (tokens > 0)
        s += ti->next();
    if (del)
        delete ti;
    return s.c_str();
}

class LevelPosAttr: public PosAttr {
    PosAttr *src;
    TokenLevel *level;
    int attrnum;
    
public:
    LevelPosAttr (PosAttr *src)
        : PosAttr (src->attr_path, src->name, src->locale, src->encoding),
          src(src) {}
    virtual ~LevelPosAttr () {}
protected:
    class TextIter: public TextIterator {
        MLTStream *mlts;
        TextIterator *src;
        LevelPosAttr *attr;
        NumOfPos toread; //how many positions to the end of current change
    public:
        TextIter (MLTStream *mlts, TextIterator *src, LevelPosAttr *attr) 
            : mlts(mlts), src(src), attr(attr), toread(0) {}
        virtual ~TextIter() {
            delete src;
            delete mlts;
        }
        virtual const char *next() {
            if (mlts->end())
                return "";
            while (!toread and !mlts->end()) {
                // next change
                toread = mlts->change_size();
                if (mlts->change_type() == MLTStream::DELETE) {
                    while (toread-- > 0)
                        src->next();
                    mlts->next();
                }
            }
            if (mlts->end())
                return "";
            if (mlts->change_type() == MLTStream::CONCAT) {
                toread = mlts->change_size();
                int val = mlts->concat_value(attr->attrnum);
                const char* ret = NULL;
                if (val > 0) {
                    for (; --val > 0; toread--)
                        src->next();
                    ret = src->next();
                }
                while (toread-- > 0)
                    src->next();
                mlts->next();
                if (ret)
                    return ret;
                return attr->id2str(mlts->concat_value(attr->attrnum));
            }
            // mlts->change_type() == MLTStream::KEEP                
            toread--;
            return src->next();
        }
    };
public:
    virtual int id_range () {return src->id_range();}
    virtual const char* id2str (int id) {return src->id2str(id);}
    virtual int str2id (const char *str) {return src->str2id(str);}
    virtual int pos2id (Position pos) {
        TokenLevel::MLTS_FromFile ls(level);
        ls.find_new(pos);
        if (ls.end())
            return -1;
        if (ls.change_type() == MLTStream::CONCAT) {
            int rel = ls.concat_value(attrnum);
            if (rel == 0)
                // concatenate
                return src->str2id(concat_from_ti (src->textat (ls.orgpos()),
                                                   ls.change_size(), true));
            else if (rel > 0)
                // selected token
                return src->pos2id(ls.orgpos() + rel -1);
            else // rel < 0
                // stored id
                return -rel;
        }
        // ls.change_type() == MLTStream::KEEP
        return src->pos2id(ls.orgpos() + (pos - ls.newpos()));
    }
    virtual const char* pos2str (Position pos) {
        TokenLevel::MLTS_FromFile ls(level);
        ls.find_new(pos);
        if (ls.end())
            return "";
        if (ls.change_type() == MLTStream::CONCAT) {
            int rel = ls.concat_value(attrnum);
            if (rel == 0)
                // concatenate
                return concat_from_ti (src->textat (ls.orgpos()),
                                       ls.change_size(), true);
            else if (rel > 0)
                // selected token
                return src->pos2str(ls.orgpos() + rel -1);
            else // rel < 0
                // stored id
                return src->id2str(-rel);
        }
        // ls.change_type() == MLTStream::KEEP
        return src->pos2str(ls.orgpos() + (pos - ls.newpos()));
    }
    virtual IDIterator *posat (Position pos) {NOTIMPLEMENTED}
    virtual TextIterator *textat (Position pos) {
        MLTStream *s = level->at_new(pos);
        return new TextIter (s, src->textat(s->orgpos()), this);
    }
    virtual FastStream *id2poss (int id) {
        return new ToLevelFStream (level->full(), src->id2poss(id));
    }
    virtual FastStream *regexp2poss (const char *pat, bool ignorecase) {
        return new ToLevelFStream (level->full(), src->regexp2poss(pat, ignorecase));
    }
    virtual FastStream *compare2poss (const char *pat, int cmp, bool icase) {
        return new ToLevelFStream (level->full(), src->compare2poss(pat, cmp, icase));
    }
    virtual Generator<int> *regexp2ids (const char *pat, bool ignorecase, 
                                        const char *filter_pat=NULL) {
        return src->regexp2ids (pat, ignorecase, filter_pat);
    }
    virtual NumOfPos size() {return level->levelsize;}
    virtual Frequency* get_stat(const char *frqtype) {return src->get_stat(frqtype);}
};

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
