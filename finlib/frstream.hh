// Copyright (c) 1999-2013  Pavel Rychly, Milos Jakubicek

#ifndef FRSTREAM_HH
#define FRSTREAM_HH

#include "fstream.hh"


class RangeStream {
protected:
    RangeStream () {}
public:
    virtual ~RangeStream () {}
    virtual bool end() const {return peek_beg() >= final();}
    virtual bool next() =0;
    virtual Position peek_beg() const =0;
    virtual Position peek_end() const =0;
    virtual void add_labels (Labels &lab) const =0;
    virtual Position find_beg (Position pos) =0;
    virtual Position find_end (Position pos) =0;
    virtual NumOfPos rest_min() const =0;
    virtual NumOfPos rest_max() const =0;
    virtual Position final() const =0;
    virtual int nesting() const =0;
    virtual bool epsilon() const =0;
};


#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
