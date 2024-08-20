//  Copyright (c) 2000-2017  Pavel Rychly, Milos Jakubicek

#ifndef DYNATTR_HH
#define DYNATTR_HH

class PosAttr;
#include "dynfun.hh"
#include <string>

PosAttr *createDynAttr (const std::string &type, const std::string &apath, 
            const std::string &n,
            DynFun *fun, PosAttr *from, const std::string &locale,
            bool trasquery=false, bool ownedByCorpus = true);

#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
