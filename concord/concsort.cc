//  Copyright (c) 1999-2013  Pavel Rychly, Milos Jakubicek

#include "concord.hh"
#include "conccrit.hh"
#include "regpref.hh"
#include <algorithm>
#include <cmath>

using namespace std;

// ==================== Concordance::sort ====================
typedef pair<vector<string>,ConcIndex> PVI;
typedef vector<PVI> VSI;
template <class P>
struct compare_first_only {
    inline bool operator() (const P &x, const P &y)
    {
        return x.first < y.first;
    }
};

static compare_first_only<PVI> compare_first_only_PVI;

void Concordance::sort (const char *crit, bool uniq)
{
    sync();
    vector<criteria*>cr;
    RangeStream *concrs = this->RS (true);
    prepare_criteria (this->corp, concrs, crit, cr);
    if (cr.empty() || size() == 0) {
        delete concrs;
        return;
    }

    ensure_view();
    VSI data (viewsize());
    vector<ConcIndex>::iterator vi;
    VSI::iterator di;
    vector<criteria*>::iterator ci;
    for (vi=view->begin(), di=data.begin(); di < data.end(); di++, vi++) {
        for (ci = cr.begin(); ci != cr.end(); ci++)
            (*ci)->push (concrs, (*di).first);
        concrs->next();
        (*di).second = *vi;
    }
    // free criteria
    for (ci = cr.begin(); ci != cr.end(); ci++)
        delete *ci;

    stable_sort (data.begin(), data.end(), compare_first_only_PVI);
    if (uniq) {
        VSI::iterator prev = di = data.begin();
        vi = view->begin();
        *(vi++) = (*(di++)).second;
        for (; di < data.end(); di++) {
            if ((*di).first != (*prev).first) {
                *(vi++) = (*di).second;
                prev = di;
            } else {
                rng[(*di).second].beg = rng[(*di).second].end = -1;
            }
        }
        if (vi != view->end())
            view->erase (vi, view->end());
    } else {
        for (vi=view->begin(), di=data.begin(); di < data.end(); di++, vi++) {
            *vi = (*di).second;
        }
    }
    delete concrs;
}


// ==================== Concordance::count_sort ====================
class count_crit {
    PosAttr *pa;
    RangeStream *concrs;
    Concordance::context *beg;
    Concordance::context *end;
public:
    count_crit (RangeStream *concrs, Corpus *corp, const string &attr,
                const char *ctxbeg, const char *ctxend)
        : pa (corp->get_attr(attr)), concrs (concrs),
          beg (prepare_context (corp, ctxbeg, true)),
          end (prepare_context (corp, ctxend, false))
    {}
    ~count_crit() 
    {
        delete beg;
        delete end;
    }
    double count_freq (regexp_pattern *only)
    {
        Position b = beg->get (concrs);
        Position e = end->get (concrs);
        if (b <= e) {
            double cnt;
            double sum = 0.0;
            IDIterator *pi = pa->posat (b);
            if (only) {
                cnt = 0;
                for (; b <= e; b++) {
                    int id = pi->next();
                    if (only->match (pa->id2str (id))) {
                        sum += log (double (pa->freq (id) +1));
                        cnt++;
                    }
                }
            } else {
                cnt = e - b +1;
                for (; b <= e; b++)
                    sum += log (double(pa->freq (pi->next()) +1));
            }
            delete pi;
            //cerr << e << '\t' << sum/cnt << '\n';
            return cnt ? - sum / cnt : 0.0;
        } else 
            return 0.0;
    }
    double count_score (map<int,double> &score)
    {
        Position b = beg->get (concrs);
        Position e = end->get (concrs);
        if (b <= e) {
            double cnt = 0.0;
            double sum = 0.0;
            IDIterator *pi = pa->posat (b);
            for (; b <= e; b++) {
                double s = score[pi->next()];
                if (s) {
                    sum += s;
                    cnt++;
                }
            }
            delete pi;
            return cnt ? - sum / cnt : 0.0;
        } else 
            return 0.0;
    }
    void prepare_score (map<int,double> &score)
    {
        Position b = beg->get (concrs);
        Position e = end->get (concrs);
        if (b <= e) {
            IDIterator *pi = pa->posat (b);
            for (; b <= e; b++)
                score[pi->next()]++;
            delete pi;
        }
    }
};

typedef pair<double,ConcIndex> PDI;
static compare_first_only<PDI> compare_first_only_PDI;

void Concordance::count_sort (const char *leftctx, const char *rightctx, 
                              const string &attr, bool words_only)
{
    sync();
    if (size() == 0)
        return;

    RangeStream *rs = this->RS();
    count_crit *cr = new count_crit (rs, corp, attr, leftctx, rightctx);
    regexp_pattern *only_pat = NULL;
    if (words_only) {
        only_pat = new regexp_pattern ("[[:alpha:]]+", 
                                       corp->get_attr(attr)->locale);
        if (only_pat->compile()) {
            cerr << "count_sort: compile pattern error\n";
            delete only_pat;
            only_pat = NULL;
        }
    }

    ensure_view();
    vector<PDI> data (viewsize());
    vector<ConcIndex>::iterator vi;
    vector<PDI>::iterator di;
    for (vi=view->begin(), di=data.begin(); di < data.end(); di++, vi++) {
        (*di).first = cr->count_freq (only_pat);
        rs->next();
        (*di).second = *vi;
    }
    // free criteria
    delete cr;
    if (only_pat) delete only_pat;

    stable_sort (data.begin(), data.end(), compare_first_only_PDI);
    for (vi=view->begin(), di=data.begin(); di < data.end(); di++, vi++) {
        *vi = (*di).second;
    }
    delete rs;
}

void Concordance::relfreq_sort (const char *leftctx, const char *rightctx, 
                                const string &attr)
{
    sync();
    if (size() == 0)
        return;

    RangeStream *rs = this->RS();
    count_crit *cr = new count_crit (rs, corp, attr, leftctx, rightctx);
    map<int,double> score;

    ensure_view();
    vector<PDI> data (viewsize());
    vector<ConcIndex>::iterator vi;
    vector<PDI>::iterator di;
    for (vi=view->begin(), di=data.begin(); di < data.end(); di++, vi++) {
        cr->prepare_score (score);
        (*di).first = cr->count_score (score);
        rs->next();
        (*di).second = *vi;
    }
    // free criteria
    delete cr;
    stable_sort (data.begin(), data.end(), compare_first_only_PDI);
    for (vi=view->begin(), di=data.begin(); di < data.end(); di++, vi++) {
        *vi = (*di).second;
    }
    delete rs;
}

// ==================== Concordance::linegroup_sort ====================
typedef pair<string,ConcIndex> PSI;
static compare_first_only<PSI> compare_first_only_PSI;

void Concordance::linegroup_sort (map<short int,string> &ordertab)
{
    sync();
    if (size() == 0)
        return;
    ensure_view();
    vector<PSI> data (viewsize());
    vector<ConcIndex>::iterator vi;
    vector<PSI>::iterator di;
    for (vi=view->begin(), di=data.begin(); di < data.end(); di++, vi++) {
        int g = get_linegroup (*vi);
        (*di).first = ordertab[g];
        if ((*di).first.empty()) {
            static char grp[3] = "  ";
            if (g) {
                grp[0] = '0' + g / 10;
                grp[1] = '0' + g % 10;
            } else {
                grp[0] = '?';
                grp[1] = '\0';
            }
            (*di).first = ordertab[g] = grp;
        }
        (*di).second = *vi;
    }
    stable_sort (data.begin(), data.end(), compare_first_only_PSI);
    for (vi=view->begin(), di=data.begin(); di < data.end(); di++, vi++) {
        *vi = (*di).second;
    }
}

// ==================== Concordance::set_sorted_view_sort ====================
void Concordance::set_sorted_view (const vector<ConcIndex> &sorted)
{
    ensure_view();
    if (sorted.size() > view->size()) {
        delete view;
        view = new vector<ConcIndex>(size());
    }   
    if (sorted.size() < view->size()) {
        vector<bool> sorted_set(size(), false);
        for (vector<ConcIndex>::const_iterator i = sorted.begin();
             i != sorted.end(); i++)
            sorted_set[*i] = true;
        ConcIndex delta = 0;
        for (ConcIndex i=view->size()-1; i >= 0; i--) {
            ConcIndex line_idx = (*view)[i];
            if (sorted_set[line_idx])
                delta++;
            else
                if (delta)
                    (*view)[i+delta] = line_idx;
        }
    }
    vector<ConcIndex>::iterator vi = view->begin();
    for (vector<ConcIndex>::const_iterator si = sorted.begin();
         si != sorted.end(); ++si, ++vi)
        *vi = *si;

}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
