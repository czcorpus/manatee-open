// Copyright (c) 1999-2019  Pavel Rychly, Milos Jakubicek

#ifndef COMPSTREAM_HH
#define COMPSTREAM_HH

#include "fstream.hh"
#include "bitio.hh"

//-------------------- GolombPosStream<bitIterator> --------------------

template <class bitIterator>
class GolombPosStream: public FastStream {
protected:
    Position b_gol;
    bitIterator bits;
    read_bits <bitIterator&> rb;
    Position finval;
    NumOfPos rest;
    Position current;
    void read_next () {
        if (rest > 0) {
            rest--;
            current += rb.golomb (b_gol);
        } else
            current = finval;
    }

public:
    GolombPosStream (bitIterator &bi, NumOfPos count, Position b, Position fin)
        : b_gol (b), rest (count), finval (fin), current (-1), 
          bits (bi), rb (bits) {read_next();}
    virtual Position peek () {return current;}
    virtual Position next () {
        Position ret = current;
        read_next();
        return ret;
    }
    virtual Position find (Position pos) {
        while (current < pos && current < finval)
            read_next();
        return current;
    }
    virtual NumOfPos rest_min () {return rest + (current!=finval);}
    virtual NumOfPos rest_max () {return rest + (current!=finval);}
    virtual Position final () {return finval;}
};


//-------------------- DeltaPosStream<bitIterator> --------------------

template <class bitIterator>
class DeltaPosStream: public FastStream {
protected:
    bitIterator bits;
    read_bits <bitIterator&, typename std::iterator_traits
              <bitIterator>::value_type, Position> rb;
    Position finval;
    NumOfPos rest;
    Position current;
    virtual void read_next () {
        if (rest > 0) {
            rest--;
            current += rb.delta();
        } else
            current = finval;
    }

public:
    DeltaPosStream (bitIterator bi, NumOfPos count, Position fin, int skipbits=0,
                    bool dont_read = false)
        : bits (bi), rb (bits, skipbits), finval (fin), rest (count), 
        current (-1)
        {if (!dont_read) read_next();}
    virtual Position peek () {return current;}
    virtual Position next () {
        Position ret = current;
        read_next();
        return ret;
    }
    virtual Position find (Position pos) {
        while (current < pos && current < finval)
            read_next();
        return current;
    }
    virtual NumOfPos rest_min () {return rest + (current!=finval);}
    virtual NumOfPos rest_max () {return rest + (current!=finval);}
    virtual Position final () {return finval;}
    bool operator == (const DeltaPosStream &x) const {return rb == x.rb;}
};


//-------------------- CollDeltaPosStream<bitIterator> --------------------

template <class bitIterator>
class CollDeltaPosStream: public DeltaPosStream<bitIterator> {
protected:
    Position current_coll;
    virtual void read_next () {
        if (this->rest > 0) {
            this->rest--;
            this->current += this->rb.delta() -1;
            current_coll = this->rb.gamma() -1;
            if (current_coll % 2 == 1)
                current_coll = - current_coll;
            current_coll /= 2;
            current_coll += this->current;
        } else
            this->current = this->finval;
    }
public:
    CollDeltaPosStream (bitIterator bi, NumOfPos count, Position fin, int skipbits=0)
        : DeltaPosStream<bitIterator>::DeltaPosStream (bi, count, fin, skipbits, true)
        {this->current=0; read_next();}
    virtual void add_labels (Labels &lab) {lab[1] = current_coll;}
};


#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
