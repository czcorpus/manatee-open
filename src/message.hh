// Copyright (c) 2001-2003  Pavel Rychly

#ifndef MESSAGES_HH
#define MESSAGES_HH

#include <cstdio>
#include "fingetopt.hh"

class message
{
    FILE *outf;
    char *progname;
    int msg_num;
public:
    message (const char *prog, const char *id=NULL, FILE *out = stderr);
    message ();
    ~message ();
    void setup (const char *prog, const char *id=NULL, FILE *out = stderr);
    void operator() (const char *msg, ...);
};


#endif
