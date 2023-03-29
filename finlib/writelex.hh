// Copyright (c) 2001-2018  Pavel Rychly, Milos Jakubicek

#ifndef WRITELEX_HH
#define WRITELEX_HH

#include "lexicon.hh"
#include "fromtof.hh"
#include <math.h>
#include <unordered_map>
#include <functional>

extern "C" {
#include <hat-trie/hat-trie.h>
}

class write_lexicon
{
    struct int_1 {
        int i;
        int_1(): i(-1) {}
    };

    const std::string filename;
    std::unordered_map<std::string,int_1> cache;
    std::unordered_map<std::string,int> added;
    FILE *lexf;
    ToFile<lexpos> *lidxf;
    ToFile<lexpos> *lsrtf;
    ToFile<lexpos> *lovff;
    off_t lexftell;
    off_t last_lexf_ovf;
    int nextid;
    lexicon *lex;
    bool append, cache_was_cleared, added_was_cleared;
    int cache_miss, cache_queries;
    int new_item (const char *str);
    void flush_hash ();
public:
    unsigned int max_added_items, max_cache_items;
    write_lexicon (const std::string &filename, int maxitems, bool app = true);
    ~write_lexicon ();
    int str2id(const char *str);
    int size() {return nextid;}
    int avg_str_size ();
    float pop_cache_miss_ratio();
    int pop_added_load();
};

int make_lex_srt_file (const std::string &filename);

class write_unique_lexicon
{
    const std::string filename;
    FILE *lexf;
    ToFile<lexpos> *lidxf;
    ToFile<lexpos> *lovff;
    off_t lexftell;
    off_t last_lexf_ovf;
    int nextid;
public:
    write_unique_lexicon (const std::string &filename);
    ~write_unique_lexicon ();
    int str2id(const char *str);
    int size() {return nextid;}
};

class write_fsalex
{
    const std::string filename;
    hattrie_t* hash;
    int nextid;
    bool write_lexfiles;
    FILE *lexf;
    ToFile<lexpos> *lsrtf, *lidxf;
    off_t lexftell;
    lexicon *oldlex;
    ToFile<lexpos> *lovff;
    off_t last_lexf_ovf;
    void open_outfiles (void);
public:
    write_fsalex (const std::string &filename, bool write_lexfiles = true, bool append = true);
    ~write_fsalex ();
    int str2id(const char *str);
    int size() {return nextid;}
};

#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
