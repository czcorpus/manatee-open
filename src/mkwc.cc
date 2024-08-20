// Copyright (c) 2021 Ondrej Herman
#include <inttypes.h>
#include <iostream>
#include "corpus.hh"

int main (int argc, const char **argv) {
    const char *corpname;
    const char *attrname;

    if (argc != 3) {
        std::cerr << "usage: " << argv[0] << " CORPUS ATTRIBUTE" << std::endl
            << "count the corpus positions not matching NONWORDRE" << std::endl;
        return 1;
    }
    corpname = argv[1];
    attrname = argv[2];

    try {
        Corpus *corp = new Corpus(corpname);
        PosAttr *attr = corp->get_attr(attrname);
        Frequency *freq = attr->get_stat("frq");
        regexp_pattern *re = new regexp_pattern(
            corp->get_conf("NONWORDRE").c_str(), attr->locale, attr->encoding);
        if (re->compile())
            throw std::runtime_error("failed to compile NONWORDRE");
        NumOfPos totalwords = 0;
        for (int id = 0; id < attr->id_range(); id++) {
            if (re->match(attr->id2str(id))) {
                // this lexicon item is a nonword
            } else {
                totalwords += freq->freq(id);
            }
        }
        std::cout << totalwords << std::endl; 
        delete re; delete freq; delete corp;
    } catch (exception &e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

