//  Copyright (c) 2011-2016  Pavel Rychly, Milos Jakubicek

#ifndef UTF8_HH
#define UTF8_HH

#include <stdint.h>

unsigned int utf8len (const char *bytes);
uint64_t utf8pos (const char *bytes, uint64_t idx);
bool utf8_with_supp_plane (const char *bytes);
int64_t utf8char (const char *bytes, int n);
const unsigned char * utf8_tolower (const unsigned char *src);
const unsigned char * utf8_toupper (const unsigned char *src);
const char * utf8suffix (const char *bytes, int n);
const unsigned char * utf8_capital (const unsigned char *bytes);
const int utf8valid(const unsigned char *bytes);

#endif
