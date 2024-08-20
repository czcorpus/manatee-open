//  Copyright (c) 2002-2017  Pavel Rychly, Milos Jakubicek

#ifndef SUBCORP_HH
#define SUBCORP_HH

#include "corpus.hh"
#include <set>

class SubCorpus : public Corpus {
    friend class SubcNums;
private:
    NumOfPos search_size_cached;
    const bool complement;
public:
    ranges *subcorp;
    SubCorpus (const Corpus *corp, const std::string &sub,
               bool complement = false);
    virtual ~SubCorpus ();
    virtual RangeStream *filter_query (RangeStream *s);
    virtual IDPosIterator *filter_idpos (IDPosIterator *it);
    virtual NumOfPos search_size();
    virtual std::string get_info();
    virtual bool is_complement() { return complement; }
    virtual WordList* get_wordlist (const string &path);
};

bool create_subcorpus (const char *subcpath, RangeStream *r,
                       Structure *s = NULL);
bool create_subcorpus (const char *subcpath, Corpus *corp,
                       const char *structname, const char *query);
void find_subcorpora (const char *subcdir, std::map<std::string,
                      std::pair<std::string,std::string> > &scs);
void merge_subcorpora(const char *subc1, const char *subc2, const char *subcout);

#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
