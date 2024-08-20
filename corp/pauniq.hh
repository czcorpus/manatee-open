//  Copyright (c) 2012  Pavel Rychly, Milos Jakubicek

#ifndef PAUNIQATTR_HH
#define PAUNIQATTR_HH

#include "posattr.hh"

PosAttr *createUniqPosAttr (const std::string &path, const std::string &n,
                            const std::string &locale, const std::string &enc,
                            int text_size = 0);
#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
