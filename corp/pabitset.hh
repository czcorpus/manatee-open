#ifndef PABITSETATTR_HH
#define PABITSETATTR_HH

#include "posattr.hh"

/**
 * Positional attribute holding a set of values (e.g. key=value pairs) at
 * every position. Files: <path>.lex* (one lexicon entry per value),
 * <path>.ltext* (sorted value ids per position, see list_text) and
 * <path>.rev* (positions of each value).
 *
 * pos2str()/textat() join the values with sep, sorted case-insensitively
 * (so UD FEATS come out in their usual order). pos2id()/posat() return a
 * single representative id per position (the lowest value id, or the id of
 * "" for an empty set, or -1 if "" is not in the lexicon); use posset() to
 * get the whole set.
 */
PosAttr *createBitsetPosAttr (const std::string &path, const std::string &n,
                              const std::string &locale,
                              const std::string &enc,
                              const std::string &sep = "|",
                              NumOfPos text_size = 0);
#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
