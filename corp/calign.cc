//  Copyright (c) 2007-2020  Pavel Rychly, Milos Jakubicek

#include "corpus.hh"
#include "frsop.hh"
#include "levels.hh"
#include <sstream>
#include <iostream>

using namespace std;


Corpus *Corpus::get_aligned (const string &corp_name)
{
    for (unsigned c = 0; c < aligned.size(); c++)
        if (aligned[c].corp_name == corp_name) {
            if (!aligned[c].corp)
                aligned[c].corp = new Corpus (corp_name);
            return aligned[c].corp;
        }
    throw CorpInfoNotFound (corp_name + " not aligned");
}

TokenLevel *Corpus::get_aligned_level (const string &corp_name)
{
    const string levelfile = get_conf ("PATH") + "align." + corp_name;
    for (unsigned c = 0; c < aligned.size(); c++)
        if (aligned[c].corp_name == corp_name) {
            if (!aligned[c].level)
                aligned[c].level = new_TokenLevel (levelfile);
            return aligned[c].level;
        }
    throw CorpInfoNotFound (corp_name + " not aligned");
}

RangeStream *Corpus::map_aligned (Corpus *al_corp, RangeStream *src, bool add_labels)
{
    int corp_num = -1;
    for (unsigned i = 0; i < aligned.size(); i++)
        if (aligned[i].corp == al_corp) {
            corp_num = i;
            break;
        }

    if (corp_num == -1)
        throw CorpInfoNotFound (al_corp->get_confpath() + " not aligned");
    if (add_labels)
        src = new AddRSLabel (src, (corp_num + 1) * 100);
    
    Structure *als = al_corp->get_struct(al_corp->get_conf("ALIGNSTRUCT"));
    FastStream *snums = als->part_nums(src);
    if (! al_corp->get_conf("ALIGNDEF").empty()) 
        snums = tolevelfs (al_corp->get_aligned_level (get_conffile()),
                           snums);
    return get_struct(get_conf("ALIGNSTRUCT"))->rng->part (snums);
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
