// Copyright (c) 2013  Pavel Rychly

#include <config.hh>
#include "fingetopt.hh"

#if ! HAVE_GETOPT

const char *optarg;
int optind = 1;

int getopt (int argc, char * const argv[], const char *optstring)
{ 
    const char *s, *par;
    if (optind >= argc) return -1;
    optarg = "";
    par = argv [optind];
    if(*par != '-') return -1;
    optind++;

    for (s = optstring; *s && *s != *par; s++)
	;
    if (*s==0) return -1;
    if (s[1]==':')
	if (par[1]) 
	    optarg = par +1; 
	else 
	    optarg = optind < argc ? argv[optind++] : "";
    return *par;
}

#endif

