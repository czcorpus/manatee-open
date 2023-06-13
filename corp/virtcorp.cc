//  Copyright (c) 2004-2021  Pavel Rychly, Milos Jakubicek

#include "corpus.hh"
#include "virtcorp.hh"
#include "lexicon.hh"
#include "regexplex.hh"
#include "log.hh"
#include <fstream>
#include <iostream>

using namespace std;



VirtualCorpus* setup_virtcorp (const string &filename)
{
    VirtualCorpus* vc = new VirtualCorpus();
    VirtualCorpus::Segment *seg = NULL;
    VirtualCorpus::Segment empty_segment;
    Position lastpos = 0;
    ifstream in_file (filename.c_str());
    if (in_file.fail())
        throw FileAccessError (filename, ": could not open file");
    string line;
    while (getline (in_file, line)) {
	if (line.length() == 0 || line[0] == '#')
	    continue;
	if (line[0] == '=') {
	    // new corpus -> new segment
            if (lastpos)
                // close previous segment
                seg->postrans.push_back(VirtualCorpus::PosTrans(100000000000LL,
                                                                lastpos));
            vc->segs.push_back(empty_segment);
            seg = &vc->segs.back();
            line.erase(0,1);
            seg->corp = new Corpus (line);
        } else {
            if (! seg) {
                CERR << filename << ": transition without corpus:" << line << '\n';
                continue;
            }
            string::size_type cidx = line.find(',');
	    if (cidx == line.npos) {
                CERR << filename << ": expecting `,': " << line << '\n';
                continue;
            }
            Position fromp = atoll (string (line, 0, cidx).c_str());
            string to (line, cidx +1);
            Position top;
            NumOfPos size = seg->corp->size();
            if (to == "$")
                top = size;
            else {
                top = atoll (to.c_str());
                if (top > size) {
                    CERR << filename << ": transition exceeds corpus size: "
                         << line << "\n-- using corpus size (" << size
                         << ") instead.\n";
                    top = size;
                }
            }
            if (fromp >= top) {
                CERR << filename << ": empty transition: " << line << '\n';
                continue;
            }
            seg->postrans.push_back(VirtualCorpus::PosTrans(fromp, lastpos));
            lastpos += top - fromp;
        }
    }
    if (lastpos)
        // close last segment
        seg->postrans.push_back(VirtualCorpus::PosTrans(100000000000LL, 
                                                        lastpos));
    else
        CERR << filename << ": empty virtual corpus\n";
    return vc;
}

VirtualCorpus::~VirtualCorpus() {
    if (ownes_corpora) {
        for(vector<VirtualCorpus::Segment>::iterator it = segs.begin();
               it != segs.end(); ++it)
           delete it->corp;
    }
}

static const char* n2str (int num, const char *suffix)
{
    static char str[16];
    sprintf (str, ".seg%i%s", num, suffix);
    return str;
}

class SingleValueFrequency : public Frequency {
    string name;
    NumOfPos default_size;
public:
    SingleValueFrequency(WordList *w, const char *n, NumOfPos size):
        name (n), default_size (size) {}
    double freq (int id) {return id == 0 ? default_size : 0;}
};

class SingleValueAttr : public PosAttr
{
    static int id;
    vector<string> def;
    NumOfPos corpsize;
    NumOfPos docsize;
    FastStream *full_fs() {return new SequenceStream (0, corpsize-1, corpsize);}
    Generator<int> *full_gen() {return new SequenceGenerator<int> (0, corpsize-1);}
public:
    SingleValueAttr (const string &path, const string &name,
                     const string &locale, const string &enc,
                     const string &defvalue, NumOfPos size,
                     NumOfPos docsize) :
        PosAttr(path, name, locale, enc), def (1, defvalue), corpsize (size),
        docsize (docsize) {}
    virtual int id_range () {return 1;}
    virtual const char* id2str (int id) {return id == 0 ? def[0].c_str() : "";}
    virtual int str2id (const char *str) {return !strcmp(str, def[0].c_str()) ? 0 : -1;}
    virtual int pos2id (Position pos) {return 0;}
    virtual const char* pos2str (Position pos) {return def[0].c_str();}
    virtual IDIterator *posat (Position pos) {return new DummyIDIter();}
    virtual IDPosIterator *idposat (Position pos) {return new DummyIDPosIter (full_fs());}
    virtual TextIterator *textat (Position pos) {return new DummyTextIter (def[0]);}
    virtual FastStream *id2poss (int id) {return id == 0 ? full_fs() : new EmptyStream();}
    virtual FastStream *regexp2poss (const char *pat, bool ignorecase) {
        regexp_pattern re (pat, locale, encoding, ignorecase);
        if (re.compile())
            return new EmptyStream();
        if (re.match(def[0].c_str()))
            return full_fs();
        return new EmptyStream();
    }
    virtual FastStream *compare2poss (const char *pat, int order, bool ignorecase) {
        int cmp = strverscmp(def[0].c_str(), pat);
        if ((order < 0 && cmp <= 0) || (order > 0 && cmp >= 0))
            return full_fs();
        return new EmptyStream();
    }
    virtual Generator<int> *regexp2ids (const char *pat, bool ignorecase,
                                        const char *filter_pat = NULL) {
        regexp_pattern re (pat, locale, encoding, ignorecase);
        if (re.compile())
            return new EmptyGenerator<int>();
        if (re.match(def[0].c_str())) {
            if (filter_pat) {
                regexp_pattern re2 (filter_pat, locale, encoding, ignorecase);
                if (re2.compile())
                    return full_gen();
                if (re2.match(def[0].c_str()))
                    return new EmptyGenerator<int>();
            }
            return full_gen();
        }
        return new EmptyGenerator<int>();
    }
    virtual IdStrGenerator *regexp2strids (const char *pat, bool ignorecase, const char *filter_pat = NULL) {
        return new AddStr<SingleValueAttr>(regexp2ids (pat, ignorecase, filter_pat), this);
    }
    virtual NumOfPos size() {return corpsize;}
    virtual IdStrGenerator *dump_str() {return new ArrayIdStrGenerator(&def, &SingleValueAttr::id);}
    virtual Frequency *get_stat (const char *frqtype) {
        return new SingleValueFrequency(this, frqtype, (!strcmp(frqtype, "docf") ? docsize : corpsize));
    }
};
int SingleValueAttr::id = 0;

class CombineFS: public FastStream {
    std::vector<const std::vector<VirtualCorpus::PosTrans>*> pt;
    PosAttr *pa;
    vector<FastStream*> srcs; // for each segment one FastStream*
    size_t sg, tr;
    NumOfPos currdelta;
    Position tranend;
    void locate () {
        Position pe, org, trsize;
        while (sg < srcs.size()) {
            pe = srcs[sg]->peek();
            //if (pe < tranend)
            //    break;
            if (tr >= pt[sg]->size() -1 // transition end
                || pe >= srcs[sg]->final()) { // empty stream
                // forward to next segment
                sg++;
                tr = 0;
                tranend = -1;
            } else if (pe >= (*pt[sg])[tr+1].orgpos)
                // forward to next transition
                tr++;
            else if (pe < (org=(*pt[sg])[tr].orgpos))
                // advance source FastStream
                srcs[sg]->find(org);
            else if (pe >= ((trsize=(*pt[sg])[tr+1].newpos
                             - (*pt[sg])[tr].newpos)
                            + org))
                // forward to next transition
                tr++;
            else {
                // peek is inside current transition
                currdelta = (*pt[sg])[tr].newpos - org;
                tranend = org + trsize;
                break;
            }
        }
    }

    Position locate_tran (Position pos, size_t &segnum, size_t &trannum) {
        // locate transition containing newpos pos, returns original position
        // XXX don't start from 0 for big postrans vectors
        segnum = trannum = 0;
        while (segnum < pt.size() &&
               pos >= pt[segnum]->back().newpos)
            segnum++;
        if (segnum >= pt.size())
            return -1;
        while (trannum < pt[segnum]->size() -1 &&
               pos >= (*pt[segnum])[trannum +1].newpos)
            trannum++;
        return pos - (*pt[segnum])[trannum].newpos
            + (*pt[segnum])[trannum].orgpos;
    }

public:
    CombineFS (std::vector<const std::vector<VirtualCorpus::PosTrans>*> pt,
             PosAttr *pa, const vector<FastStream*> &src) 
        : pt (pt), pa (pa), srcs (src), sg (0), tr (0), tranend(-1) {
        locate();
    }
    ~CombineFS () {
        for (sg=0; sg < srcs.size(); sg++)
            delete srcs[sg];
    }
    virtual Position peek() {
        if (sg >= srcs.size())
            return pa->size();
        return srcs[sg]->peek() + currdelta;
    }
    virtual Position next() {
        if (sg >= srcs.size())
            return pa->size();
        Position ret = srcs[sg]->next() + currdelta;
        locate();
        return ret;
    }
    virtual Position find (Position pos) {
        if (sg >= srcs.size())
            return pa->size();
        Position org = locate_tran(pos, sg, tr);
        if (org < 0)
            return pa->size();
        srcs[sg]->find(org);
        locate();
        if (sg >= srcs.size())
            return pa->size();
        return srcs[sg]->peek() + currdelta;
    }
    virtual NumOfPos rest_min() {
        NumOfPos ret = 0;
        for (size_t i = sg; i < srcs.size(); i++)
            ret += srcs[sg]->rest_min();
        return ret;
    }
    virtual NumOfPos rest_max() {
        NumOfPos ret = 0;
        for (size_t i = sg; i < srcs.size(); i++)
            ret += srcs[sg]->rest_max();
        return ret;
    }
    virtual Position final() {
        return pa->size();
    }
    virtual void add_labels (Labels &lab) {
        Labels tmplab;
        for (size_t i = sg; i < srcs.size(); i++) {
            tmplab.clear();
            srcs[sg]->add_labels (tmplab);
            for (Labels::const_iterator it = tmplab.begin();
                 it != tmplab.end(); it++)
                lab.insert (make_pair (it->first, it->second + currdelta));
        }
    }
};

FastStream* VirtualCorpus::combine_poss (PosAttr *pa, vector<FastStream*> &fsv)
{
    std::vector<const std::vector<VirtualCorpus::PosTrans>*> pt;
    pt.reserve(segs.size());
    for(size_t i = 0; i < segs.size(); i++)
        pt.push_back(&segs[i].postrans);
    return new CombineFS (pt, pa, fsv);
}

template <class NormClass, class FreqClass, class FloatFreqClass>
class VirtualPosAttr : public PosAttr
{
protected:
    struct Segment {
        PosAttr *src;
        MapBinFile<int> *toorgid, *tonewid;
        const vector<VirtualCorpus::PosTrans> *postrans;
        bool ownedByCorpus;
        Segment(): src(NULL), toorgid(NULL), tonewid(NULL),
                   postrans(NULL), ownedByCorpus (false) {}
        ~Segment() {
            if (!ownedByCorpus) {
                delete src;
            }
            delete toorgid;
            delete tonewid;
        }
    };
    lexicon *lex;
    vector<Segment> segs;
    friend class VirtualCorpus;
    friend class IDIter;
    Frequency *frqf;

    FastStream *combine_poss (vector<FastStream*> &fsv) {
        std::vector<const std::vector<VirtualCorpus::PosTrans>*> pt;
        pt.reserve(segs.size());
        for(size_t i = 0; i < segs.size(); i++)
            pt.push_back(segs[i].postrans);
        return new CombineFS(pt, this, fsv);
    }
public:
    VirtualPosAttr (const string &path, const string &name, 
                    const vector<VirtualCorpus::Segment> &corpsegs,
                    const string &locale, const string &enc,
                    bool ownedByCorpus = true, const string &def="",
                    const string &doc="")
        : PosAttr (path, name, locale, enc), lex (new_lexicon (path)),
          segs(corpsegs.size()), frqf (get_stat("frq"))
    {
        for (size_t i = 0; i < segs.size(); i++) {
            Segment* s=&segs[i];
            try {
                s->src = corpsegs[i].corp->get_attr(name);
                s->ownedByCorpus = ownedByCorpus;
            } catch (AttrNotFound&) {
                NumOfPos docsize = 0;
                try {
                    docsize = corpsegs[i].corp->get_struct(doc)->size();
                } catch (AttrNotFound&) {}
                s->src = new SingleValueAttr(path, name, locale, enc, def,
                                             corpsegs[i].corp->size(), docsize);
                s->ownedByCorpus = false;
            }
            s->toorgid = new MapBinFile<int32_t>(path + n2str(i, ".oid"));
            s->tonewid = new MapBinFile<int32_t>(path + n2str(i, ".nid"));
            s->postrans = &corpsegs[i].postrans;
        }
    }
    virtual ~VirtualPosAttr() {delete lex; delete frqf;}
protected:
    NumOfPos size_cached = -1;
    Position locate_tran (Position pos, size_t &segnum, size_t &trannum) {
        // locate transition containing newpos pos, returns original position
        // XXX don't start from 0 for big postrans vectors
        segnum = trannum = 0;
        while (segnum < segs.size() &&
               (segs[segnum].postrans->size() == 0 ||
               pos >= segs[segnum].postrans->back().newpos))
            segnum++;
        if (segnum >= segs.size())
            return -1;
        while (trannum < segs[segnum].postrans->size() -1 &&
               pos >= (*segs[segnum].postrans)[trannum +1].newpos)
            trannum++;
        return pos - (*segs[segnum].postrans)[trannum].newpos
            + (*segs[segnum].postrans)[trannum].orgpos;
    }
    class IDIter: public IDIterator {
        VirtualPosAttr *vpa;
        IDIterator *curr;
        NumOfPos toread; //how many positions to the end of current transition
        size_t sg, tr;
    public:
        IDIter (VirtualPosAttr *vpa, Position pos) 
            : vpa (vpa), curr(NULL), toread(0) {
            Position org = vpa->locate_tran (pos, sg, tr);
            if (org >= 0) {
                curr = vpa->segs[sg].src->posat (org);
                toread = (*vpa->segs[sg].postrans)[tr+1].newpos - pos;
            }
        }
        virtual ~IDIter() {
            if (curr) delete curr;
        }
        virtual int next() {
            if (!curr) return -1;
            if (!toread) {
                // next transition
                delete curr;
                if (++tr == vpa->segs[sg].postrans->size() -1) {
                    // next segment
                    do {
                        sg++;
                    } while (sg < vpa->segs.size() && vpa->segs[sg].postrans->size() == 0);
                    if (sg == vpa->segs.size()) {
                        // end
                        curr = 0;
                        return -1;
                    }
                    tr = 0;
                }
                curr = vpa->segs[sg].src->posat 
                    ((*vpa->segs[sg].postrans)[tr].orgpos);
                toread = (*vpa->segs[sg].postrans)[tr+1].newpos
                    - (*vpa->segs[sg].postrans)[tr].newpos;
            }
            toread--;
            return (*vpa->segs[sg].tonewid)[curr->next()];
        }
    };
    class TextIter: public TextIterator {
        VirtualPosAttr *vpa;
        TextIterator *curr;
        NumOfPos toread; //how many positions to the end of current transition
        size_t sg, tr;
    public:
        TextIter (VirtualPosAttr *vpa, Position pos) 
            : vpa (vpa), curr(NULL), toread(0) {
            Position org = vpa->locate_tran (pos, sg, tr);
            if (org >= 0) {
                curr = vpa->segs[sg].src->textat (org);
                toread = (*vpa->segs[sg].postrans)[tr+1].newpos - pos;
            }
        }
        virtual ~TextIter() {
            if (curr) delete curr;
        }
        virtual const char *next() {
            if (!curr) return "";
            if (!toread) {
                // next transition
                delete curr;
                if (++tr == vpa->segs[sg].postrans->size() -1) {
                    // next segment
                    if (++sg == vpa->segs.size()) {
                        // end
                        curr = NULL;
                        return "";
                    }
                    tr = 0;
                }
                curr = vpa->segs[sg].src->textat 
                    ((*vpa->segs[sg].postrans)[tr].orgpos);
                toread = (*vpa->segs[sg].postrans)[tr+1].newpos
                    - (*vpa->segs[sg].postrans)[tr].newpos;
            }
            toread--;
            return curr->next();
        }
    };
public:
    virtual int id_range () {return lex->size();}
    virtual const char* id2str (int id) {return lex->id2str (id);}
    virtual int str2id (const char *str) {return lex->str2id (str);}
    virtual int pos2id (Position pos) {
        size_t sg, tr;
        Position orgpos = locate_tran (pos, sg, tr);
        if (orgpos < 0)
            return -1;
        return (*segs[sg].tonewid)[segs[sg].src->pos2id (orgpos)];
    }
    virtual const char* pos2str (Position pos) {
        size_t sg, tr;
        Position orgpos = locate_tran (pos, sg, tr);
        if (orgpos < 0)
            return "";
        return segs[sg].src->pos2str (orgpos);
    }
    virtual IDIterator *posat (Position pos) {return new IDIter (this, pos);}
    virtual IDPosIterator *idposat (Position pos)
        {return new IDPosIterator (new IDIter (this, pos), size());}
    virtual TextIterator *textat (Position pos) {return new TextIter (this, pos);}
    virtual FastStream *id2poss (int id) {
        vector<FastStream*> fss;
        for (size_t s = 0; s < segs.size(); s++)
            fss.push_back(segs[s].src->id2poss((*segs[s].toorgid)[id]));
        return combine_poss(fss);
    }
    virtual FastStream *compare2poss (const char *pat, int cmp, bool icase) {
        vector<FastStream*> fss;
        for (size_t s = 0; s < segs.size(); s++)
            fss.push_back(segs[s].src->compare2poss(pat, cmp, icase));
        return combine_poss(fss);
    }
    virtual FastStream *regexp2poss (const char *pat, bool icase) {
        vector<FastStream*> fss;
        for (size_t s = 0; s < segs.size(); s++)
            fss.push_back(segs[s].src->regexp2poss(pat, icase));
        return combine_poss(fss);
    }
    virtual Generator<int> *regexp2ids (const char *pat, bool ignorecase, 
                                        const char *filter) 
        {return ::regexp2ids (lex, pat, locale, encoding, ignorecase, filter);}
    virtual IdStrGenerator *regexp2strids (const char *pat, bool ignorecase, const char *filter_pat)
        {return ::regexp2strids (lex, pat, locale, encoding, ignorecase, filter_pat);}
    virtual NumOfPos freq (int id) {
        if (id < 0) return 0;
        return frqf->freq (id);
    }
    virtual Frequency* get_stat (const char *frqtype) {
        if (!strcmp(frqtype, "frq")) return WordList::get_stat("frq:l");
        return WordList::get_stat(frqtype);
    }
    virtual NumOfPos size () {
        if (size_cached != -1)
            return size_cached;
        size_cached = 0;
        typename vector<Segment>::const_reverse_iterator it = segs.rbegin();
        for (; it != segs.rend(); ++it) {
            if ((*it).postrans->size()) {
                size_cached = (*it).postrans->back().newpos;
                break;
            }
        }
        return size_cached;
    }
    virtual IdStrGenerator *dump_str() {return lex->pref2strids("");}
};



PosAttr* setup_virtposattr (VirtualCorpus *vc, const string &path,
                            const string &name, const std::string &loc,
                            const std::string &enc, bool ownedByCorpus,
                            const std::string &def, const std::string &doc)
{
    return new VirtualPosAttr<> (path, name, vc->segs, loc, enc, ownedByCorpus, def, doc);
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
