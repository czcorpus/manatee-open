// Copyright (c) 1999-2017  Pavel Rychly, Milos Jakubicek

#include "fstream.hh"
#include "writelex.hh"
#include "fromtof.hh"
#include "fingetopt.hh"
#include "consumer.hh"
#include "log.hh"
#include "config.hh"
#include "corpconf.hh"
#include <sstream>
#include <iostream>
#include <fstream>
#include <stdarg.h>
#if defined __MINGW32__
#include <pcreposix.h>
#else
#include <regex.h>
#endif
#include <unistd.h>
#if defined DARWIN
#include <sys/sysctl.h>
#endif
#include <limits>
#include <stdexcept>
#include <signal.h>
#include <algorithm>

#define MAX_NESTING 100
#define STR(x) #x
#define XSTR(x) STR(x)
#define MEMORY_STEP (250*1024*1024)

using namespace std;

class write_region;
typedef map<string,write_region*> MWR;

unsigned long long int linenum;
const char *infilename;
MWR *show_structs = NULL;
bool append_opt = false;
long int bucket_size = 10000;
long long int max_memory = 0;
long long int free_mem;
long long int use_mem;
vector<pair <string, bool> > dynattrs;
typedef vector<pair <string, vector<string> > > VNA;
VNA normattrs;
bool skip_regexopt = false;
bool skip_normattr = false;
bool skip_dynamic=false;
bool verbose=false;
string corpname = "";
int enc_err_max = 100;
volatile sig_atomic_t sig_received = 0;
int reset_mem = 0;

static void signal_handler (int signum) {
    sig_received = signum;
}

long long int scale_bucket_size (long long int use, long long int maximum)
{
    if (use > maximum) {
        float scale = float(maximum) / use;
        bucket_size = (long int) max(100.0f, bucket_size * scale);
        CERR << "   limiting memory: bucket_size=" << bucket_size << "\n";
        return 0;
    }
    return maximum - use;
}

void process_signal (int signum)
{
    if (signum == SIGUSR1) {// increase available memory
        max_memory += MEMORY_STEP;
        free_mem += MEMORY_STEP;
        CERR << "SIGUSR1 received: memory limit increased by "
             << MEMORY_STEP / (1024*1024) << " MB, now at "
             << max_memory / (1024*1024) << " MB\n";
    } else if (signum == SIGUSR2) { // decrease available memory
        max_memory -= MEMORY_STEP;
        CERR << "SIGUSR2 received: memory limit decreased by "
             << MEMORY_STEP / (1024*1024) << " MB, now at "
             << max_memory / (1024*1024) << " MB\n";
        if (free_mem > MEMORY_STEP) {
            free_mem -= MEMORY_STEP;
        } else {
            free_mem = scale_bucket_size (use_mem, max_memory);
            reset_mem++;
        }
    }
    sig_received = 0;
}

const char *fileline();

const char *my_basename (const char *path)
{
    const char *s = strrchr (path, '/');
    return (s ? s+1 : path);
}

FILE *try_popen (const string &cmd, const char *type)
{
    FILE *f = popen (cmd.c_str(), type);
    if (!f)
        throw FileAccessError (cmd, "popen");
    else
        return f;
}

inline char *my_strtok (char *&start, char delim, bool &eos)
{
    if (!*start) {
        eos = true;
        return start;
    }
    eos = false;
    char *ret = start;
    char *d = strchr (start, delim);
    static char emptystr[1] = "";
    if (!d)
        start = emptystr;
    else {
        *d = '\0';
        start = d +1;
    }
    return ret;
}

inline void ensuresize (char *&ptr, int &allocsize, int needed, 
                        const char *where)
{
    if (allocsize >= needed) return;
    char *p = (char *)realloc (ptr, needed);
    if (!p)
        throw MemoryAllocationError (where);
    allocsize = needed;
    ptr = p;
}

TextConsumer::TextType 
type2consumer (const string &typestr, TextConsumer::TextType default_type)
{
    if (typestr == "FD_FBD")
        return TextConsumer::tdeltabig;
    else if (typestr == "FD_FGD" || typestr == "NoMem" || typestr == "MD_MGD")
        return TextConsumer::tdeltagiga;
    else if (typestr == "MD_MI")
        return TextConsumer::tint;
    else
        return default_type;
}


long int tt_scaling (TextConsumer::TextType tt, long int *base_sizes,
                     int force_size = 0)
{
    long int base;
    switch (tt) {
    case TextConsumer::tint:
        base = base_sizes[0];
        break;
    case TextConsumer::tdeltagiga:
        base = base_sizes[2];
        break;
    default:
        base = base_sizes[1];
    }
    if (force_size) {
        long int fsize = force_size * 2;
        if (fsize < 100)
            fsize = 100;
        return fsize;
    }
    return base * bucket_size;
}

// in memory lexicon size: 10.000, 100.000, 400.000
long int lex_mem[3] = {1, 10, 40};
// index sorting buffer (item=12B): 100.000, 4.000.000, 20.000.000
long int rev_mem[3] = {10, 400, 2000};

inline long int attr_mem_usage (TextConsumer::TextType tt,
                                int lexitemsize, int lexiconsize)
{
    return lexitemsize * tt_scaling (tt, lex_mem, lexiconsize)
        + tt_scaling (tt, rev_mem) * 12; // 12 B per buffer item
}

// 20B per lexicon item of attribute, 40B in structures by default
long long int estimate_mem_usage (CorpInfo *ci)
{
    long long int mem_usage = 0;
    // attributes
    for (CorpInfo::VSC::iterator i=ci->attrs.begin(); i != ci->attrs.end(); i++) {
        CorpInfo::MSS &aopts = (*i).second->opts;
        if (aopts["DYNAMIC"] == "") {
            TextConsumer::TextType tt = type2consumer (aopts["TYPE"],
                                                       TextConsumer::tdelta);
            mem_usage += attr_mem_usage(tt, 20, atol(aopts["LEXICONSIZE"].c_str()));
        }
    }
    // attributes of structures
    for (CorpInfo::VSC::iterator s=ci->structs.begin();
         s != ci->structs.end(); s++) {
        CorpInfo::VSC &at = (*s).second->attrs;
        for (CorpInfo::VSC::iterator i=at.begin(); i != at.end(); i++) {
            CorpInfo::MSS &aopts = (*i).second->opts;
            if (aopts["DYNAMIC"] == "") {
                TextConsumer::TextType tt = type2consumer (aopts["TYPE"],
                                                           TextConsumer::tint);
                mem_usage += attr_mem_usage(tt, 40, atol(aopts["LEXICONSIZE"].c_str()));
            }
        }
    }
    return mem_usage;
}

long long int available_memory ()
{
    long long int mem;
    void *x;
#if defined DARWIN
    int mib[2];
    size_t len;

    mib[0] = CTL_HW;
    mib[1] = HW_MEMSIZE; /* physical ram size */
    len = sizeof(mem);
    sysctl(mib, 2, &mem, &len, NULL, 0);
#else
    mem = ((long long int) sysconf(_SC_PHYS_PAGES)) * sysconf(_SC_PAGE_SIZE);
#endif
    mem /= 2;
    x = malloc(mem);
    while (x == NULL) {
        mem = mem * 7 / 10;
        x = malloc(mem);
    }
    free (x);
    return mem;
}

int make_bigger_cache (int cache_size, const char *cache, const char *name,
                       int lexitem_size, TextConsumer::TextType type)
{
    long int new_mem = attr_mem_usage (type, lexitem_size, 2 * cache_size)
                     - attr_mem_usage (type, lexitem_size, cache_size);
    if (free_mem > new_mem) {
        int new_size = cache_size * 2;
        if (verbose)
            CERR << "Increasing lexicon " << cache << " cache for " << name
                 << " to " << new_size << "\n";
        free_mem -= new_mem;
        return new_size;
    }
    CERR << "Not enough memory to increase lexicon " << cache << " cache for "
         << name << " (free memory = " << free_mem << " bytes, required = "
         << new_mem << " bytes)\n";
    return cache_size;
}

int make_smaller_cache (int cache_size, const char *cache, const char *name)
{
    int new_size = cache_size /= 2;
    free_mem += cache_size;
    if (verbose)
        CERR << "Decreasing lexicon " << cache << " cache for " << name
             << " to " << new_size << "\n";
    return new_size;
}

class write_attr {
public:
    const string name;
protected:
    Position currpos;
    char *default_val;
    bool noregopt;
    vector<NumOfPos> size;
public:
    write_attr (const string &filename, const char *def, bool noregexopt)
        : name (filename), default_val (strdup(def)),
          noregopt (noregexopt || skip_regexopt), size(0) {}
    virtual ~write_attr () {
        free(default_val);
        if (!size.empty()) {
            ToFile<int64_t> frqf (name + ".token");
            for (vector<NumOfPos>::const_iterator it = size.begin();
                 it != size.end(); it++)
                frqf.put (*it);
        }
    }
    virtual void write (char *str, const char *curr_fileline = 0,
                        NumOfPos size = 0) = 0;
    virtual void fill_up_to (Position pos) = 0;
    virtual void write (const char *str, Position pos, const char *curr_fileline=0,
                        NumOfPos size = 0) = 0;
    Position get_pos() {return currpos;}
    virtual void compile_regexopt () {
        if (!noregopt && !corpname.empty()) {
            string aname = name;
            size_t slash = aname.rfind('/');
            if (slash != aname.npos)
                aname = string (aname, slash + 1);
            CERR << "Creating regular expression optimization attribute "
                 << aname << endl;
            if (system (("mkregexattr " + corpname + " " + aname).c_str()) != 0)
                CERR << "ERROR: failed to create regular expression attribute "
                     << aname << endl;
        }
    }
};

class enc_err {
    string name;
    string msg;
    NumOfPos count;
public:
    enc_err (const char *name, const char *msg)
        : name (name), msg (msg), count (0) {}
    void emit_err (const char *line, ...) {
        if (count++ < enc_err_max || enc_err_max == -1) {
            va_list args;
            va_start (args, line);
            CERR << line << "warning: ";
            vfprintf(stderr, msg.c_str(), args);
            va_end (args);
            fputc('\n', stderr);
        }
        if (count == enc_err_max && enc_err_max != -1) {
            CERR << "There were already 100 similar errors in the input\n";
            CERR << "further errors will be suppressed and a summary will be\n";
            CERR << "provided at the end of the compilation.\n";
            CERR << "Use -v to emit all occurrences.\n";
        }
    }
    void emit_summary () {
        if (verbose || count)
            CERR << count << " times: warning type '" << name << "'\n";
    }
};

enc_err err_strip_space ("space in attribute value",
                         "stripping space in attribute value");
enc_err err_open_same_str ("structure opened multiple times on same position",
                           "opening structure (%s) on the same "
                           "position, ignoring the previous empty one");
enc_err err_too_nested ("structure nested too many times",
                        "closing too many nested structures (%s), "
                        "first open structure starts here, maximum allowed "
                        "nesting is " XSTR(MAX_NESTING));
enc_err err_closing_str ("closing non opened structure",
                         "closing non opened structure (%s)");
enc_err err_auto_close ("closing structure automatically",
                        "auto-closing structure (%s) opened at line %s");
enc_err err_empty_attr ("empty attribute name", "empty attribute name");
enc_err err_missing_eq ("missing equal sign", "expecting '=', got '%c'");
enc_err err_attr_undef ("undefined structure attribute",
                        "structure attribute (%s) not defined, ignoring");
enc_err err_empty_line ("empty line", "empty line, ignoring");
enc_err err_many_attr ("too many attributes", "too many attributes");
enc_err err_overlong_attr ("overlong attribute value", "overlong attribute value");

class write_full_attr: public write_attr {
    int reset_mem_counter;
    bool force_mem;
protected:
#ifdef LEXICON_USE_FSA3
    write_fsalex *lex;
#else
    write_lexicon *lex;
#endif
    RevFileConsumer *rev;
    TextConsumer *corp;
    bool multivalue;
    const char multisep;
public:
    write_full_attr (const string &filename,
                TextConsumer::TextType tt=TextConsumer::tdelta,
                const char *def="", bool multiv=false, const char multis=',',
                int lexiconsize=0, bool noregexopt=false)
        : write_attr (filename, def, noregexopt), reset_mem_counter (0),
        force_mem (lexiconsize != 0),
#ifdef LEXICON_USE_FSA3
        lex (new write_fsalex(filename, true)),
#else
        lex (new write_lexicon(filename, tt_scaling(tt, lex_mem, lexiconsize))),
#endif
        rev (RevFileConsumer::create (name, tt_scaling(tt, rev_mem), -1, append_opt)),
        corp (TextConsumer::create (tt, name, append_opt)),
        multivalue (multiv), multisep (multis) {
        currpos = append_opt ? corp->curr_size() : 0;
    }
    ~write_full_attr () {
        size_t lex_size = lex->size();
        delete lex; delete corp; delete rev;
        if (lex_size > 10000)
            compile_regexopt();
    }
    void write (char *str, const char *curr_fileline = 0, NumOfPos s = 0) {
        if (!str)
            str = default_val;
        char *begin = str;
        size_t end;	
        for (; *str == ' '; str++);
        for (end = strlen(str); end && str[end-1] == ' '; end--);
        if (begin != str || end != strlen(str)) {
            str[end] = 0;
            if (!curr_fileline) {
                curr_fileline = fileline();
            }
            err_strip_space.emit_err (curr_fileline);
        }
        if (end > 32767) {
            end = 32767;
            str[end] = 0;
            if (!curr_fileline) {
                curr_fileline = fileline();
            }
            err_overlong_attr.emit_err (curr_fileline);
        }
        unsigned id = lex->str2id (str);
        corp->put (id);
        rev->put (id, currpos);
        if (s) {
            if (id < size.size())
                size[id] += s;
            else
                size.push_back (s);
        }
        if (multivalue) {
            if (!multisep) {
                // multisep == "" --> store characters
                if (str[0] && str[1]) {
                    char value[2];
                    value[1] = 0;
                    while (*str) {
                        value[0] = *str;
                        rev->put (lex->str2id (value), currpos);
                    }
                }
            } else if (strchr (str, multisep)) {
                static int value_len = 0;
                static char *value = NULL;
                ensuresize (value, value_len, strlen (str) +1, 
                            "write_attr::write");
                strncpy (value, str, value_len);
                char *save_ptr = value;
                bool endofval;
                for (char *s = my_strtok (save_ptr, multisep, endofval); 
                     !endofval; 
                     s = my_strtok (save_ptr, multisep, endofval)) {
                    rev->put (lex->str2id (s), currpos);
                }
            }
        }
#ifndef LEXICON_USE_FSA3
        // check lexicon cache size at each 1k for structattrs, 10M for posattrs
        if (!force_mem && currpos %
                (corp->get_type() == TextConsumer::tint ? 1000 : 10000000)==0) {
            if (!force_mem && reset_mem_counter != reset_mem) {
                int new_size = tt_scaling(corp->get_type(), lex_mem, 0);
                CERR << "Decreasing lexicon cache sizes for " << name
                     << " to " << new_size << " to scale down memory usage\n";
                lex->max_added_items = new_size;
                lex->max_cache_items = new_size;
                reset_mem_counter = reset_mem;
            } else {
                // added hashmap (=new items)
                int added_load = lex->pop_added_load();
                if (added_load > 0) {
                    lex->max_added_items =
                        make_bigger_cache (lex->max_added_items, "new items",
                                           name.c_str(), lex->avg_str_size(),
                                           corp->get_type());
                } else if (lex->max_added_items > 5000 && added_load < 0) {
                    lex->max_added_items =
                        make_smaller_cache (lex->max_added_items, "new items",
                                            name.c_str());
                }
                // cache hashmap (=recent items)
                float cmr = lex->pop_cache_miss_ratio();
                if (lex->max_cache_items > 5000 && cmr > 0.8) {
                    lex->max_cache_items =
                        make_smaller_cache (lex->max_cache_items, "recent items",
                                            name.c_str());
                } else if (cmr < 0.2) {
                    lex->max_cache_items =
                        make_bigger_cache (lex->max_cache_items, "recent items",
                                           name.c_str(), lex->avg_str_size(),
                                           corp->get_type());
                }
            }
        }
#endif
        currpos++;
    }
    void fill_up_to (Position pos) {
        size_t id;
        if (currpos < pos) {
            id = lex->str2id (default_val);
            while (currpos < pos) {
                corp->put (id);
                rev->put (id, currpos++);
            }
        }
    }
    void write (const char *str, Position pos, const char *curr_fileline=0,
                NumOfPos size=0) {
        if (pos < currpos) {
            if (!curr_fileline)
                curr_fileline = fileline();
            CERR << curr_fileline
                 << "error: corrupted file (" << name 
                 << "), incorrect position: " << pos 
                 << " instead of " << currpos << '\n';
            return;
        }
        fill_up_to (pos);
        write ((char *) str, curr_fileline, size);
    }
};

class write_unique_attr: public write_attr {
protected:
    write_unique_lexicon *lex;
public:
    write_unique_attr (const string &filename, const char *def="",
                       bool noregexopt=false)
        : write_attr(filename, def, noregexopt),
          lex (new write_unique_lexicon(filename)) {
        currpos = append_opt ? lex->size() : 0;
    }
    ~write_unique_attr () {
        /* XXX check that lex is indeed unique */
        size_t lex_size = lex->size();
        delete lex;
        if (lex_size > 10000)
            compile_regexopt();
    }
    void write (char *str, const char *curr_fileline = 0, NumOfPos s = 0) {
        if (!str)
            str = default_val;
        char *begin = str;
        size_t end;
        for (; *str == ' '; str++);
        for (end = strlen(str); end && str[end-1] == ' '; end--);
        if (begin != str || end != strlen(str)) {
            str[end] = 0;
            if (!curr_fileline) {
                curr_fileline = fileline();
            }
            err_strip_space.emit_err (curr_fileline);
        }
        if (end > 32767) {
            end = 32767;
            str[end] = 0;
            if (!curr_fileline) {
                curr_fileline = fileline();
            }
            err_overlong_attr.emit_err (curr_fileline);
        }
        unsigned id = lex->str2id (str);
        if (s) {
            if (id < size.size())
                size[id] += s;
            else
                size.push_back (s);
        }
        currpos++;
    }
    void fill_up_to (Position pos) {
        for (; currpos < pos; currpos++)
            lex->str2id (default_val);
    }
    void write (const char *str, Position pos, const char *curr_fileline=0,
                NumOfPos size = 0) {
        if (pos < currpos) {
            if (!curr_fileline)
                curr_fileline = fileline();
            CERR << curr_fileline
                 << "error: corrupted file (" << name
                 << "), incorrect position: " << pos
                 << " instead of " << currpos << '\n';
            return;
        }
        fill_up_to (pos);
        write ((char *) str, curr_fileline, size);
    }

};

write_attr *new_write_attr (const string &filename, CorpInfo::MSS &aopts, 
                            TextConsumer::TextType deftt)
{
    if (aopts["TYPE"] == "UNIQUE") return
        new write_unique_attr (filename,
            aopts["DEFAULTVALUE"].empty() ? "===NONE===" : aopts["DEFAULTVALUE"].c_str(),
            CorpInfo::str2bool (aopts["NOREGEXOPT"]));
    else return
        new write_full_attr (filename, type2consumer (aopts["TYPE"], deftt),
            aopts["DEFAULTVALUE"].empty() ? "===NONE===" : aopts["DEFAULTVALUE"].c_str(),
            CorpInfo::str2bool (aopts["MULTIVALUE"]),
            aopts["MULTISEP"][0], atol(aopts["LEXICONSIZE"].c_str()),
            CorpInfo::str2bool (aopts["NOREGEXOPT"]));
}

class int_writer {
public:
    int_writer(){}
    virtual ~int_writer() noexcept(false) {}
    virtual void put (Position p) = 0;
    virtual void takeback (int count) = 0;
    virtual Position size() = 0;
};

template <class IntType>
class IntWriter: public int_writer
{
    ToFile<IntType> rng;
public:
    IntWriter(const string &path): rng(path, append_opt) {}
    virtual void takeback (int count) {rng.takeback(count);}
    virtual void put (Position p);
    virtual Position size() {return rng.tell();}
};

int_writer *create_int_writer (const string &filename, const string &type)
{
    if (type == "file64" || type == "map64")
        return new IntWriter<int64_t>(filename);
    else
        return new IntWriter<int32_t>(filename);
}

template <> void IntWriter<int64_t>::put (Position p) {rng(p);}
template <> void IntWriter<int32_t>::put (Position p) {
    if (p > numeric_limits<int32_t>::max())
        throw overflow_error ("Too many positions, use file64/map64");
    rng(p);
}

class write_region {
protected:
    const string path;
    const char *base;
    int_writer *rng;
    Position regnum;
    write_attr *vals;
    Position lastpos;
    bool allownested;
    struct Nested_rng {
        Position beg, end;
        string val_fileline;
        string value;
        Nested_rng (Position beg)
            : beg(beg), end(-1), val_fileline(fileline()) {}
    };
    vector<Nested_rng> notstored;

    virtual void flush_notstored() {
        bool nesteditem = false;
        for (vector<Nested_rng>::iterator i=notstored.begin(); 
             i != notstored.end(); i++) {
            rng->put(i->beg);
            rng->put(nesteditem ? -i->end : i->end);
            nesteditem = true;
            regnum++;
            if (!i->value.empty())
                store_val (i->value.c_str(), i->val_fileline, i->end - i->beg);
        }
        notstored.clear();
    }
    virtual void store_val (const char *valuestr, const string &val_fileline,
                            NumOfPos size) {
        if (!vals)
            vals = new write_full_attr (path + ".val", TextConsumer::tint);
        vals->write (valuestr, regnum, val_fileline.c_str(), size);
    }
public:
    write_region (const string &filename, const string &type, bool nested)
        :path (filename), base (my_basename (path.c_str())),
         rng (create_int_writer (path + ".rng", type)),
         regnum (rng->size()/2 -1), vals (NULL), lastpos (-1),
         allownested (nested) {}
    virtual ~write_region() {
        if (vals) {
            flush_notstored();
            delete vals;
        }
        delete rng;
    }
    virtual bool is_open() {
        return !notstored.empty() && notstored[0].end == -1;
    }
    virtual void print_opened (ostringstream &out, const string &name) {
        for (size_t i=0; i < notstored.size(); i++) {
            if (notstored[i].end == -1)
                out << "  inside <" << name << " " 
                    << notstored[i].value << ">\n";
        }
    }
    virtual void open (Position pos) {
        if (!allownested) {
            if (is_open())
                notstored[0].end = pos;
            if (lastpos == pos) {
                err_open_same_str.emit_err (fileline(), base);
                notstored.pop_back();
            } else {
                flush_notstored();
                lastpos = pos;
            }
        }
        if (notstored.size() >= MAX_NESTING) {
            err_too_nested.emit_err (notstored[0].val_fileline.c_str(), base);
            while (is_open()) {
                close(pos);
            }
        }
        notstored.push_back(Nested_rng(pos));
    }
    virtual void open (Position pos, const char *valuestr) {
        open (pos);
        notstored.back().value = valuestr;
    }
    virtual void close (Position pos, int warn = 0) {
        if (!is_open()) {
            err_closing_str.emit_err (fileline(), base);
            return;
        }
        for (int i = notstored.size() -1; i >= 0; i--)
            if (notstored[i].end == -1) {
                notstored[i].end = pos;
                if (warn)
                    err_auto_close.emit_err (fileline(), base,
                                             notstored[i].val_fileline.c_str());
                break;
            }
        if (allownested && !is_open())
            flush_notstored();
    }
};

static const char *read_literal (const char *txtin, char *literal, 
                                 int lit_size)
{
    while ((isalnum (*txtin) || *txtin == '_') && --lit_size > 0)
        *(literal++) = *(txtin++);
    *literal = 0;
    return txtin;
}

static const char *read_string (const char *txtin, char *str, char enclosed,
                                int str_size)
{
    while (*txtin && *txtin != enclosed && --str_size > 0) {
        if (*txtin == '\\' && (txtin[1] == enclosed || txtin[1] == '\\'))
            txtin++;
        *(str++) = *(txtin++);
    }
    txtin++;
    *str = 0;
    return txtin;
}

static const char *skip_white (const char *txtin)
{
    while (isspace (*txtin))
        txtin++;
    return txtin;
}

static write_attr *ignored_attr = (write_attr*)1;

class write_region_attr : public write_region {
    typedef map<string,write_attr*> MSW;
    MSW attrs;
    CorpInfo *structci;
public:
    write_region_attr (const string &filename, CorpInfo *strci = NULL) 
        : write_region (filename, strci ? strci->opts["TYPE"] : "", 
                        strci ? 
                        CorpInfo::str2bool(strci->opts["NESTED"]) : false),
          structci (strci) {}
    virtual ~write_region_attr () {
        flush_notstored();
        if (structci) {
            // add missing attributes present in configuration
            for (CorpInfo::VSC::iterator i = structci->attrs.begin();
                 i != structci->attrs.end(); i++) {
                CorpInfo::MSS &ao = (*i).second->opts;
                if (ao["DYNAMIC"] != "") {
                    // store dynamic attributes for postprocessing
                    if (ao["DYNTYPE"] != "plain")
                        dynattrs.push_back (make_pair(base + ("." + (*i).first),
                                                      false));
                    continue;
                }
                if (!attrs[(*i).first]) {
                    attrs[(*i).first] = new_write_attr(path + '.' + (*i).first,
                                        structci->find_attr ((*i).first),
                                        TextConsumer::tint);
                    CERR << "warning: structure attribute (" << base << "."
                         << (*i).first << ") never present\n";
                }
            }
        }
        for (MSW::iterator i=attrs.begin(); i != attrs.end(); i++) {
            if ((*i).second == ignored_attr)
                continue;
            (*i).second->fill_up_to (regnum +1);
            delete (*i).second;
        }
    }
    virtual void store_val (const char *valuestr, const string &val_fileline,
                            NumOfPos size) {
        const char *s = valuestr;
        char attr_name [80];
        static char *value = NULL;
        static int value_len = 0;
        ensuresize (value, value_len, strlen (valuestr) +1, 
                    "write_region_attr::store_val");
        while (*s) {
            s = read_literal (s, attr_name, sizeof (attr_name));
            if (attr_name[0] == 0) {
                err_empty_attr.emit_err (val_fileline.c_str());
                break;
            }
            s = skip_white (s);
            if (*s != '=') {
                err_missing_eq.emit_err (val_fileline.c_str(), *s);
                break;
            }
            s++;
            s = skip_white (s);
            if (*s == '\'' || *s == '"')
                s = read_string (s+1, value, *s, value_len);
            else
                s = read_literal (s, value, value_len);
            write_attr *a = attrs [attr_name];
            if (a == NULL) {
                if (structci) {
                    try {
                        a = new_write_attr (path + '.' + attr_name, 
                                            structci->find_attr (attr_name),
                                            TextConsumer::tint);
                    } catch (CorpInfoNotFound &err) {
                        size_t slash = path.rfind('/');
                        string msg;
                        if (slash != string::npos)
                            msg += path.substr(slash + 1) + ".";
                        msg += err.name;
                        err_attr_undef.emit_err (val_fileline.c_str(),
                                                 msg.c_str());
                        a = ignored_attr;                       
                    }
                } else {
                    a = new write_full_attr (path + '.' + attr_name,
                                        TextConsumer::tint);
                }
                attrs [attr_name] = a;
            }
            /* XXX subst_ent & iconv (value) */
            if (a != ignored_attr)
                a->write (value, regnum, val_fileline.c_str(), size);
            s = skip_white (s);
        }
    }    
};


FILE* open_source (const char *in_name, CorpInfo *ci)
{
    if (in_name == NULL || (in_name[0] == '-' && in_name[1] == '\0')) {
        infilename = "(standard input)";
        return stdin;
    } else {
        if (in_name[0] == '|') {
            FILE *p = try_popen (in_name+1, "r");
            if (p == NULL) {
                CERR << "encodevert: cannot execute command `" << in_name+1
                     << "' [" << strerror (errno) << "]\n";
                return NULL;
            }
            infilename = "(pipe)";
            return p;
        } else {
            FILE *ret;
            if (!(ret = fopen(in_name, "r"))) {
                CERR << "encodevert: cannot open file `" << in_name
                     << "' [" << strerror (errno) << "]\n";
            }
            infilename = in_name;
            return ret;
        }
    }
}

#define LINE_ALLOC_DELTA 1024
char* getline(FILE* input, char* line, size_t line_avail, size_t *line_alloc) {
    if (!fgets(line + *line_alloc - line_avail, line_avail, input)) {
        return NULL;
    } else if (line[strlen(line) - 1] != '\n' && strlen(line) == *line_alloc - 1) {
        *line_alloc += LINE_ALLOC_DELTA;
        line = (char*) realloc(line, *line_alloc);
        if (!line) {
            perror ("Out of memory.\n");
            return NULL;
        }
        return getline(input, line, LINE_ALLOC_DELTA + 1, line_alloc);
    } else {
        return line;
    }
}

int encode (const string &corp_name, const string &corp_path, 
            const char *attr_names, const char *struct_names, 
            const char *in_name, 
            TextConsumer::TextType text_type=TextConsumer::tdelta,
            bool use_xml=false)
{
    CorpInfo *ci = NULL;
    long long int avail_mem = available_memory();
    if (!corp_name.empty()) {
        corpname = corp_name;
        ci = loadCorpInfo (corp_name);
        // set memory limits to process input without running out of memory
        if (max_memory == 0 || avail_mem * 2 < max_memory)
            // allow at most 2-times more memory than available
            max_memory = avail_mem;
        use_mem = estimate_mem_usage(ci);
        CERR << "Available memory " << avail_mem / (1024 * 1024)
             << "MB, estimated usage " << use_mem / (1024 * 1024) << "MB\n";
        free_mem = scale_bucket_size (use_mem, max_memory);
    } else
        free_mem = avail_mem * 0.8;

    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; /* Restart functions if interrupted by handler */
    if (sigaction(SIGUSR1, &sa, NULL) == -1)
        CERR << "Could not set up signal handler for SIGUSR1\n";
    if (sigaction(SIGUSR2, &sa, NULL) == -1)
        CERR << "Could not set up signal handler for SIGUSR2\n";

    string path = corp_path.empty() ? ci->opts ["PATH"] : corp_path;
    if (*(path.end() -1) == '/')
        path.erase (path.length() -1);
    struct stat dir_st;
    if (stat (path.c_str(), &dir_st) < 0) {
#if defined __MINGW32__
#  define mkdir_args
#else
#  define mkdir_args , S_IRWXU | S_IRWXG | S_IXOTH | S_IROTH
#endif
        if (mkdir (path.c_str() mkdir_args)) {
            CERR << "encodevert: cannot make directory `" << path << "'\n";
            return 1;
        }
    }
    path += '/';
    typedef vector<write_attr*> VWA;
    VWA attrs;
    
    // input file
    if (in_name == NULL && ci && !ci->opts ["VERTICAL"].empty()) {
        in_name = ci->opts ["VERTICAL"].c_str();
    }
    FILE *in_file = open_source (in_name, ci);
    if (!in_file)
        return 1;

    if (attr_names[0]) {
        // given attributes
        istringstream as (attr_names);
        string a;
        write_attr *wa;
        while (getline (as, a, ',')) {
            if (ci)
                wa = new_write_attr (path + a, ci->find_attr (a), text_type);
            else
                wa = new write_full_attr (path + a, text_type);
            attrs.push_back (wa);
        }
    } else {
        // attributes from config
        for (CorpInfo::VSC::iterator i=ci->attrs.begin(); i != ci->attrs.end();
             i++) {
            CorpInfo::MSS &ao = (*i).second->opts;
            if (ao["MAPTO"] != "") {
                istringstream map_as (ao["MAPTO"]);
                string map_a;
                vector<string> map_to;
                while (getline (map_as, map_a, ','))
                    map_to.push_back (map_a);
                normattrs.push_back (make_pair((*i).first, map_to));
            }
            if (ao["DYNAMIC"] != "") {
                // store dynamic attributes for postprocessing
                if (ao["DYNTYPE"] != "plain")
                    dynattrs.push_back (make_pair ((*i).first,
                                        !CorpInfo::str2bool (ao["NOREGEXOPT"])));
                continue;
            }
            attrs.push_back (new_write_attr (path + (*i).first, ao, text_type));
        }
    }
    if (attrs.empty()) {
        CERR << "encodevert: no attributes specified!\n";
        return 2;
    }

    MWR structs;
    if (verbose) {
        show_structs = &structs;
        enc_err_max = -1;
    }
    regex_t *regpat = NULL;
    
    if ((struct_names && struct_names[0]) || (ci && !ci->structs.empty())) {
        string regpatstr = "^</?(";
        if (struct_names && struct_names[0]) {
            istringstream as (struct_names);
            string a;
            while (getline (as, a, ',')) {
                structs [a] = use_xml ? new write_region_attr (path + a) :
                    new write_region (path + a, "", false);
                regpatstr += a + '|';
            }
        } else {
            for (CorpInfo::VSC::iterator i=ci->structs.begin(); 
                 i != ci->structs.end(); i++) {
                string a = (*i).first;
                CorpInfo::VSC &at = (*i).second->attrs;
                if (at.size() != 1 || at[0].first != "val")
                    structs [a] = new write_region_attr (path +a, (*i).second);
                else
                    structs [a] = new write_region (path + a, "", false);
                regpatstr += a + '|';
            }
        }
        regpatstr.erase (regpatstr.length() -1);
        regpatstr += ")( +(.*))?/?(>)[ \t]*$";
        /* regmatch[0] line
         * regmatch[1] attrname
         * regmatch[2] rest of attr
         * regmatch[3] attrval
         * regmatch[4] '>'
         */
        regpat = new regex_t;
        if (regcomp (regpat, regpatstr.c_str(), REG_EXTENDED)) {
            CERR << "encodevert: regular expression compilation error\n";
            delete regpat;
            regpat = NULL;
        }
    }

    size_t line_alloc = LINE_ALLOC_DELTA;
    char* line = (char*) malloc(sizeof(char) * line_alloc);
    if (!line) {
        perror("Out of memory.\n");
        return 1;
    }
    int len;
#define NUM_OF_SUBEXPR 5
    regmatch_t regmatch[NUM_OF_SUBEXPR];
    linenum = 0;
    while ((line = getline (in_file, line, line_alloc, &line_alloc))) {
        if (sig_received)
            process_signal (sig_received);
        linenum++;
        if (linenum % 1000000 == 0)
            CERR << "Processed " << linenum << " lines, "
                 << attrs[0]->get_pos() << " positions.\n";

        // strip ending whitespaces
        len = strlen(line);
        while (len > 0 && (line[len-1] == ' ' || line[len-1] == '\r' 
                           || line[len-1] == '\n'))
            len--;

        // ignore empty lines
        if (len == 0) {
            err_empty_line.emit_err (fileline());
            continue;
        }
        // ignore <! and <?
        if (use_xml && line[0] == '<' && (line[1] == '?' || line[1] == '!'))
            continue;
        line[len] = '\0';
        
        if (regpat && line[0] == '<' && 
            ! regexec (regpat, line, NUM_OF_SUBEXPR, regmatch, 0)) {
            // write structure
            string sname (line, regmatch[1].rm_so, 
                          regmatch[1].rm_eo - regmatch[1].rm_so);
            write_region *s = structs [sname];
            Position currpos = attrs[0]->get_pos();
            if (line[1] == '/')
                s->close (currpos);
            else {
                len = regmatch[4].rm_so;
                // empty structure (<g/>)
                bool empty = line [len -1] == '/';
                if (regmatch[3].rm_so != regmatch[3].rm_eo) {
                    if (empty)
                        len--;
                    line [len] = '\0';
                    s->open (currpos, line + regmatch[3].rm_so);
                } else
                    s->open (currpos);
                if (empty)
                    s->close (currpos);
            }
        } else {
            // write attributes
            char *val = line;
            bool endofline;
            for (VWA::iterator i=attrs.begin(); i != attrs.end(); i++) {
                /* XXX subst_ent & iconv (val) */
                char *s = my_strtok (val, '\t', endofline);
                (*i)->write (s);
            }
            if (val[0] != '\0')
                err_many_attr.emit_err (fileline());
        }
    }

    free(line);

    bool pipe_failed = false;
    if (in_file != stdin) {
        if (in_name && in_name[0] == '|') {
            int ret = pclose(in_file);
            if (ret) {
                CERR << "ERROR: child process"
                   " exited abnormally: status " << ret << endl;
                pipe_failed = true;
            }
        } else fclose(in_file);
    }

    CERR << "Reading input file finished." << endl;
    Position finalpos = attrs[0]->get_pos();
    show_structs = NULL;
    for (MWR::iterator i=structs.begin(); i != structs.end(); i++) {
        CERR << "Closing structure " << (*i).first << " ...\n";
        while ((*i).second->is_open())
            (*i).second->close (finalpos, 1);
        delete (*i).second;
        CERR << "            ... finished.\n";
    }
    for (VWA::iterator i=attrs.begin(); i != attrs.end(); i++) {
        CERR << "Closing attribute " << (*i)->name << " ...\n";
        delete (*i);
        CERR << "            ... finished.\n";
    }

    CERR << "Processed " << linenum << " lines, "
         << finalpos << " positions.\n";


    // create dynamic attributes
    if (!skip_dynamic) {
        for (vector<pair <string, bool > >::const_iterator i=dynattrs.begin();
             i != dynattrs.end(); i++) {
            const string &n = (*i).first;
            bool do_regopt = (*i).second;
            CERR << "Creating dynamic attribute " << n << endl;
            if (system (("mkdynattr " + corp_name + " " + n).c_str()) != 0)
                CERR << "ERROR: failed to create dynamic attribute "
                     << n << endl;
            else if (do_regopt){
                CERR << "Creating regular expression optimization attribute "
                     << n << endl;
                if (system (("mkregexattr " + corp_name + " " + n).c_str()) != 0)
                    CERR << "ERROR: failed to create regular expression attribute "
                         << n << endl;
            }
        }
    }

    // create mapping to normalization attributes
    if (!skip_normattr) {
        for (VNA::const_iterator i=normattrs.begin();
             i != normattrs.end(); i++) {
            for (vector<string>::const_iterator ii=(*i).second.begin();
                 ii != (*i).second.end(); ii++) {
                CERR << "Creating mapping from " << (*i).first
                     << " to normalization attribute " << *ii << endl;
                CERR << "mknormattr " + corp_name + " " + (*i).first + " " + *ii << endl;
                if (system (("mknormattr " + corp_name + " " + (*i).first
                                           + " " + *ii).c_str()) != 0)
                    CERR << "ERROR: failed to create mapping" << endl;
            }
        }
    }

    CERR << "Summary of errors encountered in the input file:\n";
    err_strip_space.emit_summary();
    err_open_same_str.emit_summary();
    err_too_nested.emit_summary();
    err_closing_str.emit_summary();
    err_auto_close.emit_summary();
    err_empty_attr.emit_summary();
    err_missing_eq.emit_summary();
    err_attr_undef.emit_summary();
    err_empty_line.emit_summary();
    err_many_attr.emit_summary();
    err_overlong_attr.emit_summary();
    if (!verbose)
        CERR << "Only first 100 occurrences emitted, use -v to emit each occurrence.\n";

    if (pipe_failed) return 1;
    else return 0;
}

void usage (const char *progname) {
    cerr << "Usage: " << progname << " [OPTIONS] [FILENAME]\n"
         << "Creates a new corpus from a vertical text in file FILENAME or stdin.\n\n"
         << "    -c CORPNAME   corpus name in registry or corpus configuration file path\n"
         << "    -p CORPDIR    path to the corpus directory\n"
         << "    -v            verbose messages\n"
         << "    -e            append data to the end of an existing corpus\n"
         << "    -m MEM        use at most MEM bytes of memory for lexicon caches\n"
         << "    -d            do NOT compile dynamic attributes\n"
         << "    -n            do NOT compile mapping to normalization attributes\n"
         << "    -r            do NOT compile regular expression optimization attributes\n"
         << "Options below apply only if configuration file is not supplied via -c option\n"
         << "    -b            use `FD_FGD' attribute type\n"
         << "                  Use this option when creating corpora over ca 250M tokens\n"
         << "                  (encodevert will stop processing if this option is necessary and was not specified)\n"
         << "    -x            use XML style of structures\n"
         << "    -a ATTRS      comma delimited list of attributes\n"
         << "    -s STRUCTS    comma delimited list of structures\n"
         << "\nExamples:\n"
         << "   encodevert -c susanne susanne.vert\n"
         << "   encodevert -a word,lemma -x -s doc,p -a tag -p /nlp/corp/desam desam.vert\n"
         << "   cat *.src | encodevert -a word,lemma,tag,status -s doc,p,head,g -p susanne\n"
        ;
}

int main (int argc, char **argv)
{
    const char *input_name = NULL;
    const char *progname = argv[0];
    const char *corpdir = NULL;
    const char *corpname = NULL;
    string oattrs = "";
    string ostructs = "";
    bool ob = false;
    bool ox = false;
    {
        // process options
        int c;
        while ((c = getopt (argc, argv, "c:a:s:p:bxvedrnh?m:")) != -1)
            switch (c) {
            case 'c':
                corpname = optarg;
                break;
            case 'p':
                corpdir = optarg;
                break;
            case 'a':
                oattrs = oattrs.empty() ? optarg : oattrs + ',' + optarg;
                break;
            case 's':
                ostructs = ostructs.empty() ? optarg : ostructs + ',' + optarg;
                break;
            case 'b':
                ob = true;
                break;
            case 'x':
                ox = true;
                break;
            case 'v':
                verbose = true;
                break;
            case 'e':
                append_opt = true;
                break;
            case 'm':
                max_memory = atol(optarg);
                if (max_memory && max_memory < 10)
                    max_memory = 10;
                break;
            case 'd':
                skip_dynamic = true;
                break;
            case 'r':
                skip_regexopt = true;
                break;
            case 'n':
                skip_normattr = true;
                break;
            case '?':
            case 'h':
                usage (progname);
                return 2;
            default:
              abort ();
            }
        
        if (optind < argc) {
            input_name = argv [optind++];
            if (optind < argc) {
                usage (progname);
                return 2;
            }
        }
    }
    if (oattrs.empty() && corpname == NULL) {
        CERR << "You have to specify corpus name or at least one attribute!\n";
        usage (progname);
        return 2;
    }
    if (corpdir == NULL && corpname == NULL) {
        CERR << "You have to specify corpus name or corpus directory!\n";
        usage (progname);
        return 2;
    }

    try {
        if (corpdir == NULL) corpdir = "";
        if (corpname == NULL) corpname = "";
        return encode (corpname, corpdir, oattrs.c_str(), ostructs.c_str(), 
                       input_name, 
                       (ob ? TextConsumer::tdeltagiga : TextConsumer::tdelta), 
                       ox);
    } catch (exception &ex) {
        CERR << "encodevert error: " << ex.what() << '\n';
        return 1;
    }
}

const char *fileline()
{
    static string buff;
    ostringstream out;
    if (show_structs) {
        for (MWR::iterator i=show_structs->begin(); 
             i != show_structs->end(); i++) {
            if ((*i).second->is_open())
                (*i).second->print_opened (out, (*i).first);
        }
    }
    out << infilename << ":" << linenum << ": ";
    buff = out.str();
    return buff.c_str();
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:

