//  Copyright (c) 2003-2022  Pavel Rychly, Milos Jakubicek
#pragma once
#include "frstream.hh"
#include "corpus.hh"

class FilterCond {
public:
    virtual bool eval (Labels &labs) = 0;
    virtual ~FilterCond () {};
};

class RQFilterCond: public RangeStream {
protected:
    RangeStream *src;
    FilterCond *fc;
    bool active;
    void locate();
public:
    RQFilterCond (RangeStream *rs, FilterCond *fc);
    ~RQFilterCond();
    virtual bool next ();
    virtual Position peek_beg () const;
    virtual Position peek_end () const;
    virtual void add_labels (Labels &lab) const;
    virtual Position find_beg (Position pos);
    virtual Position find_end (Position pos);
    virtual NumOfPos rest_min () const;
    virtual NumOfPos rest_max () const;
    virtual Position final () const;
    virtual int nesting() const;
    virtual bool epsilon() const;
};

class FilterCondAnd : public FilterCond {
    FilterCond *lop, *rop;
public:
    FilterCondAnd (FilterCond *l, FilterCond *r) : lop (l), rop (r) { };
    virtual bool eval (Labels &labs) {
        return lop->eval (labs) && rop->eval(labs);
    }
    virtual ~FilterCondAnd () { delete lop; delete rop; }
};

class FilterCondOr : public FilterCond {
    FilterCond *lop, *rop;
public:
    FilterCondOr (FilterCond *l, FilterCond *r) : lop (l), rop (r) { };
    virtual bool eval (Labels &labs) {
        return lop->eval (labs) || rop->eval(labs);
    }
    virtual ~FilterCondOr () { delete lop; delete rop; }
};

class FilterCondNot : public FilterCond {
    FilterCond *op;
public:
    FilterCondNot (FilterCond *o) : op (o) { };
    virtual bool eval (Labels &labs) {
        return not op->eval (labs);
    }
    virtual ~FilterCondNot () { delete op; }
};

class FilterCondFreq : public FilterCond {
public:
    typedef enum {F_EQ, F_LEQ, F_GEQ} Op;
private:
    PosAttr *attr;
    int label;
    int64_t limit;
    Op binop;
    bool neg;
public:
    FilterCondFreq (PosAttr *pa, int lab, uint64_t lim, Op op, bool ne)
        : attr (pa), label (lab), limit (lim), binop (op), neg (ne) { };
    virtual bool eval (Labels &labs) {
        int id = attr->pos2id (labs [label]);
        switch (binop) {
            case F_EQ: return (attr->freq(id) == limit) == !neg;
            case F_LEQ: return (attr->freq(id) <= limit) == !neg;
            case F_GEQ: return (attr->freq(id) >= limit) == !neg;
            default: return false;
        }
    }
    virtual ~FilterCondFreq () {}
};

class FilterCondVal : public FilterCond {
public:
    typedef enum {F_EQ, F_NEQ} Op;
private:
    PosAttr *attr1, *attr2;
    int arg1, arg2;
    Op op;
public:
    FilterCondVal (Corpus *c, PosAttr *pa1, PosAttr *pa2, int lab1, int lab2, Op binop)
        : attr1 (pa1), attr2 (pa2), arg1 (lab1), arg2 (lab2), op (binop)
    {
        auto ao = c->conf->find_attr(attr1->name);
        bool multival = c->conf->str2bool (ao["MULTIVALUE"]);
        if (multival)
            attr1->prepare_multivalue_map(ao["MULTISEP"].c_str());
        ao = c->conf->find_attr(attr2->name);
        multival = c->conf->str2bool (ao["MULTIVALUE"]);
        if (multival)
            attr2->prepare_multivalue_map(ao["MULTISEP"].c_str());
    }

    virtual bool eval (Labels &labs) {
        int id1 = attr1->pos2id (labs [arg1]);
        int id2 = attr2->pos2id (labs [arg2]);
        bool match = false;
        attr1->expand_multivalue_id(id1, [=,&match](int nid1){
            attr2->expand_multivalue_id(id2, [=,&match](int nid2) {
                if (attr1 == attr2) {
                   if (nid1 == nid2) match |= true;
                } else {
                    // XXX we need to copy result of pos2str() for the case when attr1 and attr2 are
                    // two dynamic attributes using the same dynamic function, and not having .ridx
                    // mapping file
                    string s1 (attr1->id2str (nid1));
                    string s2 (attr2->id2str (nid2));
                    if (s1 == s2) match |= true;
                }
            });
        });

        if (op == F_EQ) return match;
        else return not match;
    }
    virtual ~FilterCondVal () {}
};

RQFilterCond::RQFilterCond (RangeStream *rs, FilterCond *fc)
    : src (rs), fc (fc)
{
    active = src->peek_beg() < src->final();
    locate();
}

void RQFilterCond::locate()
{
    if (!active) return;
    Labels lab;
    do {
        lab.clear();
        src->add_labels (lab);
        if (fc->eval(lab)) break;
    } while ((active = src->next()));
}

bool RQFilterCond::next()
{
    active = src->next();
    locate();
    return active;
}

Position RQFilterCond::find_beg (Position pos)
{
    active = src->find_beg (pos) < src->final();
    locate();
    return src->peek_beg();
}

Position RQFilterCond::find_end (Position pos)
{
    active = src->find_end (pos) < src->final();
    locate();
    return src->peek_beg();
}

RQFilterCond::~RQFilterCond () {delete src; delete fc;}
Position RQFilterCond::peek_beg() const {return src->peek_beg();}
Position RQFilterCond::peek_end() const {return src->peek_end();}
void RQFilterCond::add_labels (Labels &lab) const {src->add_labels (lab);}
NumOfPos RQFilterCond::rest_min() const {return src->rest_min();}
NumOfPos RQFilterCond::rest_max() const {return src->rest_max();}
Position RQFilterCond::final() const {return src->final();}
int RQFilterCond::nesting() const {return src->nesting();}
bool RQFilterCond::epsilon() const {return src->epsilon();}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
