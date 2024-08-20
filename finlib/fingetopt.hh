// Copyright (c) 2013  Pavel Rychly

#ifndef FINGETOPT_HH
#define FINGETOPT_HH

#include <config.hh>

#if ! HAVE_GETOPT

extern const char *optarg;
extern int optind;

int getopt (int argc, char * const argv[], const char *optstring);

#endif


#endif
