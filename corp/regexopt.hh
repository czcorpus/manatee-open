// Copyright (c) 2016-2020 Milos Jakubicek

#ifndef REGEXOPT_HH
#define REGEXOPT_HH

#include "frstream.hh"
class WordList;

FastStream * optimize_regex (WordList *a, const char *pat, const char *encoding);

class RegexOptException: public std::exception {
    const std::string _what;
public:
    RegexOptException (const std::string &name)
	:_what ("regexopt: " + name) {}
    virtual const char* what () const throw () {return _what.c_str();}
    virtual ~RegexOptException() throw() {}
};

#endif
