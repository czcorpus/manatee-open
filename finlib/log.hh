// Copyright (c) 2013  Milos Jakubicek

#ifndef LOG_HH
#define LOG_HH

const char *currtime();
// extern "C" so that it can be checked by autoconf
extern "C" const char *finlib_version();
#define CERR std::cerr << currtime()

#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
