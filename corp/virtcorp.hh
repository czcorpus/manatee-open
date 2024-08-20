//  Copyright (c) 2004-2020  Pavel Rychly, Milos Jakubicek

#ifndef VIRTCORP_HH
#define VIRTCORP_HH

#include "fstream.hh"
#include "range.hh"
#include "binfile.hh"

class Corpus;
class PosAttr;
template <class NormClass=MapBinFile<int64_t>,
          class FreqClass=MapBinFile<uint32_t>,
          class FloatFreqClass=MapBinFile<float> >
class VirtualPosAttr;

class VirtualCorpus {
    bool ownes_corpora;
public:
    struct PosTrans {
        // transition from original positions to new positions,
        // the size of the included region is computed as a difference of
        // next newpos and current newpos (in the postrans array)
        Position orgpos;
        Position newpos;
        PosTrans (Position o, Position n) : orgpos(o), newpos(n){}
    };
    struct Segment {
        Corpus *corp = NULL;
        // last item in postrans indicates the end of the last
        // transition region and the start position of the next segment
        std::vector<PosTrans> postrans;
    };
    std::vector<Segment> segs;

    virtual Position size() {return segs.back().postrans.back().newpos;}
    virtual ~VirtualCorpus();
    VirtualCorpus(bool oc = true) : ownes_corpora (oc){}
    FastStream *combine_poss (PosAttr *pa, std::vector<FastStream*> &fsv);
};

VirtualCorpus* setup_virtcorp (const std::string &filename);
PosAttr* setup_virtposattr (VirtualCorpus *vc, const std::string &path,
                            const std::string &name, const std::string &locale,
                            const std::string &enc, bool ownedByCorpus=true,
                            const std::string &def="", const std::string &doc="");
ranges* setup_virtstructrng (VirtualCorpus *vc, const std::string &name);
VirtualCorpus* virtcorp2virtstruc (VirtualCorpus *vc, const std::string &name);

#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
