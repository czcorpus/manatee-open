//  Copyright (c) 1999-2015  Pavel Rychly, Milos Jakubicek

#ifndef CONCORD_HH
#define CONCORD_HH

#include "corpus.hh"

#include <iostream>
#include <stdint.h>

class ConcNotFound: public std::exception {
    const std::string _what;
public:
    const std::string name;
    ConcNotFound (const std::string &name)
        :_what ("Concordance `" + name + "' not defined"), name (name) {}
    virtual const char* what () const throw () {return _what.c_str();}
    virtual ~ConcNotFound() throw () {}
};

typedef int32_t ConcIndex; // must be signed

struct ConcItem {
    Position beg;
    Position end;
    ConcItem () {}
    ConcItem (Position b, Position e): beg(b), end(e) {}
};

struct collocitem {
    int8_t beg, end;
};

class ConcStream; // forward declare

class Concordance {
    void load_file (Corpus *corp, FILE *f, const string fname="");
    void save (FILE *f, const char *filename=NULL, bool save_linegroup=false,
               bool partial=false, bool append=false);
public:
    //-------------------- Context --------------------
    class context {
    public:
        int chars;
        context (int ch=0): chars (ch) {}
        virtual ~context () {};
        virtual Position get (RangeStream *r) = 0;
    };
    class criteria;
protected:
    //-------------------- Collocations --------------------
    std::vector<collocitem*> colls;
    std::vector<ConcIndex> coll_count;
    static const int cnotdef = -128;
    static const int kwicnotdef = -1;

    //-------------------- private members --------------------
    ConcItem *rng;
    int nestval;
    ConcIndex allocated, used;
    std::vector<ConcIndex> *view;
    typedef std::vector<short int> linegroup_t;
    linegroup_t *linegroup;
    void *rng_mutex;
    void *thread_id;
    RangeStream *query;
    bool is_finished;
    ConcIndex sample_size;
    NumOfPos full_size;
    int label;
    bool added_align; // true for parallel corpus added by add_aligned()
    int maxkwic;
    //------------------- CorpData ------------------------
    struct CorpData {
        std::vector<collocitem*> colls;
        std::vector<ConcIndex> coll_count;
        ConcItem *rng;
        Corpus *corp;
        int label;
        bool added_align; // true for parallel corpus added by add_aligned()
    };
    std::vector<CorpData*> aligned;
    
    struct set_collocation_data;
    friend void *evaluate_query (Concordance *conc);
    friend void *evaluate_colloc (set_collocation_data *data);
    friend void *eval_query_thread (void *conc);
    friend void free_rngmutex (void *conc);
    friend class ConcStream;
    void lock();
    void unlock();
    void ensure_view();
    template <class ConcData>
    void delete_lines (ConcData *data, ConcIndex newsize, int collnum,
                       bool positive, std::vector<ConcIndex> *view,
                       std::vector<ConcIndex> *revview);
    template <class ConcData>
    void delete_subpart_lines (ConcData *data, std::vector<ConcIndex> *view,
                               std::vector<ConcIndex> *revview);
    template <class ConcData1, class ConcData2>
    void filter_aligned_lines (ConcData1 *aligned, ConcData2 *data,
                               std::vector<ConcIndex> *view,
                               std::vector<ConcIndex> *revview, ConcIndex size);
    template <class ConcData>
    void delete_struct_repeats_lines (ConcData *data,
         std::vector<ConcIndex> *view, std::vector<ConcIndex> *revview,
         const char *struc);
    //-------------------- public members --------------------
public:
    static const int lngroup_labidx;
    Corpus * corp;
    NumOfPos corp_size;

    Concordance ();
    Concordance (Corpus *corp, RangeStream *query, int inccolln=0,
                 ConcIndex sample_size=0, NumOfPos full_size=0);
    Concordance (Concordance &x);
    Concordance (Corpus *corp, int fileno);
    Concordance (Corpus *corp, const char *filename);
    ~Concordance ();
    void load_from_file(Corpus *corp, const char *filename);
    void load_from_fileno(Corpus *corp, int fileno);
    void load_from_rs(Corpus *corp, RangeStream *rs,
                 ConcIndex sample_size, NumOfPos full_size);
    Concordance* copy();
    void save (int fileno, bool save_linegroup=false,
               bool partial=false, bool append=false);
    void save (const char *filename, bool save_linegroup=false,
               bool partial=false, bool append=false);
    void sync ();
    ConcIndex size() const {return used;}
    int nesting() const {return nestval;}
    int numofcolls() const {return colls.size();}
    ConcIndex viewsize() const {return view ? view->size() : size();}
    NumOfPos fullsize() const {return full_size > 0 ? full_size : size();}
    bool finished() const {return is_finished;}

    void set_collocation (int collnum, const std::string &cquery, 
                          const char *lctx, const char *rctx, int rank,
                          bool exclude_kwic = false);

    Position beg_at (ConcIndex idx) {
        lock();
        Position ret = rng[idx].beg;
        unlock();
        return ret;
    }
    Position end_at (ConcIndex idx) {
        lock();
        Position ret = rng[idx].end;
        unlock();
        return ret;
    }
    Position coll_beg_at (int coll, ConcIndex idx) {
        //XXX zamykani
        if (coll <= 0 || unsigned (coll) > colls.size())
            return beg_at (idx);
        if (! colls[coll-1] || beg_at (idx) == -1)
            return -1;
        int d = colls [coll-1][idx].beg;
        return d == cnotdef ? -1 : beg_at (idx) + d;
    }
    Position coll_end_at (int coll, ConcIndex idx) {
        //XXX zamykani
        if (coll <= 0 || unsigned (coll) > colls.size())
            return end_at (idx);
        if (! colls[coll-1] || beg_at (idx) == -1)
            return -1;
        Position d = colls [coll-1][idx].end;
        return d == cnotdef ? -1 : beg_at (idx) + d;
    }
    FastStream *begs_FS();
    RangeStream *RS (bool useview = false, ConcIndex beg = 0, ConcIndex end=0);
    void get_aligned (std::vector<std::string> &corpnames);

    //-------------------- linegroups --------------------
    void set_linegroup (ConcIndex linenum, int group);
    void set_linegroup_globally (int group);
    int set_linegroup_at_pos (Position pos, int group);
    void set_linegroup_from_conc (Concordance *master);
    int get_linegroup (ConcIndex lineidx) {
        return (linegroup && lineidx >= 0 && lineidx < size()) ? 
            (*linegroup)[lineidx] : 0;
    }
    void get_linegroup_stat (std::map<short int,ConcIndex> &lgs);
    int get_new_linegroup_id();
    void delete_linegroups (const char *grps, bool invert);


    //-------------------- KWICLines --------------------
    friend class KWICLines;
    void tcl_get (std::ostream &out, ConcIndex beg, ConcIndex end, 
                  const char *left, const char *right,
                  const char *ctxa, const char *kwica,
                  const char *struca, const char *refa);
    void tcl_get_reflist (std::ostream &out, ConcIndex idx, const char *reflist);

    //-------------------- statistics --------------------
    int distribution (std::vector<int> &vals, std::vector<int> &beginnings,
                      int yrange, bool normalize=true);
    NumOfPos redfreq ();
    double compute_ARF();
    void sort (const char *crit, bool uniq = 0);
    void count_sort (const char *leftctx, const char *rightctx, 
                     const std::string &attr, bool words_only);
    void relfreq_sort (const char *leftctx, const char *rightctx, 
                       const std::string &attr);
    void linegroup_sort (std::map<short int,std::string> &ordertab);
    /*
     * define view order for the (first part of) the concordance
     *     sorted is a list of line numbers
     */
    void set_sorted_view (const std::vector<ConcIndex> &sorted);
    void find_coll (std::ostream &out, const std::string &attr, 
                    const std::string &cmpr, NumOfPos minfreq, NumOfPos minbgr,
                    int fromw, int tow, int maxlines, 
                    const char *stats="mtrf");
    void sort_idx (const char *crit, std::vector<std::string> &chars,
                   std::vector<int> &idxs, bool firstCharOnly = true);
    void poss_of_selected_lines (std::ostream &out, const char *rngs);
    void delete_lines (const char *rngs);
    void reduce_lines (const char *crit);
    void delete_view_lines (const char *rngs);
    void reduce_view_lines (const char *crit);
    void delete_pnfilter (int collnum, bool positive);
    void swap_kwic_coll (int collnum);
    void extend_kwic_coll (int collnum);
    void shuffle();
    void make_grouping();
    void switch_aligned (const char *corpname);
    void add_aligned (const char *corpname);
    void filter_aligned (const char *corpname);
    void delete_subparts ();
    void delete_struct_repeats (const char *struc);
};

#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
