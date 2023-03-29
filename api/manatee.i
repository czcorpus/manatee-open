/* -*- C++ -*- */
//  Copyright (c) 1999-2020  Pavel Rychly, Milos Jakubicek

%module manatee

// prevent colisions with wmap
#define SwigPyIterator manatee_SwigPyIterator

%ignore regexp_pattern::regexp_pattern();

%{
//  Copyright (c) 1999-2020  Pavel Rychly, Milos Jakubicek
#include "concget.hh"
#include "concstat.hh"
#include "cqpeval.hh"
#include "frsop.hh"
#include "subcorp.hh"
#include "keyword.hh"
#include <finlib/bigram.hh>
#include <finlib/fsstat.hh>
#include <finlib/regpref.hh>
#include <math.h>
#include <algorithm>
%}

%include common.i
%include typemaps.i
%include std_unordered_set.i
%template() std::unordered_set<std::string>;

void languages (std::vector<std::string> &out);

#ifdef SWIGPYTHON
%extend swig::manatee_SwigPyIterator {
    %rename(next) dumb_next();
    %pythonappend {
        if getEncoding() and type(val) == str:
            val = unicode(val, getEncoding(), errors='replace')
    }
    void dumb_next() {};
}
#endif

template <class Value>
class Generator {
public:
    virtual Value next() = 0;
    virtual bool end() = 0;
};

%template(IntGenerator)  Generator<int>;
%template(StringGenerator)  Generator<std::string>;

class IdStrGenerator {
public:
    virtual void next();
    virtual bool end();
    virtual int getId();
    virtual std::string getStr();
};

/** Stream of ranges */
class RangeStream {
public:
    /** Moves stream to next range */
    virtual bool next() throw (FileAccessError)=0;
    /** Tests whether the stream is at the end */
    virtual bool end() const;
    /** Gets begin of current range  */
    virtual Position peek_beg() const;
    /** Gets end of current range  */
    virtual Position peek_end() const;
    /** Gets begin of the first range starting on or after position pos */
    virtual Position find_beg (Position pos);
    /** Gets begin of the first range ending on or after position pos */
    virtual Position find_end (Position pos);
    %extend {
        /** Gets number of remaining ranges in the stream */
        NumOfPos count_rest() {
            NumOfPos cnt = 0;
            while (!self->end()) {
                cnt++;
                self->next();
            }
            return cnt;
        }
        /** Fills in a list of collocations, odd positions are label numbers,
         * even positions are collocation positions relative to KWIC beg */
        void collocs (std::vector<int> &colls) const {
            Labels lab;
            self->add_labels (lab);
            for (Labels::iterator it = lab.begin(); it != lab.end(); it++) {
                colls.push_back ((*it).first);
                colls.push_back ((*it).second - self->peek_beg());
            }
        }
    }
};

/** Iterator over IDs */
class IDIterator {
public:
    virtual int next()=0;
    %extend {
        int __getitem__(int array_index) {
            int p = self->next();
            if (p < 0)
                throw IndexError ("IDIterator");
            return p;
        }
        int __len__ () {
            return -1;
        }
    }
};

/** Iterator over Text */
class TextIterator {
public:
    virtual const char *next()=0;
};

/** Corpus positional attribute */
class PosAttr : public WordList
{
public:
    /** Gets number of IDs */
    virtual int id_range ()=0;
    /** Translates attribute value to its ID */
    virtual const char* id2str (int id);
    /** Translates attribute ID to its value */
    virtual int str2id(const char *str);
    /** Translates attribute position to its ID */
    virtual int pos2id (Position pos);
    /** Translates attribute position to its value */
    virtual const char* pos2str (Position pos);
    %newobject posat;
    /** Gets IDIterator at the given position */
    virtual IDIterator *posat (Position pos);
    %newobject textat;
    /** Gets TextIterator at the given position */
    virtual TextIterator *textat (Position pos);
    %newobject id2poss;
    /** Translates attribute ID to a FastStream of its positions */
    virtual FastStream *id2poss (int id);
    %newobject dynid2srcids;
    /** Translates dynamic attribute ID to the mapped IDs of the source attribute */
    virtual FastStream *dynid2srcids (int id);
    %newobject regexp2poss;
    /** Translates a regular expression over attribute value to a FastStream of its positions */
    virtual FastStream *regexp2poss (const char *pat, bool ignorecase);
    %newobject regexp2ids;
    /** Translates a regular expression over attribute value to a FastStream of IDs */
    virtual Generator<int> *regexp2ids (const char *pat, bool ignorecase, const char *filter_pat = NULL);
    %extend {
        %newobject regexp2ids_filtered;
        virtual Generator<int> *regexp2ids_filtered (const char *pat, const char *filter_pat) {
            return self->regexp2ids(pat, false, filter_pat);
        }
    }
    /** Iterates over all strings */
    virtual Generator<std::string> *dump_str();
    /** Gets frequency of a given ID */
    virtual NumOfPos freq (int id) throw (FileAccessError);
    /** Gets size (number of positions) of a given ID */
    virtual NumOfPos size ();
    /** Name of the attribute */
    const std::string name;
    /** Data path of the attribute */
    const std::string attr_path;
};


/** Corpus structure */
class Structure {
public:
    /** Gets number of structures */
    NumOfPos size()=0;
    /** Gets number of structures (differs from size() for subcorpora) */
    NumOfPos search_size()=0;
    /** Creates a PosAttr object for a given attribute of this structure */
    PosAttr *get_attr (const char *attr_name) throw (AttrNotFound);
    /** Name of the structure */
    const std::string name;
    %newobject whole;
    %newobject attr_val;
    %extend {
        /** Gets number of the structure for the given position */
        NumOfPos num_at_pos (Position pos) {return self->rng->num_at_pos (pos);}
        /** Gets number of the structure for the position after the given position */
        NumOfPos num_next_pos (Position pos) {return self->rng->num_next_pos (pos);}
        /** Gets the beginning position of a structure with id = n */
        Position beg (Position n) {return self->rng->beg_at(n);}
        /** Gets the ending position of a structure with id = n */
        Position end (Position n) {return self->rng->end_at(n);}
        /** Gets a RangeStream of all structure ranges */
        RangeStream *whole () {return self->rng->whole();}
        /** Gets a RangeStream of structure ranges where the attribute aname has ID attr_id*/
        RangeStream *attr_val (const char *aname, int attr_id) throw (AttrNotFound) {
            return self->rng->part(self->get_attr (aname)->id2poss (attr_id));
        }
    }
};

/** Corpus */
class Corpus {
public:
    /** Opens corpus corp_name */
    Corpus (const char *corp_name) throw (CorpInfoNotFound);
    /** Gets corpus size */
    NumOfPos size();
    /** Gets corpus search size (differs from size() for subcorpora) */
    NumOfPos search_size();
    /** Creates PosAttr for the given corpus positional attribute name */
    PosAttr* get_attr (const char *name, bool struct_attr = false) throw (AttrNotFound);
    /** Creates Structure for the given corpus structure name */
    Structure* get_struct (const char *name) throw (AttrNotFound);
    /** Gets the INFO attribute of this corpus */
    const std::string get_info();
    /** Gets the given configuration option of this corpus */
    const std::string get_conf (const char *item);
    /** Gets the name of the corpus configuration file */
    const std::string get_conffile ();
    /** Gets the path to the corpus configuration file */
    const std::string get_confpath ();
    /** Sets default attribute to attname */
    void set_default_attr (const char *attname) throw (AttrNotFound);
    /** Sets the refence corpus to refcorp */
    void set_reference_corpus (const char *refcorp) throw (AttrNotFound);
    %newobject filter_query;
    /** Filters the given RangeStream by this (sub)corpus */
    RangeStream *filter_query (DisownRangeStream *s);
    /** Gets sizes overview */
    const std::string get_sizes();
    /**
     * Computes attribute 'attr' frequency for each value
     */
    void compile_frq (const char *attr);
    /**
     * Computes attribute 'attr' ARF frequency for each value
     */
    void compile_arf (const char *attr);
    /**
     * Computes attribute 'attr' ALD frequency for each value
     */
    void compile_aldf (const char *attr);
    /**
     * Computes star rating for attribute 'attr' as average value of structure
     * attribute 'structattr' on structure 'struc'
     */
    void compile_star (const char *attr, const char *docstruc, const char *structattr);
    /**
     * Computes document structure frequency, i.e. in how many documents
     * indicated by the "struc" structure each item of the "attr" positional
     * attribute appears
     */
    void compile_docf (const char *attr, const char *docstruc);
    /** Compute frequency distribution of the input RangeStream
     * @param[in] r Source RangeStream
     * @param[in] crit Distribution criteria
     * @param[in] limit Minimum frequency
     * @param[out] words Resulting attribute values
     * @param[out] freqs Resulting frequencies
     * @param[out] norms Resulting norms
     */
    void freq_dist (DisownRangeStream *r, const char *crit, int limit,
                    Tokens &words, std::vector<NumOfPos> &freqs,
                    std::vector<NumOfPos> &norms);
    /** Open wordlist at the given path */
    WordList *get_wordlist (const std::string path) throw (FileAccessError);

    %newobject eval_query;
    %newobject filter_fstream;
    %extend {
        /** Creates PosAttr for the given corpus structure positional attribute name */
        PosAttr* get_struct_attr (const char *name)
            throw (AttrNotFound) {
            return self->get_attr(name, true);
        }
        /** freq_dist() variant for FastStream */
        void freq_dist (DisownFastStream *r, const char *crit, int limit,
                        Tokens &words, std::vector<NumOfPos> &freqs,
                        std::vector<NumOfPos> &norms) {
            self->freq_dist (new Pos2Range(r),crit,limit,words,freqs,norms);
        }
        void freq_dist_from_fs (DisownFastStream *r, const char *crit, int limit,
                        Tokens &words, std::vector<NumOfPos> &freqs,
                        std::vector<NumOfPos> &norms) {
            self->freq_dist (new Pos2Range(r),crit,limit,words,freqs,norms);
        }
        /** Creates RangeStream from FastStream */
        RangeStream *Pos2Range (DisownFastStream *f) {
            return new Pos2Range (f);
        }
        /** Creates FastStream from RangeStream */
        FastStream *Range2Pos (DisownRangeStream *r) {
            return new BegsOfRStream (r);
        }
        /** Evaluates CQL query and returns resulting RangeStream*/
        RangeStream *eval_query (const char *query) {
            return self->filter_query (eval_cqpquery 
                                 ((std::string (query) + ';').c_str(), self));
        }
        /** Filters the given FastStream by this (sub)corpus */
        FastStream *filter_fstream (DisownFastStream *s) {
            return new BegsOfRStream (self->filter_query (new Pos2Range(s)));
        }
        /** Gets number of remaining ranges in the stream */
        NumOfPos count_rest (DisownFastStream *s) {
            RangeStream *rs = self->filter_query (new Pos2Range(s));
            NumOfPos cnt = 0;
            while (!rs->end()) {
                cnt++;
                rs->next();
            }
            delete rs;
            return cnt;
        }

        /** Gets the frequencies of structure attribute values */
        void count_structattr_vals(const char* structname, const char *saname,
                bool docsizes, std::vector<NumOfPos> &freqs) {
            Structure *struc = self->get_struct(structname);
            PosAttr *wlattr = struc->get_attr(saname);
            freqs.resize(wlattr->id_range());
            for (ssize_t i = 0; i < wlattr->id_range(); i++) {
                RangeStream *rs =
                    self->filter_query(struc->rng->part(wlattr->id2poss(i)));
                while(!rs->end()) {
                    if(docsizes) freqs[i] += 1;
                    else freqs[i] += rs->peek_end() - rs->peek_beg();
                    rs->next();
                }
                delete rs;
            }
        }
    }
};

typedef std::vector<std::string> Tokens;

/** Corpus concordance */
class Concordance {
public:
    /** Create empty concordance that should be set up by `load_from_*` methods */
    Concordance ();
    /** Load saved corpus concordance from a file
     * @param[in] corp Corpus object
     * @param[in] filename Concordance file to load from
     */
    Concordance (Corpus *corp, const char *filename) throw (FileAccessError, ConcNotFound);
    /** Load saved corpus concordance from a file descriptor
     * @param[in] corp Corpus object
     * @param[in] filename Concordance file to load from
     */
    Concordance (Corpus *corp, int fileno) throw (FileAccessError, ConcNotFound);
    /** Create new concordance from a RangeStream
     * @param[in] corp Corpus object
     * @param[in] rs input RangeStream (e.g. result of query)
     * @param[in] inccoll Ignored, kept for backward compatibility only
     */
    Concordance (Corpus *corp, DisownRangeStream *rs, ConcIndex inccoll=0) throw (FileAccessError);
    /** Copy concordance object */
    Concordance (Concordance &x);
    /** Load saved corpus concordance from a file
     * (use only with the empty constructor)
     * @param[in] corp Corpus object
     * @param[in] filename Concordance file to load from
     */
    void load_from_file(Corpus *corp, const char *filename) throw (FileAccessError, ConcNotFound);
    /** Load saved corpus concordance from a file descriptor
     * (use only with the empty constructor)
     * @param[in] corp Corpus object
     * @param[in] fileno Concordance file descriptor to load from
     */
    void load_from_fileno(Corpus *corp, int fileno) throw (FileAccessError, ConcNotFound);
    /** Create new concordance from a RangeStream
     * (use only with the empty constructor)
     * @param[in] corp Corpus object
     * @param[in] rs input RangeStream (e.g. result of query)
     * @param[in] sample_size Size of the sample to be created,
     * 0 means ignore (no sample, whole result), any positive number means
     * create a sample of that size.
     * @param[in] full_size Size of result, 0 means ignore (samples are
     * created from the beginning of the result), -1 is like 0 but size of the
     * result should be computed, any positive number indicates the result size
     * and samples are created randomly from that range.
     */
    void load_from_rs(Corpus *corp, DisownRangeStream *rs,
                 ConcIndex sample_size, NumOfPos full_size) throw (FileAccessError);
    /** Save corpus concordance into a file
     * @param[in] filename Concordance file to save to
     * @param[in] save_linegroup Whether to save line annotations
     * @param[in] partial Whether to save partial (incomplete) concordance or
     * wait for it to finish
     * @param[in] append Whether to append to existing file or rewrite it
     */
    void save (const char *filename, bool save_linegroup=false,
               bool partial=false, bool append=false);
    /** Save corpus concordance into a file
     * @param[in] fileno File descriptor to write to
     * @param[in] save_linegroup Whether to save line annotations
     * @param[in] partial Whether to save partial (incomplete) concordance or
     * wait for it to finish
     * @param[in] append Whether to append to existing file or rewrite it
     */
    void save (int fileno, bool save_linegroup=false,
               bool partial=false, bool append=false);

    %extend {
        /** Save corpus concordance into a file without line annotations (linegroup)
         * @param[in] filename Concordance file to save to
         */
        void save_with_linegroup (const char *filename) {
            self->save(filename, false, false, false);
        }
        /** Save corpus concordance into a file with line annotations (linegroup)
         * @param[in] filename Concordance file to save to
         */
        void save_without_linegroup (const char *filename) {
            self->save(filename, true, false, false);
        }
        /** Save corpus concordance into a file
         * @param[in] fileno File descriptor to write to
         * @param[in] save_linegroup Whether to save line annotations
         * @param[in] partial Whether to save partial (incomplete) concordance or
         * wait for it to finish
         */
        void save_fileno (int fileno, bool save_linegroup, bool partial) {
            self->save(fileno, save_linegroup, partial, false);
        }
    }
    /** Gets concordance size (number of lines */
    ConcIndex size();
    /** Gets number of collocations */
    int numofcolls();
    /** Gets concordance view size */
    ConcIndex viewsize();
    /** Gets full size of the input stream (differs from size in case of sampled concordance */
    NumOfPos fullsize();
    /** Tests whether the concordance has been finished */
    bool finished();
    /** Waits for the concordance to finish */
    void sync ();
    /** Sets new collocation
     * @param[in] collnum number of new collocation
     * @param[in] cquery source query
     * @param[in] lctx left context
     * @param[in] rctx right context
     * @param[in] rank ranking of the collocation
     * @param[in] exclude_kwic Whether to exclude KWIC from the cquery
     */
    void set_collocation (int collnum, const char *cquery, 
                          const char *lctx, const char *rctx, int rank,
                          bool exclude_kwic = false);
    /** Sorts concordance
     * @param[in] crit Sorting criteria
     * @param[in] uniq Whether to filter duplicates
     */
    void sort (const char *crit, bool uniq = 0);
    /** Sorts according to relative frequencies
     * @param[in] leftctx left context
     * @param[in] rightctx right context
     * @param[in] attr attribute name
     */
    void relfreq_sort (const char *leftctx, const char *rightctx, 
                       const char *attr);
    /**
     * Sets a new view on the concordance
     * @param[in] input mapping of concordance lines
     */
    void set_sorted_view (std::vector<int> &sorted);
    /** Deletes lines matching the given criteria */
    void reduce_lines (const char *crit);
    /** Deletes lines according to positive/negative filter
     * @param[in] collnum collocation number indicating the filter
     * @param[in] positive Whether the filter is positive or negative
     */
    void delete_pnfilter (int collnum, bool positive);
    /** Swap KWIC with collocation collnum */
    void swap_kwic_coll (int collnum);
    /** Extend KWIC up to collocation collnum */
    void extend_kwic_coll (int collnum);
    /** Compute positions of unique values in sorted concordance
     * @param[in] crit Sorting criteria
     * @param[out] idxs Resulting vector of indices (concordance lines)
     * @param[in] firstCharOnly Whether to consider only first character
     */
    void sort_idx (const char *crit,Tokens &chars,std::vector<int> &idxs,
                   bool firstCharOnly);
    /** Compute the corpus position hstogram of the elements
     * @param[out] vals Histogram bin values
     * @param[out] beginnings Corpus positions of the bin beginnings
     * @param[in] yrange Number of bins to calculate
     * @param[in] normalize Scale vals to [0, yrange-1], otherwise provide raw counts
     * @return Maximum raw bin value
     */
    void distribution (std::vector<int> &vals, std::vector<int> &beginnings,
                       int yrange, bool normalize=true);
    /** Compute Average Reduced Frequency (ARF) */
    double compute_ARF();
    /** Annotates concordance line linenum by the given group */
    void set_linegroup (ConcIndex linenum, int group);
    /** Annotates whole concordance by the given group */
    void set_linegroup_globally (int group);
    /** Annotates given position pos by the given group */
    int set_linegroup_at_pos (Position pos, int group);
    /** Copies annotation from a master concordance */
    void set_linegroup_from_conc (Concordance *master);
    /** Gets a new (unused) annotation group ID */
    int get_new_linegroup_id();
    /** Deletes selected linegroups, or all others iff invert is true */
    void delete_linegroups (const char *grps, bool invert);
    %newobject begs_FS;
    /** Creates FastStream of concordance line beginnings */
    FastStream *begs_FS();
    %newobject RS;
    /** Create RangeStream from concordance */
    RangeStream *RS (bool useview = false, ConcIndex beg = 0, ConcIndex end=0);
    /** Shuffles concordance lines randomly */
    void shuffle();
    /** Gets beginning of concordance line indicated by idx */
    Position beg_at (ConcIndex idx);
    /** Gets ending of concordance line indicated by idx */
    Position end_at (ConcIndex idx);
    /** Gets beginning of concordance collocation number coll for line indicated by idx */
    Position coll_beg_at (int coll, ConcIndex idx);
    /** Gets ending of concordance collocation number coll for line indicated by idx */
    Position coll_end_at (int coll, ConcIndex idx);
    /** Switch aligned concordance into corpus corpname */
    void switch_aligned (const char *corpname);
    /** Get vector of all aligned corpora that were part of the query */
    void get_aligned (std::vector<std::string> &aligned);
    /** Add a new aligned corpus that was not part of the query */
    void add_aligned (const char *corpname);
    /** Filters concordance by preserving only lines defined in the given aligned corpus */
    void filter_aligned (const char *corpname);
    /** Deletes lines with subpart matches */
    void delete_subparts ();
    /** Deleted all except first hit in the given structure */
    void delete_struct_repeats (const char *struc);

    %extend {
    /**
     * Creates concordance from a query
     * @param[in] corp Corpus object
     * @param[in] query Search query
     * @{param[in]} sample_size Size of the sample to be created,
     * 0 means ignore (no sample, whole result), any positive number means
     * create a sample of that size.
     * @{param[in]} full_size Size of result, 0 means ignore (samples are
     * created from the beginning of the result), -1 is like 0 but size of the 
     * result should be computed, any positive number indicates the result size
     * and samples are created randomly from that range.
     * @param[in] inccoll Ignored, kept for backward compatibility only
     */
    Concordance (Corpus *corp, std::string query, ConcIndex sample_size,
                 NumOfPos full_size, ConcIndex inccoll=0)
                 throw (FileAccessError) {
        query += ';';
        return new Concordance (corp, corp->filter_query
                                (eval_cqpquery (query.c_str(), corp)), inccoll,
                                sample_size, full_size);
    }
    /**
     * Creates concordance from a FastStream
     * @param[in] corp Corpus object
     * @param[in] fs Source FastStream
     * @param[in] inccoll Ignored, kept for backward compatibility only
     */
    Concordance (Corpus *corp, DisownFastStream *fs, int inccoll=0)
        throw (FileAccessError) {
        return new Concordance (corp, corp->filter_query (new Pos2Range (fs)),
                                inccoll);
    }
    /** Create RangeStream from concordance using the view */
    RangeStream *RS_using_view () {
        return self->RS (true);
    }
    /** Create RangeStream from limited concordance using the view */
    RangeStream *RS_using_view_limited (ConcIndex beg, ConcIndex end) {
        return self->RS (true, beg, end);
    }
    /** Create new concordance from a FastStream
     * (use only with the empty constructor)
     * @param[in] corp Corpus object
     * @param[in] fs input FastStream
     * @param[in] sample_size Size of the sample to be created,
     * 0 means ignore (no sample, whole result), any positive number means
     * create a sample of that size.
     * @param[in] full_size Size of result, 0 means ignore (samples are
     * created from the beginning of the result), -1 is like 0 but size of the
     * result should be computed, any positive number indicates the result size
     * and samples are created randomly from that range.
     */
    void load_from_fs(Corpus *corp, DisownFastStream *fs,
                 ConcIndex sample_size, NumOfPos full_size)
        throw (FileAccessError) {
        self->load_from_rs(corp, new Pos2Range(fs), sample_size, full_size);
    }
    /** Create new concordance from a FastStream
     * (use only with the empty constructor)
     * @param[in] corp Corpus object
     * @param[in] fs input FastStream
     * @param[in] sample_size Size of the sample to be created,
     * 0 means ignore (no sample, whole result), any positive number means
     * create a sample of that size.
     * @param[in] full_size Size of result, 0 means ignore (samples are
     * created from the beginning of the result), -1 is like 0 but size of the
     * result should be computed, any positive number indicates the result size
     * and samples are created randomly from that range.
     */
    void load_from_query(Corpus *corp, std::string query,
                 ConcIndex sample_size, NumOfPos full_size) throw (FileAccessError) {
        query += ';';
        RangeStream *rs = corp->filter_query (eval_cqpquery (query.c_str(), corp));
        self->load_from_rs (corp, rs, sample_size, full_size);
    }
    /**
     * Get annotation group statistics
     * @param[out] ids Vector of annotations group IDs
     * @param[out] freqs Vector of annotation group frequencies
     */
    void get_linegroup_stat (std::vector<int> &ids, std::vector<int> &freqs) {
        std::map<short int,ConcIndex> lgs;
        self->get_linegroup_stat (lgs);
        ids.clear();
        freqs.clear();
        for (std::map<short int,ConcIndex>::const_iterator i = lgs.begin();
             i != lgs.end(); i++) {
            ids.push_back ((*i).first);
            freqs.push_back ((*i).second);
        }
    }
    /** Sort annotation groups */
    void linegroup_sort (std::vector<int> &ids, std::vector<std::string> &strs)
    {
        std::map<short int,std::string> ordertab;
        std::vector<int>::const_iterator idsi = ids.begin();
        std::vector<std::string>::const_iterator strsi = strs.begin();
        while (idsi != ids.end())
            ordertab[*idsi++] = *strsi++;
        self->linegroup_sort (ordertab);
    }
    /** Get underlying Corpus instance */
    Corpus *corp() {return self->corp;}
    }
};

/** KWIC lines */
class KWICLines {
public:
    /** Create new KWICLines from RangeStream
     * @param[in] corp Corpus
     * @param[in] r Source RangeStream
     * @param[in] left Left context
     * @param[in] right Right context
     * @param[in] kwica KWIC attributes
     * @param[in] ctxa Context attributes
     * @param[in] struca Structure attributes
     * @param[in] refa Reference attributes
     * @param[in] maxctx Maximum context size
     * @param[in] useview Whether to use view (e.g. sorted view)
     */
    KWICLines (Corpus *corp, DisownRangeStream *r, const char *left,
               const char *right, const char *kwica, const char *ctxa,
               const char *struca, const char *refa, int maxctx=0);
    /** Move to the next context */
    bool nextcontext ();
    /** Move to the next line */
    bool nextline ();
    /** Skip count items and move to the next context */
    bool skip (ConcIndex count);
    /** Gets KWIC position */
    Position get_pos();
    /** Gets KWIC length */
    int get_kwiclen();
    /** Gets position of the context begin */
    Position get_ctxbeg();
    /** Gets position of the context end */
    Position get_ctxend();
    /** Gets list of references */
    Tokens get_ref_list();
    /** Gets reference string */
    std::string get_refs();
    /** Gets left context */
    Tokens get_left();
    /** Gets KWIC */
    Tokens get_kwic();
    /** Gets right context */
    Tokens get_right();
    /** Gets annotation group */
    int get_linegroup();
};

/** Collocation items */
class CollocItems {
public:
    /** Creates collocation items from Concordance
     * @param[in] conc Concordance
     * @param[in] attr_name Attribute name
     * @param[in] sort_fun_code Code of the sorting function
     * @param[in] minfreq Minimum frequency
     * @param[in] minbgr Minimum bigram frequency
     * @param[in] fromw Beginning of context
     * @param[in] tow End of context
     * @param[in] maxitems Maximum number of collocation items
     */
    CollocItems (Concordance *conc, const char *attr_name, char sort_fun_code, 
                 NumOfPos minfreq, NumOfPos minbgr, int fromw, int tow, 
                 int maxitems);
    /** Move to the next item */
    void next();
    /** Tests end of stream */
    bool eos();
    /** Gets current item */
    const char *get_item();
    /** Gets frequency of current item */
    NumOfPos get_freq();
    /** Gets count of items */
    NumOfPos get_cnt();
    /** Gets bigram frequency of current item */
    double get_bgr (char bgr_code);
};

/** Corpus Region */
class CorpRegion {
public:
    /** Creates CorpRegion
     * @param[in] corp Corpus object
     * @param[in] attra Attribute list
     * @param[in] struca Structure list
     * @param[in] ignore_nondef Whether to ignore undefined attributes and
     * structures
     */
    CorpRegion (Corpus *corp, const char *attra, const char *struca,
                bool ignore_nondef=true);
    /** Retrieve a region
     * @param[in] frompos Position of the beginning of the region
     * @param[in] topos Position of the end of the region
     * @param[in] posdelim Positions delimiter
     * @parma[in] attrdelim Attribute delimiter
     */
    Tokens region (Position frompos, Position topos, char posdelim = ' ',
                   char attrdelim = '/');
};

//-------------------- SubCorpus --------------------
/** Subcorpus */
class SubCorpus : public Corpus {
public:
    /** Opens a subcorpus of corpus corp stored in file sub*/
    SubCorpus (Corpus *corp, const char *sub, bool complement = false);
};

%{
void find_subcorpora (const char *subcdir, std::vector<std::string> &scs)
    {
        typedef std::map<std::string, std::pair<std::string,std::string> > MS;
        MS temp;
        find_subcorpora (subcdir, temp);
        for (MS::const_iterator i=temp.begin(); i != temp.end(); i++)
            scs.push_back ((*i).first);
    }
%}

/** Creates new subcorpus from a RangeStream
 * @param[in] subcpath Path where the subcorpus should be saved to
 * @param[in] r RangeStream indicating subcorpus ranges
 * @param[in] s optional Structure that will be indicating subcorpus ranges
 *              whenever containing the given RangeStream
 *              (equals <s/> containing [RangeStream aka CQL result]
 */
bool create_subcorpus (const char *subcpath, DisownRangeStream *r,
                       Structure *s)
    throw (FileAccessError, EvalQueryException);
/** Creates new subcorpus from a CQL query filtering a structure
 * @param[in] subcpath Path where the subcorpus should be saved to
 * @param[in] corp Corpus object
 * @param[in] structname Name of the structure defining the subcorpus
 * @param[in] CQL query on structname
 * @deprecated use create_subcorpus_from_query
 */
bool create_subcorpus (const char *subcpath, Corpus *corp, 
                       const char *structname, const char *query)
    throw (FileAccessError, AttrNotFound, CorpInfoNotFound, EvalQueryException);

%{
    bool create_subcorpus_from_query (const char *subcpath, Corpus *corp,
                       const char *structname, const char *query) {
        return create_subcorpus (subcpath, corp, structname, query);
    }
%}

/** Creates new subcorpus from a CQL query filtering a structure
 * @param[in] subcpath Path where the subcorpus should be saved to
 * @param[in] corp Corpus object
 * @param[in] structname Name of the structure defining the subcorpus
 * @param[in] CQL query on structname
 */
bool create_subcorpus_from_query (const char *subcpath, Corpus *corp,
                   const char *structname, const char *query)
throw (FileAccessError, AttrNotFound, CorpInfoNotFound, EvalQueryException);

/** Retrieve list of available subcorpora
 * @param[in] subcdir Subcorpus directory
 * @param[out] scs List of found subcorpora
 */
void find_subcorpora (const char *subcdir, std::vector<std::string> &scs);

/** Create a union subcorpus from two subcopora
 * @param[in] subc1 Path of the first source subcorpus
 * @param[in] subc1 Path of the second source subcorpus
 * @param[in] subcout Path of the union subcorpus to be created
 */
void merge_subcorpora(const char *subc1, const char *subc2, const char *subcout);

/** A memory mapping of Bigram file */
class map_int_sort_bigrams {
public:
    /** Create new mapping from file filename */
    map_int_sort_bigrams (const char *filename);
    /** Get maximum ID */
    int maxid ();
    /** Get count of bigrams for bigram id */
    int count (int id);
    /** Get bigram value for id1, id 2*/
    int value (int id1, int id2);

};

/** Compute Average Reduced Frequency
 * @param[in] s Source FastStream
 * @param[in] freq Frequency of the ID
 * @param[in] size Size of the corpus
 */
double compute_ARF (FastStream *s, NumOfPos freq, Position size);
/** Compute fALD
 * @param[in] s Source FastStream
 * @param[in] size Size of the corpus
 */
double compute_fALD (FastStream *s, Position size);

%{
struct labstat { int psum, pcount, nsum, ncount; Position pos; };
void estimate_colloc_poss(FastStream *s, std::vector<Position> &collposs, int maxcoll) {
    Labels lab;
    std::vector<labstat> stats;
    labstat lstat = {0, 0, 0, 0, 0};
    stats.resize(maxcoll, lstat);
    std::vector <std::pair <int, Position> > colls;
    colls.push_back(std::make_pair (0, s->peek()));
    while (s->peek() < s->final()) {
        s->add_labels(lab);
        for (Labels::iterator it = lab.begin(); it != lab.end(); it++) {
            int labnum = it->first - 1;
            if (!stats[labnum].pos)
                stats[labnum].pos = it->second;
            Position delta = it->second - s->peek();
            if (delta > 0) {
                stats[labnum].psum += delta;
                stats[labnum].pcount++;
            } else {
                stats[labnum].nsum += delta;
                stats[labnum].ncount++;
            }
        }
        lab.clear();
        s->next();
    }
    for (unsigned i = 0; i < stats.size(); i++) {
        if (!stats[i].pcount && !stats[i].ncount)
            continue;
        else if (stats[i].pcount > stats[i].ncount)
            colls.push_back(std::make_pair(floor(((float) stats[i].psum) /
                        stats[i].pcount), stats[i].pos));
        else
            colls.push_back(std::make_pair(floor(((float) stats[i].nsum) /
                        stats[i].ncount), stats[i].pos));
    }
    std::sort(colls.begin(), colls.end());
    for (std::vector <std::pair <int, Position> >::iterator it = colls.begin();
         it != colls.end(); it++)
        collposs.push_back(it->second);
}
%}
/** Estimate average position of collocations to KWIC (before/after)
 * @param[in] s Source FastStream
 * @param[out] collposs Vector of collocation positions
 * @param[in] maxcoll Number of collocations
 */
void estimate_colloc_poss(FastStream *s, std::vector<Position> &collposs, int maxcoll);

/** Encapsulates corpus configuration file info */
class CorpInfo {
public:
    /** "Clever" bool check (y/Yes/t/True/1 vs n/No/f/False/O) */
    static bool str2bool (const std::string &str);
    /** Dumps the configuration in proper format to a string */
    std::string dump (int indent=0);
    /** Adds attribute to configuration */
    CorpInfo* add_attr (const char *path);
    /** Adds structure to configuration */
    CorpInfo* add_struct (const char *path);
    /** Removes attribute from configuration */
    void remove_attr (const char *path);
    /** Removes attribute from configuration */
    void remove_struct (const char *path);
    /** Sets option in configuration */
    void set_opt (const char *path, const char *val);
    /** Retrieves option from configuration */
    std::string find_opt (const char *path);
    /** Gets structure from configuration */
    CorpInfo* find_struct (const char *path);
};
/** Creates CorpInfo object by path/name of corpus
 * @deprecated use loadCorpInfoDefaults or loadCorpInfoNoDefaults instead
 */
CorpInfo *loadCorpInfo (const char *corp_name_or_path,
                        bool no_defaults = false);

/** Creates CorpInfo object by path/name of corpus while inserting default values */
CorpInfo *loadCorpInfoDefaults (const char *corp_name_or_path);

/** Creates CorpInfo object by path/name of corpus only using the registry contents */
CorpInfo *loadCorpInfoNoDefaults (const char *corp_name_or_path);

%nodefaultctor kwitem;
struct kwitem {
    int id1, id2;
    float score;
    std::string str;
    double *freqs;
    %extend {
        PyObject * get_freqs(int size) {
            int i;
            PyObject *l = PyList_New(size);
            for (i = 0; i < size; i++) {
                PyObject *o = PyFloat_FromDouble((double) self->freqs[i]);
                PyList_SetItem(l, i, o);
            }
            return l;
        }
    }
};

%apply std::vector<std::string>& INPUT {std::vector<std::string> &filters }
%apply std::vector<string>& INPUT {std::vector<string> &addfreqs }
%apply std::unordered_set<std::string>& INPUT {std::unordered_set<std::string> &whitelist, std::unordered_set<std::string> &blacklist }

class Keyword {
public:
    Keyword (Corpus *c1, Corpus *c2, WordList *wl1, WordList *wl2, float N,
             unsigned maxlen, int minfreq, int maxfreq, std::unordered_set<std::string> &blacklist,
             std::unordered_set<std::string> &whitelist, const char *frqtype, std::vector<string> &addfreqs,
             std::vector<std::string> pos_filters, std::vector<std::string> neg_filters, FILE* progress);
    int size();
    const kwitem* next();
    %extend {
        PyObject * get_totals() {
            PyObject *l = PyList_New(3);
            PyList_SetItem(l, 0, PyLong_FromLongLong(self->totalcount));
            PyList_SetItem(l, 1, PyLong_FromLongLong(self->totalfreq1));
            PyList_SetItem(l, 2, PyLong_FromLongLong(self->totalfreq2));
            return l;
        }
    }
};


class TokenLevel;

class MLTStream {
public:
    typedef enum {KEEP=1, CONCAT=2, DELETE=3, INSERT=4, MORPH=5} change_type_t;
    // KEEP -- keep tokens -- 1-1 mapping of the whole region [n*(1-1)]
    // CONCAT -- concatenate the whole range into one token  [n-1]
    // DELETE -- delete all tokens in the region  [n-0]
    // INSERT -- insert new tokens  [0-n]
    // MORPH -- n->m change of tokens  [n-m]

    // info about the current change
    virtual change_type_t change_type() =0;
    virtual NumOfPos change_size() =0;
    virtual NumOfPos change_newsize() =0;
    virtual int concat_value(int attrn) =0;
    //  -- defined for CONCAT type only
    //     0=concatenate; 1,2,...=from that token
    //     -n = attribute value n [id2str(n)]
    virtual Position orgpos() =0;
    virtual Position newpos() =0;

    virtual bool end() =0;
    virtual Position newfinal() =0;
    virtual void next() =0;
    virtual Position find_org(Position pos) =0;
    virtual Position find_new(Position pos) =0;
    virtual ~MLTStream() {};
};

TokenLevel *new_TokenLevel (const char *path);
MLTStream *full_level (TokenLevel *level);

%apply NumOfPos& OUTPUT { NumOfPos &out_count, NumOfPos &out_count_total };
%apply std::vector<std::string>& INPUT {std::vector<std::string> &whitelist, std::vector<std::string> &blacklist, std::vector<std::string> &wlnums }
%apply std::vector<Frequency*>& INPUT {std::vector<Frequency*> &addfreqs }
%apply std::vector<std::string>& OUTPUT {std::vector<std::string> &out_list };
void wordlist (WordList *wl, const char *wlpat, std::vector<Frequency*> &addfreqs, Frequency *sortfreq,
               std::vector<std::string> &whitelist, std::vector<std::string> &blacklist,
               NumOfPos wlminfreq, NumOfPos wlmaxfreq, NumOfPos wlmaxitems, const char *nonwordre,
               std::vector<std::string> &out_list, NumOfPos &out_count, NumOfPos &out_count_total);

%apply NumOfPos& OUTPUT { NumOfPos &docf };
%apply double& OUTPUT { double &avg };
void calc_average_structattr (Corpus *c, const char *structname, const char
     *attrname, DisownRangeStream *q, double &avg, NumOfPos &docf);

class regexp_pattern {
public:
    %extend {
        regexp_pattern(char *pattern, char* locale=0, char *encoding=0) {
            regexp_pattern *re = new regexp_pattern(pattern, locale, encoding);
            if (re->compile()) throw runtime_error("failed to compile regexp");
            return re;
        };
    }

    bool match(char *str);
};
// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:

