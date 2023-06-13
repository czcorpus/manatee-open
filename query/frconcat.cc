// Copyright (c) 1999-2011  Pavel Rychly, Milos Jakubicek

#include <assert.h>
#include "frsop.hh"

//XXXX vsechny peek_* vraci pri zarazce stejnou hodnotu (finval) !!!
//XXXX co se stane, kdyz volam find_beg/end (pos) pro pos > finval ???
//     -- MilosJ, 2011: pro find_end to zacykli
using namespace std;

template <class T>
T inline_max (T a, T b)
{
    return a >= b ? a : b;
}


RQConcatLeftEndSorted::RQConcatLeftEndSorted (RangeStream *s1, RangeStream *s2)
    :src1 (s1), src2 (s2), opt1 (s1->epsilon()), opt2 (s2->epsilon()), 
     fin1(src1->final()), fin2(src2->final()), finval (inline_max (fin1, fin2))
{
    assert(finval);
    locate();
}

Position RQConcatLeftEndSorted::locate()
{
    begs.clear();
    ends.clear();
    labs1.clear();
    labs2.clear();
    currend = currbeg = 0;
    Position v1 = src1->peek_end(), v2 = src2->peek_beg();
    // locate connection (v1 == v2)
    while (true) {
        if ((v1 >= fin1 && v2 >= fin2) || (v1 >= fin1 && !opt1) ||
            (v2 >= fin2 && !opt2))
            goto finished;
        if (v1 == v2)
            break;
        if (v1 < v2) {
            if (opt2) {
                v2 = v1;
                break;
            }
            if (v2 >= fin1)
                goto finished;
            src1->find_end (v2);
            v1 = src1->peek_end();
        } else {
            if (opt1) {
                v1 = v2;
                break;
            }
            if (v1 >= fin2)
                goto finished;
            v2 = src2->find_beg (v1);
        }
    }

    // fill up begs
    if (opt1) {
        Labels l;
        labs1.push_back (l);
        begs.push_back (v2);
    }
    while (src1->peek_end() == v1 && v1 < fin1) {
        Labels l;
        src1->add_labels (l);
        labs1.push_back (l);
        begs.push_back (src1->peek_beg());
        src1->next();
    }

    // fill up ends
    while (src2->peek_beg() == v2 && v2 < fin2) {
        Labels l;
        src2->add_labels (l);
        labs2.push_back (l);
        ends.push_back (src2->peek_end());
        src2->next();
    }
    if (opt2) {
        Labels l;
        labs2.push_back (l);
        ends.push_back (v1);
    }

    return begs[0];
finished:
    begs.push_back (finval);
    ends.push_back (finval);
    labs1.push_back (Labels());
    labs2.push_back (Labels());
    return finval;
}

bool RQConcatLeftEndSorted::next()
{
    if (++currend >= ends.size()) {
        currend = 0;
        if (++currbeg >= begs.size())
            return locate() < finval;
    }
    return true;
}

Position RQConcatLeftEndSorted::peek_beg() const
{
    return begs [currbeg];
}

Position RQConcatLeftEndSorted::peek_end() const
{
    return ends [currend];
}

void RQConcatLeftEndSorted::add_labels (Labels &lab) const
{
    lab.insert (labs1 [currbeg].begin(), labs1 [currbeg].end());
    lab.insert (labs2 [currend].begin(), labs2 [currend].end());
}

Position RQConcatLeftEndSorted::find_beg (Position pos)
{
    if (begs [currbeg] >= pos)
        return begs [currbeg];
    if (begs [begs.size() -1] >= pos) {
        if (begs [currbeg] < pos)
            currend = 0;
        while (begs [currbeg] < pos)
            currbeg++;
        return begs [currbeg];
    }
    src1->find_beg (pos);
    return locate();
}

Position RQConcatLeftEndSorted::find_end (Position pos)
{
    if (ends [ends.size() -1] >= pos) {
        while (ends [currend] < pos)
            currend++;
        return begs [currbeg];
    }
    src2->find_end (pos);
    return locate();
}

NumOfPos RQConcatLeftEndSorted::rest_min () const
{
    return begs[0] == finval ? 0 :
        (ends.size() - currend +1 + ends.size() * (begs.size() - currbeg));
}

NumOfPos RQConcatLeftEndSorted::rest_max () const
{
    return begs[0] == finval ? 0 :
        (ends.size() - currend + ends.size() * (begs.size() - currbeg)
         + src1->rest_max() * src2->rest_max());
}

Position RQConcatLeftEndSorted::final () const
{
    return finval;
}

int RQConcatLeftEndSorted::nesting() const
{
    // XXX chtelo by to presneji
    return (src1->nesting() +1) * (src2->nesting() +1);
}

bool RQConcatLeftEndSorted::epsilon() const
{
    return opt1 && opt2;
}


//-------------------- RQSortBeg --------------------
void RQSortBeg::updatefirst()
{
    while (src->peek_beg() < finval &&
           (que.empty() || src->peek_beg() < que.top().end)) {
        PosPair pp (src->peek_beg(), src->peek_end());
        src->add_labels (pp.lab);
        que.push (pp);
        src->next();
    }
    if (que.empty())
        que.push (PosPair (finval, finval));
}

bool RQSortBeg::next ()
{
    Position prev_beg = que.top().beg;
    Position prev_end = que.top().end;
    if (prev_beg == finval)
        return false;
    do {
        que.pop(); 
    } while (!que.empty() && 
             prev_beg == que.top().beg && prev_end == que.top().end);
    updatefirst();
    return que.top().beg != finval;
}

Position RQSortBeg::peek_beg () const 
{
    return que.top().beg;
}

Position RQSortBeg::peek_end () const 
{
    return que.top().end;
}

void RQSortBeg::add_labels (Labels &lab) const
{
    const Labels &l = que.top().lab;
    lab.insert (l.begin(), l.end());
}

Position RQSortBeg::find_beg (Position pos) 
{
    if (src->peek_end() < pos) {
        src->find_beg (pos);
        que = priority_queue<PosPair>();
        updatefirst();
    }
    if (pos >= finval)
        pos = finval;
    while (que.top().beg < pos)
        next();
    return que.top().beg;
}

Position RQSortBeg::find_end (Position pos) 
{
    if (src->peek_end() < pos - MAX_RANGE_SIZE) {
        src->find_end (pos);
        que = priority_queue<PosPair>();
        updatefirst();
    }
    if (pos >= finval)
        pos = finval;
    while (que.top().end < pos)
        next();
    return que.top().beg;
}

NumOfPos RQSortBeg::rest_min () const 
{
    return src->rest_min() + que.size();
}

NumOfPos RQSortBeg::rest_max () const 
{
    return src->rest_max() + que.size();
}

Position RQSortBeg::final () const 
{
    return finval;
}

int RQSortBeg::nesting() const 
{
    return src->nesting();
}

bool RQSortBeg::epsilon() const
{
    return src->epsilon();
}

//-------------------- RQSortEnd --------------------
void RQSortEnd::updatefirst()
{
    while (src->peek_beg() < finval &&
           (que.empty() || src->peek_beg() < que.top().end)) {
        PosPair pp (src->peek_beg(), src->peek_end());
        src->add_labels (pp.lab);
        que.push (pp);
        src->next();
    }
    if (que.empty())
        que.push (PosPair (finval, finval));
}

bool RQSortEnd::next ()
{
    Position prev_beg = que.top().beg;
    Position prev_end = que.top().end;
    if (prev_beg == finval)
        return false;
    do {
        que.pop(); 
    } while (!que.empty() && 
             prev_beg == que.top().beg && prev_end == que.top().end);
    updatefirst();
    return que.top().beg != finval;
}

Position RQSortEnd::peek_beg () const 
{
    return que.top().beg;
}

Position RQSortEnd::peek_end () const 
{
    return que.top().end;
}

void RQSortEnd::add_labels (Labels &lab) const
{
    const Labels &l = que.top().lab;
    lab.insert (l.begin(), l.end());
}

Position RQSortEnd::find_beg (Position pos) 
{
    if (src->peek_end() < pos) {
        src->find_beg (pos);
        que = priority_queue<PosPair>();
        updatefirst();
    }
    if (pos >= finval)
        pos = finval;
    while (que.top().beg < pos)
        next();
    return que.top().beg;
}

Position RQSortEnd::find_end (Position pos) 
{
    if (src->peek_end() < pos - MAX_RANGE_SIZE) {
        src->find_end (pos);
        que = priority_queue<PosPair>();
        updatefirst();
    }
    if (pos >= finval)
        pos = finval;
    while (que.top().end < pos)
        next();
    return que.top().beg;
}

NumOfPos RQSortEnd::rest_min () const 
{
    return src->rest_min() + que.size();
}

NumOfPos RQSortEnd::rest_max () const 
{
    return src->rest_max() + que.size();
}

Position RQSortEnd::final () const 
{
    return finval;
}

int RQSortEnd::nesting() const 
{
    return src->nesting();
}


bool RQSortEnd::epsilon() const
{
    return src->epsilon();
}

//-------------------- RSFindBack --------------------
RSFindBack::RSFindBack (RangeStream *s) 
    : src (s), finval (s->final()), curridx(0) {
    buff.push_back (rangeitem (src->peek_beg(), src->peek_end()));
}

void RSFindBack::strip_buff (Position pos)
{
    vector<rangeitem>::iterator delto = buff.begin();
    Position p = pos - MAX_RANGE_SIZE;
    while ((*delto).beg < p)
        delto++;
    if (delto != buff.begin())
        buff.erase (buff.begin(), delto);
    curridx = buff.size() -1;
}

bool RSFindBack::next ()
{
    if (++curridx < buff.size()) {
        return true;
    } else {
        Position p = buff.back().beg;
        buff.push_back (rangeitem (src->peek_beg(), src->peek_end()));
        strip_buff (p);
        return src->next();
    }
}

Position RSFindBack::peek_beg () const 
{
    return buff [curridx].beg;
}

Position RSFindBack::peek_end () const 
{
    return buff [curridx].end;
}

void RSFindBack::add_labels (Labels &lab) const
{
}

Position RSFindBack::find_beg (Position pos) 
{
    if (pos <= buff.back().beg) {
        // mame ulozeno
        curridx=0; 
        while (pos > buff [curridx].beg)
            curridx++;
        return buff [curridx].beg;
    } else {
        if (buff.back().beg + MAX_RANGE_SIZE < pos) {
            // jsme daleko, muzeme vyprazdnit
            buff.clear();
            src->find_beg (pos - MAX_RANGE_SIZE);
        }
        if (pos >= finval)
            pos = finval;
        Position p;
        do {
            buff.push_back (rangeitem (p=src->peek_beg(), src->peek_end()));
            src->next();
        } while (p < pos);
        strip_buff (pos);
        return p;
    }
}

Position RSFindBack::find_end (Position pos) 
{
    if (pos >= finval)
        pos = finval;
    if (buff.back().beg + 2 * MAX_RANGE_SIZE < pos) {
        // jsme daleko, muzeme vyprazdnit
        buff.clear();
        src->find_beg (pos - 2 * MAX_RANGE_SIZE);
    } else {
        curridx = 0;
        while (curridx < buff.size() && pos > buff [curridx].end)
            curridx++;
        if (curridx < buff.size())
            return buff [curridx].beg;
    }
    Position p;
    do {
        buff.push_back (rangeitem (src->peek_beg(), p=src->peek_end()));
        src->next();
    } while (p < pos);
    strip_buff (pos);
    return buff.back().beg;
}

NumOfPos RSFindBack::rest_min () const 
{
    return src->rest_min() + buff.size() - curridx;
}

NumOfPos RSFindBack::rest_max () const 
{
    return src->rest_max() + buff.size() - curridx;
}

Position RSFindBack::final () const 
{
    return finval;
}

int RSFindBack::nesting() const 
{
    return src->nesting();
}


bool RSFindBack::epsilon() const
{
    return src->epsilon();
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
