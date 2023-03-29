//  Copyright (c) 1999-2013  Pavel Rychly, Milos Jakubicek

#ifndef CONCCRIT_HH
#define CONCCRIT_HH

#include "concord.hh"

/**
 * criteria for sorting concordance or freq. distribution of concordance
 */
class Concordance::criteria {
public:
    char separator;
    const char *multisep;
    criteria(): separator('\t'), multisep (NULL) {}
    virtual ~criteria();
    virtual void push (RangeStream *r, std::vector<std::string> &v) =0;
    virtual const char *get (RangeStream *r, bool case_retro=false) =0;
    virtual PosAttr *get_attr();
};

void 
prepare_criteria (Corpus *c, RangeStream *r, const char *critstr,
                  std::vector<Concordance::criteria*> &vec);

Concordance::context*
prepare_context (Corpus *c, const char *ctxstr, bool toleft, int maxctx=0);

#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
