//  Copyright (c) 1999-2016  Pavel Rychly, Milos Jakubicek

#include "config.hh"
#include "concord.hh"
#include "conccrit.hh"
#include "bgrstat.hh"
#include "utf8.hh"
#include "fsstat.hh"
#include "srtruns.hh"
#include "frsop.hh"
#include <algorithm>
#include <unordered_map>
#include <functional>

using namespace std;

typedef unordered_map<string,NumOfPos> data_t;

void combine_multivalue_attrs(const string *tocomp, vector<Concordance::criteria*> *cr,
       data_t* data, int cr_idx, vector<string> *multival);

void process_attr(const string *tocomp, vector<Concordance::criteria*> *cr,
        data_t* data, size_t cr_idx, vector<string> *multival, const string *attr) {

    string attr_list;
    if (cr_idx != 0) {
        attr_list = *tocomp + '\t' + *attr;
    } else {
        attr_list = *attr;
    }
    if (cr_idx != (*cr).size() - 1) {
        combine_multivalue_attrs(&attr_list, cr, data, cr_idx + 1, multival);
    } else {
        (*data)[attr_list]++;
    }
}

void combine_multivalue_attrs(const string *tocomp, vector<Concordance::criteria*> *cr,
       data_t* data, int cr_idx, vector<string> *multival) {

    size_t pos = string::npos;
    size_t last_pos = 0;
    char multisep;
    if ((*cr)[cr_idx]->multisep) { //NULL if MULTIVALUE disabled
        multisep = (*cr)[cr_idx]->multisep[0];
        if (multisep == '\0') { // separated by chars
            pos = last_pos + 1;
        } else {
            pos = (*multival)[cr_idx].find(multisep, 0);
        }
    }

    string attr;
    while (pos != string::npos && pos < (*multival)[cr_idx].size()) {
        attr = (*multival)[cr_idx].substr(last_pos, pos - last_pos);
        process_attr(tocomp, cr, data, cr_idx, multival, &attr);
        if (multisep == '\0') { // separated by chars
            last_pos = pos;
            pos = last_pos + 1;
        } else {
            last_pos = pos + 1;
            pos = (*multival)[cr_idx].find(multisep, last_pos);
        }
    }
    attr = (*multival)[cr_idx].substr(last_pos, (*multival)[cr_idx].size() - last_pos);
    process_attr(tocomp, cr, data, cr_idx, multival, &attr);
}

// ==================== RangeStream::freq_dist ====================

void Corpus::freq_dist (RangeStream *r, ostream &out, const char *crit, NumOfPos limit)
{
    if (r->end()) {
        delete r;
        return;
    }
    vector<Concordance::criteria*>cr;
    prepare_criteria (this, r, crit, cr);
    if (cr.empty()) {
        delete r;
        return;
    }

    data_t data;
    vector<Concordance::criteria*>::iterator ci;
    do {
        if (r->peek_beg() == -1) continue;
        vector<string> multival;
        for (ci = cr.begin(); ci != cr.end(); ci++) {
            multival.push_back((*ci)->get (r, true));
        }
        const string tocomp = "";
        combine_multivalue_attrs(&tocomp, &cr, &data, 0, &multival);
    } while (r->next());

    // free criteria
    for (ci = cr.begin(); ci != cr.end(); ci++)
        delete *ci;
    // output results
    for (data_t::iterator mi=data.begin(); mi != data.end(); mi++)
        if ((*mi).second >= limit)
            out << (*mi).second << '\t' << (*mi).first << '\n';
    delete r;
}

void Corpus::freq_dist (RangeStream *r, const char *crit, NumOfPos limit,
                vector<string> &words, vector<NumOfPos> &freqs,
                vector<NumOfPos> &norms)
{
    if (r->end()) {
        delete r;
        return;
    }
    vector<Concordance::criteria*>cr;
    prepare_criteria (this, r, crit, cr);
    if (cr.empty()) {
        delete r;
        return;
    }

    data_t data;
    vector<Concordance::criteria*>::iterator ci;
    for (ci = cr.begin(); ci != cr.end(); ci++)
            (*ci)->separator = '\v';
    do {
        if (r->peek_beg() == -1) continue;
        vector<string> multival;
        for (ci = cr.begin(); ci != cr.end(); ci++) {
            multival.push_back((*ci)->get (r, true));
        }
        const string tocomp = "";
        combine_multivalue_attrs(&tocomp, &cr, &data, 0, &multival);
    } while (r->next());
    PosAttr* first_struct_attr = NULL;
    Frequency* norm = NULL;
    if (cr[0]->get_attr() && strchr(cr[0]->get_attr()->name.c_str(), '.')) {
        first_struct_attr = cr[0]->get_attr();
        norm = first_struct_attr->get_stat("token:l");
    }
    // free criteria
    for (ci = cr.begin(); ci != cr.end(); ci++)
        delete *ci;
    // output results
    for (data_t::iterator mi=data.begin(); mi != data.end(); mi++)
        if ((*mi).second >= limit) {
            words.push_back ((*mi).first);
            freqs.push_back ((*mi).second);
            NumOfPos normval = 0;
            if (first_struct_attr) {
                int id = first_struct_attr->str2id ((*mi).first.c_str());
                if (id >= 0)
                    normval = norm->freq (id);
            }
            norms.push_back (normval);
        }
    delete norm;
    delete r;
}

void Concordance::sort_idx (const char *crit, vector<string> &chars,
                             vector<int> &idxs, bool firstCharOnly)
{
    if (!view) { // concordance is not sorted
        return;
    }
    vector<criteria*>cr;
    RangeStream *concrs = this->RS (true);
    prepare_criteria (this->corp, concrs, crit, cr);
    if (cr.empty()) {
        delete concrs;
        return;
    }

    criteria *prepcrit = cr[0];
    data_t data;
    string last;

    // XXX could be speeded up by binary search
    for (Position idx=0; idx < size(); idx++) {
        string curr(prepcrit->get (concrs, true));
        if (((firstCharOnly && utf8char(curr.c_str(),0) != utf8char(last.c_str(),0)) ||
             (!firstCharOnly && curr != last)) && data.find(curr) == data.end()) {
            data[curr] = idx;
            last = curr;
        }
        concrs->next();
    }
    // free criteria
    vector<criteria*>::iterator ci;
    for (ci = cr.begin(); ci != cr.end(); ci++)
        delete *ci;
    // sort according to idx
    typedef pair<NumOfPos, const char*> str_idx;
    vector<str_idx> sorted_data;
    for (data_t::iterator mi=data.begin(); mi != data.end(); mi++) {
        sorted_data.push_back (make_pair ((*mi).second, (*mi).first.c_str()));
    }
    std::sort (sorted_data.begin(), sorted_data.end());
    // output results
    for (vector<str_idx>::iterator mi=sorted_data.begin(); mi != sorted_data.end(); mi++) {
        idxs.push_back ((*mi).first);
        chars.push_back ((*mi).second);
    }
    delete concrs;
    return;
}


// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
