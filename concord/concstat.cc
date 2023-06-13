//  Copyright (c) 1999-2011  Pavel Rychly, Milos Jakubicek

#include "config.hh"
#include "concord.hh"
#include "conccrit.hh"
#include "concstat.hh"
#include "bgrstat.hh"
#include "fsstat.hh"
#include "srtruns.hh"
#include "frsop.hh"
#include <algorithm>
#include <math.h>
#include <unordered_map>

using namespace std;

int Concordance::distribution (vector<int> &vals, vector<int> &beginnings,
                               int yrange, bool normalize)
{
    vector<int>::iterator i;
    for (i = vals.begin(); i < vals.end(); i++)
        *i = 0;
    for (i = beginnings.begin(); i < beginnings.end(); i++)
        *i = 0;
    double mod = (corp->size() + 1.0) / vals.size();
    lock();
    for (ConcItem *r = rng; r < rng + size(); r++) {
        if (r->beg == -1) continue;
        int bin = int(r->beg / mod);
        vals [bin] ++;
        if (! beginnings [bin])
            beginnings [bin] = r - rng;
    }
    unlock();
    int maxy = *max_element (vals.begin(), vals.end());
    if (normalize) {
        double mult = (yrange - 1.0) / maxy;
        for (i = vals.begin(); i < vals.end(); i++)
            *i = int (round (*i * mult));
    }
    return maxy;
}


class Conc_BegsFS: public FastStream {
    Concordance *conc;
    Position curridx;
public:
    Conc_BegsFS (Concordance *c)
        : conc(c), curridx(0) {}
    virtual ~Conc_BegsFS() {}

    virtual Position peek() {
        if (curridx < conc->size())
            return conc->beg_at(curridx);
        else
            return final();
    }
    virtual Position next() {
        if (curridx < conc->size())
            return conc->beg_at(curridx++);
        else
            return final();
    }
    virtual Position find (Position pos) {
        while (curridx < conc->size() && pos > conc->beg_at(curridx))
            curridx++;
        return peek();
    }
    virtual NumOfPos rest_min() {return conc->size() - curridx;}
    virtual NumOfPos rest_max() {return conc->size() - curridx;}
    virtual Position final() {return conc->corp->size();}
    virtual void add_labels (Labels &lab) {
        for (int i = 1; i <= conc->numofcolls(); i++)
            lab[i] = conc->coll_beg_at (i, curridx);
    }
};

FastStream *Concordance::begs_FS()
{
    return new Conc_BegsFS(this);
}


NumOfPos Concordance::redfreq ()
{
    double avr = double (corp->size()) / size();
    double curr = 0.0;
    NumOfPos last = corp->size();
    Conc_BegsFS s (this);
    NumOfPos freq = 0;
    while (s.peek() < last) {
        s.find (Position (curr));
        curr += avr;
        if (s.peek() < Position (curr) && s.peek() < last)
            freq++;
    }
    return freq;
}

double Concordance::compute_ARF()
{
    return ::compute_ARF (new Conc_BegsFS(this), size(), corp->size());
}


// ==================== Concordance::find_coll ====================
struct statdata {
    int id;
    double cmp;
    NumOfPos freq;
    NumOfPos cnt;
    statdata (int id, double s, NumOfPos f, NumOfPos c)
        : id (id), cmp(s), freq (f), cnt (c) {}
    static bool greater_cmp(const statdata& x, const statdata& y) { 
        return x.cmp > y.cmp;
    }

    static bool greater_cnt(const statdata& x, const statdata& y) { 
        return x.cnt > y.cnt;
    }
    typedef bool compare (const statdata&, const statdata&);
};



double static bgr_pure_freq (double f_AB, double, double, double)
{
    return f_AB;
}


void Concordance::find_coll (ostream &out, const string &attr, 
                             const string &cmpr, NumOfPos minfreq, 
                             NumOfPos minbgr, int fromw, int tow, 
                             int maxlines, const char *stats)
{
    if (fromw > tow || !stats || !stats[0])
        return;
    PosAttr *a = corp->get_attr (attr);
    bigram_fun *cmpfn = code2bigram_fun (cmpr[0]);
    statdata::compare *cmp = 
        cmpfn ? statdata::greater_cmp : statdata::greater_cnt;

    bigram_fun* *stat_funs = new bigram_fun*[strlen (stats) +1];
    {   
        bigram_fun **s = stat_funs;
        for (const char *c = stats; *c; c++, s++) {
            *s = (*c == 'f') ? bgr_pure_freq : code2bigram_fun (*c);
        }
        *s = 0;
    }

    unordered_map<int,NumOfPos> bs;  // (attr_id, count)
    for (Position i = 0; i < size(); i++) {
        if (rng[i].beg == -1) continue;
        if (fromw < 0) {
            IDIterator *pi = a->posat (rng[i].beg + fromw);
            for (int d = fromw; d < 0 && d <= tow; d++) {
                int id = pi->next();
                if (a->freq (id) >= minfreq)
                    bs [id] ++;
            }
            delete pi;
        }
        if (tow > 0) {
            int d = fromw > 0 ? fromw : 1;
            IDIterator *pi = a->posat (rng[i].end -1 + d);
            for (; d <= tow; d++) {
                int id = pi->next();
                if (a->freq (id) >= minfreq)
                    bs [id] ++;
            }
            delete pi;
        }
    }

    typedef vector<statdata> resvect;
    resvect results;
    double N = corp_size;
    double f_B = viewsize();
    for (unordered_map<int,NumOfPos>::iterator i = bs.begin(); i != bs.end(); ++i) {
        if ((*i).second < minbgr) continue;
        NumOfPos freq = a->freq ((*i).first);
        results.push_back 
            (statdata((*i).first,
                      cmpfn ? (*cmpfn)((*i).second, freq, f_B, N) : 0.0,
                      freq, (*i).second));
    }

    

    // jenom prvnich nekolik
    resvect::iterator last;
    if (int (results.size()) <= maxlines) {
        last = results.end();
        ::sort (results.begin(), results.end(), cmp);
    } else {
        last = results.begin() + maxlines;
        ::partial_sort (results.begin(), last, results.end(), cmp);
    }

    out.precision(4);
    for (resvect::const_iterator i = results.begin(); i != last; ++i) {
        out << a->id2str ((*i).id);
        for (bigram_fun **s = stat_funs; *s; s++) {
            out << '\t';
            if (*s == bgr_pure_freq) 
                out << (*i).cnt;
            else if (*s == cmpfn)
                out << (*i).cmp;
            else
                out << (*s)((*i).cnt, (*i).freq, f_B, N);
        }
        out << '\n';
    }
    delete[] stat_funs;
}


//==================== CollocItems ====================

struct CollItem {
    int id;
    double cmp;
    NumOfPos freq;
    NumOfPos cnt;
    CollItem (int id, double s, NumOfPos f, NumOfPos c)
        : id (id), cmp(s), freq (f), cnt (c) {}
    CollItem () {}
    bool operator< (const CollItem& that) const { 
        return cmp < that.cmp;
    }
};


CollocItems::
CollocItems (Concordance *conc, const string &attr_name, char sort_fun_code, 
             NumOfPos minfreq, NumOfPos minbgr, int fromw, int tow, 
             int maxitems)
    : attr (conc->corp->get_attr (attr_name)),
      results (new CollItem [maxitems]), 
      f_B (conc->viewsize()), N (conc->corp_size)
{
    unordered_map<int,NumOfPos> bs;
    set<int> collocs;
    set<int>::iterator it;
    for (Position i = 0; i < conc->size(); i++) {
        if (conc->beg_at(i) == -1) continue;
        if (fromw < 0) {
            Position beg = conc->beg_at(i) + fromw;
            IDIterator *pi = attr->posat (beg);
            for (int d = (beg < 0 ? fromw - beg : fromw); d < 0 && d <= tow; d++) {
                int id = pi->next();
                if (attr->freq (id) >= minfreq)
                    collocs.insert(id);
            }
            delete pi;
        }
        if (tow > 0) {
            int d = fromw > 0 ? fromw : 1;
            IDIterator *pi = attr->posat (conc->end_at(i) -1 + d);
            for (; d <= tow; d++) {
                int id = pi->next();
                if (id < 0)
                    break;
                if (attr->freq (id) >= minfreq)
                    collocs.insert(id);
            }
            delete pi;
        }
        for (it = collocs.begin(); it != collocs.end(); it ++) {
            bs [*it] ++;
        }
        collocs.clear();
    }

    bigram_fun *sortfn = code2bigram_fun (sort_fun_code);
    StoreTopItems<CollItem> toponly (maxitems, results);
    for (unordered_map<int,NumOfPos>::iterator i = bs.begin(); i != bs.end(); ++i) {
        if ((*i).second < minbgr) continue;
        NumOfPos freq = attr->freq ((*i).first);
        toponly (CollItem((*i).first,
                          (*sortfn)((*i).second, freq, f_B, N), 
                          freq, (*i).second));
    }

    ::sort (toponly.begin(), toponly.end());
    curr = toponly.end() -1;
    last = toponly.begin();
}

CollocItems::~CollocItems()
{
    delete results;
}

void CollocItems::next()
{
    curr--;
}

bool CollocItems::eos()
{
    return curr < last;
}

const char *CollocItems::get_item()
{
    return attr->id2str ((*curr).id);
}

NumOfPos CollocItems::get_freq() 
{
    return (*curr).freq;
}

NumOfPos CollocItems::get_cnt() 
{
    return (*curr).cnt;
}

double CollocItems::get_bgr (char bgr_code)
{
    return (code2bigram_fun (bgr_code))((*curr).cnt, (*curr).freq, f_B, N);
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:

