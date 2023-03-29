// Copyright (c) 1999-2013  Pavel Rychly, Milos Jakubicek

#include "binfile.hh"
#include "range.hh"
#include "fromtof.hh"
#include <cstdio>


//IntType = int32_t, int64_t
//    - have to be signed, negative end means it is a nested range
template <class IntType>
struct rangeitem {
    IntType beg;
    IntType end;
    rangeitem () {}
    rangeitem (IntType b, IntType e): beg(b), end(e) {}
};

template <class IntType>
IntType abs(IntType v)
{
    return v < 0 ? -v : v;
}

//RngStorage = BinFile<rangeitem> 
template <class RngStorage>
class int_ranges : public ranges {
public:
    RngStorage rngf;
    typedef typename RngStorage::const_iterator rng_iter;

    int_ranges (const std::string &filename)
        : rngf (filename) {}
    virtual ~int_ranges() {}
    virtual NumOfPos size ()
        {return rngf.size();}
    virtual Position beg_at (NumOfPos idx)
        {return rngf[idx].beg;}
    virtual Position end_at (NumOfPos idx)
        {return abs(rngf[idx].end);}
    virtual NumOfPos num_at_pos (Position pos);
    virtual NumOfPos num_next_pos (Position pos);
    virtual RangeStream *whole();
    virtual RangeStream *part(FastStream *p);
    virtual int nesting_at (NumOfPos idx)
        {return rngf[idx].end < 0 ? 1 : 0;}
};



template <class IntRanges>
class whole_range: public RangeStream {
protected:
public:
    typename IntRanges::rng_iter curr, last;
    Position finval;
    int nestval;
    whole_range (IntRanges *r, int nest = 0);
    whole_range (typename IntRanges::rng_iter from, 
                 typename IntRanges::rng_iter to,
                 Position fin = 2000000000, int nest = 0);
    ~whole_range () {}
    virtual bool next () {return (++curr < last);}
    virtual Position peek_beg () const;
    virtual Position peek_end () const;
    virtual void add_labels (Labels &lab) const {}
    virtual Position find_beg (Position pos);
    virtual Position find_end (Position pos);
    virtual NumOfPos rest_min () const {return (last - curr);}
    virtual NumOfPos rest_max () const {return (last - curr);}
    virtual Position final () const {return finval;}
    virtual int nesting () const {return nestval;}
    virtual bool epsilon () const {return false;}
};


template <class IntRanges>
class part_range: public RangeStream {
protected:
    NumOfPos curr;
    IntRanges *rng;
    FastStream *pos;
    Position finval;
    NumOfPos last;
    int nestval;
    bool locate();
public:
    part_range (IntRanges *r, FastStream *p, int nest = 0);
    ~part_range() {delete pos;}
    virtual bool next ();
    virtual Position peek_beg () const;
    virtual Position peek_end () const;
    virtual void add_labels (Labels &lab) const {pos->add_labels (lab);}
    virtual Position find_beg (Position pos);
    virtual Position find_end (Position pos);
    virtual NumOfPos rest_min () const {return (last - curr);/*XXX pos->rest_min();*/}
    virtual NumOfPos rest_max () const {return (last - curr);/*XXX*/}
    virtual Position final () const {return finval;}
    virtual int nesting () const {return nestval;}
    virtual bool epsilon () const {return false;}
};



template <class RngStorage>
NumOfPos int_ranges<RngStorage>::num_at_pos (Position pos)
{
    whole_range<int_ranges<RngStorage> > wr (this);
    wr.find_end (pos +1);
    if (wr.peek_beg() >= wr.final())
        return -1;
    if (wr.peek_beg() <= pos) {
        NumOfPos rngnum = wr.curr - rngf.at(0);
        NumOfPos rngsize = wr.peek_end() - wr.peek_beg();
        // try to find smaller range in nested ones
        ++wr.curr;
        while (wr.peek_beg() != wr.final() && (*wr.curr).end < 0 
               && wr.peek_beg() <= pos ) {
            // nested ranges
            if (wr.peek_end() > pos && wr.peek_end() - wr.peek_beg() < rngsize) {
                rngnum = wr.curr - rngf.at(0);
                rngsize = wr.peek_end() - wr.peek_beg();
            }
            ++wr.curr;
        }
        return rngnum;
    }
    // handling of empty structures around pos
    if (wr.peek_beg() == wr.peek_end() && wr.peek_beg() == pos+1)
        // empty struct following pos
        return wr.curr - rngf.at(0);
    if (rngf.at(0) < wr.curr)
        --wr.curr;
    if (wr.peek_beg() == wr.peek_end() && wr.peek_beg() == pos)
        // empty struct preceeding pos
        return wr.curr - rngf.at(0);
    return -1;
}

template <class RngStorage>
NumOfPos int_ranges<RngStorage>::num_next_pos (Position pos)
{
    whole_range<int_ranges<RngStorage> > wr (this);
    wr.find_end (pos +1);
    return wr.curr - rngf.at(0);
}


template <class RngStorage>
RangeStream *int_ranges<RngStorage>::whole()
{
    return new whole_range<int_ranges<RngStorage> > (this);
}

template <class RngStorage>
RangeStream *int_ranges<RngStorage>::part(FastStream *p)
{
    return new part_range<int_ranges<RngStorage> > (this, p);
}


//-------------------- whole_range --------------------

template <class IntRanges>
whole_range<IntRanges>::whole_range (IntRanges *r, int nest)
    : curr (r->rngf.at(0)), last (r->rngf.at (r->size())), 
    finval (r->end_at (r->size()-1) +1), nestval (nest) 
{
}

template <class IntRanges>
whole_range<IntRanges>::whole_range (typename IntRanges::rng_iter from, 
                                     typename IntRanges::rng_iter to,
                                     Position fin, int nest)
    : curr (from), last (to), finval (fin), nestval (nest) 
{
}



template <class IntRanges>
Position whole_range<IntRanges>::peek_beg () const 
{
    return (curr < last) ? (*curr).beg : finval;
}

template <class IntRanges>
Position whole_range<IntRanges>::peek_end () const
{
    return (curr < last) ? abs((*curr).end) : finval;
}

template <class IntRanges>
Position whole_range<IntRanges>::find_beg (Position pos)
{
    if (not (curr < last))
        return finval;
    typename IntRanges::rng_iter prev = curr;
    off_t incr = 1;
    while ((curr + incr) < last && (*(curr + incr)).beg <= pos) {
        curr += incr;
        incr *= 2;
    }
    while (incr > 0) {
        if ((curr + incr) < last && (*(curr + incr)).beg <= pos) {
            curr += incr;
        }
        incr /= 2;
    }
    if ((*curr).beg < pos) 
        ++curr;
    else if (prev < curr){
        typename IntRanges::rng_iter curr_1 = curr;
        --curr_1;
        while (prev < curr && (*curr_1).beg == pos) {
            --curr;
            --curr_1;
        }
    }
    return (curr < last) ? (*curr).beg : finval;
}

template <class IntRanges>
Position whole_range<IntRanges>::find_end (Position pos)
{
    if (not (curr < last))
        return finval;
    typename IntRanges::rng_iter prev = curr;
    off_t incr = 1;
    while ((curr + incr) < last && abs((*(curr + incr)).end) <= pos) {
        curr += incr;
        incr *= 2;
    }
    while (incr > 0) {
        if ((curr + incr) < last && abs((*(curr + incr)).end) <= pos) {
            curr += incr;
        }
        incr /= 2;
    }
    // go back out of nested ranges (end < 0)
    while (prev < curr && (*curr).end < 0)
        --curr;
    // go forward to locate the matching end
    while (curr < last && abs((*curr).end) < pos)
        ++curr;
    return (curr < last) ? (*curr).beg : finval;
}

//-------------------- part_range --------------------
template<class T>
inline T mymin (T x, T y) { return x < y ? x : y; }

template <class IntRanges>
part_range<IntRanges>::part_range (IntRanges *r, FastStream *p, int nest)
    :curr (0), rng (r), pos (p), finval (r->end_at (r->size()-1) +1),
     last (mymin (Position (r->size()), p->final())), nestval(nest) 
{
    locate();
}

template <class IntRanges>
Position part_range<IntRanges>::peek_beg () const 
{
    return (curr < last) ? rng->rngf[curr].beg : finval;
}

template <class IntRanges>
Position part_range<IntRanges>::peek_end () const
{
    return (curr < last) ? abs(rng->rngf[curr].end) : finval;
}


template <class IntRanges>
bool part_range<IntRanges>::locate()
{
    if (curr <= pos->peek())
        curr = pos->peek();
    else
        curr = pos->find (curr);
    return curr < last;
}

template <class IntRanges>
bool part_range<IntRanges>::next ()
{
    pos->next();
    return locate();
}

template <class IntRanges>
Position part_range<IntRanges>::find_end (Position pos)
{
    NumOfPos prev = curr;
    int incr = 1;
    while ((curr + incr) < last && abs(rng->rngf [curr + incr].end) <= pos) {
        curr += incr;
        incr *= 2;
    }
    while (incr > 0) {
        if ((curr + incr) < last && abs(rng->rngf [curr + incr].end) <= pos) {
            curr += incr;
        }
        incr /= 2;
    }

    // go back out of nested ranges (end < 0)
    while (prev < curr && rng->rngf [curr].end < 0)
        --curr;
    // go forward to locate the matching end
    while (curr < last && abs(rng->rngf [curr].end) < pos)
        ++curr;
    return locate() ? rng->rngf [curr].beg : finval;
}

template <class IntRanges>
Position part_range<IntRanges>::find_beg (Position pos)
{
    NumOfPos prev = curr;
    int incr = 1;
    while ((curr + incr) < last && rng->rngf [curr + incr].beg <= pos) {
        curr += incr;
        incr *= 2;
    }
    while (incr > 0) {
        if ((curr + incr) < last && rng->rngf [curr + incr].beg <= pos) {
            curr += incr;
        }
        incr /= 2;
    }
    if (rng->rngf [curr].beg < pos)
        ++curr;
    else if (prev < curr){
        NumOfPos curr_1 = curr;
        --curr_1;
        while (prev < curr && rng->rngf [curr_1].beg == pos) {
            --curr;
            --curr_1;
        }
    }
    return locate() ? rng->rngf [curr].beg : finval;
}


    

//============================================================
//==================== create_ranges =========================

ranges* create_ranges(const std::string &path, const std::string &type)
{
    if (type == "file32") {
        return new int_ranges<MapBinFile<rangeitem<int32_t> > > (path);
    } else if (type == "map32") {
        return new int_ranges<MapBinFile<rangeitem<int32_t> > > (path);
    } else if (type == "file64") {
        return new int_ranges<MapBinFile<rangeitem<int64_t> > > (path);
    } else if (type == "map64") {
        return new int_ranges<MapBinFile<rangeitem<int64_t> > > (path);
    } else
        return new int_ranges<MapBinFile<rangeitem<int32_t> > > (path);
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
