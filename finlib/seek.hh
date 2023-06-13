// Copyright (c) 2000-2004  Pavel Rychly

#ifndef SEEK_HH
#define SEEK_HH

#include <config.hh>
#include <cstdio>

#ifndef HAVE_FSEEKO
#define fseeko fseek
#define ftello ftell
#endif


#endif
