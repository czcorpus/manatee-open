//  Copyright (c) 2011-2016  Pavel Rychly, Milos Jakubicek

#include <stdint.h>
#include <string.h>
#include <cstdlib>
#include <new>

inline bool is_utf8char_begin (const unsigned char c)
{
    return (c & 0xc0) != 0x80;
}

bool utf8_with_supp_plane (const char *bytes)
{
    for (; *bytes; bytes++)
        if ((*bytes & 0xf0) >= 0xf0)
            return true;
    return false;
}

unsigned int utf8len (const char *bytes)
{
    unsigned int len = 0;
    for (; *bytes; bytes++)
        if (is_utf8char_begin (*bytes))
            len++;
    return len;
}

/* count characters starting at or before a given byte position */
uint64_t utf8pos (const char *bytes, uint64_t idx)
{
    uint64_t pos = 0;
    for (uint64_t i = 0; bytes[i] && i <= idx; i++)
        if (is_utf8char_begin (bytes[i]))
            pos++;
    return pos;  
}

/* returns n-th char in bytes */
int64_t utf8char (const char *bytes, int n)
{
    int64_t ret = 0;
    if (n < 0)
        return ret;
    const char *beg = bytes;
    for (n++; *bytes; bytes++) {
        if (is_utf8char_begin(*bytes)) {
            if (n--)
                beg = bytes;
            else
                break;
        }
    }
    if (n == -1 || (n == 0 && !*bytes))
        memcpy(&ret, beg, bytes - beg);
    return ret;
}

/* returns pointer to the last n chars */
const char * utf8suffix (const char *bytes, int n)
{
    size_t len = strlen(bytes);
    while (len && n) {
        len--;
        if (is_utf8char_begin(bytes[len]))
            n--;
    }
    return bytes + len;
}

unsigned int utf82uni (const unsigned char *&bytes)
{
    if (*bytes < 0x80)    /* 1000 0000 */
        return *bytes++;
    /* clear leading 1s */
    int uni = *bytes ^ 0xc0;  /* 1100 0000 */
    int bit = 0x20;           /* 0010 0000 */
    while (uni & bit) {
        uni ^= bit;
        bit >>= 1;
    }
    /* copy data bits */
    for (bytes++; *bytes && !is_utf8char_begin(*bytes); bytes++)
        uni = (uni << 6) | (*bytes ^ 0x80);
    return uni;
}

void uni2utf8 (unsigned int uni, unsigned char *&bytes)
{
    if (uni < 0x80) {       /* 1000 0000 */
        *bytes = uni;
    } else if (uni < 0x0800) {
        *bytes = 0xc0 | (uni >> 6);            /* 1100 0000 */
        *++bytes = 0x80 | (uni & 0x3f);        /* 0011 1111 */
    } else if (uni < 0x010000) {
        *bytes = 0xe0 | (uni >> 12);           /* 1110 0000 */
        *++bytes = 0x80 | ((uni >> 6) & 0x3f); /* 0011 1111 */
        *++bytes = 0x80 | (uni & 0x3f);        /* 0011 1111 */
    } else {
        *bytes = 0xf0 | (uni >> 18);            /* 1111 0000 */
        *++bytes = 0x80 | ((uni >> 12) & 0x3f); /* 0011 1111 */
        *++bytes = 0x80 | ((uni >> 6) & 0x3f);  /* 0011 1111 */
        *++bytes = 0x80 | (uni & 0x3f);         /* 0011 1111 */
    }
    ++bytes;
}

/* generated by: ./gentolowertable.py 100000  */
typedef struct {uint16_t first, tolower, blocklen;} uni_to_lower_t;
static uni_to_lower_t uni_to_lower[] = {
  {0x0041, 0x0061, 26},   {0x00c0, 0x00e0, 23},   {0x00d8, 0x00f8,  7},
  {0x0100, 0x0101, 48},   {0x0130, 0x0069,  1},   {0x0132, 0x0133,  6},
  {0x0139, 0x013a, 16},   {0x014a, 0x014b, 46},   {0x0178, 0x00ff,  1},
  {0x0179, 0x017a,  6},   {0x0181, 0x0253,  1},   {0x0182, 0x0183,  4},
  {0x0186, 0x0254,  1},   {0x0187, 0x0188,  2},   {0x0189, 0x0256,  2},
  {0x018b, 0x018c,  2},   {0x018e, 0x01dd,  1},   {0x018f, 0x0259,  1},
  {0x0190, 0x025b,  1},   {0x0191, 0x0192,  2},   {0x0193, 0x0260,  1},
  {0x0194, 0x0263,  1},   {0x0196, 0x0269,  1},   {0x0197, 0x0268,  1},
  {0x0198, 0x0199,  2},   {0x019c, 0x026f,  1},   {0x019d, 0x0272,  1},
  {0x019f, 0x0275,  1},   {0x01a0, 0x01a1,  6},   {0x01a6, 0x0280,  1},
  {0x01a7, 0x01a8,  2},   {0x01a9, 0x0283,  1},   {0x01ac, 0x01ad,  2},
  {0x01ae, 0x0288,  1},   {0x01af, 0x01b0,  2},   {0x01b1, 0x028a,  2},
  {0x01b3, 0x01b4,  4},   {0x01b7, 0x0292,  1},   {0x01b8, 0x01b9,  2},
  {0x01bc, 0x01bd,  2},   {0x01c4, 0x01c6,  1},   {0x01c5, 0x01c6,  2},
  {0x01c7, 0x01c9,  1},   {0x01c8, 0x01c9,  2},   {0x01ca, 0x01cc,  1},
  {0x01cb, 0x01cc, 18},   {0x01de, 0x01df, 18},   {0x01f1, 0x01f3,  1},
  {0x01f2, 0x01f3,  4},   {0x01f6, 0x0195,  1},   {0x01f7, 0x01bf,  1},
  {0x01f8, 0x01f9, 40},   {0x0220, 0x019e,  1},   {0x0222, 0x0223, 18},
  {0x023a, 0x2c65,  1},   {0x023b, 0x023c,  2},   {0x023d, 0x019a,  1},
  {0x023e, 0x2c66,  1},   {0x0241, 0x0242,  2},   {0x0243, 0x0180,  1},
  {0x0244, 0x0289,  1},   {0x0245, 0x028c,  1},   {0x0246, 0x0247, 10},
  {0x0370, 0x0371,  4},   {0x0376, 0x0377,  2},   {0x0386, 0x03ac,  1},
  {0x0388, 0x03ad,  3},   {0x038c, 0x03cc,  1},   {0x038e, 0x03cd,  2},
  {0x0391, 0x03b1, 17},   {0x03a3, 0x03c3,  9},   {0x03cf, 0x03d7,  1},
  {0x03d8, 0x03d9, 24},   {0x03f4, 0x03b8,  1},   {0x03f7, 0x03f8,  2},
  {0x03f9, 0x03f2,  1},   {0x03fa, 0x03fb,  2},   {0x03fd, 0x037b,  3},
  {0x0400, 0x0450, 16},   {0x0410, 0x0430, 32},   {0x0460, 0x0461, 34},
  {0x048a, 0x048b, 54},   {0x04c0, 0x04cf,  1},   {0x04c1, 0x04c2, 14},
  {0x04d0, 0x04d1, 84},   {0x0531, 0x0561, 38},   {0x10a0, 0x2d00, 38},
  {0x1e00, 0x1e01, 150},   {0x1e9e, 0x00df,  1},   {0x1ea0, 0x1ea1, 96},
  {0x1f08, 0x1f00,  8},   {0x1f18, 0x1f10,  6},   {0x1f28, 0x1f20,  8},
  {0x1f38, 0x1f30,  8},   {0x1f48, 0x1f40,  6},   {0x1f59, 0x1f51,  1},
  {0x1f5b, 0x1f53,  1},   {0x1f5d, 0x1f55,  1},   {0x1f5f, 0x1f57,  1},
  {0x1f68, 0x1f60,  8},   {0x1f88, 0x1f80,  8},   {0x1f98, 0x1f90,  8},
  {0x1fa8, 0x1fa0,  8},   {0x1fb8, 0x1fb0,  2},   {0x1fba, 0x1f70,  2},
  {0x1fbc, 0x1fb3,  1},   {0x1fc8, 0x1f72,  4},   {0x1fcc, 0x1fc3,  1},
  {0x1fd8, 0x1fd0,  2},   {0x1fda, 0x1f76,  2},   {0x1fe8, 0x1fe0,  2},
  {0x1fea, 0x1f7a,  2},   {0x1fec, 0x1fe5,  1},   {0x1ff8, 0x1f78,  2},
  {0x1ffa, 0x1f7c,  2},   {0x1ffc, 0x1ff3,  1},   {0x2126, 0x03c9,  1},
  {0x212a, 0x006b,  1},   {0x212b, 0x00e5,  1},   {0x2132, 0x214e,  1},
  {0x2160, 0x2170, 16},   {0x2183, 0x2184,  2},   {0x24b6, 0x24d0, 26},
  {0x2c00, 0x2c30, 47},   {0x2c60, 0x2c61,  2},   {0x2c62, 0x026b,  1},
  {0x2c63, 0x1d7d,  1},   {0x2c64, 0x027d,  1},   {0x2c67, 0x2c68,  6},
  {0x2c6d, 0x0251,  1},   {0x2c6e, 0x0271,  1},   {0x2c6f, 0x0250,  1},
  {0x2c72, 0x2c73,  2},   {0x2c75, 0x2c76,  2},   {0x2c80, 0x2c81, 100},
  {0xa640, 0xa641, 32},   {0xa662, 0xa663, 12},   {0xa680, 0xa681, 24},
  {0xa722, 0xa723, 14},   {0xa732, 0xa733, 62},   {0xa779, 0xa77a,  4},
  {0xa77d, 0x1d79,  1},   {0xa77e, 0xa77f, 10},   {0xa78b, 0xa78c,  2},
  {0xff21, 0xff41, 26}, };
/*  {0x10400, 0x10428, 40}, 
# Deseret alphabet not supported
# The Mormon Church published four books using the Deseret alphabet,
# the alphabet was not actively promoted after 1869.
# http://www.omniglot.com/writing/deseret.htm
 */

const uni_to_lower_t *utl_end = uni_to_lower +
    sizeof (uni_to_lower) / sizeof (uni_to_lower_t);

static uint8_t utl_index[] = {
    0, 3, 51, 63, 78, 84, 86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 86, 87, 87, 
    87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 87, 90, 116, 116, 122, 
};

const int utl_index_len = sizeof (utl_index);

unsigned int uni_tolower (unsigned int ucharn) {
    uni_to_lower_t *b; 
    int i = ucharn / 256;
    if (i >= utl_index_len)
        b = uni_to_lower + utl_index[utl_index_len -1];
    else
        b = uni_to_lower + utl_index[i];
    while (b < utl_end && b->first + b->blocklen <= ucharn)
        b++;
    if (b >= utl_end)
        return ucharn;

    if (ucharn < b->first || ucharn >= b->first + b->blocklen)
        return ucharn;
    int delta = b->tolower - b->first;
    if (delta == 1) {
        if ((ucharn - b->first) % 2 == 1)
            return ucharn;
        return ucharn +1;
    } else
        return ucharn + delta;
}

unsigned int uni_toupper (unsigned int ucharn) {
    uni_to_lower_t *b;
    int i = ucharn / 256;
    if (i >= utl_index_len)
        b = uni_to_lower + utl_index[utl_index_len -1];
    else
        b = uni_to_lower + utl_index[i];
    while (b < utl_end && b->tolower + b->blocklen <= ucharn)
        b++;
    if (b >= utl_end)
        return ucharn;

    if (ucharn < b->tolower || ucharn >= b->tolower + b->blocklen)
        return ucharn;
    int delta = b->tolower - b->first;
    if (delta == 1) {
        if ((ucharn - b->tolower) % 2 == 1)
            return ucharn;
        return ucharn -1;
    } else
        return ucharn - delta;
}

inline const unsigned char * utf8_case (const unsigned char *src, bool tolower)
{
    static unsigned char *result = NULL;
    static size_t ressize = 0;
    size_t len = strlen ((const char *) src);
    if (ressize <= 2 * len) {
        ressize = 2 * len + 1;
        result = (unsigned char *) realloc (result, ressize);
        if (!result)
            throw std::bad_alloc();
    }
    unsigned char *r = result;
    while (*src) {
        if (tolower)
            uni2utf8 (uni_tolower(utf82uni (src)), r);
        else
            uni2utf8 (uni_toupper(utf82uni (src)), r);
    }
    *r = '\0';
    return result;
}

const unsigned char * utf8_tolower (const unsigned char *src) {
    return utf8_case (src, true);
}

const unsigned char * utf8_toupper (const unsigned char *src) {
    return utf8_case (src, false);
}

/* returns bytes with first char capitalized */
const unsigned char * utf8_capital (const unsigned char *bytes)
{
    static unsigned char *result = NULL;
    static size_t ressize = 0;
    size_t len = strlen ((const char *) bytes);
    if (ressize <= 2 * len) {
        ressize = 2 * len + 1;
        result = (unsigned char *) realloc (result, ressize);
        if (!result)
            throw std::bad_alloc();
    }
    if (!*bytes) return (unsigned char *) "";
    unsigned char *r = result;
    uni2utf8 (uni_toupper(utf82uni (bytes)), r);
    strcpy ((char *) r, (const char *) bytes);
    return result;
}

const unsigned char trailing_bytes_for_UTF8(unsigned char c)
{
    if (c < 192) return 0;
    if (c < 224) return 1;
    if (c < 240) return 2;
    if (c < 248) return 3;
    if (c < 252) return 4;
    return 5;
}

// based on public domain https://github.com/JeffBezanson/cutef8/blob/master/utf8.c
const int utf8valid(const unsigned char *bytes)
{
    size_t length = strlen ((const char*) bytes);
    const unsigned char *p, *pend = (unsigned char*)bytes + length;
    unsigned char c;
    int ret = 1; /* ASCII */
    size_t ab;

    for (p = (unsigned char*)bytes; p < pend; p++) {
        c = *p;
        if (c < 128)
            continue;
        ret = 2; /* non-ASCII UTF-8 */
        if ((c & 0xc0) != 0xc0)
            return 0;
        ab = trailing_bytes_for_UTF8(c);
        if (length < ab)
            return 0;
        length -= ab;

        p++;
        /* Check top bits in the second byte */
        if ((*p & 0xc0) != 0x80)
            return 0;

        /* Check for overlong sequences for each different length */
        switch (ab) {
            /* Check for xx00 000x */
        case 1:
            if ((c & 0x3e) == 0) return 0;
            continue;   /* We know there aren't any more bytes to check */

            /* Check for 1110 0000, xx0x xxxx */
        case 2:
            if (c == 0xe0 && (*p & 0x20) == 0) return 0;
            break;

            /* Check for 1111 0000, xx00 xxxx */
        case 3:
            if (c == 0xf0 && (*p & 0x30) == 0) return 0;
            break;

            /* Check for 1111 1000, xx00 0xxx */
        case 4:
            if (c == 0xf8 && (*p & 0x38) == 0) return 0;
            break;

            /* Check for leading 0xfe or 0xff,
               and then for 1111 1100, xx00 00xx */
        case 5:
            if (c == 0xfe || c == 0xff ||
                (c == 0xfc && (*p & 0x3c) == 0)) return 0;
            break;
        }

        /* Check for valid bytes after the 2nd, if any; all must start 10 */
        while (--ab > 0) {
            if ((*(++p) & 0xc0) != 0x80) return 0;
        }
    }

    return ret;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
