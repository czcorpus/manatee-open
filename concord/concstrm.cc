//  Copyright (c) 1999-2016  Pavel Rychly, Milos Jakubicek

#include "concord.hh"

class ConcStream : public RangeStream {
    Concordance *conc;
    bool useview;
    ConcIndex curr, endidx;
    Position finval;
public:
    ConcStream (Concordance *conc, bool useview = false,
                ConcIndex beg = 0, ConcIndex end = 0);
    virtual ~ConcStream () {}
    virtual bool next () {return ++curr < endidx;}
    virtual Position peek_beg () const;
    virtual Position peek_end () const;
    virtual Position find_beg (Position pos);
    virtual Position find_end (Position pos);
    virtual NumOfPos rest_min () const {return endidx - curr;}
    virtual NumOfPos rest_max () const {return endidx - curr;}
    virtual Position final () const {return finval;}
    virtual int nesting() const {return conc->nesting();}
    virtual bool epsilon() const {return false;}
    virtual void add_labels (Labels &lab) const;
    virtual bool end() const {return curr >= endidx;}
protected:
    virtual Position get_curr (Position pos = -1) const;
};

ConcStream::ConcStream (Concordance *conc, bool useview,
                        ConcIndex beg, ConcIndex end)
    : conc (conc), useview (useview && conc->view), 
      curr (beg), endidx (end), finval (conc->corp->size())
{
    if (!endidx || endidx > conc->size())
        endidx = conc->size();
    if (useview && endidx > conc->viewsize())
        endidx = conc->viewsize();
}

Position ConcStream::peek_beg () const
{
    return curr >= endidx ? finval : conc->beg_at (get_curr());
}

Position ConcStream::peek_end () const
{
    return curr >= endidx ? finval : conc->end_at (get_curr());
}

Position ConcStream::find_beg (Position pos)
{
    if (curr >= endidx)
        return finval;
    NumOfPos incr = 1;
    Position prev = curr;
    while ((curr + incr) < endidx && conc->beg_at (get_curr (curr + incr)) <= pos) {
        curr += incr;
        incr *= 2;
    }
    while (incr > 0) {
        if ((curr + incr) < endidx && conc->beg_at (get_curr (curr + incr)) <= pos) {
            curr += incr;
        }
        incr /= 2;
    }
    if (conc->beg_at (get_curr()) < pos)
        ++curr;
    else if (prev < curr){
        Position curr_1 = curr;
        --curr_1;
        while (prev < curr && conc->beg_at (get_curr (curr_1)) == pos) {
            --curr;
            --curr_1;
        }
    }
    return (curr < endidx) ? conc->beg_at (get_curr()) : finval;
}

Position ConcStream::find_end (Position pos)
{
    if (curr >= endidx)
        return finval;
    NumOfPos incr = 1;
    while ((curr + incr) < endidx && conc->end_at (get_curr (curr + incr)) <= pos) {
        curr += incr;
        incr *= 2;
    }
    while (incr > 0) {
        if ((curr + incr) < endidx && conc->end_at (get_curr (curr + incr)) <= pos) {
            curr += incr;
        }
        incr /= 2;
    }
    if (conc->end_at (get_curr()) < pos)
        ++curr;
    return (curr < endidx) ? conc->beg_at (get_curr()) : finval;
}

void ConcStream::add_labels (Labels &lab) const
{
    for (int i = 0; i < conc->numofcolls(); i++) {
        Position pos = conc->coll_beg_at (i + 1, get_curr());
        if (pos != -1)
            lab[i + 1] = pos;
        pos = conc->coll_end_at (i + 1, get_curr());
        if (pos != -1)
            lab[-i - 1] = pos;
    }
    if (conc->linegroup)
        lab[Concordance::lngroup_labidx] = conc->get_linegroup (get_curr());
}

inline Position ConcStream::get_curr (Position pos) const
{
    if (pos == -1)
        pos = curr;
    if (useview)
        return (*conc->view)[pos];
    return pos;
}

RangeStream * Concordance::RS (bool useview, ConcIndex beg, ConcIndex end)
{
    return new ConcStream (this, useview, beg, end);
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
