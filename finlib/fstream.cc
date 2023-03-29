// Copyright (c) 1999-2017  Pavel Rychly, Milos Jakubicek

#include "fsop.hh"
#include <algorithm>
#include <functional>
#include <iostream>


using namespace std;

//-------------------- LabeledStream --------------------
AddLabel::AddLabel (FastStream *source, int lab)
    : label (lab), src (source) {}
AddLabel::~AddLabel () {delete src;}
Position AddLabel::peek () {return src->peek();}
Position AddLabel::next () {return src->next();}
Position AddLabel::find (Position pos) {return src->find (pos);}
NumOfPos AddLabel::rest_min () {return src->rest_min();}
NumOfPos AddLabel::rest_max () {return src->rest_max();}
Position AddLabel::final () {return src->final();}
void AddLabel::add_labels (Labels &lab) {
    if (label) lab [label] = src->peek();
    else src->add_labels(lab);
}

ChangeLabel::ChangeLabel (FastStream *source, int fromlab, int tolab)
    : fromlabel (fromlab), tolabel (tolab), src (source) {}
ChangeLabel::~ChangeLabel () {delete src;}
Position ChangeLabel::peek () {return src->peek();}
Position ChangeLabel::next () {return src->next();}
Position ChangeLabel::find (Position pos) {return src->find (pos);}
NumOfPos ChangeLabel::rest_min () {return src->rest_min();}
NumOfPos ChangeLabel::rest_max () {return src->rest_max();}
Position ChangeLabel::final () {return src->final();}
void ChangeLabel::add_labels (Labels &lab) {
    Labels tmplab;
    src->add_labels(tmplab);
    if (tmplab.find (fromlabel) != tmplab.end()) {
        tmplab [tolabel] = tmplab [fromlabel];
        tmplab.erase (fromlabel);
    }
    lab.insert(tmplab.begin(), tmplab.end());
}

// -------------------- FastBuffStream --------------------

FastBuffStream::FastBuffStream (FastStream *source, int size)
    :src (source), size (size)
{
    last = current = buff = new Position[size];
    *(last++) = src->next();
}

FastBuffStream::~FastBuffStream ()
{
    delete src;
}

Position FastBuffStream::peek ()
{
    return (current != last) ? *current : src->peek();
}


Position FastBuffStream::next ()
{
    if (current == last) {
        if (last == buff + size) {
            current--;
            last--;
            memmove (buff, buff+1, sizeof (Position) * (size -1));
        }
        *last++ = src->next();
    }
    return *(current++);
}

Position FastBuffStream::find (Position pos)
{
    for (current = buff; current < last && *current < pos; current++)
        ;
    if (current == last) {
        src->find (pos - size);
        last = buff;
        while ((*(last++) = src->next()) < pos)
            ;
        current = last -1;
    }
    return *current;
}

NumOfPos FastBuffStream::rest_min ()
{
    return (last - current) + src->rest_min();
}

NumOfPos FastBuffStream::rest_max ()
{
    return (last - current) + src->rest_max();
}

Position FastBuffStream::final ()
{
    return src->final();
}

// -------------------- FileFastStream --------------------


Position FileFastStream::peek ()
{
    return (rest > 0) ? file [current] : finval;
}

Position FileFastStream::next ()
{
    if (rest > 0) {
        rest--;
        return file[current++];
    } else {
        return finval;
    }
}

Position FileFastStream::find (Position pos)
{
    NumOfPos incr = 1;
    while (incr < rest && (file [current + incr]) <= pos) {
        current += incr;
        rest -= incr;
        incr *= 2;
    }
    while (incr > 0) {
        if (incr < rest && (file [current + incr]) <= pos) {
            current += incr;
            rest -= incr;
        }
        incr /= 2;
    }
    if (file [current] < pos) {
        current ++; 
        rest --;
    }
    if (rest > 0) {
        return file [current];
    } else {
        return finval;
    }
}

NumOfPos FileFastStream::rest_min ()
{
    return rest;
}

NumOfPos FileFastStream::rest_max ()
{
    return rest;
}

Position FileFastStream::final ()
{
    return finval;
}

// -------------------- Query operations --------------------
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


// -------------------- QNotNode --------------------

QNotNode::QNotNode (FastStream *source, Position finval)
    :src (source), current(0), srcnext (src->next()), srcfin (src->final()),
     nodefin (finval ? finval : srcfin)
{
    updatecurrent();
}

QNotNode::~QNotNode ()
{
    delete src;
}

void QNotNode::updatecurrent ()
{
    if (current == nodefin)
        return;
    while (srcnext == current && current < nodefin) {
        current++;
        srcnext = src->next();
    }
    if (current > nodefin)
        current = nodefin;
}

Position QNotNode::peek ()
{
    return current;
}

Position QNotNode::next ()
{
    Position ret = current;
    current++;
    updatecurrent();
    return ret;
}

Position QNotNode::find (Position pos)
{
    if (pos <= current) return current;
    if (pos >= nodefin) return current = nodefin;
    if (pos < srcnext) return current = pos;
    if (pos > srcnext) {
        src->find (pos);
        srcnext = src->next();
    }
    current = pos;
    if (srcnext == pos) {
        updatecurrent();
    }
    return current;
}

NumOfPos QNotNode::rest_min ()
{
    return inline_max (Position(0), nodefin - current - src->rest_max());
}

NumOfPos QNotNode::rest_max ()
{
    if (nodefin >= srcfin)
        return nodefin - current - src->rest_max();
    else 
        return nodefin - current 
            - inline_max (Position(0), src->rest_min() - srcfin + nodefin);
}

Position QNotNode::final ()
{
    return nodefin;
}


// -------------------- QAndNode --------------------

QAndNode::QAndNode (FastStream *source1, FastStream *source2)
{
    if (source1->final() <= source2->final()) {
        src[0] = source1;
        src[1] = source2;
    } else {
        src[0] = source2;
        src[1] = source1;
    }
    finval = src[0]->final();
    last_peek = -1;
}

QAndNode::~QAndNode ()
{
    delete src[0];
    delete src[1];
}

Position QAndNode::peek ()
{
    if (last_peek != -1)
        return last_peek;
    Position v0 = src [0]->peek(), v1 = src [1]->peek();
    while (v0 != v1 && v0 < finval) {
        if (v0 < v1)
            v0 = src [0]->find (v1);
        else
            v1 = src [1]->find (v0);
    }
    return v0;
}

Position QAndNode::next ()
{
    Position curr = peek();
    Labels tmp;
    src[1]->add_labels (tmp);
    if (last_peek == -1) {
        src[1]->next();
        if (src[1]->peek() == curr)
            return curr;
    } else {
        last_peek = -1;
        last_labels.clear();
    }
    src[0]->next();
    if (src[0]->peek() == curr) {
        last_peek = curr;
        last_labels = tmp;
    }
    return curr;
}

Position QAndNode::find (Position pos)
{
    src [0]->find (pos);
    last_peek = -1;
    last_labels.clear();
    return peek();
}

NumOfPos QAndNode::rest_min ()
{
    return 0;
}

NumOfPos QAndNode::rest_max ()
{
    return inline_min (src[0]->rest_max(), src[1]->rest_max());
}

Position QAndNode::final ()
{
    return finval;
}

void QAndNode::add_labels (Labels &lab)
{
    src[0]->add_labels (lab);
    if (last_peek == -1)
        src[1]->add_labels (lab);
    else
        lab.insert(last_labels.begin(), last_labels.end());
}


// -------------------- QOrNode --------------------

QOrNode::QOrNode (FastStream *source1, FastStream *source2)
{
    if (source1->final() >= source2->final()) {
        src[0] = source1;
        src[1] = source2;
    } else {
        src[0] = source2;
        src[1] = source1;
    }
    peek_s[0] = src[0]->peek();
    peek_s[1] = src[1]->peek();
    finval[0] = src[0]->final();
    finval[1] = src[1]->final();
    updatefirst();
}

QOrNode::~QOrNode ()
{
    delete src[0];
    delete src[1];
}

inline void QOrNode::updatefirst()
{
    first = (peek_s[0] > peek_s[1] && peek_s[1] < finval[1]);
}

Position QOrNode::peek ()
{
    return peek_s [first];
}

Position QOrNode::next ()
{
    Position ret = peek_s [first];
    src [first]->next();
    peek_s [first] = src [first]->peek();
    if (peek_s [!first] == ret) {
        src [!first]->next();
        peek_s [!first] = src [!first]->peek();
    }
    updatefirst();
    return ret;
}

Position QOrNode::find (Position pos)
{
    peek_s[0] = src [0]->find (pos);
    peek_s[1] = src [1]->find (pos);
    updatefirst();
    return peek_s [first];
}

NumOfPos QOrNode::rest_min ()
{
    return inline_max (src[0]->rest_min(), src[1]->rest_min());
}

NumOfPos QOrNode::rest_max ()
{
    return src[0]->rest_max() + src[1]->rest_max();
}

Position QOrNode::final ()
{
    return finval[0];
}

void QOrNode::add_labels (Labels &lab) 
{
    src[first]->add_labels(lab);
    if (peek_s[!first] < finval[!first] && peek_s[0] == peek_s[1])
        src[!first]->add_labels(lab);
}

// -------------------- QOrVNode --------------------

struct less_peek_struct
    : public binary_function<pos_stream_pair,pos_stream_pair, bool> {
    bool operator() (const pos_stream_pair &a, 
                     const pos_stream_pair &b)
        const {return a.first < b.first;}
};

static less_peek_struct less_peek;

QOrVNode::QOrVNode (vector<FastStream*> *source, bool skip_duplicates)
    : skip (skip_duplicates), src (), finval (0)
{
    for (vector<FastStream*>::iterator i=source->begin(); 
         i < source->end(); ++i) {
        if ((*i)->peek() >= (*i)->final()) {
            delete *i;
            continue;
        }
        src.push_back (pos_stream_pair ((*i)->peek(), *i));
        if ((*i)->final() > finval)
            finval = (*i)->final();
    }
    delete source;
    if (src.empty()) {
        FastStream *dummy = new EmptyStream();
        src.push_back (pos_stream_pair (dummy->peek(), dummy));
    }
    make_heap (src.begin(), src.end(), not2(less_peek));
}

QOrVNode::~QOrVNode ()
{
    for (pss_vector::iterator i=src.begin(); i < src.end(); ++i)
        delete (*i).second;
}

inline void QOrVNode::updatefirst()
{
    size_t len = src.size();
    pos_stream_pair first = src[0];
    Position value = first.second->peek();
    if (value >= first.second->final())
        value = finval;
    first.first = value;
    size_t hole = 0;
    size_t child = 2;
    while (child <= len) {
        if ((child == len) || less_peek (src[child -1], src[child]))
            child--;
        if (src[child].first >= value)
            break;
        src[hole] = src[child];
        hole = child;
        child = 2 * child + 2;
    }
    src[hole] = first;
}

Position QOrVNode::peek ()
{
    return src [0].first;
}

Position QOrVNode::next ()
{
    Position ret = src [0].second->next();
    updatefirst();
    while (skip && src[0].first == ret && src[0].first < finval) {
        //throw out the same values
        src[0].second->next();
        updatefirst();
    }
    return ret;
}

Position QOrVNode::find (Position pos)
{
    do {
        src[0].second->find (pos);
        updatefirst();
    } while (pos > src[0].first && src[0].first < finval);
    return src[0].first;
}



NumOfPos QOrVNode::rest_min ()
{
    NumOfPos r, result = 0;
    pss_vector::iterator first = src.begin();
    pss_vector::iterator last = src.end();
    while (first != last) {
        if (result < (r = (*first++).second->rest_max()))
            result = r;
    }
    return result;
}

NumOfPos QOrVNode::rest_max ()
{
    NumOfPos sum = 0;
    pss_vector::iterator first = src.begin();
    pss_vector::iterator last = src.end();
    while (first != last) {
        sum += (*first++).second->rest_max();
    }
    return sum;
}

Position QOrVNode::final ()
{
    return finval;
}

void QOrVNode::add_labels (Labels &lab)
{
    src[0].second->add_labels(lab);
    for (unsigned int i = 1;
         skip && i < src.size() && src[0].first == src[i].first; i++)
        src[i].second->add_labels(lab);
}

FastStream * QOrVNode::create (std::vector<FastStream*> *fsv,
                               bool skip_duplicates)
{
    switch (fsv->size()) {
    case 0:
        delete fsv;
        return new EmptyStream();
    case 1: {
        FastStream *f = fsv->front();
        delete fsv;
        return f;
    }
    default:
        return new QOrVNode (fsv, skip_duplicates);
    }
}

// -------------------- QMoveNode --------------------

QMoveNode::QMoveNode (FastStream *source, int delta)
    : src (source), delta (delta), finval (source->final())
{
    
    if (delta < 0) {
        while (src->peek() + delta < 0 && src->rest_max() > 0)
            src->next();
    }
    updatecurrent();
}

QMoveNode::~QMoveNode ()
{
    delete src;
}

inline void QMoveNode::updatecurrent ()
{
    Position p = src->peek();
    if (p == finval || p + delta >= finval)
        current = finval;
    else
        current = p + delta;
}

Position QMoveNode::peek ()
{
    return current;
}

Position QMoveNode::next ()
{
    Position ret = current;
    src->next();
    updatecurrent();
    return ret;
}

Position QMoveNode::find (Position pos)
{
    src->find (pos - delta);
    updatecurrent();
    return current;
}

NumOfPos QMoveNode::rest_min ()
{
    if (current == finval)
        return 0;
    if (delta < 0) 
        return src->rest_min();
    else 
        return inline_max (Position(1), src->rest_min() - delta);
}

NumOfPos QMoveNode::rest_max ()
{
    return src->rest_max();
}

Position QMoveNode::final ()
{
    return finval;
}

void QMoveNode::add_labels (Labels &lab)
{
    src->add_labels(lab);
}

// -------------------- QFilterNode --------------------

QFilterNode::QFilterNode (FastStream *source)
    : src (source), finval (src->final())
{
}

QFilterNode::~QFilterNode ()
{
    delete src;
}

void QFilterNode::updatecurrent()
{
    do {
        current = src->next();
    } while (current < finval && !correct_pos (current));
}

Position QFilterNode::peek ()
{
    return current;
}

Position QFilterNode::next ()
{
    Position ret = current;
    updatecurrent();
    return ret;
}

Position QFilterNode::find (Position pos)
{
    src->find (pos);
    updatecurrent();
    return current;
}

NumOfPos QFilterNode::rest_min ()
{
    return current < finval ? 1 : 0;
}

NumOfPos QFilterNode::rest_max ()
{
    return src->rest_max() + rest_min();
}

Position QFilterNode::final ()
{
    return finval;
}

// -------------------- QFilterNode --------------------

QFilterFnNode::QFilterFnNode (FastStream *source, filterfn *fn)
    : QFilterNode (source), corrfn (fn)
{
    updatecurrent();
}

QFilterFnNode::~QFilterFnNode ()
{
}

bool QFilterFnNode::correct_pos (Position pos)
{
    return corrfn (pos);
}

//-------------------- SwapKwicColl --------------------

#define MAX_COLL_DISTANCE 128

void SwapKwicColl::updatefirst()
{
    while (src->peek() < finval &&
           (que.empty() || que.top().coll + MAX_COLL_DISTANCE < src->peek())) {
        PosLabel pl (src->peek());
        src->add_labels (pl.lab);
        Labels::const_iterator it = pl.lab.find (collnum);
        src->next();
        if (it == pl.lab.end())
            continue;
        pl.coll = it->second;
        que.push (pl);
    }
    if (que.empty())
        que.push (PosLabel (finval));
}

Position SwapKwicColl::next ()
{
    Position prev = que.top().coll;
    if (prev == finval)
        return finval;
    do {
        que.pop();
    } while (!que.empty() && prev == que.top().coll);
    updatefirst();
    return que.top().coll;
}

Position SwapKwicColl::peek ()
{
    return que.top().coll;
}

void SwapKwicColl::add_labels (Labels &lab)
{
    const Labels &l = que.top().lab;
    lab.insert (l.begin(), l.end());
    lab[collnum] = que.top().pos; // switch coll to KWIC
}

Position SwapKwicColl::find (Position pos)
{
    while (peek() < pos) next();
    return peek();
}

NumOfPos SwapKwicColl::rest_min ()
{
    return src->rest_min() + que.size();
}

NumOfPos SwapKwicColl::rest_max ()
{
    return src->rest_max() + que.size();
}

Position SwapKwicColl::final ()
{
    return finval;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
