// Copyright (c) 1999-2016  Pavel Rychly, Milos Jakubicek

#ifndef REVIDX_HH
#define REVIDX_HH

#include <config.hh>
#include "compstream.hh"
#include "binfile.hh"
#include "bititer.hh"
#include "fsop.hh"
#include "fromtof.hh"
#include <string>
#include <vector>
#include <iterator>
#include <unordered_map>
#include <functional>

struct FreqIter {
    virtual int64_t next() = 0;
    virtual ~FreqIter() { };
};

template <class RevFileClass, class IdxFileClass=MapBinFile<uint32_t>,
          template <class X> class RevStreamClass=DeltaPosStream>
class delta_revidx
{
protected:
    RevFileClass crevf, *crdxf1;
    IdxFileClass *crdxf, *crdxf0, *cntf;
    const Position maxpos_val;
    int maxid_val;
    int alignmult;
    std::unordered_map<int, NumOfPos> cnt_ovf;
    static const size_t isize = sizeof (typename std::iterator_traits
                         <typename RevFileClass::const_iterator>::value_type);
    typedef read_bits<typename RevFileClass::const_iterator,
                      typename std::iterator_traits
                               <typename RevFileClass::const_iterator>::value_type,
                      uint64_t> idx1_bits;
    inline void locate_seek_cnt (int id, NumOfPos &cnt, off_t &seek) {
        lldiv_t idx0_seg_off = lldiv (id, 64);
        off_t idx1_seek = (*crdxf0) [idx0_seg_off.quot];
        lldiv_t idx1_seg_off = lldiv (idx1_seek, isize);
        idx1_bits bits (crdxf1->at (idx1_seg_off.quot), idx1_seg_off.rem * 8);
        seek = 0;
        do {
            seek += bits.delta();
            cnt = bits.gamma() - 1;
        } while (idx0_seg_off.rem--);
    }
public:
    delta_revidx (const std::string &filename, Position mpos=maxPosition,
                  int align=1) 
        : crevf (filename + ".rev"), crdxf1 (NULL), crdxf (NULL), crdxf0 (NULL),
          cntf (NULL), maxpos_val (mpos), alignmult (align) {
        if (memcmp("\243finDR", (const char*) crevf.base, 6) == 0) {
            crdxf = new IdxFileClass (filename + ".rev.idx");
            try {
                cntf = new IdxFileClass (filename + ".rev.cnt");
            } catch (FileAccessError &e) {
                delete crdxf;
            }
            maxid_val = cntf->size();
            try {
                FromFile<int64_t> cnt64(filename + ".rev.cnt64");
                while (!cnt64)
                    cnt_ovf[cnt64.get()] = cnt64.get();
            } catch (FileAccessError &e) {
                if (e.err != ENOENT) {
                    delete crdxf;
                    delete cntf;
                    throw;
                }
            }
            if ((*crdxf)[0] > 0) {
                // read alignmult
                 RevStreamClass<typename RevFileClass::const_iterator>
                     dps (crevf.at (6 / isize), 1, 1024, 6 % isize * 8);
                 alignmult = dps.next();
            }
        } else if (memcmp("\250finDR", (const char*) crevf.base, 6) == 0) {
            /* no alignmult, two-level indexing instead to find rev positions
             * for a particular ID:
             * idx0 -> fixed-length 32bit offset to a block of 64 ID-items in idx1
             * idx1 -> variable-length delta-gamma pairs
             *         delta:
             *              first ID-item in block is an absolute byte seek to .rev
             *              next 63 ID-items are differences to the previous item
             *         gamma:
             *              count of positions for this ID
             *
            */
            crdxf0 = new IdxFileClass (filename + ".rev.idx0");
            try {
                crdxf1 = new RevFileClass (filename + ".rev.idx1");
            } catch (FileAccessError &e) {
                delete crdxf0;
            }
            maxid_val = (*crdxf0)[crdxf0->size() - 1];
        } else {
            throw runtime_error(filename + ".rev: bad header");
        }
    }
    ~delta_revidx () {delete crdxf; delete crdxf0; delete crdxf1; delete cntf;}
    FastStream *id2poss (int id) {
        if (id < 0 || id >= maxid_val)
            return new EmptyStream();
        NumOfPos c;
        off_t seek;
        if (crdxf) {
            if ((c = count(id)) <= 0)
                return new EmptyStream();
            seek = off_t((*crdxf) [id]) * off_t(alignmult);
        } else
            locate_seek_cnt (id, c, seek);
        FastStream *fs =
            new RevStreamClass<typename RevFileClass::const_iterator>
                (crevf.at (seek / isize), c, maxpos_val, seek % isize * 8);
        if (c < RevFileClass::buffsize)
            fs = new MemFastStream <Position> (fs, c);
        return fs;
    }
    NumOfPos count (int id) {
        if (id < 0 || id >= maxid_val)
            return 0;
        if (crdxf) {
            std::unordered_map<int, NumOfPos>::const_iterator it = cnt_ovf.find(id);
            if (it != cnt_ovf.end())
                return it->second;
            return (*cntf)[id];
        } else {
            NumOfPos c;
            off_t seek;
            locate_seek_cnt (id, c, seek);
            return c;
        }
    }
    int id_range() {return maxid();}
    int maxid () {return maxid_val;}
    off_t data_size () const {return crevf.size();}
    Position maxpos() const {return maxpos_val;}

    class RevFreqIterNew : public FreqIter {
        delta_revidx *rev;
        int id;
        idx1_bits bits;
        int rem_items;
    public:
        RevFreqIterNew (delta_revidx *rev) : rev (rev), id (0), bits (rev->crdxf1->at(0), 0), rem_items (0) { };
        virtual int64_t next() {
            if (rem_items == 0) {
                lldiv_t idx0_seg_off = lldiv (id, 64);
                off_t idx1_seek = (*(rev->crdxf0)) [idx0_seg_off.quot];
                lldiv_t idx1_seg_off = lldiv (idx1_seek, isize);
                bits.~idx1_bits();
                new(&bits) idx1_bits (rev->crdxf1->at (idx1_seg_off.quot), idx1_seg_off.rem * 8);
                rem_items = idx0_seg_off.rem;
                if (rem_items == 0) return -1;
            }
            rem_items -= 1;
            id += 1;
            bits.delta();
            return bits.gamma() - 1;
        };
    };

    class RevFreqIterOld : public FreqIter {
        delta_revidx *rev;
        int id;
    public:
        RevFreqIterOld (delta_revidx *rev) : rev (rev), id (0) { };
        virtual int64_t next() { if (id >= rev->maxid ()) return -1; return rev->count (id++); };
    };

    FreqIter* iter_freqs() {
        if (crdxf) return new RevFreqIterOld(this);
        else return new RevFreqIterNew(this);
    }
};

typedef delta_revidx<MapBinFile<uint64_t> > map_delta_revidx;
typedef delta_revidx<MapBinFile<uint64_t>,
                     MapBinFile<uint32_t> > map_map_delta_revidx;
typedef delta_revidx<MapBinFile<uint64_t>,MapBinFile<uint32_t>,
                     CollDeltaPosStream> map_colldelta_revidx;

#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
