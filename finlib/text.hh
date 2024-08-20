// Copyright (c) 1999-2007  Pavel Rychly

#ifndef TEXT_HH
#define TEXT_HH

#include "binfile.hh"
#include "fstream.hh"

template <class TextFileClass>
class delta_text {
protected:
    int seg_size;
    NumOfPos text_size;

    TextFileClass wordseq;
    MapBinFile<uint32_t> segments;
public:
    class const_iterator {
	typedef typename TextFileClass::const_iterator iter;
	read_bits<iter> rb;
	NumOfPos rest;
    public:
	const_iterator (iter i, NumOfPos count, int skipbits=0)
	    : rb (i, skipbits), rest (count) {}
	int next() {return rest-- > 0 ? rb.delta() -1 : -1;}
    };
    delta_text (const std::string &filename, NumOfPos ts=0)
	:wordseq (filename + ".text"), segments (filename + ".text.seg") {
	const_iterator t (wordseq.at (16), 3);
	seg_size = t.next();
	text_size = t.next();
    }
    const_iterator at (Position pos) {
	if (pos < 0) pos = 0;
	if (pos > text_size) pos = text_size;
	NumOfPos rest = pos % seg_size;
	uint32_t seek = segments [pos / seg_size];
	const_iterator t (wordseq.at (seek/8), text_size - pos + rest, seek%8);
	while (rest--)
	    t.next();
	return t;
    }
    int pos2id (Position pos) {
	return at(pos).next();
    }
    NumOfPos size () {
	return text_size;
    }
};

template <class TextFileClass>
class big_delta_text : public delta_text<TextFileClass> {
protected:
    MapBinFile<int8_t> segments_bits;
public:
    typedef typename delta_text<TextFileClass>::const_iterator const_iterator;
    big_delta_text (const std::string &filename, NumOfPos ts=0)
	: delta_text<TextFileClass> (filename),
	segments_bits (filename + ".text.seg2") {}
    const_iterator at (Position pos) {
	if (pos < 0) pos = 0;
	if (pos > this->text_size) pos = this->text_size;
	NumOfPos rest = pos % this->seg_size;
	const_iterator t (this->wordseq.at(this->segments[pos/this->seg_size]),
			  this->text_size - pos + rest, 
			  segments_bits [pos / this->seg_size]);
	while (rest--)
	    t.next();
	return t;
    }
    int pos2id (Position pos) {
	return at(pos).next();
    }
};


typedef delta_text<MapBinFile<unsigned char> > map_delta_text;
typedef delta_text<BinCachedFile<unsigned char> > file_delta_text;
typedef big_delta_text<MapBinFile<unsigned char> > map_big_delta_text;
typedef big_delta_text<BinCachedFile<unsigned char> > file_big_delta_text;

template<class TextFileClass, class OffsetClass=MapBinFile<uint16_t>,
	 class SegmentClass=MapBinFile<uint32_t> >
class giga_delta_text {
protected:
    NumOfPos text_size;
    TextFileClass wordseq;
    OffsetClass offsets;
    SegmentClass segments;
public:
    class const_iterator {
	typedef typename TextFileClass::const_iterator iter;
	read_bits<iter> rb;
	NumOfPos rest;
    public:
	const_iterator (iter i, NumOfPos count, int skipbits=0)
	    : rb (i, skipbits), rest (count) {}
	int next() {return rest-- > 0 ? rb.delta() -1 : -1;}
    };
    giga_delta_text (const std::string &filename, NumOfPos ts=0)
	:wordseq (filename + ".text"), offsets (filename + ".text.off"),
	 segments (filename + ".text.seg") {
	read_bits<typename TextFileClass::const_iterator, 
	    unsigned char, NumOfPos> rb (wordseq.at (16));
	rb.delta(); // skip seg_size
	text_size = rb.delta() -1;
     }
    const_iterator at (Position pos) {
	if (pos < 0) pos = 0;
	if (pos > text_size) pos = text_size;
	NumOfPos rest = pos % 64;
	off_t  seek = offsets [pos / 64];
	int skipbits = seek % 8;
	seek = (seek / 8) + off_t (segments [pos / (64 * 16)]) * 2048;
	const_iterator t (wordseq.at (seek), text_size - pos + rest, skipbits);
	while (rest--)
	    t.next();
	return t;
    }
    int pos2id (Position pos) {
	return at(pos).next();
    }
    NumOfPos size () {
	return text_size;
    }
};

typedef giga_delta_text<BinCachedFile<uint8_t> > file_giga_delta_text;
typedef giga_delta_text<BinCachedFile<uint8_t>,
			BinFile<uint16_t>,
			BinFile<uint32_t> > file_file_giga_delta_text;
typedef giga_delta_text<MapBinFile<uint8_t>,
			MapBinFile<uint16_t>,
			MapBinFile<uint32_t> > map_map_giga_delta_text;


class cqp_text: public MapNetIntFile {
public:
    cqp_text (const std::string &filename, int =0) 
	: MapNetIntFile (filename + ".corpus") {}
    int pos2id (Position pos) {return (*this)[pos];}
};

class int_text {
protected:
    MapBinFile<int> textf;
    static const int head_size = 16 / sizeof(int);
public:
    class const_iterator {
	const int *p;
	const int *last;
    public:
	const_iterator (const int *init, const int *last) 
	    : p(init), last (last) {}
	int next() {return p < last ? *p++ : -1;}
    };
    int_text (const std::string &filename, int =0)
	: textf (filename + ".text") {
	textf.skip_header (head_size);
    }
    int pos2id (Position pos) {return textf [pos];}
    const_iterator at (Position pos) {
	return const_iterator (textf.at (pos), textf.at (textf.size()));
    }
    NumOfPos size () {return textf.size();}
};

#endif
