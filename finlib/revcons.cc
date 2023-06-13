// Copyright (c) 1999-2019  Pavel Rychly, Milos Jakubicek

#include <config.hh>
#include "consumer.hh"
#include "srtruns.hh"
#include "bitio.hh"
#include "bititer.hh"
#include "revidx.hh"
#include "fromtof.hh"
#include "util.hh"
#include <vector>
#include <limits>


using namespace std;

static void write_signature (OutFileBits_tell &ofb, char version)
{
    static const char drevsignature[6] = "finDR";
    *ofb = version;
    ++ofb;
    for (const char *c=drevsignature; *c; c++, ++ofb)
        *ofb = *c;
    *ofb=0;
}


typedef pair<int,Position> idpos;

struct idposcoll {
    int first;
    Position second, coll;
    idposcoll ():
        first(0), second(0), coll(0) {}
    idposcoll (int f, Position s):
        first(f), second(s), coll(0) {}
    idposcoll (int f, Position s, Position c):
        first(f), second(s), coll(c) {}
    bool operator== (const idposcoll &x) const {
        return first == x.first && second == x.second && coll == x.coll;
    }
    bool operator< (const idposcoll &x) const {
        if (first < x.first)
            return true;
        else if (first > x.first)
            return false;
        else {
            if (second < x.second)
                return true;
            else if (second > x.second)
                return false;
            else
                return coll < x.coll;
        }
    }
};

static string numbered_name (const string &base, int num)
{
    char s[12];
    sprintf (s, "#%i", num);
    return base + s;
}

static void rename_rev_files (string frombase, string tobase)
{
    checked_rename((frombase + ".rev.cnt").c_str(), (tobase + ".rev.cnt").c_str());
    checked_rename((frombase + ".rev.cnt64").c_str(), (tobase + ".rev.cnt64").c_str());
    checked_rename((frombase + ".rev.idx").c_str(), (tobase + ".rev.idx").c_str());
    checked_rename((frombase + ".rev").c_str(),     (tobase + ".rev").c_str());
}

typedef write_bits<OutFileBits_tell&,unsigned char, OutFileBits_tell&, 
                   Position> PosWriter;

template <typename Value>
class tempdeltarev {
    struct uniq_data {
        int filenum;
        Value prev;
        ToFile<uint32_t> *cntf, *idxf;
        ToFile<int64_t> *cntf64;
        FILE *revf;
        OutFileBits_tell *fiter;
        PosWriter *crevs;
        int nextid;
        NumOfPos id_count;
        uniq_data () :filenum (0), prev (-1,0), cntf (NULL), idxf(NULL),
            cntf64(NULL), nextid (0), id_count (0) {}
    };
    uniq_data *data;
    const string filename;
    const int alignmult;
    bool origin;

    void open_next ();
    void write_id (int newid, bool open_new=true);

public:
    int num_of_files() const {return data->filenum;}
    tempdeltarev (tempdeltarev &c) 
        : data (c.data), filename (c.filename), alignmult (c.alignmult), 
          origin (false) {}
    tempdeltarev (const char *filebase, int align, bool append=false)
        : data (new uniq_data), filename (filebase),
          alignmult (max(1, align)), origin (true)
    { 
        if (append) {
            rename_rev_files (filename, numbered_name (filename, 0));
            data->filenum++;
        }
        open_next(); 
    }
    ~tempdeltarev () {
        if (origin) {
            write_id (data->prev.first, false);
            delete data->crevs; delete data->fiter; delete data->cntf64;
            delete data->cntf; fclose (data->revf); delete data->idxf;
            delete data;
        }
    }
    void operator() (Value x);
    void nextRun() {}
    off_t get_curr_seek();
};

template <typename Value>
void tempdeltarev<Value>::open_next ()
{
    if (data->cntf) {
        delete data->crevs; delete data->fiter; delete data->cntf64;
        delete data->cntf; fclose (data->revf); delete data->idxf;
    }
    string s = numbered_name (filename, data->filenum);
    data->cntf = new ToFile<uint32_t>(s + ".rev.cnt");
    data->cntf64 = new ToFile<int64_t>(s + ".rev.cnt64");
    data->idxf = new ToFile<uint32_t>(s + ".rev.idx");
    data->revf = fopen ((s + ".rev").c_str(), "wb");
    data->filenum ++;
    data->fiter = new OutFileBits_tell (data->revf);
    data->crevs = new PosWriter (*data->fiter);
    write_signature (*data->fiter, '\243');
    data->crevs->delta (alignmult +1);
}

template <>
void tempdeltarev<idpos>::operator() (idpos x)
{
    if (data->prev.first != x.first || x.second < data->prev.second) {
        write_id (x.first);
        data->id_count = 1;
        data->crevs->delta (x.second +1);
    } else if (x.second != data->prev.second) {
        data->id_count++;
        data->crevs->delta (x.second - data->prev.second);
    }
    data->prev = x;
}

template <>
void tempdeltarev<idposcoll>::operator() (idposcoll x)
{
    if (data->prev.first != x.first || x.second < data->prev.second) {
        write_id (x.first);
        data->id_count = 1;
        data->crevs->delta (x.second +1);
    } else {
        data->id_count++;
        data->crevs->delta (x.second - data->prev.second +1);
    }
    int c = x.coll - x.second;
    c *= 2;
    if (c < 0)
        c = -c +1;
    data->crevs->gamma(c + 1);
    data->prev = x;
}

template<typename Value>
void tempdeltarev<Value>::write_id (int newid, bool open_new)
{
    if (data->prev.first != -1) {
        if (data->id_count > numeric_limits<uint32_t>::max()) {
            data->cntf64->put (data->prev.first);
            data->cntf64->put (data->id_count);
            data->cntf->put (0);
        } else {
            data->cntf->put (data->id_count);
        }
    }

    data->crevs->new_block();
    off_t seek = get_curr_seek();

    data->idxf->put (seek);
    if (open_new && (data->prev.first >= newid || seek > numeric_limits<uint32_t>::max())) {
        open_next();
        data->nextid = 0;
        data->crevs->new_block();
        seek = get_curr_seek();
        data->idxf->put (seek);
    }
    while (data->nextid++ < newid) {
        data->cntf->put (0);
        data->idxf->put (seek);
    }
}

template<typename Value>
off_t tempdeltarev<Value>::get_curr_seek()
{
    off_t seeko = data->fiter->tell();
    while (seeko % alignmult) {
        *++(*data->fiter) = 0;
        ++seeko;
    }
    return seeko / alignmult;
}

typedef vector<mfile_mfile_delta_revidx*> Revs;
typedef vector<mfile_mfile_delta_collrevidx*> CollRevs;

void revs_reservation (const string &outfilename, bool open) {
    static FILE *revf = NULL, *cntf = NULL, *cntf64 = NULL, *idxf = NULL;
    if (open) {
        revf = fopen ((outfilename + ".rev").c_str(), "wb");
        cntf = fopen ((outfilename + ".rev.cnt").c_str(), "wb");
        cntf64 = fopen ((outfilename + ".rev.cnt64").c_str(), "wb");
        idxf = fopen ((outfilename + ".rev.idx").c_str(), "wb");
    } else {
        fclose (revf);
        fclose (cntf);
        fclose (cntf64);
        fclose (idxf);
    }
}
#define RESERVE_REVS(x) revs_reservation(x, true)
#define UNRESERVE_REVS(x) revs_reservation(x, false)

inline void write_pos (FastStream *s, Position &lastpos, NumOfPos &total,
                       PosWriter &crevs)
{
    Position d = s->next();
    if (d > lastpos)
        crevs.delta (d - lastpos);
    else
        total--;
    lastpos = d;
}

inline void write_collpos (FastStream *s, Position &lastpos, Position &lastcoll,
                           NumOfPos &total, PosWriter &crevs)
{
    Labels lab;
    s->add_labels (lab);
    Position d = s->next();
    if (d > lastpos || lab[1] != lastcoll) {
        crevs.delta (d - lastpos +1);
        int c = lab[1] - d;
        c *= 2;
        if (c < 0)
            c = -c +1;
        crevs.gamma(c + 1);
    } else
        total--;
    lastpos = d;
    lastcoll = lab[1];
}

/*
 * Assumes that positions are sorted only within a single run,
 * not across all runs
 */

// without collocation

static NumOfPos join_id_part_sort (int id, Revs &inrevs, PosWriter &crevs)
{
    vector<FastStream*> *fsv = new vector<FastStream*>(inrevs.size());
    NumOfPos total = 0;
    Position lastpos = -1;
    Revs::iterator f = inrevs.begin();
    int i = 0;
    while (f != inrevs.end()) {
        if ((*f)->maxid() <= id) {
            delete *f;
            f = inrevs.erase (f);
            fsv->pop_back();
            continue;
        }
        NumOfPos count = (*f)->count (id);
        if (count) {
            total += count;
            (*fsv)[i++] = (*f)->id2poss (id);
        } else
            fsv->pop_back();
        f++;
    }
    if (!fsv->size()) {
        delete fsv;
        return 0;
    }
    FastStream *fss;
    if (fsv->size() == 1) {
        fss = (*fsv)[0];
        delete fsv;
    } else {
        fss = new QOrVNode (fsv);
    }
    while (fss->peek() < fss->final())
        write_pos (fss, lastpos, total, crevs);
    delete fss;
    return total;
}

// with collocation

static NumOfPos join_id_part_sort (int id, CollRevs &inrevs, PosWriter &crevs)
{
    vector<FastStream*> *fsv = new vector<FastStream*>(inrevs.size());
    NumOfPos total = 0;
    Position lastpos = 0, lastcoll = -1;
    CollRevs::iterator f = inrevs.begin();
    int i = 0;
    while (f != inrevs.end()) {
        if ((*f)->maxid() <= id) {
            delete *f;
            f = inrevs.erase (f);
            fsv->pop_back();
            continue;
        }
        NumOfPos count = (*f)->count (id);
        if (count) {
            total += count;
            (*fsv)[i++] = (*f)->id2poss (id);
        } else
            fsv->pop_back();
        f++;
    }
    if (!fsv->size()) {
        delete fsv;
        return 0;
    }
    FastStream *fss;
    if (fsv->size() == 1) {
        fss = (*fsv)[0];
        delete fsv;
    } else {
        fss = new QOrVNode (fsv);
    }
    while (fss->peek() < fss->final())
        write_collpos (fss, lastpos, lastcoll, total, crevs);
    delete fss;
    return total;
}

/*
 * Assumes that positions are sorted within each run and across all runs, i.e.
 * all positions in run #i are smaller than all positions in runs #i+1,...,#n
 */

// without collocation

static NumOfPos join_id_total_sort (int id, Revs &inrevs, PosWriter &crevs)
{
    NumOfPos total = 0;
    Position lastpos = -1;
    Revs::iterator f=inrevs.begin();
    while (f != inrevs.end()) {
        if ((*f)->maxid() <= id) {
            delete *f;
            f = inrevs.erase (f);
            continue;
        }
        NumOfPos count = (*f)->count (id);
        total += count;

        FastStream *s = (*f)->id2poss (id);
        while (count--)
            write_pos (s, lastpos, total, crevs);
        delete s;
        f++;
    }
    return total;
}

// with collocation

static NumOfPos join_id_total_sort (int id, CollRevs &inrevs, PosWriter &crevs)
{
    NumOfPos total = 0;
    Position lastpos = 0, lastcoll = -1;
    CollRevs::iterator f=inrevs.begin();
    while (f != inrevs.end()) {
        if ((*f)->maxid() <= id) {
            delete *f;
            f = inrevs.erase (f);
            continue;
        }
        NumOfPos count = (*f)->count (id);
        total += count;

        FastStream *s = (*f)->id2poss (id);
        while (count--)
            write_collpos (s, lastpos, lastcoll, total, crevs);
        delete s;
        f++;
    }
    return total;
}

template <class RevClass>
static void joinrevfiles (vector<RevClass*> &inrevs, const string &outfilename,
                          int alignmult, bool partially_sorted)
{
    {
        // test file size of the result 
        long long totaldatasize = 0;
        for (typename vector<RevClass*>::iterator f=inrevs.begin();
             f != inrevs.end(); f++)
            totaldatasize += (*f)->data_size();
        if (totaldatasize > ((long long) alignmult) * 2147483647) {
            alignmult = totaldatasize / 2147483647 +1;
        }
    }
    FILE *revf = fopen ((outfilename + ".rev").c_str(), "wb");
    ToFile<uint32_t> cntf (outfilename + ".rev.cnt");
    ToFile<int64_t> cntf64 (outfilename + ".rev.cnt64");
    ToFile<uint32_t> idxf (outfilename + ".rev.idx");
    {
        //XXX musi tady byt vlozeny blok, aby se volal destruktor pred fclose
        OutFileBits_tell _xxx (revf);
        write_signature (_xxx, '\243');
        PosWriter crevs (_xxx);
        crevs.delta (alignmult +1);
        int id = 0;
        while (inrevs.size()) {
            crevs.new_block();
            off_t seeko = _xxx.tell();
            while (seeko % alignmult) {
                *++_xxx = 0;
                ++seeko;
            }
            idxf (seeko / alignmult);
            NumOfPos cnt;
            if (partially_sorted)
                cnt = join_id_part_sort (id, inrevs, crevs);
            else
                cnt = join_id_total_sort (id, inrevs, crevs);
            if (cnt > numeric_limits<uint32_t>::max()) {
                cntf64 (id);
                cntf64 (cnt);
                cntf (0);
            } else {
                cntf (cnt);
            }
            id++;
        }
        //XXX tady se vola destruktor _xxx
    }
    fclose (revf);
}

template <class RevClass>
static void joinrevfiles_idx1 (vector<RevClass*> &inrevs,
                               const string &outfilename, bool partially_sorted)
{
    unlink ((outfilename + ".rev.idx").c_str()); // may have been reserved
    unlink ((outfilename + ".rev.cnt").c_str()); // may have been reserved
    unlink ((outfilename + ".rev.cnt64").c_str()); // may have been reserved
    FILE *revf = fopen ((outfilename + ".rev").c_str(), "wb");
    ToFile<uint32_t> idxf0 (outfilename + ".rev.idx0");
    FILE *idxf1 = fopen ((outfilename + ".rev.idx1").c_str(), "wb");
    int id = 0;
    {
        //XXX musi tady byt vlozeny blok, aby se volal destruktor pred fclose
        OutFileBits_tell rev_bits (revf);
        OutFileBits_tell idxf1_bits (idxf1);
        write_signature (rev_bits, '\250');
        PosWriter crevs (rev_bits);
        PosWriter cidxf1 (idxf1_bits);
        off_t curr_seek = 0;
        while (inrevs.size()) {
            crevs.new_block();
            off_t seeko = rev_bits.tell();
            if (id % 64 == 0) {
                cidxf1.new_block();
                idxf0.put (idxf1_bits.tell());
                cidxf1.delta (seeko);
            } else
                cidxf1.delta (seeko - curr_seek);
            curr_seek = seeko;
            NumOfPos cnt;
            if (partially_sorted)
                cnt = join_id_part_sort (id, inrevs, crevs);
            else
                cnt = join_id_total_sort (id, inrevs, crevs);
            cidxf1.gamma (cnt + 1);
            id++;
        }
        //XXX tady se vola destruktor rev_bits, idxf1_bits
    }
    idxf0.put (id - 1);
    fclose (revf);
    fclose (idxf1);
}


template <class RevClass>
void finish_rev_files (const string &outname, int num_of_files, int align,
                       bool part_sorted)
{
    if (num_of_files > 1 || align == -1) {
        // join parts
        vector<RevClass*> r;
        int from_num = 0;
        while (from_num < num_of_files) {
            RESERVE_REVS (outname);
            r.clear();
            if (from_num > 0) {
                r.push_back (new RevClass (numbered_name (outname, 0)));
            }
            bool finished = true;
            for (int i = from_num; i < num_of_files; i++) {
                try {
                    r.push_back (new RevClass (numbered_name (outname, i)));
                } catch (FileAccessError &e) {
                    if ((e.err == EMFILE || e.err == ENFILE)
                        && i > from_num + 1) {
                        errno = 0;
                        delete r.back();
                        r.erase (r.end() -1);
                        delete r.back();
                        r.erase (r.end() -1);
                        //cerr << "partial join " << from_num << '-' << i-2 << endl;
                        UNRESERVE_REVS (outname);
                        joinrevfiles<RevClass> (r, outname, align, part_sorted);
                        rename_rev_files (outname, numbered_name (outname, 0));
                        from_num = i -2;
                        finished = false;
                        break;
                    } else {
                        throw e;
                    }
                }
            }
            if (finished) break;
        }
        UNRESERVE_REVS (outname);
        if (align != -1)
            joinrevfiles<RevClass> (r, outname, align, part_sorted);
        else
            joinrevfiles_idx1<RevClass> (r, outname, part_sorted);
        if (getenv ("MANATEE_DEBUG") == NULL) {
            for (int i = 0; i < num_of_files; i++) {
                string s = numbered_name (outname, i);
                unlink ((s + ".rev").c_str());
                unlink ((s + ".rev.cnt").c_str());
                unlink ((s + ".rev.cnt64").c_str());
                unlink ((s + ".rev.idx").c_str());
            }
        }
    } else {
        // rename the only part
        rename_rev_files (numbered_name (outname, 0), outname);
    }
}

class DeltaCollRevFileConsumer : public CollRevFileConsumer
{
    tempdeltarev<idposcoll> *oi;
    SortedRuns<tempdeltarev<idposcoll>, idposcoll> *drevcons;
    const string outname;
    const int alignmult;
    bool part_sorted;
public:
    DeltaCollRevFileConsumer (const char *filename, int buff_size, int align,
                          bool append=false, bool part_sort=false)
        : oi (new tempdeltarev<idposcoll> (filename, align, append)),
          drevcons (new SortedRuns<tempdeltarev<idposcoll>, idposcoll> (oi, buff_size)),
          outname (filename), alignmult (align), part_sorted (part_sort)
    {}
    virtual ~DeltaCollRevFileConsumer() {
        drevcons->flush();
        int num_of_files = oi->num_of_files();
        delete drevcons;
        finish_rev_files<mfile_mfile_delta_collrevidx>
            (outname, num_of_files, alignmult, part_sorted);
    }
    virtual void put (int id, Position pos, Position coll) {
        (*drevcons) (idposcoll(id, pos, coll));
    }
};

CollRevFileConsumer*
CollRevFileConsumer::create (const string &path, int buff_size, int align,
                             bool append, bool part_sort)
{
    return new DeltaCollRevFileConsumer
           (path.c_str(), buff_size, align, append, part_sort);
}

class DeltaRevFileConsumer : public RevFileConsumer
{
    tempdeltarev<idpos> *oi;
    SortedRuns<tempdeltarev<idpos>, idpos> *drevcons;
    const string outname;
    const int alignmult;
    bool part_sorted;
public:
    DeltaRevFileConsumer (const char *filename, int buff_size, int align,
                          bool append=false, bool part_sort=false)
        : oi (new tempdeltarev<idpos> (filename, align, append)),
          drevcons (new SortedRuns<tempdeltarev<idpos>, idpos> (oi, buff_size)),
          outname (filename), alignmult (align), part_sorted (part_sort)
    {}
    virtual ~DeltaRevFileConsumer() {
        drevcons->flush();
        int num_of_files = oi->num_of_files();
        delete drevcons;
        finish_rev_files<mfile_mfile_delta_revidx>
            (outname, num_of_files, alignmult, part_sorted);
    }
    virtual void put (int id, Position pos) {
        (*drevcons) (idpos(id, pos));
    }
};

RevFileConsumer* 
RevFileConsumer::create (const string &path, int buff_size, int align,
                         bool append, bool part_sort)
{
    return new DeltaRevFileConsumer
           (path.c_str(), buff_size, align, append, part_sort);
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
