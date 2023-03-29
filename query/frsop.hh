// Copyright (c) 1999-2019  Pavel Rychly, Milos Jakubicek

#ifndef FINLIB_FRSOP_HH
#define FINLIB_FRSOP_HH

#include <vector>
#include <queue>
#include <map>
#include <set>
#include "frstream.hh"
#include "fsop.hh"

//XXX asi by to melo byt nejak jinak
#define MAX_RANGE_SIZE 100


#define REQUIRED_RangeStream_METHODS \
    virtual bool next ();                        \
    virtual Position peek_beg () const;          \
    virtual Position peek_end () const;          \
    virtual void add_labels (Labels &lab) const; \
    virtual Position find_beg (Position pos);    \
    virtual Position find_end (Position pos);    \
    virtual NumOfPos rest_min () const;          \
    virtual NumOfPos rest_max () const;          \
    virtual Position final () const;             \
    virtual int nesting() const;                 \
    virtual bool epsilon() const


class Pos2Range: public RangeStream {
    FastStream *in;
    const Position finin, finout;
    int delta_beg;
    int delta_end;
public:
    Pos2Range (FastStream *input, int beg=0, int end=1)
        : in (input), finin (in->final()), finout ((finin + end < 0) ? maxPosition : finin + end),
          delta_beg (beg), delta_end (end) {}
    virtual ~Pos2Range() {delete in;}
    REQUIRED_RangeStream_METHODS;
};

class RQinNode: public RangeStream {
protected:
    RangeStream *inside, *outside;
    Position infin, outfin;
    bool finished;
    virtual Position locate ();
public:
    RQinNode (RangeStream *in, RangeStream *out, bool do_locate = true);
    virtual ~RQinNode () {delete inside; delete outside;}
    REQUIRED_RangeStream_METHODS;
};

class RQnotInNode: public RQinNode {
    virtual Position locate ();
public:
    RQnotInNode (RangeStream *in, RangeStream *out);
    virtual ~RQnotInNode () {}
};


class RQoutsideNode: public RangeStream {
    RangeStream *src;
    Position finval;
    Position curr_beg, curr_end;
    void locate();
public:
    RQoutsideNode (RangeStream *rs, Position maxend);
    virtual ~RQoutsideNode () {delete src;}
    REQUIRED_RangeStream_METHODS;
    static RangeStream *create (RangeStream *src, Position maxpos);
};


class RQcontainNode: public RangeStream {
protected:
    RangeStream *inside, *outside;
    Position infin, outfin;
    bool finished;
    virtual Position locate ();
public:
    RQcontainNode (RangeStream *out, RangeStream *in, bool do_locate = true);
    virtual ~RQcontainNode () {delete inside; delete outside;}
    REQUIRED_RangeStream_METHODS;
};

class RQnotContainNode: public RQcontainNode {
    virtual Position locate ();
public:
    RQnotContainNode (RangeStream *out, RangeStream *in);
    virtual ~RQnotContainNode () {}
    virtual Position find_end (Position pos);
};


class RQUnionNode: public RangeStream {
    void updatefirst();
protected:
    class PosPair {
    public:
        Position beg,end;
        bool operator < (const PosPair &x) const 
        {return beg < x.beg || (beg == x.beg && end < x.end);}
        bool operator == (const PosPair &x) const 
        {return beg == x.beg && end == x.end;}
    };
    RangeStream *src[2];
    PosPair peek_s[2];
    Position finval[2];
    int first;
public:
    RQUnionNode (RangeStream *s1, RangeStream *s2);
    virtual ~RQUnionNode () {delete src[0]; delete src[1];}
    REQUIRED_RangeStream_METHODS;
};

class RQRepeatFSNode: public RangeStream {
    FastStream *src;
    Position finval;
    int minr, maxr;
    Position repbeg, repend, nextend;
    bool epsilonval;
    void locate ();
public:
    RQRepeatFSNode (FastStream *s, int minrep, int maxrep);
    virtual ~RQRepeatFSNode () {delete src;}
    REQUIRED_RangeStream_METHODS;
};

class RQRepeatNode: public RangeStream {
    RangeStream *src;
    Position finval;
    int minr, maxr;
    bool epsilonval;
    typedef std::vector<Position> PosList;
    std::map<Position,PosList> pool;
    std::queue<Position> begs;
    std::set<Position> output;
    void locate();
    void search_pool (Position start, int currrep);
public:
    RQRepeatNode (RangeStream *s, int minrep, int maxrep);
    virtual ~RQRepeatNode () {delete src;}
    REQUIRED_RangeStream_METHODS;
};

class RQOptionalNode: public RangeStream {
    RangeStream *src;
public:
    RQOptionalNode (RangeStream *s) : src (s) {}
    virtual ~RQOptionalNode () {delete src;}
    REQUIRED_RangeStream_METHODS;
};

class RQConcatLeftEndSorted: public RangeStream {
    RangeStream *src1, *src2;
    bool opt1, opt2;
    Position fin1, fin2, finval;
    std::vector<Position> begs, ends;
    std::vector<Labels> labs1, labs2;
    unsigned int currbeg, currend;
    Position locate();
public:
    RQConcatLeftEndSorted (RangeStream *s1, RangeStream *s2);
    virtual ~RQConcatLeftEndSorted () {delete src1; delete src2;}
    REQUIRED_RangeStream_METHODS;
};


class RQSortBeg: public RangeStream {
    void updatefirst();
protected:
    class PosPair {
    public:
        Position beg, end;
        Labels lab;
        PosPair (Position b, Position e) : beg (b), end (e) {}
        bool operator < (const PosPair &x) const 
        {return beg > x.beg || (beg == x.beg && end > x.end);}
    };
    RangeStream *src;
    Position finval;
    std::priority_queue<PosPair> que;
public:
    RQSortBeg (RangeStream *s) : src (s), finval (s->final()) {updatefirst();}
    virtual ~RQSortBeg () {delete src;}
    REQUIRED_RangeStream_METHODS;
};

class RQSortEnd: public RangeStream {
    void updatefirst();
protected:
    class PosPair {
    public:
        Position beg,end;
        Labels lab;
        PosPair (Position b, Position e) : beg (b), end (e) {}
        bool operator < (const PosPair &x) const 
        {return end > x.end || (end == x.end && beg > x.beg);}
    };
    RangeStream *src;
    Position finval;
    std::priority_queue<PosPair> que;
public:
    RQSortEnd (RangeStream *s) : src (s), finval (s->final()) {updatefirst();}
    virtual ~RQSortEnd () {delete src;}
    REQUIRED_RangeStream_METHODS;
};


//  class RQSortStream: public RangeStream {
//      void updatefirst();
//  protected:
//      class PosPair {
//      public:
//      Position beg,end;
//      PosPair (Position b, Position e) : beg (b), end (e) {}
//      };
//      RangeStream *src;
//      Position finval;
//      priority_queue<PosPair> que;
//  public:
//      typedef binary_function<PosPair, PosPair, bool> comp_op;
//      RQSortStream (RangeStream *s, comp_op &c) 
//      : src (s), finval (s->final()), que (c) {updatefirst();}
//      virtual ~RQSortStream () {delete src;}
//      REQUIRED_RangeStream_METHODS;

//      // priority_queue tridi od nejvetsiho!
//      struct beg_first : public comp_op {
//      bool operator () (const PosPair &x, const PosPair &y) const 
//      {return x.beg > y.beg || (x.beg == y.beg && x.end > y.end);}
//      };
//      struct end_first : public comp_op {
//      bool operator () (const PosPair &x, const PosPair &y) const 
//      {return x.end > y.end || (x.end == y.end && x.beg > y.beg);}
//      };
//};


inline RangeStream *RQConcatNode (RangeStream *s1, RangeStream *s2)
{
    if (s1->end() && s1->epsilon())
        return s2;
    if (s2->end() && s2->epsilon())
        return s1;
    if (s1->end() || s2->end())
        return new Pos2Range(new EmptyStream());
    return new RQSortBeg (new RQConcatLeftEndSorted (new RQSortEnd (s1), s2));
}

inline RangeStream *RQConcatNodeUnsort (RangeStream *s1, RangeStream *s2)
{
    if (s1->end() && s1->epsilon())
        return s2;
    if (s2->end() && s2->epsilon())
        return s1;
    if (s1->end() || s2->end())
        return new Pos2Range(new EmptyStream());
    return new RQConcatLeftEndSorted (new RQSortEnd (s1), s2);
}


class BegsOfRStream: public FastStream {
    RangeStream *in;
public:
    BegsOfRStream (RangeStream *input) : in (input) {}
    virtual ~BegsOfRStream() {delete in;}

    virtual Position peek ();
    virtual Position next ();
    virtual Position find (Position pos);
    virtual NumOfPos rest_min ();
    virtual NumOfPos rest_max ();
    virtual Position final ();
    virtual void add_labels (Labels &lab);
};

class EndsOfRStream: public FastStream {
    RangeStream *in;
public:
    EndsOfRStream (RangeStream *input) : in (new RQSortEnd (input)) {}
    virtual ~EndsOfRStream() {delete in;}

    virtual Position peek ();
    virtual Position next ();
    virtual Position find (Position pos);
    virtual NumOfPos rest_min ();
    virtual NumOfPos rest_max ();
    virtual Position final ();
};


class RSFindBack: public RangeStream {
protected:
    struct rangeitem {
        Position beg;
        Position end;
        rangeitem () {}
        rangeitem (Position b, Position e): beg(b), end(e) {}
    };
    RangeStream *src;
    Position finval;
    std::vector<rangeitem> buff;
    unsigned curridx;
    void strip_buff (Position pos);
public:
    RSFindBack (RangeStream *s);
    virtual ~RSFindBack () {delete src;}
    REQUIRED_RangeStream_METHODS;
};

class AddRSLabel: public RangeStream {
    int label;
    RangeStream *src;
public:
    AddRSLabel (RangeStream *source, int lab = 0) : label (lab), src (source) {}
    virtual ~AddRSLabel () {delete src;}
    REQUIRED_RangeStream_METHODS;
};

class NonEmptyRS: public RangeStream {
    RangeStream *src;
    void skip_empty();
public:
    NonEmptyRS (RangeStream *source) : src (source) { skip_empty(); }
    virtual ~NonEmptyRS () {delete src;}
    REQUIRED_RangeStream_METHODS;
};

class FlattenRS: public RangeStream {
    RangeStream *src;
    Position curbeg, curend;
public:
    FlattenRS (RangeStream *source) : src (source), curbeg(final()),
         curend(final()) {next();}
    virtual ~FlattenRS () {delete src;}
    REQUIRED_RangeStream_METHODS;
};

#endif
// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
