//  Copyright (c) 1999-2015  Pavel Rychly, Milos Jakubicek

#ifndef CONCGET_HH
#define CONCGET_HH

#include "concord.hh"


//--------------------  --------------------
typedef std::vector<PosAttr*> PAvec;
class OutStruct;
class OutRef;
typedef std::vector<OutRef*> ORvec;
typedef std::vector<OutStruct*> OSvec;
typedef std::vector<std::string> Tokens;

//-------------------- KWICLines --------------------
class KWICLines {
protected:
    Corpus *corp;
    RangeStream *rstream;
    Concordance::context *leftctx, *rightctx;
    PAvec kwicattrs, ctxattrs;
    OSvec structs;
    ORvec refs;
    bool inutf8;

    Position kwbeg, kwend, ctxbeg, ctxend;
    Tokens outrefs, outleft, outkwic, outright;
    ConcIndex lineidx;
    Labels lab;

    Position reduce_ctxbeg();
    Position reduce_ctxend();
public:
    KWICLines (Corpus *corp, RangeStream *r, const char *left,
               const char *right,  const char *kwica, const char *ctxa,
               const char *struca, const char *refa, int maxctx=0,
               bool ignore_nondef=true);
    ~KWICLines();
    bool nextcontext (); // setting {kw,ctx}{beg,end} only
    bool nextline ();  // nexcontext + setting out*
    bool skip (ConcIndex count); // skip count lines
    bool is_defined(); // is the current line defined?
    const Position get_pos() {return kwbeg;}
    const int get_kwiclen() {return kwend - kwbeg;}
    const Position get_ctxbeg() {return ctxbeg;}
    const Position get_ctxend() {return ctxend;}
    const Tokens &get_ref_list() {return outrefs;}
    std::string get_refs();
    const Tokens &get_left() {return outleft;}
    const Tokens &get_kwic() {return outkwic;}
    const Tokens &get_right() {return outright;}
    const int get_linegroup();
};

//-------------------- CorpRegion --------------------
class CorpRegion {
protected:
    Corpus *corp;
    PAvec attrs;
    OSvec structs;
    bool ignore_nondef;
    Tokens output;
public:
    CorpRegion (Corpus *corp, const char *attra, const char *struca,
                bool ignore_nondef=true);
    ~CorpRegion();
    const Tokens &region (Position frompos, Position topos,
                          char posdelim = ' ', char attrdelim = '/');
};

#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
