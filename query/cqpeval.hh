// Copyright (c) 2001-2016  Pavel Rychly

#ifndef CQPEVAL_HH
#define CQPEVAL_HH

#include "frstream.hh"
#include "corpus.hh"

class EvalQueryException: public std::exception {
    const std::string _what;
public:
    EvalQueryException (const std::string &name)
     :_what (name) {}
    virtual const char* what () const throw () {return _what.c_str();}
    virtual ~EvalQueryException() throw() {}
};


RangeStream *eval_cqpquery (const char *query, Corpus *corp);
FastStream *eval_cqponepos (const char *query, Corpus *corp);


#endif
