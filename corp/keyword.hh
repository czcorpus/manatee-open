//  Copyright (c) 2016-2023  Milos Jakubicek

#include <finlib/regpref.hh>
#include "subcorp.hh"
#include <unordered_set>
#include <fstream>
#include <iterator>

struct kwitem {
    int id1, id2;
    float score;
    string str;
    double *freqs;
    kwitem (int id1, int id2, float score, string &s, double *f) :
            id1(id1), id2(id2), score(score), str(s), freqs(f) {}
    ~kwitem() {delete[] freqs;}
};

class Keyword {
    struct kwitem_cmp {
        bool operator()(const kwitem *a,const kwitem *b) const{
            return a->score > b->score;
        }
    };
    class strid_gen {
        int id;
        IdStrGenerator *strid_it;
        WordListLeftJoin *wl_it;
    public:
        strid_gen (IdStrGenerator *it) : id(0), strid_it(it), wl_it (NULL) {}
        strid_gen (WordListLeftJoin *it) : strid_it (NULL), wl_it(it) {}
        ~strid_gen() {delete strid_it; delete wl_it;}
        void next (string &str, int &id1, int &id2)
        {
            if (strid_it) {
                str = strid_it->getStr();
                id1 = id2 = strid_it->getId();
                strid_it->next();
            } else
                wl_it->next (str, id1, id2);
        }
        bool end() {return (strid_it) ? strid_it->end() : wl_it->end();}
    };
    vector<kwitem*> heap;
    int curr;
public:
    NumOfPos totalcount, totalfreq1, totalfreq2;
    Keyword (Corpus *c1, Corpus *c2, WordList *wl1, WordList *wl2, float N,
             unsigned maxlen, int minfreq, int maxfreq, unordered_set<string> &blacklist,
             unordered_set<string> &whitelist, const char *frqtype, vector<string> &addfreqs,
             vector<string> pos_filters, vector<string> neg_filters,
             FILE* progress);
    ~Keyword() {for (auto &p: heap) delete p;}
    int size() {return heap.size();}
    const kwitem* next() {return (curr < size()) ? heap[curr++] : NULL;}
};

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
