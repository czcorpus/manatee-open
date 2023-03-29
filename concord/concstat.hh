//  Copyright (c) 2004-2007  Pavel Rychly

#ifndef CONCSTAT_HH
#define CONCSTAT_HH

#include "concord.hh"

struct CollItem;

class CollocItems {
protected:
    PosAttr *attr;
    CollItem *results;
    CollItem *curr;
    CollItem *last;
    double f_B;
    double N;
public:
    CollocItems (Concordance *conc, const std::string &attr_name, 
		 char sort_fun_code, NumOfPos minfreq, NumOfPos minbgr, 
		 int fromw, int tow, int maxitems);
    ~CollocItems();
    void next();
    bool eos();
    const char *get_item();
    NumOfPos get_freq();
    NumOfPos get_cnt();
    double get_bgr (char bgr_code);
};


#endif
