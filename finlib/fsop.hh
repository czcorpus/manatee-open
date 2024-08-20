// Copyright (c) 1999-2017  Pavel Rychly, Milos Jakubicek

#ifndef FSOP_HH
#define FSOP_HH

#ifndef SWIG
#include "fstream.hh"
#include "binfile.hh"
#include "generator.hh"
#include <vector>
#include <queue>
#include <stdexcept>
#include <limits>

using namespace std;

class EmptyStream: public FastStream {
protected:
    Position value;
public:
    EmptyStream (Position finval=0): value (finval) {}
    virtual ~EmptyStream() {}
    virtual Position peek () {return value;}
    virtual Position next () {return value;}
    virtual Position find (Position pos) {return value;}
    virtual NumOfPos rest_min () {return 0;}
    virtual NumOfPos rest_max () {return 0;}
    virtual Position final () {return value;}
};

#endif

class SequenceStream: public FastStream {
protected:
    Position curr;
    Position last;
    Position finval;
public:
    SequenceStream (Position first, Position last, Position finval)
        : curr (first), last (last), finval (finval) 
    {if (curr > last) curr = finval;}
    virtual ~SequenceStream() {}
    virtual Position peek () {return curr;}
    virtual Position next () {
        if (curr == finval) return finval;
        Position ret = curr++;
        if (curr > last) curr = finval;
        return ret;
    }
    virtual Position find (Position pos) {
        if (pos > curr) {
            if (pos > last) 
                curr = finval;
            else
                curr = pos;
        }
        return curr;
    }
    virtual NumOfPos rest_min () {return finval == curr ? 0 : last - curr +1;}
    virtual NumOfPos rest_max () {return finval == curr ? 0 : last - curr +1;}
    virtual Position final () {return finval;}
};

template <class Value>
class Gen2Fast: public FastStream {
private:
    Generator<Value> *gen;
    Position next_item;
public:
    Gen2Fast (Generator<Value> *generator):
        gen(generator) {
            next_item = gen->end() ? final() : gen->next();
        }
    virtual ~Gen2Fast() {delete gen;}
    virtual Position peek () {return next_item;}
    virtual Position next () {
        Position curr = next_item;
        if (!gen->end())
            next_item = gen->next();
        else
            next_item = final();
        return curr;
    }
    virtual Position find (Position pos) {
        while (next_item < pos && !gen->end())
            next();
        if (next_item < pos) // => gen->end()
            return next();
        return next_item;
    }
    virtual NumOfPos rest_min () {return 0;}
    virtual NumOfPos rest_max () {return gen->size();}
    virtual Position final () {return maxPosition;}
};

template <class Value>
class Fast2Gen: public Generator<Value> {
protected:
    FastStream *f;
    const Value last;
public:
    Fast2Gen (FastStream *fs): f(fs), last(fs->final()) {}
    virtual Value next() {return f->next();}
    virtual bool end() {return f->peek() >= last;}
    virtual const int size() const {
        return max(int(f->rest_max()), numeric_limits<int>::max());
    }
    virtual ~Fast2Gen() {delete f;}
};



#ifndef SWIG

class FastBuffStream: public FastStream {
protected:
    FastStream *src;
    Position *buff;
    Position *current;
    Position *last;
    int size;
public:
    FastBuffStream (FastStream *source, int size);
    virtual ~FastBuffStream ();
    virtual Position peek ();
    virtual Position next ();
    virtual Position find (Position pos);
    virtual NumOfPos rest_min ();
    virtual NumOfPos rest_max ();
    virtual Position final ();
};


template <class AtomType>
class MemFastStream: public FastStream {
private:
    AtomType *current;
    AtomType *last;
    Position finval;
    AtomType *mem;
public:
    MemFastStream (AtomType *base, NumOfPos size, Position finval=maxPosition)
        :current (base), last (base + size -1), finval (finval), mem (base)
        {}
    MemFastStream (FastStream *src, NumOfPos size) {
        AtomType *mem = new AtomType[size];
        AtomType *it = mem;
        Position p, fin = src->final();
        while ((p = src->next()) < fin && it < mem + size)
            *it++ = p;
        if (it != mem + size || p < fin)
            throw runtime_error("FastStream size does not match expected size");
        delete src;
        MemFastStream<AtomType> (mem, size, fin);
    }
    virtual ~MemFastStream () { delete [] mem;}
    virtual Position peek ()
        {return (current <= last) ? *current : finval;}
    virtual Position next ()
        {return (current <= last) ? *(current++) : finval;}
    virtual Position find (Position pos);
    virtual NumOfPos rest_min ()
        {return (last - current + 1);}
    virtual NumOfPos rest_max  ()
        {return (last - current + 1);}
    virtual Position final ()
        {return finval;}
};

template <class AtomType>
Position MemFastStream<AtomType>::find (Position pos)
{
    NumOfPos incr = 1;
    while ((current + incr) <= last && *(current + incr) <= pos) {
        current += incr;
        incr *= 2;
    }
    while (incr > 0) {
        if ((current + incr) <= last && *(current + incr) <= pos) {
            current += incr;
        }
        incr /= 2;
    }
    if (current > last)
        return finval;
    if (*current < pos)
        current ++;
    if (current > last)
        return finval;
    return *current;
}



class FileFastStream: public FastStream {
private:
    const BinFile<Position> &file;
    off_t current;
    NumOfPos rest;
    Position finval;
public:
    FileFastStream (const BinFile<Position> &file, off_t offset, NumOfPos size,
                   Position finval = 0)
        :file (file), current (offset), rest (size), finval (finval)
        {if (finval == 0) finval = file.size();}
    virtual Position peek ();
    virtual Position next ();
    virtual Position find (Position pos);
    virtual NumOfPos rest_min ();
    virtual NumOfPos rest_max ();
    virtual Position final ();
};

template <class forward_iterator>
class FIFastStream: public FastStream {
private:
    forward_iterator current;
    NumOfPos rest;
    Position finval;
public:
    FIFastStream (forward_iterator base, NumOfPos size, 
                  Position finval=maxPosition)
        :current (base), rest (size), finval (finval)
        {}
    virtual Position peek () {return (rest > 0) ? *current : finval;}
    virtual Position next () {
        if (rest > 0) {
            Position ret = *current;
            rest--;
            ++current;
            return ret;
        } else {
            return finval;
        }
    }
    virtual Position find (Position pos);
    virtual NumOfPos rest_min () {return rest;}
    virtual NumOfPos rest_max () {return rest;}
    virtual Position final () {return finval;}
};


template <class forward_iterator>
Position FIFastStream<forward_iterator>::find (Position pos)
{
    NumOfPos incr = 1;
    while (incr < rest && *(current + incr) <= pos) {
        current += incr;
        rest -= incr;
        incr *= 2;
    }
    while (incr > 0) {
        if (incr < rest && *(current + incr) <= pos) {
            current += incr;
            rest -= incr;
        }
        incr /= 2;
    }
    if (rest <= 0)
        return finval;
    if (*current < pos) {
        ++current;
        rest--;
    }
    if (rest <= 0)
        return finval;
    return *current;
}


#endif

// -------------------- Query operations --------------------


class QNotNode: public FastStream {
    void updatecurrent();
protected:
    FastStream *src;
    Position current;
    Position srcnext;
    Position srcfin;
    Position nodefin;
public:
    QNotNode (FastStream *source, Position finval=0);
    virtual ~QNotNode ();
    virtual Position peek ();
    virtual Position next ();
    virtual Position find (Position pos);
    virtual NumOfPos rest_min ();
    virtual NumOfPos rest_max ();
    virtual Position final ();
};

class QAndNode: public FastStream {
protected:
    FastStream *src[2];
    Position finval, last_peek;
    Labels last_labels;
public:
    QAndNode (FastStream *source1, FastStream *source2);
    virtual ~QAndNode ();
    virtual Position peek ();
    virtual Position next ();
    virtual Position find (Position pos);
    virtual NumOfPos rest_min ();
    virtual NumOfPos rest_max ();
    virtual Position final ();
    virtual void add_labels (Labels &lab);
};

class QOrNode: public FastStream {
    inline void updatefirst();
protected:
    FastStream *src[2];
    Position peek_s[2];
    Position finval[2];
    int first;
public:
    QOrNode (FastStream *source1, FastStream *source2);
    virtual ~QOrNode ();
    virtual Position peek ();
    virtual Position next ();
    virtual Position find (Position pos);
    virtual NumOfPos rest_min ();
    virtual NumOfPos rest_max ();
    virtual Position final ();
    virtual void add_labels (Labels &lab);
};

typedef std::pair<Position,FastStream*> pos_stream_pair;
typedef std::vector<pos_stream_pair> pss_vector;
class QOrVNode: public FastStream {
    inline void updatefirst();
    bool skip;
protected:
    pss_vector src;
    Position finval;
public:
    QOrVNode (std::vector<FastStream*> *source, bool skip_duplicates = true);
    virtual ~QOrVNode ();
    virtual Position peek ();
    virtual Position next ();
    virtual Position find (Position pos);
    virtual NumOfPos rest_min ();
    virtual NumOfPos rest_max ();
    virtual Position final ();
    virtual void add_labels (Labels &lab);
    static FastStream *create (std::vector<FastStream*> *source,
                               bool skip_duplicates = true);
};

template<class Source>
class QOrVNode_clean: public QOrVNode
{
    Source *data_to_del;
public:
    QOrVNode_clean (std::vector<FastStream*> *source, Source *data)
        : QOrVNode (source, false), data_to_del (data) {}
    virtual ~QOrVNode_clean() {delete data_to_del;}
};
    
class QMoveNode: public FastStream {
    inline void updatecurrent();
protected:
    FastStream *src;
    int delta;
    Position finval;
    Position current;
public:
    QMoveNode (FastStream *source, int delta);
    virtual ~QMoveNode ();
    virtual Position peek ();
    virtual Position next ();
    virtual Position find (Position pos);
    virtual NumOfPos rest_min ();
    virtual NumOfPos rest_max ();
    virtual Position final ();
    virtual void add_labels (Labels &lab);
};
    
class QFilterNode: public FastStream {
protected:
    void updatecurrent();
    FastStream *src;
    Position current;
    Position finval;
    virtual bool correct_pos (Position pos) = 0;
public:
    QFilterNode (FastStream *source);
    virtual ~QFilterNode ();
    virtual Position peek ();
    virtual Position next ();
    virtual Position find (Position pos);
    virtual NumOfPos rest_min ();
    virtual NumOfPos rest_max ();
    virtual Position final ();
};
    


class QFilterFnNode: public QFilterNode {
public:
    typedef bool filterfn (Position);
protected:
    filterfn *corrfn;
    virtual bool correct_pos (Position pos);
public:
    QFilterFnNode (FastStream *source, filterfn *fn);
    virtual ~QFilterFnNode ();
};
    
class AddLabel : public FastStream {
    int label;
    FastStream *src;
public:
    AddLabel (FastStream *source, int lab = 0);
    virtual ~AddLabel ();
    virtual Position peek ();
    virtual Position next ();
    virtual Position find (Position pos);
    virtual NumOfPos rest_min ();
    virtual NumOfPos rest_max ();
    virtual Position final ();
    virtual void add_labels (Labels &lab);
};

class ChangeLabel : public FastStream {
    int fromlabel, tolabel;
    FastStream *src;
public:
    ChangeLabel (FastStream *source, int fromlab = 0, int tolab = 0);
    virtual ~ChangeLabel ();
    virtual Position peek ();
    virtual Position next ();
    virtual Position find (Position pos);
    virtual NumOfPos rest_min ();
    virtual NumOfPos rest_max ();
    virtual Position final ();
    virtual void add_labels (Labels &lab);
};

class SwapKwicColl: public FastStream {
    void updatefirst();
protected:
    class PosLabel {
    public:
        Position pos;
        Labels lab;
        Position coll;
        PosLabel (Position p) : pos (p), coll (p) {}
        bool operator < (const PosLabel &x) const {return coll > x.coll;}
    };
    FastStream *src;
    Position finval;
    int collnum;
    std::priority_queue<PosLabel> que;
public:
    SwapKwicColl (FastStream *s, int c) : src (s), finval (s->final()),
                                          collnum (c) {updatefirst();}
    virtual ~SwapKwicColl () {delete src;}
    virtual Position peek ();
    virtual Position next ();
    virtual Position find (Position pos);
    virtual NumOfPos rest_min ();
    virtual NumOfPos rest_max ();
    virtual Position final ();
    virtual void add_labels (Labels &lab);
};

#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
