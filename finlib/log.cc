// Copyright (c) 2013  Milos Jakubicek

#include "config.hh"
#include <ctime>
#include <iostream>

const char *currtime()
{
    time_t rawtime;
    struct tm* timeinfo;
    static char buffer [80];
    time (&rawtime);
    timeinfo = localtime (&rawtime);
    strftime (buffer, 80, "[%Y%m%d-%H:%M:%S] ", timeinfo);
    return (const char*) &buffer;
}

// extern "C" so that it can be checked by autoconf
extern "C" const char *finlib_version() {return "2.36.7";}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
