//  Copyright (c) 1999-2015  Pavel Rychly, Milos Jakubicek

#include "concord.hh"
#include "levels.hh"
#include <cstdlib>
#include <cstring>

using namespace std;

//-------------------- simple_context --------------------
class simple_context : public Concordance::context {
public:
    class context_fun {
    public:
        virtual Position operator() (Position pos, NumOfPos size) = 0;
        virtual ~context_fun() {}
    };
protected:
    bool begin;
    int collnum;
    context_fun *fun;
public:
    simple_context (bool b, context_fun *f, int coll=0, int ch=0) 
        : context(ch), begin(b), collnum(coll), fun(f) {}
    virtual ~simple_context () {delete fun;};
    virtual Position get (RangeStream *r);
};

Position simple_context::get (RangeStream *r)
{
    if (collnum > 0) {
        Labels lab;
        r->add_labels (lab);
        Position beg = lab[collnum];
        Position end = max(NumOfPos(0), lab[-collnum] -1);
        NumOfPos size = end - beg;
        return begin ? (*fun)(beg, size) : (*fun)(end, size);
    } else {
        Position beg = r->peek_beg();
        Position end = max(NumOfPos(0), r->peek_end() -1);
        NumOfPos size = end - beg;
        return begin ? (*fun)(beg, size) : (*fun)(end, size);
    }
}

//-------------------- ctx_* --------------------

class ctx_identity : public simple_context::context_fun {
public:
    ctx_identity () {}
    virtual Position operator() (Position pos, NumOfPos size) {return pos;}
};

class ctx_add_pos : public simple_context::context_fun {
    int delta;
public:
    ctx_add_pos (int delta) : delta (delta) {}
    virtual Position operator() (Position pos, NumOfPos size) {return pos + delta;}
};

class ctx_struct_beg : public simple_context::context_fun {
    ranges *rng;
    int delta;
public:
    ctx_struct_beg (ranges *rng, int delta) : rng (rng), delta (delta) {}
    virtual Position operator() (Position pos, NumOfPos size) {
        NumOfPos n = rng->num_at_pos (pos);
        if (n == -1)
            return pos - 15;
        Position p = rng->beg_at (min(max(NumOfPos(0), n + delta),
                                      rng->size()-1));
        if (p == pos && size == -1) {
            n = rng->num_at_pos (pos - 1);
            if (n == -1)
                return pos - 15;
            return rng->beg_at (min(max(NumOfPos(0), n + delta),
                                    rng->size()-1));
        }
        return p;
    }
};

class ctx_struct_end : public simple_context::context_fun {
    ranges *rng;
    int delta;
public:
    ctx_struct_end (ranges *rng, int delta) : rng (rng), delta (delta) {}
    virtual Position operator() (Position pos, NumOfPos size) {
        NumOfPos n = rng->num_at_pos (pos);
        if (n == -1)
            return pos + 15;
        Position p = rng->end_at (min(max(NumOfPos(0), n + delta),
                                      rng->size()-1)) -1;
        if (p == pos && size == -1) {
            n = rng->num_at_pos (pos + 1);
            if (n == -1)
                return pos + 15;
            return rng->end_at (min(max(NumOfPos(0), n + delta),
                                    rng->size()-1)) -1;
        }
        return p;
    }
};

class ctx_aligned : public simple_context::context_fun {
    Corpus *to_corp;
    Structure *to_align;
    MLTStream *map;
    bool begin;
public:
    ctx_aligned (const char *fc, Corpus *tc, bool begin)
        : to_corp (tc), map (NULL), begin (begin) {
        to_align = to_corp->get_struct(to_corp->get_conf("ALIGNSTRUCT"));
        Corpus *from_corp = to_corp->get_aligned (fc);
        if (! from_corp->get_conf("ALIGNDEF").empty())
            map = full_level (from_corp->get_aligned_level
                              (to_corp->get_conffile()));
    }
    ~ctx_aligned() {delete map;}
    virtual Position operator() (Position pos, NumOfPos size) {
        NumOfPos align_num, align_end;
        align_num = to_align->rng->num_next_pos (pos);
        if (map) {
            map->find_new(align_num);
            if (map->end())
                return to_corp->size();
            switch (map->change_type()) {
            case MLTStream::KEEP:
                align_end = align_num;
                break;
            default: // MLTStream::CHANGE
                align_num = map->newpos();
                align_end = align_num + map->change_newsize() -1;
            }
        } else {
            align_end = align_num;
        }
        if (begin)
            return to_align->rng->beg_at (align_num);
        else
            return to_align->rng->end_at (align_end) -1;
    }
};

//-------------------- max,min_context --------------------

class max_context: public Concordance::context 
{
    Concordance::context *ctx1;
    Concordance::context *ctx2;
public:
    max_context (Concordance::context *c1, Concordance::context *c2)
        : Concordance::context(0), ctx1 (c1), ctx2 (c2) {}
    virtual ~max_context () {};
    virtual Position get (RangeStream *r) {
        return max (ctx1->get (r), ctx2->get (r));
    }
};

class min_context: public Concordance::context 
{
    Concordance::context *ctx1;
    Concordance::context *ctx2;
public:
    min_context (Concordance::context *c1, Concordance::context *c2)
        : Concordance::context(0), ctx1 (c1), ctx2 (c2) {}
    virtual ~min_context () {delete ctx1; delete ctx2;}
    virtual Position get (RangeStream *r) {
        return min (ctx1->get (r), ctx2->get (r));
    }
};


//-------------------- prepare_context --------------------
Concordance::context* prepare_context (Corpus *c, const char *ctxstr,
                                       bool toleft, int maxctx)
{
    if (!maxctx)
        maxctx = c->get_maxctx();
    if (ctxstr[0] == 'a') {// aligned segment
        string align = c->get_conf("ALIGNSTRUCT");
        if (ctxstr[1] != ',' || strlen(ctxstr) <= 2 || align.empty())
            return new simple_context (toleft, new ctx_add_pos (0), 0);
        if (!strcmp(c->get_conffile(), &ctxstr[2])) { // =primary corpus
            ranges *rng = c->get_struct (align)->rng;
            if (toleft)
                return new simple_context (toleft,
                                           new ctx_struct_beg (rng, 0), 0);
            else
                return new simple_context (toleft,
                                           new ctx_struct_end (rng, 0), 0);
        }
        return new simple_context (toleft,
                                   new ctx_aligned (&ctxstr[2], c, toleft));
    }
    int num = atol (ctxstr);
    if (strchr (ctxstr, '#') != NULL) {
        if (num == 0)
            return new simple_context (toleft, new ctx_add_pos (0), 0);
        if (num < 0)
            num = - num;
        int npos = num/2 +1;
        if (maxctx && npos > maxctx) 
            npos = maxctx;
        return new simple_context (toleft, 
                                   new ctx_add_pos (toleft ? - npos : npos),
                                   0, num);
    } else {
        int coll = 0;
        bool fromdir = toleft;
        const char *p;
        if ((p = strchr (ctxstr, '<')) != NULL) {
            fromdir = true;
            if (*++p)
                coll = *p - '0';
        } else if ((p = strchr (ctxstr, '>')) != NULL) {
            fromdir = false;
            if (*++p)
                coll = *p - '0';
        }
        if ((p = strchr (ctxstr, ':')) != NULL) {
            // structure name
            char struct_name[80];
            char *s = struct_name;
            for (p++; isalpha (*p); p++, s++)
                *s = *p;
            *s = '\0';
            try {
                if (num == 0)
                    return new simple_context (fromdir, 
                                               new ctx_add_pos (0), coll);
                bool numgt0 = num > 0;
                if (num < 0) num++;
                if (num > 0) num--;
                ranges *rng = c->get_struct (struct_name)->rng;
                Concordance::context *c;
                if (toleft)
                    c = new simple_context (fromdir, 
                                            new ctx_struct_beg (rng, num), 
                                            coll);
                else
                    c = new simple_context (fromdir, 
                                            new ctx_struct_end (rng, num),
                                            coll);
                if (!maxctx)
                    return c;
                else {
                    if (numgt0) {
                        return new min_context (c, new simple_context
                                                (fromdir, 
                                                 new ctx_add_pos (maxctx),
                                                 coll));
                    } else {
                        return new max_context (c, new simple_context
                                                (fromdir, 
                                                 new ctx_add_pos (-maxctx),
                                                 coll));
                    }

                }
                
            } catch (AttrNotFound &e) {
                return new simple_context (toleft, 
                                           new ctx_add_pos(toleft ? -3 : 3), 
                                           coll);
            }
        } else {
            if (maxctx) {
                if (num > maxctx) {
                    num = maxctx;
                    if (toleft) num++;
                } else if (num <= - maxctx) {
                    num = - maxctx -1;
                    if (toleft) num++;
                }
            }
            return new simple_context (fromdir, new ctx_add_pos (num), coll);
        }
    }
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
