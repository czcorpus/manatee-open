// Copyright (c) 1999-2015  Pavel Rychly, Milos Jakubicek

#include "frstream.hh"
#include "frsop.hh"
#include <iostream>

using namespace std;

template <class T>
T inline_max (T a, T b)
{
    return a >= b ? a : b;
}

template <class T>
T inline_min (T a, T b)
{
    return a <= b ? a : b;
}

//-------------------- Pos2Range --------------------
bool Pos2Range::next ()
{
    in->next(); return in->peek() < finin;
}

Position Pos2Range::peek_beg () const 
{
    Position p = in->peek();
    return p < finin ? p + delta_beg : finout;
}

Position Pos2Range::peek_end () const 
{
    Position p = in->peek();
    return p < finin ? p + delta_end : finout;
}

void Pos2Range::add_labels (Labels &lab) const
{
    in->add_labels (lab);
}

Position Pos2Range::find_beg (Position pos) 
{
    Position p = in->find (pos - delta_beg);
    return p < finin ? p + delta_beg : finout;
}

Position Pos2Range::find_end (Position pos) 
{
    Position p = in->find (pos - delta_end);
    return p < finin ? p + delta_beg : finout;
}

NumOfPos Pos2Range::rest_min () const 
{
    return in->rest_min();
}

NumOfPos Pos2Range::rest_max () const 
{
    return in->rest_max();
}

Position Pos2Range::final () const 
{
    return finout;
}

int Pos2Range::nesting() const 
{
    return 0;
}

bool Pos2Range::epsilon() const
{
    return false;
}

//-------------------- RQinNode --------------------

RQinNode::RQinNode (RangeStream *in, RangeStream *out, bool do_locate)
    :inside (in), outside (out), infin (in->final()), outfin (out->final()),
     finished (false)
{
    if (do_locate)
        locate();
}

Position RQinNode::locate ()
{
    if (finished)
        return infin;
    while (true) {
        if (inside->peek_beg() >= infin || outside->peek_beg() >= outfin) {
            finished = true;
            return infin;
        }
        if (inside->peek_beg() < outside->peek_beg()) {
            inside->find_beg (outside->peek_beg());
            continue;
        }
        if (inside->peek_end() > outside->peek_end()) {
            outside->find_end (inside->peek_end());
            continue;
        } else
            return inside->peek_beg();
    }
}

bool RQinNode::next ()
{
    if (finished)
        return false;
    inside->next();
    return locate() < infin;
}

Position RQinNode::peek_beg () const
{
    return finished ? infin : inside->peek_beg();
}

Position RQinNode::peek_end () const
{
    return finished ? infin : inside->peek_end();
}

void RQinNode::add_labels (Labels &lab) const
{
    outside->add_labels (lab);
    inside->add_labels (lab);
}

Position RQinNode::find_beg (Position pos)
{
    if (finished)
        return infin;
    inside->find_beg (pos);
    return locate();
}

Position RQinNode::find_end (Position pos)
{
    if (finished)
        return infin;
    inside->find_end (pos);
    return locate();
}

NumOfPos RQinNode::rest_min () const
{
    return 0;
}

NumOfPos RQinNode::rest_max () const
{
    return inside->rest_max();
}

Position RQinNode::final () const
{
    return infin;
}

int RQinNode::nesting() const
{
    return inside->nesting();
}

bool RQinNode::epsilon() const
{
    return inside->epsilon();
}

//-------------------- RQnotInNode --------------------

RQnotInNode::RQnotInNode (RangeStream *in, RangeStream *out)
    :RQinNode (in, out, false)
{
    locate();
}

Position RQnotInNode::locate ()
{
    if (finished)
        return infin;
    while (true) {
        if (inside->peek_beg() >= infin) {
            finished = true;
            return infin;
        }
        if (!outside->end() && outside->peek_end() < inside->peek_end()) {
            outside->find_end (inside->peek_end());
            continue;
        }
        if (!outside->end() && inside->peek_beg() >= outside->peek_beg()
                            && inside->peek_end() <= outside->peek_end()) {
            inside->next();
            continue;
        } else
            return inside->peek_beg();
    }
}

//-------------------- RQoutsideNode --------------------

RQoutsideNode::RQoutsideNode (RangeStream *rs, Position maxend)
    :src (rs), finval (maxend + 1), curr_beg (0), curr_end (0)
{
    locate();
}

void RQoutsideNode::locate ()
{
    while (!src->end() && src->peek_beg() <= curr_end) {
        curr_end = inline_max (curr_end, src->peek_end());
        src->next();
    }
    if (src->end()) {
        curr_beg = curr_end;
        curr_end = finval - 1;
    } else {
        curr_beg = curr_end;
        curr_end = src->peek_beg();
    }
}

bool RQoutsideNode::next ()
{
    if (curr_end >= finval - 1) {
        curr_beg = curr_end = finval;
        return false;
    }
    curr_beg = curr_end;
    curr_end = src->peek_end();
    src->next();
    locate();
    return curr_beg < finval;
}

Position RQoutsideNode::peek_beg () const
{
    return curr_beg;
}

Position RQoutsideNode::peek_end () const
{
    return curr_end;
}

void RQoutsideNode::add_labels (Labels &lab) const
{
}

Position RQoutsideNode::find_beg (Position pos)
{
    while (curr_beg < pos && curr_beg < finval)
        next();
    return curr_beg;
}

Position RQoutsideNode::find_end (Position pos)
{
    while (curr_end < pos && curr_beg < finval)
        next();
    return curr_end;
}

NumOfPos RQoutsideNode::rest_min () const
{
    return 0;
}

NumOfPos RQoutsideNode::rest_max () const
{
    return src->rest_max() +1;
}

Position RQoutsideNode::final () const
{
    return finval;
}

int RQoutsideNode::nesting() const
{
    return 1;
}

bool RQoutsideNode::epsilon() const
{
    return false;
}

//-------------------- RQcontainNode --------------------

RQcontainNode::RQcontainNode (RangeStream *out, RangeStream *in, bool do_locate)
    :inside (in), outside (out), infin (in->final()), outfin (out->final()),
     finished (false)
{
    if (do_locate)
        locate();
}

Position RQcontainNode::locate ()
{
    if (finished)
        return outfin;
    while (true) {
        if (inside->peek_beg() >= infin || outside->peek_beg() >= outfin) {
            finished = true;
            return outfin;
        }
        if (inside->peek_beg() < outside->peek_beg()) {
            inside->find_beg (outside->peek_beg());
            continue;
        }
        if (inside->peek_end() > outside->peek_end()) {
            outside->find_end (inside->peek_end());
            continue;
        } else
            return outside->peek_beg();
    }
}

bool RQcontainNode::next ()
{
    if (finished)
        return false;
    outside->next();
    return locate() < outfin;
}

Position RQcontainNode::peek_beg () const
{
    return finished ? outfin : outside->peek_beg();
}

Position RQcontainNode::peek_end () const
{
    return finished ? outfin : outside->peek_end();
}

void RQcontainNode::add_labels (Labels &lab) const
{
    inside->add_labels (lab);
    outside->add_labels (lab);
}

Position RQcontainNode::find_beg (Position pos)
{
    if (finished)
        return outfin;
    outside->find_beg (pos);
    return locate();
}

Position RQcontainNode::find_end (Position pos)
{
    if (finished)
        return outfin;
    outside->find_end (pos);
    return locate();
}

NumOfPos RQcontainNode::rest_min () const
{
    return 0;
}

NumOfPos RQcontainNode::rest_max () const
{
    return outside->rest_max();
}

Position RQcontainNode::final () const
{
    return outfin;
}

int RQcontainNode::nesting() const
{
    return outside->nesting();
}

bool RQcontainNode::epsilon() const
{
    return outside->epsilon();
}

//-------------------- RQnotContainNode --------------------

RQnotContainNode::RQnotContainNode (RangeStream *out, RangeStream *in)
    :RQcontainNode (out, in, false)
{
    locate();
}

Position RQnotContainNode::locate ()
{
    if (finished)
        return outfin;
    while (true) {
        if (outside->peek_beg() >= outfin) {
            finished = true;
            return outfin;
        }
        if (!inside->end() && inside->peek_beg() < outside->peek_beg()) {
            inside->find_beg (outside->peek_beg());
            continue;
        }
        if (!inside->end() && inside->peek_beg() >= outside->peek_beg()
                           && inside->peek_end() <= outside->peek_end()) {
            outside->next();
            continue;
        } else
            return outside->peek_beg();
    }
}

Position RQnotContainNode::find_end (Position pos)
{
    if (finished)
        return outfin;
    outside->find_end (pos);
    return locate();
}

//-------------------- RQUnionNode --------------------

inline void RQUnionNode::updatefirst()
{
    peek_s[0].beg = src[0]->peek_beg();
    peek_s[0].end = src[0]->peek_end();
    peek_s[1].beg = src[1]->peek_beg();
    peek_s[1].end = src[1]->peek_end();
    first = (peek_s[1] < peek_s[0] && peek_s[1].beg < finval[1]);
}

RQUnionNode::RQUnionNode (RangeStream *s1, RangeStream *s2)
{
    if (s1->final() >= s2->final()) {
        src[0] = s1;
        src[1] = s2;
    } else {
        src[0] = s2;
        src[1] = s1;
    }
    finval[0] = src[0]->final();
    finval[1] = src[1]->final();
    updatefirst();
}

bool RQUnionNode::next ()
{
    if (peek_s[0] == peek_s[1])
        src [!first]->next();
    src [first]->next();
    updatefirst();
    return peek_s [first].beg < finval [first];
}

Position RQUnionNode::peek_beg () const
{
    return peek_s [first].beg;
}

Position RQUnionNode::peek_end () const
{
    return peek_s [first].end;
}

void RQUnionNode::add_labels (Labels &lab) const
{
    src [first]->add_labels (lab);
    if (peek_s [!first].beg < finval [!first] && peek_s[0] == peek_s[1])
        src [!first]->add_labels(lab);
}

Position RQUnionNode::find_beg (Position pos)
{
    src [0]->find_beg (pos);
    src [1]->find_beg (pos);
    updatefirst();
    return peek_s [first].beg;
}

Position RQUnionNode::find_end (Position pos)
{
    src [0]->find_end (pos);
    src [1]->find_end (pos);
    updatefirst();
    return peek_s [first].beg;
}

NumOfPos RQUnionNode::rest_min () const
{
    return inline_max (src[0]->rest_min(), src[1]->rest_min());
}

NumOfPos RQUnionNode::rest_max () const
{
    return src[0]->rest_max() + src[1]->rest_max();
}

Position RQUnionNode::final () const
{
    return finval[0];
}

int RQUnionNode::nesting() const
{
    //XXX
    return src[0]->nesting() + src[0]->nesting();
}

bool RQUnionNode::epsilon() const
{
    return src[0]->epsilon() || src[1]->epsilon();
}

//-------------------- RQRepeatFSNode --------------------

RQRepeatFSNode::RQRepeatFSNode (FastStream *s, int minrep, int maxrep)
    :src (s), finval (src->final()), minr (minrep), maxr (maxrep), 
    repbeg(0), repend(0), nextend(0), epsilonval (false)
{
    if (minr == 0) {
        epsilonval = true;
        minr = 1;
    }
    if (maxr == -1)
        maxr = MAX_RANGE_SIZE;
    if (maxr < minr)
        maxr = minr;
    locate();
}

void RQRepeatFSNode::locate ()
{
    while (repbeg < finval && repbeg + minr > repend) {
        repbeg = repend = src->next();
        while (++repend == src->peek())
            src->next();
    }
    nextend = repbeg + minr;
}

bool RQRepeatFSNode::next ()
{
    if (nextend < repend && repbeg + maxr > nextend)
        nextend ++;
    else {
        if (repend == src->peek()) {
            repend++;
            src->next();
        }
        if (repbeg + minr < repend) {
            repbeg++;
            nextend = repbeg + minr;
        } else {
            repbeg = repend;
            locate();
        }
    }
    return repbeg < finval;
}

Position RQRepeatFSNode::find_beg (Position pos)
{
    if (pos <= repbeg)
        return repbeg;
    if (pos + minr <= repend) {
        repbeg = pos;
        nextend = repbeg + minr;
    } else {
        repbeg = src->find (pos);
        locate();
    }
    return repbeg;
}

Position RQRepeatFSNode::find_end (Position pos) {
    Position beg = find_beg (pos - maxr);
    if (peek_end() < pos && pos <= repend)
        nextend = pos;
    return beg;
}

Position RQRepeatFSNode::peek_beg () const {return repbeg;}
Position RQRepeatFSNode::peek_end () const {return nextend;}
void RQRepeatFSNode::add_labels (Labels &lab) const {}
NumOfPos RQRepeatFSNode::rest_min () const {return 0;}
NumOfPos RQRepeatFSNode::rest_max () const {
    return (src->rest_max() - minr) * (maxr - minr +1);
}

Position RQRepeatFSNode::final () const {return finval;}
int RQRepeatFSNode::nesting() const {return maxr - minr;}
bool RQRepeatFSNode::epsilon() const {return epsilonval;}

//-------------------- RQRepeatNode --------------------
RQRepeatNode::RQRepeatNode (RangeStream *s, int minrep, int maxrep)
    : src (s), finval (src->final()), minr (minrep), maxr (maxrep), 
     epsilonval (false)
{
    if (minr == 0 || src->epsilon()) {
        epsilonval = true;
        minr = 1;
    }
    if (maxr == -1)
        maxr = MAX_RANGE_SIZE;
    if (maxr < minr)
        maxr = minr;
    locate();
}

void RQRepeatNode::search_pool (Position start, int currrep)
{
    map<Position,PosList>::const_iterator i = pool.find (start);
    if (i == pool.end())
        return;
    for (PosList::const_iterator j = (*i).second.begin();
         j != (*i).second.end(); j++) {
        if (currrep >= minr)
            output.insert (*j);
        if (currrep < maxr && *j > start)
            search_pool (*j, currrep +1);
    }
}

void RQRepeatNode::locate()
{
    Position b = src->peek_beg();
    while (output.empty() && (b != finval || !begs.empty())) {
        if (begs.empty())
            begs.push (b);
        // fill up pool
        while (b != finval && b <= begs.front() + MAX_RANGE_SIZE) {
            if (begs.back() != b)
                begs.push (b);
            pool[b].push_back (src->peek_end());
            src->next();
            b = src->peek_beg();
        }
    
        // continuity test
        search_pool (begs.front(), 1);

        // cut pool
        pool.erase (begs.front());
        if (output.empty())
            begs.pop();
    }
}

bool RQRepeatNode::next()
{
    if (output.empty())
        return false;
    else {
        output.erase (output.begin());
        if (output.empty()) {
            begs.pop();
            locate();
        }
        return !output.empty();
    }
}

Position RQRepeatNode::find_beg (Position pos)
{
    if (output.empty())
        return finval;
    if (pos <= begs.front())
        return pos;
    
    output.clear();
    if (pos > begs.back()) {
        begs = queue<Position>();
        pool.clear();
        src->find_beg (pos);
    } else {
        while (begs.front() < pos) {
            pool.erase (begs.front());
            begs.pop();
        }
    }
    locate();
    return peek_beg();
}

Position RQRepeatNode::find_end (Position pos) {
    find_beg (pos - MAX_RANGE_SIZE);
    while (!output.empty() && *output.begin() < pos)
        next();
    return peek_beg();
}

Position RQRepeatNode::peek_beg () const {
    return output.empty() ? finval : begs.front();
}

Position RQRepeatNode::peek_end () const {
    return output.empty() ? finval : *output.begin();
}

NumOfPos RQRepeatNode::rest_min () const {
    return output.size() + (minr > 1 ? 0 : src->rest_min());
}

NumOfPos RQRepeatNode::rest_max () const {
    /// XXX + pool
    return output.size() + (src->rest_max() - minr) * (maxr - minr +1);
}

Position RQRepeatNode::final () const {return finval;}
int RQRepeatNode::nesting() const {return maxr - minr;} // XXX
bool RQRepeatNode::epsilon() const {return epsilonval;}
void RQRepeatNode::add_labels (Labels &lab) const {}


//-------------------- RQOptionalNode --------------------
bool RQOptionalNode::next () {return src->next();}
Position RQOptionalNode::peek_beg () const {return src->peek_beg();}
Position RQOptionalNode::peek_end () const {return src->peek_end();}
void RQOptionalNode::add_labels (Labels &lab) const {src->add_labels (lab);}
Position RQOptionalNode::find_beg (Position pos) {return src->find_beg(pos);}
Position RQOptionalNode::find_end (Position pos) {return src->find_end(pos);}
NumOfPos RQOptionalNode::rest_min () const {return src->rest_min();}
NumOfPos RQOptionalNode::rest_max () const {return src->rest_max();}
Position RQOptionalNode::final () const {return src->final();}
int RQOptionalNode::nesting() const {return src->nesting();}
bool RQOptionalNode::epsilon() const {return true;}


//-------------------- BegsOfRStream --------------------
Position BegsOfRStream::peek () {return in->peek_beg();}
NumOfPos BegsOfRStream::rest_min () {return in->rest_min();}
NumOfPos BegsOfRStream::rest_max () {return in->rest_max();}
Position BegsOfRStream::final () {return in->final();}
void BegsOfRStream::add_labels (Labels &lab) {return in->add_labels(lab);}

Position BegsOfRStream::next () 
{
    Position p = in->peek_beg(); 
    in->next(); 
    return p;
}

Position BegsOfRStream::find (Position pos)
{
    return in->find_beg (pos);
}

//-------------------- EndsOfRStream --------------------
Position EndsOfRStream::peek () {return in->peek_end();}
NumOfPos EndsOfRStream::rest_min () {return in->rest_min();}
NumOfPos EndsOfRStream::rest_max () {return in->rest_max();}
Position EndsOfRStream::final () {return in->final();}

Position EndsOfRStream::next () 
{
    Position p = in->peek_end(); 
    in->next(); 
    return p;
}

Position EndsOfRStream::find (Position pos)
{
    in->find_end (pos);
    if (pos > in->final())
        pos = in->final();
    while (in->peek_end() < pos)
        in->next();
    return in->peek_end();
}

//--------------------- AddRSLabel --------------------
bool AddRSLabel::next () {return src->next();}
Position AddRSLabel::peek_beg () const {return src->peek_beg();}
Position AddRSLabel::peek_end () const {return src->peek_end();}
void AddRSLabel::add_labels (Labels &lab) const {
    if (label) {
        lab [label] = src->peek_beg();
        lab [-label] = src->peek_end();
    }
}
Position AddRSLabel::find_beg (Position pos) {return src->find_beg (pos);}
Position AddRSLabel::find_end (Position pos) {return src->find_end (pos);}
NumOfPos AddRSLabel::rest_min () const {return src->rest_min();}
NumOfPos AddRSLabel::rest_max () const {return src->rest_max();}
Position AddRSLabel::final () const {return src->final();}
int AddRSLabel::nesting() const {return src->nesting();}
bool AddRSLabel::epsilon() const {return src->epsilon();}


//--------------------- NonEmptyRS --------------------
void NonEmptyRS::skip_empty() {
    while (src->peek_beg() == src->peek_end() && !src->end())
        src->next();
}
bool NonEmptyRS::next () {src->next(); skip_empty(); return !src->end();}
Position NonEmptyRS::peek_beg () const {return src->peek_beg();}
Position NonEmptyRS::peek_end () const {return src->peek_end();}
Position NonEmptyRS::find_beg (Position pos) {src->find_beg (pos); skip_empty(); return src->peek_beg();}
Position NonEmptyRS::find_end (Position pos) {src->find_end (pos); skip_empty(); return src->peek_end();}
NumOfPos NonEmptyRS::rest_min () const {return src->rest_min();}
NumOfPos NonEmptyRS::rest_max () const {return src->rest_max();}
Position NonEmptyRS::final () const {return src->final();}
void NonEmptyRS::add_labels (Labels &lab) const {src->add_labels (lab);}
int NonEmptyRS::nesting() const {return src->nesting();}
bool NonEmptyRS::epsilon() const {return src->epsilon();}

//--------------------- FlattenRS --------------------
bool FlattenRS::next() {
    if(src->end()) {
        if(!end()) curbeg = curend = final();
        return false;
    }

    curbeg = src->peek_beg();
    curend = src->peek_end();

    while(!src->end()) {
        if(src->peek_beg() > curend) break;
        else if(src->peek_end() > curend)
            curend = src->peek_end();
        src->next();
    }

    return true;
}

Position FlattenRS::peek_beg () const {return curbeg;}
Position FlattenRS::peek_end () const {return curend;}
Position FlattenRS::find_beg (Position pos) {throw std::logic_error("not implemented");}
Position FlattenRS::find_end (Position pos) {throw std::logic_error("not implemented");}
NumOfPos FlattenRS::rest_min () const {return 1;}
NumOfPos FlattenRS::rest_max () const {return src->rest_max();}
Position FlattenRS::final () const {return src->final();}
void FlattenRS::add_labels (Labels &lab) const {throw std::logic_error("not implemented");}
int FlattenRS::nesting() const {return 1;}
bool FlattenRS::epsilon() const {return src->epsilon();}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
