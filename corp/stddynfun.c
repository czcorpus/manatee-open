// Copyright (c) 1999-2014  Pavel Rychly, Milos Jakubicek

const char * getnchar (const char* x, int n)
{
    static char ch[2] = " ";
    if ((int)strlen (x) < n)
        ch[0] = '\0';
    else
        ch[0] = x[n-1];
    return ch;
}

const char * getnextchar (const char* x, const char attr)
{
    static char ch[3] = "  ";
    ch[0] = '\0';
    while (*x) {
        if (*x++ == attr) {
            ch[0] = attr;
            ch[1] = *x;
            break;
        }
    }
    return ch;
}

const char * getnextchars (const char* x, const char attr, int len)
{
    static char ch[11] = " ";
    char *s;
    if (len > 10)
        len = 10;
    while (*x) {
        if (*x++ == attr) {
            ch[0] = *x;
            break;
        }
    }
    for (s=ch; *x && len; len--)
        *s++ = *x++;
    *s = 0;
    return ch;
}

const char * getfirstn (const char* x, int len)
{
    static int ressize = 32;
    static char *result = (char *) malloc(ressize);
    if (len < 0)
        len = 0;
    if (ressize <= len) {
        ressize = len + 1;
        result = (char *) realloc (result, ressize);
    }
    strncpy (result, x, len);
    result[len] = '\0';
    return result;
}

const char * getnbysep (const char* x, const char sep, int n)
{
    const char *p;
    while ((p = strchr (x, sep))) {
        if (--n)
            x = p + 1;
        else
            break;
    }
    if (!p) return (n == 1) ? x : "";
    return getfirstn (x, p - x);
}

const char * getfirstbysep (const char* x, const char sep)
{
    const char *p = strchr (x, sep);
    if (!p) return x;
    return getfirstn (x, p - x);
}

const char * getlastn (const char* x, int len)
{
    int x_len = strlen(x);
    if (x_len <= len)
        return x;
    static int ressize = 32;
    static char *result = (char *) malloc(ressize);
    if (len < 0)
        len = 0;
    if (ressize <= len) {
        ressize = len + 1;
        result = (char *) realloc (result, ressize);
    }
    strcpy (result, x + x_len - len);
    result[len] = '\0';
    return result;
}

const char * utf8getlastn (const char* x, int len)
{
    if (!utf8valid((const unsigned char*) x)) return "\xef\xbf\xbd";
    return utf8suffix (x, len);
}

const char * striplastn (const char* x, int cnt)
{
    return getfirstn (x, strlen (x) - cnt);
}

const char * lowercase (const char* x, const char *locale)
{
    static char *result = NULL;
    static size_t ressize = 0;
    size_t len = strlen (x);
    if (ressize <= len) {
        ressize = len + 1;
        result = (char *) realloc (result, ressize);
    }
    char *r;
    const char *prev_locale = setlocale (LC_CTYPE, locale);
    for (r = result; *x; x++, r++)
        *r = tolower (*x);
    setlocale (LC_CTYPE, prev_locale);
    *r = '\0';
    return result;
}

#define TLD(tld) tld,
static const std::unordered_set<std::string> tlds = {
    #include "tlds.h"
};
#undef TLD

const char * url3domain (const char* url, int level)
{
    static char *result = NULL;
    static size_t ressize = 0;
    const char *firstslash = strchr (url, '/');
    if (firstslash && firstslash != url &&
        *(firstslash - 1) == ':' && *(firstslash + 1) == '/')
        url = firstslash + 2; // skip protocol name
    if (!strncmp(url, "www.", 4))
        url += 4;
    const char *end = url;
    while (*end && *end != '/')
        end++;
    while (end != url && (isdigit (*(end - 1)) || *(end - 1) == ':'))
        end--; // skip port number
    const char *beg = (level ? end : url);
    if(level) {
        level++;
        const char* parts[level];
        while (level--) {
            if (beg != url)
                beg--;
            while (beg != url && *beg != '.')
                beg--;
            parts[(sizeof parts / sizeof *parts) - level - 1] = beg == url ? beg : beg + 1;
        }

        if((sizeof parts / sizeof *parts) >= 2) {
            std::string s(parts[1], end - parts[1]);
            if (tlds.count(s)) {
                beg = parts[(sizeof parts / sizeof *parts) - 1];
            } else {
                beg = parts[(sizeof parts / sizeof *parts) - 2];
            }
        }
    }

    size_t len = end - beg;
    if (ressize <= len) {
        ressize = len + 1;
        result = (char *) realloc (result, ressize);
    }
    strncpy (result, beg, len);
    result[len] = '\0';
    return result;
}

const char * url2domain (const char* url, int level)
{
    static char *result = NULL;
    static size_t ressize = 0;
    const char *firstslash = strchr (url, '/');
    if (firstslash && firstslash != url &&
        *(firstslash - 1) == ':' && *(firstslash + 1) == '/')
        url = firstslash + 2; // skip protocol name
    if (!strncmp(url, "www.", 4))
        url += 4;
    const char *end = url;
    while (*end && *end != '/')
        end++;
    while (end != url && (isdigit (*(end - 1)) || *(end - 1) == ':'))
        end--; // skip port number
    const char *beg = (level ? end : url);
    while (level--) {
        if (beg != url)
            beg--;
        while (beg != url && *beg != '.')
            beg--;
    }
    if (beg != url)
        beg++;
    size_t len = end - beg;
    if (ressize <= len) {
        ressize = len + 1;
        result = (char *) realloc (result, ressize);
    }
    strncpy (result, beg, len);
    result[len] = '\0';
    return result;
}


const char * ascii (const char *x, const char *encoding, const char *locale)
{
    const char *prev_locale = setlocale (LC_CTYPE, locale);
    static iconv_t iconv_h = (iconv_t) -1;
    static char enc[32] = "";
    if (iconv_h == (iconv_t) -1 || strcmp (enc, encoding))
        iconv_h = iconv_open ("ASCII//TRANSLIT", encoding);
    static size_t ressize = 32;
    static char *result = (char *) malloc(ressize);
    size_t len = strlen (x);
    if (ressize <= len) {
        ressize = len + 1;
        result = (char *) realloc (result, ressize);
    }
    char *in = (char*) x, *out = result;
    size_t in_len = len, out_len = ressize;
    iconv (iconv_h, &in, &in_len, &out, &out_len);
    result [ressize - out_len] = '\0';
    setlocale (LC_CTYPE, prev_locale);
    return result;
}

const char * utf8lowercase (const char *src)
{
    if (!utf8valid((const unsigned char*) src)) return "\xef\xbf\xbd";
    return (const char*) utf8_tolower ((const unsigned char*) src);
}

const char * utf8uppercase (const char *src)
{
    if (!utf8valid((const unsigned char*) src)) return "\xef\xbf\xbd";
    return (const char*) utf8_toupper ((const unsigned char*) src);
}

const char * utf8capital (const char *src)
{
    if (!utf8valid((const unsigned char*) src)) return "\xef\xbf\xbd";
    return (const char*) utf8_capital ((const unsigned char*) src);
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
