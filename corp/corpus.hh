//  Copyright (c) 1999-2018  Pavel Rychly, Milos Jakubicek

#ifndef CORPUS_HH
#define CORPUS_HH

#include "range.hh"
#include "posattr.hh"
#include "corpconf.hh"
#include "virtcorp.hh"
#include "excep.hh"
#include "levels.hh"
#include "wordlist.hh"
#include <vector>
#include <unordered_set>

using namespace std;

class Structure;

class Corpus {
public:
    typedef enum {Corpus_type, Struct_type} type_t;
    typedef vector<pair<string,PosAttr*> > VSA;
    typedef vector<pair<string,Structure*> > VSS;
protected:
    struct AlignedCorpus {
        string corp_name;
        TokenLevel *level;
        Corpus *corp;
        AlignedCorpus (const string &c):
            corp_name (c), level (NULL), corp (NULL) {}
    };
    void init (CorpInfo *ci);
    Corpus (CorpInfo *ci, type_t t=Corpus_type);
    VSA attrs;
    VSS structs;
    vector<AlignedCorpus> aligned;
    PosAttr *defaultattr;
    int maxctx;
    int hardcut;
    VirtualCorpus *virt;
    virtual PosAttr *setup_attr (const string &name, bool skip_subcorp = false);
    virtual Structure *setup_struct (const string &name);
    PosAttr *get_struct_pos_attr (const string &strname,
                                  const string &attname);
    friend void *eval_query_thread (void *conc);
    friend class Concordance;
public:
    type_t type;
    CorpInfo *conf;

    Corpus (const string &corp_name);
    virtual ~Corpus ();
    virtual RangeStream *filter_query (RangeStream *s) {return s;}
    virtual IDPosIterator *filter_idpos (IDPosIterator *it) {return it;}
    PosAttr *get_attr (const string &name, bool struct_attr = false);
    PosAttr *get_default_attr();
    void set_default_attr (const string &attname);
    void set_reference_corpus (const string &refcorp);
    Structure *get_struct (const string &name);
    virtual Position size() {
        return get_default_attr()->size();
    }
    virtual Position search_size() {return size();}
    virtual string get_info();
    const string &get_conf (const string &item) {
        return conf->find_opt (item);
    }
    const string &get_confpath () {
        return conf->conffile;
    }
    const char *get_conffile () {
        const char *path = conf->conffile.c_str();
        size_t slash = conf->conffile.rfind("/");
        if (slash != string::npos)
            path += slash + 1;
        return path;
    }
    int get_hardcut() {return hardcut;}
    int get_maxctx() {return maxctx;}
    Corpus *get_aligned (const string &corp_name);
    TokenLevel *get_aligned_level (const string &corp_name);
    virtual RangeStream *map_aligned (Corpus *al_corp, RangeStream *src,
                                      bool add_labels = true);
    const string get_sizes();
    void compile_docf (const char *attr, const char *docstruc);
    void compile_star (const char *attr, const char *docstruc, const char *strucattr);
    void compile_frq (const char *attr);
    void compile_arf (const char *attr);
    void compile_aldf (const char *attr);
    void freq_dist (RangeStream *r, ostream &out, const char *crit,
                    NumOfPos limit);
    void freq_dist (RangeStream *r, const char *crit, NumOfPos limit,
                    vector<string> &words, vector<NumOfPos> &freqs,
                    vector<NumOfPos> &norms);
    virtual bool is_complement() { return false; }
    virtual WordList *get_wordlist (const string &path);
};


class Structure: public Corpus {
public:
    Corpus *corpus;
    ranges *rng;
    const string name;
    string endtagstring;
    bool complement;
private:
    NumOfPos cached_size = -1;
public:
    Structure (CorpInfo *i, const string &path, const string &n, Corpus *c);
    Structure (CorpInfo *i, const string &n, VirtualCorpus *vc);
    virtual ~Structure() {delete rng;}
    virtual Position size () {return rng->size();}
    virtual Position search_size () {
        if (cached_size != -1)
            return cached_size;
        if (corpus && !conf->opts ["SUBCPATH"].empty()) {
            RangeStream *rs = corpus->filter_query(rng->whole());
            while (!rs->end()) {
                cached_size++;
                rs->next();
            }
            delete rs;
            cached_size++;
        } else
            cached_size = rng->size();
        return cached_size;
    }
    virtual bool is_complement() {return complement;}
    virtual FastStream *part_nums (RangeStream *r);
//      RangeStream *whole();
//      RangeStream *part (FastStream *filter);
};


const char *manatee_version();
void wordlist (WordList *wl, const char *wlpat, vector<Frequency*> &addfreqs, Frequency *sortfreq,
               vector<string> &whitelist, vector<string> &blacklist,
			   NumOfPos wlminfreq, NumOfPos wlmaxfreq, NumOfPos wlmaxitems, const char *nonwordre,
               vector<string> &out_list, NumOfPos &out_count, NumOfPos &out_count_total);
void calc_average_structattr (Corpus *c, const char *structname,
                              const char *attrname, RangeStream *q, double &avg,
                              NumOfPos &docf);
#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:

