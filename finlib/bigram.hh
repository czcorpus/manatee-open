// Copyright (c) 1999-2003  Pavel Rychly

#ifndef BIGRAM_HH
#define BIGRAM_HH

#include "binfile.hh"
#include "fromtof.hh"
#include <utility>   // class pair<T1,T2>

typedef std::pair<int,int> bgrpair;

class map_int_bigrams
{
protected:
    MapBinFile<bgrpair> cntf;
    MapBinFile<int> idxf;
public:
    map_int_bigrams (const std::string &filename)
	: cntf (filename + ".cnt"), idxf (filename + ".idx")
	{}
    int maxid ()
	{return idxf.size() -2;}
    int count (int id) 
	{return idxf [id+1] - idxf [id];}
    bgrpair* ids_from (int id)
	{return cntf.at (idxf [id]);}
    bgrpair* ids_to (int id)
	{return cntf.at (idxf [id +1]);}
    int value (int id1, int id2) {
	if (id1 < 0 || id1 >= idxf.size())
	    return 0;
	bgrpair* p;
	bgrpair* q;
	for (p = ids_from (id1),q = ids_to (id1); p < q; p++)
	    if (p->first == id2)
		return p->second;
	return 0;
    }
};
   

class map_int_sort_bigrams: public map_int_bigrams
{
public:
    map_int_sort_bigrams (const std::string &filename):
	map_int_bigrams (filename) {}
    int value (int id1, int id2) {
	if (id1 < 0 || id1 >= idxf.size())
	    return 0;
	bgrpair* p = ids_from (id1);
	bgrpair* q = ids_to (id1);
	while (p < q) {
	    bgrpair* m = p + (q-p)/2;
	    if (m->first == id2)
		return m->second;
	    if (m->first < id2)
		p = m +1;
	    else
		q = m;
	}
	return 0;
    }
};


// -------------------- BigramStream --------------------

struct BigramItem {
    int first, second, value;
};

class BigramStream {
    FromFile<bgrpair> cntf;
    FromFile<int> idxf;
    BigramItem curr;
    int cnt_position, next_id_start;
    bool have_data;
public:
    BigramStream (const std::string &filename, int firstid = 0)
	: cntf (filename + ".cnt"), 
	  idxf (filename + ".idx")
    {
	seek (firstid);
    }
    const BigramItem& operator* () const {return curr;}
    BigramStream& operator++ () {
	if (!(have_data = !++cntf))
	    return *this;
	cnt_position++;
	while (cnt_position == next_id_start && !idxf) {
	    next_id_start = *(++idxf);
	    curr.first++;
	}
	curr.second = (*cntf).first;
	curr.value = (*cntf).second;
	return *this;
    }
    void seek (int id) {
	idxf.seek (id);
	if (!(have_data = !idxf))
	    return;
	cnt_position = *idxf;
	next_id_start = *(++idxf);
	while (cnt_position == next_id_start && !idxf) {
	    next_id_start = *(++idxf);
	    id++;
	}
	curr.first = id;
	cntf.seek (cnt_position);
	curr.second = (*cntf).first;
	curr.value = (*cntf).second;
	have_data = !cntf;
    }
    bool operator !() {return have_data;}
};

#endif
