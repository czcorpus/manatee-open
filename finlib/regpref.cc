// Copyright (c) 1999-2015 Pavel Rychly, Milos Jakubicek

#include "regpref.hh"
#include <cstring>
#include <cstdlib>
#include <new>

#if defined USE_REGEX
#elif defined USE_PCRE
#include <clocale>
#include <map>
#include <string>
#elif defined USE_ICU
#include <iostream>
using namespace std;
#endif


#define META_NOPIPE "\\^$.[(?*+{"
#define METACHARS META_NOPIPE"|"

#if defined USE_REGEX
    #define REGPAT_OPTS 0
    #define REGPAT_ICASE REG_ICASE
    #define REGPAT_UTF8 0
#elif defined USE_PCRE
    #define REGPAT_OPTS 0
    #define REGPAT_ICASE PCRE_CASELESS
    #define REGPAT_UTF8 ( PCRE_UTF8 | PCRE_UCP )
#elif defined USE_ICU
    #define REGPAT_OPTS 0
    #define REGPAT_ICASE UREGEX_CASE_INSENSITIVE
    #define REGPAT_UTF8 0
#else
    #define REGPAT_OPTS 0
    #define REGPAT_ICASE 0
    #define REGPAT_UTF8 0
#endif


#ifdef USE_PCRE

class pcre_locale_tables
{
    typedef const unsigned char * tab_t;
    typedef std::map<std::string, tab_t> CT;
    CT loc2tab;

public:
    pcre_locale_tables () : loc2tab() {
        loc2tab.insert (CT::value_type ("", NULL));
        loc2tab.insert (CT::value_type ("C", NULL));
        loc2tab.insert (CT::value_type ("POSIX", NULL));
#ifdef BUILDIN_PCRE_TABLES
#define INSTALL_PCRE(x) loc2tab.insert (CT::value_type (#x, pcre_tables_##x))
#include "buildin_tables.cc"
#endif
    }
    tab_t operator [] (const char *locale) {
        if (locale == NULL)
            return NULL;
        CT::const_iterator i = loc2tab.find (locale);
        if (i != loc2tab.end())
            return (*i).second;
        else {
            tab_t tab;
#ifdef BUILDIN_PCRE_TABLES
            tab = NULL;
#else
            char *prev_locale = setlocale (LC_CTYPE, locale);
            tab = pcre_maketables();
            setlocale (LC_CTYPE, prev_locale);
#endif
            loc2tab.insert (CT::value_type (locale, tab));
            return tab;
        }
    }
};

pcre_locale_tables locale_tabs;

//==================================================

const unsigned char *get_pcre_lower_table (const char *locale)
{
    return locale_tabs [locale];
}

// from pcre/internal.h  (same in savepcretables.cc)
#define cbit_length  320      /* Length of the cbits table */
#define cbits_offset  512
#define ctypes_offset (cbits_offset + cbit_length)
#define tables_length (ctypes_offset + 256)


const unsigned char *get_pcre_collate_table (const char *locale)
{
    const unsigned char *tab = locale_tabs [locale];
    return tab ? tab + tables_length : tab;
}

#endif //USE_PCRE

//==================== regexp_pattern ====================

regexp_pattern::regexp_pattern (const char *pattern, const char *loc, 
                                const char *enc, bool ignorecase, int addopt)
    : comp (NULL), locale (loc), encoding (enc), pref (NULL),
      options (addopt |REGPAT_OPTS), no_meta (false), anything (false)
{
    if (ignorecase)
        options |= REGPAT_ICASE;
    if (enc && !strcasecmp(enc, "utf-8"))
        options |= REGPAT_UTF8;

#if defined USE_PCRE
    int len = strlen(pattern);
    pat = new char[len + 7];
    strcpy (pat, "^(?:");
    strcpy (pat+4, pattern);
    strcpy (pat+4+len, ")$");
#elif defined USE_REGEX
    int len = strlen(pattern);
    pat = new char[len + 5];
    strcpy (pat, "^(");
    strcpy (pat+2, pattern);
    strcpy (pat+2+len, ")$");
#else
    pat = strdup (pattern);
#endif

    // remove unnecessary parentheses
    char *str_orig = strdup (pattern);
    if (!str_orig)
        throw std::bad_alloc();
    char *str = str_orig;
    char *end = str + strlen (str);
    if (*str == '(' && *(end - 1) == ')') {
        str++;
        *(end - 1) = '\0';
    }

    // check whether pattern matches anything
    char anylang[] = ".*";
    const char *p = str;
    if (str[0] != '\0') {
        while (!strncmp(anylang, p, 2))
            p += 2;
        if (*p == '\0')
            anything = true;
    }

    // does pattern contain OR?
    const char *pipe = pattern;
    while ((pipe = strchr (pipe, '|'))) {
        if (pipe > pattern && *(pipe-1) == '\\') {
            pipe++;
            continue;
        }
        pref = strdup("");
        break;
    }

    if (pref) { // is it disjunctive form - "word1|word2|..."?
        if (!find_meta (str, META_NOPIPE)) {
            char *part = strchr (str, '|');
            while (part) {
                if (part > str && *(part-1) == '\\') {
                    part = strchr (part + 1, '|');
                    continue;
                }
                char *s = strndup (str, part - str);
                if (!s)
                    throw std::bad_alloc();
                unescape (s);
                disparts.push_back (s);
                free (s);
                str = part + 1;
                part = strchr (str, '|');
            }
            unescape (str);
            disparts.push_back (str);
        }
    } else {
        p = find_meta (str, METACHARS);
        if (p == NULL) {
            pref = strdup(str);
            no_meta = true;
        } else {
            if (p && (*p == '?' || *p == '*' || *p == '{')) {
                p--;
                if (enc && !strcasecmp(enc, "utf-8"))
                    while(p > str && (*p & 0xc0) == 0x80) p--;
            }
            if (p <= str)
                pref = strdup("");
            else
                pref = strndup (str, p - str);
        }
    }
    free (str_orig);
    if (!pref)
        throw std::bad_alloc();
    unescape (pref);
#if defined USE_ICU
    {
        UErrorCode err = U_ZERO_ERROR;
        conv = ucnv_open (encoding, &err);
        if (U_FAILURE (err))
            cerr << "regexp_pattern: ucnv_open(" << encoding << "): " 
                 << u_errorName(err) << endl;
    }
#endif
}

void regexp_pattern::unescape (char *str)
{
#if defined USE_PCRE || defined USE_ICU
    size_t len = strlen (str);
    size_t i, j;
    for (i = 0, j = 0; j < len; i++, j++) {
        if (str[j] == '\\')
            str[i] = str[++j];
        else
            str[i] = str[j];
    }
    str[i] = '\0';
#endif
}

const char * regexp_pattern::find_meta (const char *p, const char *meta)
{
    while (1) {
        p = strpbrk (p, meta);
#if defined USE_PCRE || defined USE_ICU
        if (p && *p == '\\' && strchr (METACHARS, *(p+1)))
            p += 2; // skip escaped metacharacters
        else
            break;
#else
            break;
#endif
    }
    return p;
}

bool regexp_pattern::compile()
{
#if defined USE_REGEX
    comp = new regex_t;
    if (regcomp (comp, pat, options)) {
        delete comp;
        comp = NULL;
    }
    return comp == NULL;
#elif defined USE_PCRE
    const char *errptr;
    int erroffset;
    comp = pcre_compile (pat, options, &errptr, &erroffset, 
                         locale_tabs [locale]);
    return comp == NULL;
#elif defined USE_ICU
    UErrorCode err = U_ZERO_ERROR;
    UnicodeString uspat (pat, -1, conv, err);
    comp = RegexPattern::compile (uspat, options, err);
    if (U_FAILURE (err))
        cerr << "regexp_pattern::compile: " << u_errorName(err) << endl;
    return U_FAILURE (err);
#endif
}

regexp_pattern::~regexp_pattern()
{
    if (comp) {
#if defined USE_REGEX
        regfree (comp);
#elif defined USE_PCRE
        free (comp);
#elif defined USE_ICU
        //delete comp;
#endif
    }
#if defined USE_ICU
    if (conv)
        ucnv_close (conv);
#endif
#if defined USE_PCRE || defined USE_REGEX
    delete[] pat;
#else
    free (pat);
#endif
    free (pref);
}

bool regexp_pattern::match (const char *str)
{
    if (comp == NULL)
        return false;
    else {
#if defined USE_REGEX
        return regexec (comp, str, 0, NULL, 0) == 0;
#elif defined USE_PCRE
        return pcre_exec (comp, NULL, str, strlen (str), 0, 0, NULL, 0) >= 0;
#elif defined USE_ICU
        UErrorCode err = U_ZERO_ERROR;
        UnicodeString us (str, -1, conv, err);
        if (U_FAILURE (err)) {
            cerr << "Conversion error: " << u_errorName(err) << endl;
            return false;
        }
        RegexMatcher *rex = comp->matcher (us, err);
        if (U_FAILURE (err)) {
            cerr << "RegexPattern::matcher " << u_errorName(err) << endl;
            return false;
        }
        bool ret = rex->matches (err);
        if (U_FAILURE (err)) {
            cerr << "RegexMatcher::matches " << u_errorName(err) << endl;
            return false;
        }
        delete rex;
        return ret;
#endif
    }
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
