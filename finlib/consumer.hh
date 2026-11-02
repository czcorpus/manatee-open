// Copyright (c) 2003-2017  Pavel Rychly, Milos Jakubicek

#ifndef CONSUMER_HH
#define CONSUMER_HH

#include "fstream.hh"
#include "revidx.hh"
#include "bitio.hh"
#include <string>
#include <vector>

class TextConsumer {
protected:
    TextConsumer() {}
public:
    virtual ~TextConsumer() noexcept (false) {}
    void operator() (int id) {this->put (id);}
    virtual void put (int id) =0;
    virtual Position curr_size() =0;
    typedef enum {tdelta, tdeltabig, tint, tdeltagiga} TextType;
    virtual TextType get_type() {return type;}
    static TextConsumer* create (TextType tt, const std::string &path, 
                                 bool append=false);
private:
    TextType type;
};


class OutFileBits_tell;
template <class ValueType> class ToFile;

// writes a list_text (text.hh): one set of ids per position
class ListTextConsumer {
    typedef write_bits<OutFileBits_tell&,unsigned char,unsigned char*> Writer;
    std::string path;
    int seg_size;
    Position text_size;
    FILE *textf;
    OutFileBits_tell *bitfile;
    Writer *wb;
    ToFile<uint64_t> *seg;
    std::vector<int> buf;
    static const char signature[7];
public:
    ListTextConsumer (const std::string &path, int seg_size=64);
    ~ListTextConsumer() noexcept(false);
    // ids need not be sorted; duplicates are dropped, negative ids rejected
    void put (const std::vector<int> &ids);
    Position curr_size() {return text_size;}
};


class RevFileConsumer {
protected:
    RevFileConsumer() {}
public:
    virtual ~RevFileConsumer() {}
    void operator() (int id, Position pos) {this->put (id, pos);}
    virtual void put (int id, Position pos) =0;

    static RevFileConsumer* create (const std::string &path, 
                                    int buff_size=1048576, int align=1,
                                    bool append=false, bool part_sort=false);
};


class CollRevFileConsumer {
protected:
    CollRevFileConsumer() {}
public:
    virtual ~CollRevFileConsumer() {}
    void operator() (int id, Position pos, Position coll) {this->put (id, pos, coll);}
    virtual void put (int id, Position pos, Position coll) =0;

    static CollRevFileConsumer* create (const std::string &path,
                                        int buff_size=1048576, int align=1,
                                        bool append=false, bool part_sort=false);
};

template <class RevClass>
void finish_rev_files (const std::string &outname, int num_of_files, 
                       int align, bool part_sort=false);

typedef delta_revidx<MapBinFile<uint8_t>,
                     MapBinFile<uint32_t> > mfile_mfile_delta_revidx;
typedef delta_revidx<MapBinFile<uint64_t>, // MUST NOT USE BinCachedFile because of Fast2Mem without add_labels
                     MapBinFile<uint32_t>,
                     CollDeltaPosStream> mfile_mfile_delta_collrevidx;

#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
