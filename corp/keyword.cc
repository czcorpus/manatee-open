//  Copyright (c) 2016-2023  Milos Jakubicek

#include <finlib/regpref.hh>
#include "subcorp.hh"
#include "keyword.hh"

bool check_string (string &str, vector<regexp_pattern*> pos_filters, vector<regexp_pattern*> neg_filters,
                   unordered_set<string> &blist, unordered_set<string> &wlist)
{
    for (auto it = neg_filters.begin(); it != neg_filters.end(); it++) {
        if ((*it)->match(str.c_str()))
            return false;
    }
    for (auto it = pos_filters.begin(); it != pos_filters.end(); it++) {
        if (!(*it)->match(str.c_str()))
            return false;
    }
    if (!blist.empty() && blist.find(str) != blist.end())
        return false;
    if (!wlist.empty() && wlist.find(str) == wlist.end())
        return false;
    return true;
}

class AllowMissingFrequency : public Frequency {
    unique_ptr<Frequency> src;
public:
    AllowMissingFrequency (WordList *wl, const char *frqtype) {
        try {
            src.reset(wl->get_stat(frqtype));
        } catch (FileAccessError&) {}
    }
    double freq(int id) {return src ? src->freq(id) : -1;}
};

Keyword::Keyword (Corpus *c1, Corpus *c2, WordList *wl1, WordList *wl2, float N,
             unsigned maxlen, int minfreq, int maxfreq, unordered_set<string> &blacklist,
             unordered_set<string> &whitelist, const char *frqtype, vector<string> &addfreqs,
             vector<string> pos_regex_filters, vector<string> neg_regex_filters, FILE* progress)
            : curr (0), totalcount(0), totalfreq1(0), totalfreq2(0)
{
    vector<regexp_pattern*> pos_regpats;
    for (auto it = pos_regex_filters.begin(); it != pos_regex_filters.end(); it++) {
        if (!(*it).size())
            continue;
        regexp_pattern *re = new regexp_pattern((*it).c_str(), wl1->locale, wl1->encoding);
        if (re->compile())
            throw new CorpInfoNotFound("Invalid regular expression for Keyword");
        pos_regpats.push_back(re);
    }

    vector<regexp_pattern*> neg_regpats;
    for (auto it = neg_regex_filters.begin(); it != neg_regex_filters.end(); it++) {
        if (!(*it).size())
            continue;
        regexp_pattern *re = new regexp_pattern((*it).c_str(), wl1->locale, wl1->encoding);
        if (re->compile())
            throw new CorpInfoNotFound("Invalid regular expression for Keyword");
        neg_regpats.push_back(re);
    }

    strid_gen *it = NULL;
    if (c1->get_confpath() == c2->get_confpath())
        it = new strid_gen (wl1->dump_str());
    else
        it = new strid_gen (new WordListLeftJoin (wl1->attr_path, wl2->attr_path));

    const float c1size = c1->search_size();
    const float c2size = c2->search_size();
    const float wl1_maxid = wl1->id_range();
    Frequency *stat1 = wl1->get_stat(frqtype);
    Frequency *stat2 = wl2->get_stat(frqtype);
    vector<AllowMissingFrequency> addfreqs1, addfreqs2;
    for (auto it = addfreqs.begin(); it != addfreqs.end(); it++) {
        addfreqs1.emplace_back(wl1, (*it).c_str());
        addfreqs2.emplace_back(wl2, (*it).c_str());
    }

    while (!it->end()) {
        string str;
        int id1, id2;
        it->next (str, id1, id2);
        if (progress && id1 % 100000 == 0)
            fprintf(progress, "\r%.2f %%", id1 / wl1_maxid * 100);
        NumOfPos f1 = stat1->freq(id1);
        NumOfPos f2;
        if (id2 == -1)
            f2 = 0;
        else
            f2 = stat2->freq(id2);
        if (minfreq && f1 < minfreq)
            continue;
        if (maxfreq && f1 > maxfreq)
            continue;
        totalcount++;
        totalfreq1 += f1;
        totalfreq2 += f2;
        float fpm1 = f1 * 1000000 / c1size;
        float fpm2 = f2 * 1000000 / c2size;
        // adding 1 additional field to `freqs`
        double *freqs = new double[2*addfreqs.size() + 4 + 1];
        freqs[0] = f1; freqs[1] = f2;
        freqs[2] = fpm1; freqs[3] = fpm2;
        for (unsigned i = 0; i < addfreqs.size(); i++) {
            freqs[2*i+4] = addfreqs1[i].freq(id1);
            freqs[2*i+5] = id2 == -1 ? 0 : addfreqs2[i].freq(id2);
        }

        float score;
        if (scoretype == "logL") {
            double e1 = c1size * (f1 + f2) / c12size;
            double e2 = c2size * (f1 + f2) / c12size;
            // log-likelihood
            score = 2 * (f1*log(f1/e1) + f2*log(f2/e2));
        } else if (scoretype == "chi2") {
            double e1 = c1size * (f1 + f2) / c12size;
            double e2 = c2size * (f1 + f2) / c12size;
            // Pearson's chi-squared statistic (TODO Yates correction?)
            score = (f1-e1)*(f1-e1)/e1 + (f2-e2)*(f2-e2)/e2;
        } else {
            score = (fpm1 + N) / (fpm2 + N);
        }
        // DIN size effect
        freqs[2*addfreqs.size() + 4] = 100 * ((fpm1 - fpm2) / (fpm1 + fpm2));

        if (heap.size() < maxlen) {
            if (!check_string (str, pos_regpats, neg_regpats, blacklist, whitelist)) {
                delete[] freqs;
                continue;
            }
            kwitem *k = new kwitem (id1, id2, score, str, freqs);
            heap.push_back (k);
            push_heap (heap.begin(), heap.end(), kwitem_cmp());
        } else if (heap.front()->score < score) {
            if (!check_string (str, pos_regpats, neg_regpats, blacklist, whitelist)) {
                delete[] freqs;
                continue;
            }
            pop_heap (heap.begin(), heap.end(), kwitem_cmp());
            delete heap.back();
            heap.pop_back();
            kwitem *k = new kwitem (id1, id2, score, str, freqs);
            heap.push_back (k);
            push_heap (heap.begin(), heap.end(), kwitem_cmp());
        } else
            delete[] freqs;
    }
    delete it;
    delete stat1; delete stat2;
    for (auto it = pos_regpats.begin(); it != pos_regpats.end(); it++)
        delete *it;
    for (auto it = neg_regpats.begin(); it != neg_regpats.end(); it++)
        delete *it;
    if (progress)
        fprintf(progress, "\r100 %%     \n");
    sort_heap (heap.begin(), heap.end(), kwitem_cmp());
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
