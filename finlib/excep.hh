// Copyright (c) 1999-2020  Pavel Rychly, Milos Jakubicek, Ondrej Herman
#pragma once
#include <string>
#include <exception>
#include <cerrno>
#include <cstring>
#include <sstream>

// -------------------- FileAccessError --------------------

class FileAccessError: public std::exception {
    const std::string _what;
public:
    const std::string filename;
    const std::string where;
    const int err;
    FileAccessError (const std::string &filename, const std::string &where)
	:_what ("FileAccessError (" + filename + ") in " + where 
		+ " [" + strerror (errno) + ']'), 
	filename (filename), where (where), err (errno) {}
    virtual const char* what () const throw () {return _what.c_str();}
    virtual ~FileAccessError() throw () {};
};

// -------------------- MemoryAllocationError --------------------

class MemoryAllocationError: public std::exception {
    const std::string _what;
public:
    const std::string where;
    MemoryAllocationError (const std::string &where)
	:_what ("Cannot allocate memory in " + where), where (where) {}
    virtual const char* what () const throw () {return _what.c_str();}
    virtual ~MemoryAllocationError() throw () {};
};

class CorpInfoNotFound: public std::exception {
    const std::string _what;
public:
    const std::string name;
    CorpInfoNotFound (const std::string &name)
        :_what ("CorpInfoNotFound (" + name + ")"), name (name) {}
    virtual const char* what () const throw () {return _what.c_str();}
    virtual ~CorpInfoNotFound() throw() {}
};

class AttrNotFound: public std::exception {
    const std::string _what;
public:
    const std::string name;
    AttrNotFound (const std::string &name)
        :_what ("AttrNotFound (" + name + ")"), name (name) {}
    virtual const char* what () const throw () {return _what.c_str();}
    virtual ~AttrNotFound() throw() {}
};

class NotImplemented: public std::exception {
    std::string _what;
public:
    NotImplemented (const std::string func, const std::string file, int line) {
        std::stringstream ss;
        ss << func << " not implemented (" << file << ": " << line << ")";
        _what = ss.str();
    }
    virtual const char* what () const throw () {return _what.c_str();}
    virtual ~NotImplemented() throw () {}
};
#define NOTIMPLEMENTED throw NotImplemented(__func__,__FILE__,__LINE__);

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
