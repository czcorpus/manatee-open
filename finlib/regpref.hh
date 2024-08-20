// Copyright (c) 1999-2015  Pavel Rychly, Milos Jakubicek

#ifndef REGPREF_HH
#define REGPREF_HH

#include <config.hh>
#include <vector>
#include <string>

#if defined USE_REGEX
#include <cstdlib>
#include <regex.h>
#elif defined USE_PCRE
#include <pcre.h>
#include <pcreposix.h>
#elif defined USE_PCRE2
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <pcre2posix.h>
#elif defined USE_ICU
#include <unicode/ucnv.h>
#include <unicode/regex.h>
using namespace icu;
#endif

class regexp_pattern
{
protected:
#if defined USE_REGEX
    regex_t *comp;
#elif defined USE_PCRE
    pcre *comp;
#elif defined USE_PCRE2
    pcre2_code *comp;
    pcre2_match_data *md;
#elif defined USE_ICU
    UConverter *conv;
    RegexPattern *comp;
#endif
    const char *locale;
    const char *encoding;
    char *pat;
    char *pref;
    int options;
    bool no_meta, anything, disjunct;
    std::vector<std::string> disparts;
    regexp_pattern () {}
    regexp_pattern (regexp_pattern &rp) {}
    const char *find_meta (const char *p, const char *meta);
    void unescape (char *str);
public:
    regexp_pattern (const char *pattern, const char *loc = 0,
                    const char *enc = 0, bool ignorecase = false,
                    int addopt = 0);
    ~regexp_pattern();
    const char *prefix() {return pref;};
    bool compile();
    bool match (const char *str);
    bool no_meta_chars() {return no_meta;}
    bool any() {return anything;}
    std::vector<std::string>* disjoint_parts() {return &disparts;}
};

#ifdef USE_PCRE
const unsigned char *get_pcre_lower_table (const char *locale);
const unsigned char *get_pcre_collate_table (const char *locale);
#endif

#endif //REGPREF_HH

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
