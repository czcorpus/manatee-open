//  Copyright (c) 2012-2017  Pavel Rychly, Milos Jakubicek

#include "corpus.hh"
#include "virtcorp.hh"
#include <iostream>

using namespace std;


class VirtualRanges: public ranges
{
public:
    struct Trans {
        // transition from original positions to new positions,
        // the size of the included region is computed as a difference of
        // next newpos and current newpos (in the postrans array)
        Position orgpos;
        Position newpos;
        // transition of structure numbers
        NumOfPos orgnum;
        NumOfPos newnum;
        Trans () {}
        Trans (Position op, Position np, NumOfPos on, NumOfPos nn) 
            : orgpos(op), newpos(np), orgnum(on), newnum(nn) {}
    };

    struct Segment {
        ranges *src;
        vector<Trans> *postrans;
        Segment(): src(NULL), postrans(NULL) {}
        ~Segment() {
            delete postrans;
        }
    };
    vector<Segment> segs;
    Position finval;
    inline Position org2newpos (Position pos, size_t sg, size_t tr) {
        Trans &t  = (*segs[sg].postrans)[tr];
        return pos - t.orgpos + t.newpos;
    }
    inline Position new2orgpos (Position pos, size_t sg, size_t tr) {
        Trans &t  = (*segs[sg].postrans)[tr];
        return pos - t.newpos + t.orgpos;
    }
    inline NumOfPos org2newnum (NumOfPos num, size_t sg, size_t tr) {
        Trans &t  = (*segs[sg].postrans)[tr];
        return num - t.orgnum + t.newnum;
    }
    inline NumOfPos new2orgnum (NumOfPos num, size_t sg, size_t tr) {
        Trans &t  = (*segs[sg].postrans)[tr];
        return num - t.newnum + t.orgnum;
    }
    // original position inside the sg/tr transition
    inline bool intrans (Position pos, size_t sg, size_t tr) {
        Trans *t  = &(*segs[sg].postrans)[tr];
        return pos >= t->orgpos && pos < t->orgpos - t->newpos + (t+1)->newpos;
    }
public:
    VirtualRanges (VirtualCorpus *vc, const string &name)
        : segs (vc->segs.size()), finval (vc->size()), size_cached (-1)
    {
        NumOfPos lastnum = 0;
        for (size_t i = 0; i < segs.size(); i++) {
            Segment* s=&segs[i];
            if (vc->segs[i].corp->get_struct(name)->size())
                s->src = vc->segs[i].corp->get_struct(name)->rng;
            else {
                s->postrans = new vector<Trans>();
                continue; // empty segment
            }
            vector<VirtualCorpus::PosTrans> &pt = vc->segs[i].postrans;
            s->postrans = new vector<Trans>(pt.size());
            for (size_t j = 0; j < pt.size(); j++) {
                Trans &t = (*s->postrans)[j];
                t.orgpos = pt[j].orgpos;
                t.newpos = pt[j].newpos;
                if (j+1 < pt.size()) {
                    t.orgnum = s->src->num_next_pos(t.orgpos);
                    if (t.orgnum < 0)
                        t.orgnum = s->src->size();
                    NumOfPos tonum = s->src->num_next_pos(t.orgpos - t.newpos
                                                          + pt[j+1].newpos);
                    if (tonum < 0)
                        tonum = s->src->size();
                    t.newnum = lastnum;
                    lastnum += tonum - t.orgnum;
                } else {
                    // last transition
                    t.orgnum = 100000000000LL;
                    t.newnum = lastnum;
                }
            }
        }
    }
    virtual ~VirtualRanges() {}
protected:
    NumOfPos size_cached;
    Position locate_tran_pos (Position pos, unsigned &segnum, unsigned &trannum) {
        // locate transition containing newpos pos, returns original position
        // XXX don't start from 0 for big postrans vectors
        segnum = trannum = 0;
        while (segnum < segs.size() && 
                ( !segs[segnum].postrans->size()
                  || pos >= segs[segnum].postrans->back().newpos)
              )
            segnum++;
        if (segnum >= segs.size())
            return -1;
        while (trannum < segs[segnum].postrans->size() -1 &&
               pos >= (*segs[segnum].postrans)[trannum +1].newpos)
            trannum++;
        return new2orgpos (pos, segnum, trannum);
    }

    Position locate_tran_num (NumOfPos idx, unsigned &segnum, unsigned &trannum) {
        // locate transition containing newpos pos, returns original position
        // XXX don't start from 0 for big postrans vectors
        segnum = trannum = 0;
        while (segnum < segs.size() && 
                ( !segs[segnum].postrans->size()
                  || idx >= segs[segnum].postrans->back().newnum)
              )
            segnum++;
        if (segnum >= segs.size())
            return -1;
        while (trannum < segs[segnum].postrans->size() -1 &&
               idx >= (*segs[segnum].postrans)[trannum +1].newnum)
            trannum++;
        return new2orgnum (idx, segnum, trannum);
    }

    class WholeRStream: public RangeStream {
    protected:
        VirtualRanges *vr;
        unsigned sg, tr;
        RangeStream *curr;
        void locate () {
            do {
                if (curr->end() || tr >= vr->segs[sg].postrans->size()) {
                    delete curr;
                    while (++sg < vr->segs.size() && !vr->segs[sg].src)
                        ; // skip empty segments
                    if (sg >= vr->segs.size()) {
                        curr = NULL;
                        break;
                    }
                    curr = vr->segs[sg].src->whole();
                    tr = 0;
                    curr->find_beg((*vr->segs[sg].postrans)[tr].orgpos);
                } else if (! vr->intrans(curr->peek_beg(), sg, tr)) {
                    ++tr;
                    curr->find_beg((*vr->segs[sg].postrans)[tr].orgpos);
                }
            } while (! vr->intrans (curr->peek_beg(), sg, tr));
        }
    public:
        WholeRStream (VirtualRanges *vr) 
            : vr(vr), sg(0), tr(0) {
            while (!vr->segs[sg].src && ++sg < vr->segs.size())
                ; // skip empty segments
            if (sg >= vr->segs.size())
                curr = NULL;
            else {
                curr = vr->segs[sg].src->whole();
                locate();
            }
        }
        ~WholeRStream () {}
        virtual bool next () {
            if (! curr)
                return false;
            curr->next();
            locate();
            return curr != NULL;
        }
        virtual Position peek_beg () const {
            if (! curr)
                return vr->finval;
            return vr->org2newpos (curr->peek_beg(), sg, tr);
        }
        virtual Position peek_end () const {
            if (! curr)
                return vr->finval;
            return vr->org2newpos (curr->peek_end(), sg, tr);
        }
        virtual void add_labels (Labels &lab) const {curr->add_labels(lab);}
        virtual Position find_beg (Position pos) {
            if (! curr)
                return vr->finval;
            unsigned old_sg = sg;
            Position orgpos = vr->locate_tran_pos (pos, sg, tr);
            if (orgpos < 0) {
                curr = NULL;
                return vr->finval;
            }
            if (old_sg != sg) {
                delete curr;
                curr = vr->segs[sg].src->whole();
            }
            curr->find_beg (orgpos);
            locate();
            if (! curr)
                return vr->finval;
            return vr->org2newpos (curr->peek_beg(), sg, tr);
        }
        virtual Position find_end (Position pos) {
            if (! curr)
                return vr->finval;
            unsigned old_sg = sg;
            Position orgpos = vr->locate_tran_pos (pos, sg, tr);
            if (orgpos < 0) {
                curr = NULL;
                return vr->finval;
            }
            if (old_sg != sg) {
                delete curr;
                curr = vr->segs[sg].src->whole();
            }
            curr->find_end (orgpos);
            locate();
            if (! curr)
                return vr->finval;
            return vr->org2newpos (curr->peek_beg(), sg, tr);
        }
        virtual NumOfPos rest_min () const {return 0;}
        virtual NumOfPos rest_max () const {return curr->rest_max();} //XXX sum
        virtual Position final () const {return vr->finval;}
        virtual int nesting () const {return 0;} // nested structures not supported
        virtual bool epsilon () const {return false;}
    };

    class PartRStream: public RangeStream {
    protected:
        VirtualRanges *vr;
        unsigned sg, tr;
        FastStream *filter;
        Position filterlast;
        NumOfPos currnum;
        bool locate () {
            if (currnum >= filterlast)
                return false;
            if (currnum <= filter->peek())
                currnum = filter->peek();
            else
                currnum = filter->find (currnum);

            while (sg < vr->segs.size() && 
                   currnum >= vr->segs[sg].postrans->back().newnum)
                sg++;
            if (sg >= vr->segs.size())
                return false;
            while (tr < vr->segs[sg].postrans->size() -1 &&
                   currnum >= (*vr->segs[sg].postrans)[tr +1].newnum)
                tr++;

            return currnum < filterlast; 
        }
    public:
        PartRStream (VirtualRanges *vr, FastStream *filter)
            : vr (vr),  sg(0), tr(0), filter (filter), 
              filterlast (filter->final()), currnum (0)
        { locate(); }
        ~PartRStream () { delete filter; }
        virtual bool next () {
            filter->next();
            return locate();
        }
        virtual Position peek_beg () const {
            if (currnum >= filterlast)
                return vr->finval;
            return vr->org2newpos (vr->segs[sg].src->beg_at
                                   (vr->new2orgnum(currnum, sg, tr)),
                                   sg, tr);
        }
        virtual Position peek_end () const {
            if (currnum >= filterlast)
                return vr->finval;
            return vr->org2newpos (vr->segs[sg].src->end_at
                                   (vr->new2orgnum(currnum, sg, tr)),
                                   sg, tr);
        }
        // no labels
        virtual void add_labels (Labels &lab) const {}
        virtual Position find_beg (Position pos) {
            if (currnum >= filterlast)
                return vr->finval;
            Position p = vr->locate_tran_pos (pos, sg, tr);
            if (p < 0)
                return vr->finval;
            NumOfPos orgidx = vr->segs[sg].src->num_next_pos(p - 1);
            if (vr->org2newpos(vr->segs[sg].src->beg_at(orgidx), sg, tr) < pos)
                orgidx++;
            currnum = vr->org2newnum (orgidx, sg, tr);
            if (locate()) {
                orgidx = vr->new2orgnum (currnum, sg, tr);
                return vr->org2newpos (vr->segs[sg].src->beg_at(orgidx), 
                                       sg, tr);
            }
            return vr->finval;
        }
        virtual Position find_end (Position pos) {
            if (currnum >= filterlast)
                return vr->finval;
            Position p = vr->locate_tran_pos (pos, sg, tr);
            if (p < 0)
                return vr->finval;
            NumOfPos orgidx = vr->segs[sg].src->num_next_pos(p - 1);
            if (vr->org2newpos(vr->segs[sg].src->end_at(orgidx), sg, tr) < pos)
                orgidx++;
            currnum = vr->org2newnum (orgidx, sg, tr);
            if (locate()) {
                orgidx = vr->new2orgnum (currnum, sg, tr);
                return vr->org2newpos (vr->segs[sg].src->beg_at(orgidx), 
                                       sg, tr);
            }
            return vr->finval;
        }
        virtual NumOfPos rest_min () const {return 0;}
        virtual NumOfPos rest_max () const {return filterlast - currnum;}
        virtual Position final () const {return vr->finval;}
        virtual int nesting () const {return 0;} // nested structures not supported
        virtual bool epsilon () const {return false;}

    };

public:
    virtual NumOfPos size () {
        if (size_cached != -1)
            return size_cached;
        size_cached = 0;
        typename vector<Segment>::const_reverse_iterator it = segs.rbegin();
        for (; it != segs.rend(); ++it) {
            if ((*it).postrans->size()) {
                size_cached = (*it).postrans->back().newnum;
                break;
            }
        }
        return size_cached;
    }

    virtual Position beg_at (NumOfPos idx) {
        unsigned sg=0, tr=0;
        Position n = locate_tran_num (idx, sg, tr);
        if (n < 0)
            return finval;
        return segs[sg].src->beg_at(n) - (*segs[sg].postrans)[tr].orgpos
            + (*segs[sg].postrans)[tr].newpos; 
    }
    virtual Position end_at (NumOfPos idx) {
        unsigned sg=0, tr=0;
        Position n = locate_tran_num (idx, sg, tr);
        if (n < 0)
            return finval;
        return segs[sg].src->end_at(n) - (*segs[sg].postrans)[tr].orgpos
            + (*segs[sg].postrans)[tr].newpos;
    }
    virtual NumOfPos num_at_pos (Position pos) {
        unsigned sg=0, tr=0;
        Position p = locate_tran_pos (pos, sg, tr);
        if (p < 0)
            return -1;
        return segs[sg].src->num_at_pos(p) - (*segs[sg].postrans)[tr].orgnum
            + (*segs[sg].postrans)[tr].newnum;
    }
    virtual NumOfPos num_next_pos (Position pos) {
        unsigned sg=0, tr=0;
        Position p = locate_tran_pos (pos, sg, tr);
        if (p < 0)
            return size();
        Position x = segs[sg].src->num_next_pos(p) - (*segs[sg].postrans)[tr].orgnum
            + (*segs[sg].postrans)[tr].newnum;
        return x;
    }
    virtual RangeStream *whole() {
        return new WholeRStream (this);
    }
    virtual RangeStream *part(FastStream *p) {
        return new PartRStream (this, p);
    }
    virtual int nesting_at (NumOfPos idx) {
        // nested structures not supported
        return 0;
    }
};




ranges* setup_virtstructrng (VirtualCorpus *vc, const string &name)
{
    return new VirtualRanges (vc, name);
}

VirtualCorpus* virtcorp2virtstruc (VirtualCorpus *vc, const std::string &name)
{
    VirtualRanges* vr = new VirtualRanges(vc, name);
    VirtualCorpus* vs = new VirtualCorpus(false);

    VirtualCorpus::Segment *seg = NULL;
    VirtualCorpus::Segment empty_segment;
    for (size_t i = 0; i < vr->segs.size(); i++) {
        vs->segs.push_back(empty_segment);
        seg = &vs->segs.back();
        seg->corp = vc->segs[i].corp->get_struct(name);

        vector<VirtualRanges::Trans> &pt = *(vr->segs[i].postrans);
        for (size_t j = 0; j < pt.size(); j++) {
            seg->postrans.push_back(VirtualCorpus::PosTrans(pt[j].orgnum, 
                                                            pt[j].newnum));
        }
    }
    delete vr;
    return vs;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
