// Copyright (c) 1999-2008  Pavel Rychly

#ifndef FROMTOF_HH
#define FROMTOF_HH

#include <config.hh>
#include "excep.hh"
#include "seek.hh"
#include <iterator>

//-------------------- FromFile --------------------
template <class Value, int buffsize=4096/sizeof(Value)>
class FromFile : public
#if defined __GNUC__ && __GNUC__ >= 3
    std::iterator<std::forward_iterator_tag,Value>
#else
    std::forward_iterator<Value, void>
#endif
{
    Value buff[buffsize];
    Value *curr;
    int rest;
    FILE *input;
    bool closeatend;
public:
    FromFile (FILE *in)
        : rest(0), input (in), closeatend(false) {++*this;}
    FromFile (const std::string & filename) 
        : rest(0), closeatend(true) {
        input = (fopen (filename.c_str(), "rb"));
        if (input == NULL)
            throw FileAccessError (filename, "FromFile: fopen");
        ++*this;
    }
    ~FromFile () {
        if (rest) 
            fseeko (input, -rest * (off_t) sizeof (Value), SEEK_CUR);
        if (closeatend) 
            fclose (input);
    }
    FromFile &operator++ ();
    const Value& operator* () const {return *curr;}
    Value get() {Value r = *curr; ++*this; return r;}
    bool operator! () const {return  rest > 0;}
    void seek (off_t pos) {
        fseeko (input, pos * sizeof(Value), SEEK_SET);
        rest = 0;
        ++*this;
    }
};

template <class Value, int buffsize>
FromFile<Value, buffsize> &FromFile<Value, buffsize>::operator++ ()
{
    if (rest <= 1) {
        rest = fread (&buff, sizeof (Value), buffsize, input);
        curr = buff;
    } else {
        curr++;
        rest--;
    }
    return *this;
}


//-------------------- ToFile --------------------
template <class Value>
class ToFile 
{
    FILE *output;
    bool closeatend;
    std::string filename;
public:
    ToFile (FILE *file): output (file), closeatend (false) {}
    ToFile (const std::string &fname, bool append=false)
        : output (fopen (fname.c_str(), append ? "ab" : "wb")),
          closeatend(true), filename (fname)
    {
        if (output == NULL)
            throw FileAccessError (filename, "ToFile: fopen");
#if defined(__WIN32__) || defined(_WIN32)
        if (append)
            fseek (output, 0, SEEK_END);
#endif
    }
    ~ToFile () noexcept(false) {
        if (closeatend)
            if (fclose (output))
                throw FileAccessError (filename, "ToFile: fwrite");
    }
    void put (Value x) {
        if (fwrite (&x, sizeof (Value), 1, output) != 1)
            throw FileAccessError (filename, "ToFile: fwrite");
    }
    void operator()(Value x) {put (x);}
    off_t tell() {return ftello (output) / sizeof (Value);}
    void flush() {
        if (fflush (output))
            throw FileAccessError (filename, "ToFile: fflush");
    }
    void takeback (int items) {
        if (fseeko (output, -items * (off_t) sizeof (Value), SEEK_CUR))
            throw FileAccessError (filename, "ToFile: fseeko");
    }
};


#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
