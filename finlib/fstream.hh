// Copyright (c) 1999-2011  Pavel Rychly, Milos Jakubicek

#ifndef FSTREAM_HH
#define FSTREAM_HH

#include <config.hh>
#include <map>
#include <climits>

typedef long long int Position; // must be signed
typedef Position NumOfPos;
typedef std::map<int,Position> Labels;
#define STR2NUMPOS(s) atoll(s)
const Position maxPosition = LLONG_MAX;

class FastStream {
protected:
    FastStream() {}
public:
    virtual ~FastStream() {}
    virtual void add_labels (Labels &lab) {};
    virtual Position peek() = 0;
    virtual Position next() = 0;
    virtual Position find (Position pos) = 0;
    virtual NumOfPos rest_min() = 0;
    virtual NumOfPos rest_max() = 0;
    virtual Position final() = 0;
};

#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
