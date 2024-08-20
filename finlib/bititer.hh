// Copyright (c) 1999-2007  Pavel Rychly

#ifndef BITITER_HH
#define BITITER_HH

#include "seek.hh"
#include <iterator>


class OutFileBits
{
protected:
    FILE *file;
    unsigned char curr;
    bool end_close;
public:
    OutFileBits (FILE *infile) : file (infile), end_close (false) {}
    OutFileBits (const char *name)
	:file (fopen (name, "wb")), end_close (true) {}
    ~OutFileBits () {if (end_close) fclose (file);}
    unsigned char& operator* () {return curr;}
    OutFileBits& operator++() {putc (curr, file); return *this;}
};

class OutFileBits_tell: public OutFileBits
{
protected:
    off_t filetell;
public:
    OutFileBits_tell (FILE *infile)
	: OutFileBits (infile), filetell (ftello (file)) {}
    OutFileBits_tell (const char *name)
	: OutFileBits (name), filetell (ftello (file)) {}
    OutFileBits_tell& operator++() {
	OutFileBits::operator++(); filetell++; return *this;
    }
    off_t tell() {return filetell;}
};


class InFileBits
{
protected:
    FILE *file;
    unsigned char curr;
    bool end_close;
public:
    InFileBits (FILE *infile)
	: file (infile), end_close (false)
	{curr = getc (file);}
    InFileBits (const char *name)
	: file (fopen (name, "rb")), end_close (true)
	{curr = getc (file);}
    ~InFileBits () {if (end_close) fclose (file);}
    unsigned char operator* () {return curr;}
    InFileBits& operator++() {curr = getc (file); return *this;}
    bool eof() {return feof(file);}
};


template <class AtomType>
class SafeInBits
{
protected:
    AtomType *mem;
    AtomType curr;
public:
    SafeInBits (AtomType *init)
	: mem (init), curr (*init) {}
    AtomType& operator* () {return curr;}
    SafeInBits& operator++() {curr = *(++mem); return *this;}
    int operator== (const SafeInBits<AtomType> &x) const 
	{return mem == x.mem;}
};

typedef SafeInBits<unsigned char> SafeBytes;


#endif
