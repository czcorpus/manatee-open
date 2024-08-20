//  Copyright (c) 2013  Pavel Rychly

//#include "log.hh"
//#include "config.hh"
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
            delete src;
        }
    };
    vector<Segment> segs;
    Position finval;
public:
    VirtualRanges (VirtualCorpus *vc, const string &name)
        : segs (vc->segs.size()), finval (vc->size())
    {
        NumOfPos lastnum = 0;
        for (size_t i = 0; i < segs.size(); i++) {
            Segment* s=&segs[i];
            s->src = vc->segs[i].corp->get_struct(name)->rng;
            vector<VirtualCorpus::PosTrans> &pt = vc->segs[i].postrans;
            s->postrans = new vector<Trans>(pt.size());
            for (size_t j = 0; j < pt.size(); j++) {
                Trans &t = (*s->postrans)[j];
                t.orgpos = pt[j].orgpos;
                t.newpos = pt[j].newpos;
                if (j+1 < pt.size()) {
                    t.orgnum = s->src->num_at_pos(t.orgpos);
                    NumOfPos tonum = s->src->num_at_pos(t.orgpos - t.newpos
                                                        + pt[j+1].newpos);
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
    virtual NumOfPos size () {NOTIMPLEMENTED};
    virtual Position beg_at (NumOfPos idx) {NOTIMPLEMENTED};
    virtual Position end_at (NumOfPos idx) {NOTIMPLEMENTED};
    virtual NumOfPos num_at_pos (Position pos) {NOTIMPLEMENTED};
    virtual NumOfPos num_next_pos (Position pos) {NOTIMPLEMENTED};
    virtual RangeStream *whole() {NOTIMPLEMENTED};
    virtual RangeStream *part(FastStream *p) {NOTIMPLEMENTED};
    virtual int nesting_at (NumOfPos idx) {NOTIMPLEMENTED};

};

int main (int argc, const char **argv)
{
    if (argc < 2 || argc > 3) {
         cerr << "usage: dumpvirt VIRTDEF [STRUCT]\n";
        return 1;
    }
    try {
        VirtualCorpus *vc = setup_virtcorp (argv[1]);
        for (unsigned int i = 0; i < vc->segs.size(); i++) {
            cout << "Corpus: " << vc->segs[i].corp->get_confpath() << '\n';
            vector<VirtualCorpus::PosTrans> &pt = vc->segs[i].postrans;
            for (unsigned int t=0; t < pt.size(); t++)
                cout << pt[t].orgpos << " - " << pt[t].newpos << '\n';
        }
        if (argc == 3) {
            VirtualRanges *vr = new VirtualRanges(vc, argv[2]);
            for (unsigned int i = 0; i < vr->segs.size(); i++) {
                cout << "Corpus: " << vr->segs[i].src << '\n';
                //cout << "Corpus: " << '\n';
                vector<VirtualRanges::Trans> &pt = *(vr->segs[i].postrans);
                for (unsigned int t=0; t < pt.size(); t++)
                    cout << pt[t].orgpos << " - " << pt[t].newpos << " : "
                         << pt[t].orgnum << " - " << pt[t].newnum << '\n';
            }
        }
    } catch (exception &e) {
        cerr << "dumpvirt error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
