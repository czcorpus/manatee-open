//  Copyright (c) 2010-2013  Pavel Rychly, Milos Jakubicek
//  Multi Level Tokenization

#ifndef LEVEL_HH
#define LEVEL_HH

#include "fstream.hh"
#include <string>

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

TokenLevel *new_TokenLevel (const std::string &path);
void delete_TokenLevel (TokenLevel *level);
MLTStream *full_level (TokenLevel *level);
FastStream *tolevelfs (TokenLevel *level, FastStream *src);

#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
