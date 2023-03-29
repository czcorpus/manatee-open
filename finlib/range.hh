// Copyright (c) 1999-2011  Pavel Rychly, Milos Jakubicek

#ifndef RANGE_HH
#define RANGE_HH

#include "frstream.hh"
#include "fstream.hh"
#include <string>


class ranges {
public:
    ranges () {};
    virtual ~ranges() {};
    virtual NumOfPos size () = 0;
    virtual Position beg_at (NumOfPos idx) = 0;
    virtual Position end_at (NumOfPos idx) = 0;
    virtual NumOfPos num_at_pos (Position pos) = 0;
    virtual NumOfPos num_next_pos (Position pos) = 0;
    virtual RangeStream *whole() = 0;
    virtual RangeStream *part(FastStream *p) = 0;
    virtual int nesting_at (NumOfPos idx) = 0;
};

ranges* create_ranges(const std::string &path, const std::string &type);


#endif
