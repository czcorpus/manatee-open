//  Copyright (c) 1999-2015  Pavel Rychly, Milos Jakubicek

#include "concget.hh"
#include "conccrit.hh"
#include "utf8.hh"
#include <algorithm>
#include <list>
#include <sstream>
#include <cstdlib>
#include <cstring>

using namespace std;


//-------------------- pos_event --------------------
typedef enum {ev_BEG, ev_END, ev_BKW, ev_EKW, ev_STRUCT, 
              ev_BTAG, ev_ETAG, ev_TEXT} event_t;

class pos_event {
public:
    Position pos;
    int sub;   // priority
    event_t type;
    std::string value;
    pos_event (event_t t, Position pos, int sub, const std::string &val="") 
        :pos (pos), sub(sub), type(t), value (val) {}
    pos_event (Position pos, int sub, const std::string &val)
        :pos (pos), sub (sub), type (ev_STRUCT), value (val) {}
};

//-------------------- OutStruct --------------------
class OutStruct {
protected:
    Structure *str;
    Corpus::VSA attrs;
    bool display_tag, display_empty;
    string display_class, display_begin, display_end;
    list< pair< pair<int, int>, PosAttr*> > display_begin_attrs, display_end_attrs;
public:
    OutStruct (Structure *s) 
        : str(s), display_tag (str->get_conf("DISPLAYTAG") != "0"),
          display_empty (false),
          display_class (str->get_conf("DISPLAYCLASS")), 
          display_begin (str->get_conf("DISPLAYBEGIN")), 
          display_end (str->get_conf("DISPLAYEND"))
    { 
        if (display_begin == "_EMPTY_") {
            display_begin = "";
            display_empty = true;
        } else {
            parse_attr_values(&display_begin, &display_begin_attrs);
        }
        parse_attr_values(&display_end, &display_end_attrs);
    }
    /**
     * Search display_text for "%(attribute)"-style patterns and store the start/end of the pattern and attribute in attrs
     */
    void parse_attr_values(string *display_text, list< pair< pair<int, int>, PosAttr*> > *attrs) {
        int attr_start = -1;
        for (size_t i = 0; i < display_text->length(); i++) {
            if (display_text->at(i) == '%' && display_text->at(i + 1) == '(') {
                attr_start = i;
            }
            if (display_text->at(i) == ')' && attr_start != -1) {
                attrs->push_back(make_pair(make_pair(attr_start, i), str->get_attr(display_text->substr(attr_start + 2, i - attr_start - 2).c_str())));
                attr_start = -1;
            }
        }
    }
    /**
     * Substitute all attribute patterns in attrs with the value of the attribute at the position pos
     */
    const string substitute_attr_values(string display_text, list< pair< pair<int, int>, PosAttr*> > *attrs, Position *pos) {
        int offset = 0;
        for (list< pair <pair<int, int>, PosAttr*> >::iterator iter=attrs->begin(); iter != attrs->end(); iter++) {
            const int attr_start = (*iter).first.first;
            const int attr_end = (*iter).first.second;
            const char* attr_value = (*iter).second->pos2str(*pos);
            display_text.replace(offset + attr_start, attr_end - attr_start + 1, attr_value);
            offset = offset + (strlen(attr_value) - (attr_end - attr_start) - 1); //compute offset after each substitution
        }
        return display_text;
    }
    void add_struct_events (int priority, Position fpos, Position tpos, vector<pos_event> &events) {
        for (Position num = str->rng->num_next_pos (fpos); 
             num < str->rng->size(); num++) {
            Position b = str->rng->beg_at (num);
            Position e = str->rng->end_at (num);
            int nest = 3 * str->rng->nesting_at (num);
            if (b > tpos)
                break;
            if (b >= fpos) {
                if (!display_class.empty() && e > b) {
                    events.push_back (pos_event (ev_BTAG, b, priority +2 +nest,
                                                 display_class));
                }
                if (!display_begin.empty()) {
                    if (!display_begin_attrs.empty()) {
                        events.push_back (pos_event (b, e == b ? priority +11 : priority +1 +nest, substitute_attr_values(display_begin, &display_begin_attrs, &num)));
                    } else {
                        events.push_back (pos_event (b, e == b ? priority +11 : priority +1 +nest, display_begin));
                    }
                }
                if (display_empty)
                    events.push_back (pos_event (ev_TEXT, b, priority +1, ""));
                if (display_tag) {
                    string btag = begstr (num) + (b == e ? "/>" : ">");
                    events.push_back (pos_event(b, priority +nest, btag));
                }
            }
            if (e <= tpos && e >= b && e > fpos) {
                if (display_tag && e > b)
                    events.push_back (pos_event(e, -priority -nest,
                                                str->endtagstring));
                if (!display_end.empty()) {
                    if (!display_end_attrs.empty()) {
                        events.push_back (pos_event(e, e == b ? (priority +12) : (-priority -1 -nest), substitute_attr_values(display_end, &display_end_attrs, &num)));
                    } else {
                        events.push_back (pos_event(e, e == b ? (priority +12) : (-priority -1 -nest), display_end));
                    }
                }
                if (!display_class.empty() && e > b)
                    events.push_back (pos_event(ev_ETAG, e, -priority -2 -nest,
                                                display_class));
            }
        }
    }
    string begstr (ConcIndex idx) {
         string ret = '<' + str->name;
         for (Corpus::VSA::iterator i = attrs.begin(); i != attrs.end(); i++)
             ret += ' ' + (*i).first + '=' + (*i).second->pos2str (idx);
         return ret;
    }
    void add_attr (const string &aname) {
        for (Corpus::VSA::iterator i = attrs.begin(); i != attrs.end(); i++)
            if ((*i).first == aname)
                return;
        PosAttr *a = str->get_attr (aname);
        attrs.push_back (make_pair (aname, a));
    }
    void add_all () {
        attrs.clear();
        for (CorpInfo::VSC::iterator i = str->conf->attrs.begin();
             i != str->conf->attrs.end(); i++) {
            attrs.push_back (make_pair ((*i).first, 
                                        str->get_attr ((*i).first)));
        }
    }
};


//-------------------- split_structures --------------------

void split_structures (Corpus *corp, const char *astr, OSvec &attrs, 
                       bool ignore_nondef)
{
    istringstream as (astr);
    string item; 
    map<string,OutStruct*> ss;
    while (getline (as, item, ',')) {
        if (item.empty() || item[0] == '-')
            continue;
        string::size_type idx = item.find (".");
        string attr (item, idx +1);
        if (idx != item.npos)
            item.erase (idx);
        OutStruct *s = ss [item];
        try {
            if (s == NULL) {
                ss [item] = s = new OutStruct (corp->get_struct(item));
                attrs.push_back(s);
            }
            if (idx != item.npos) {
                // attributes of structures
                if (attr == "*")
                    s->add_all();
                else
                    s->add_attr (attr);
            }
        } catch (AttrNotFound &e) {
            if (!ignore_nondef)
                throw e;
        }
    }
}


//-------------------- OutRef --------------------
class OutRef {
public:
    // return true if there was any output
    virtual bool output (ostream &out, Position pos)=0;
    virtual ~OutRef() {}
};

class ORdummy: public OutRef {
public:
    ORdummy() {}
    virtual bool output (ostream& out, Position pos) {
        return false;
    }
    virtual ~ORdummy() {}
};

class ORpossion: public OutRef {
public:
    ORpossion() {}
    virtual ~ORpossion() {}
    virtual bool output (ostream &out, Position pos) {
        out << '#' << pos;
        return true;
    }
};

class ORstructnum: public OutRef {
    Structure *str;
public:
    ORstructnum(Structure *s) : str(s) {}
    virtual ~ORstructnum() {}
    virtual bool output (ostream &out, Position pos) {
        Position n = str->rng->num_at_pos (pos);
        if (n == -1)
            return false;
        out << str->name << '#' << n;
        return true;
    }
};

class ORstructval: public OutRef {
    Structure *str;
    PosAttr *attr;
    const string pref;
public:
    ORstructval (Structure *s, const string &attrname, const string &p="",
                 bool value_only=false)
        : str(s), attr (s->get_attr (attrname)), 
          pref (value_only ? "" :
                (p.empty() ? (s->name + '.' + attrname + '=') : (p + '='))) {}
    virtual ~ORstructval() {}
    virtual bool output (ostream &out, Position pos) {
        Position n = str->rng->num_at_pos (pos);
        if (n == -1)
            return false;
        out << pref << attr->pos2str (n);
        return true;
    }
};

class ORstructall: public OutRef {
    Structure *str;
public:
    ORstructall(Structure *s) : str(s) {}
    virtual ~ORstructall() {}
    virtual bool output (ostream &out, Position pos) {
        Position n = str->rng->num_at_pos (pos);
        if (n == -1)
            return false;
        out << '<' << str->name;
        for (CorpInfo::VSC::iterator i = str->conf->attrs.begin(); 
             i != str->conf->attrs.end(); i++)
            out << ' ' << (*i).first << '=' 
                << str->get_attr ((*i).first)->pos2str (n);
        out << '>';
        return true;
    }
};

void split_references (Corpus *corp, const char *astr, ORvec &refs, 
                       bool ignore_nondef)
{
    istringstream as (astr);
    string item; 
    string::size_type idx;
    OutRef *r;
    while (getline (as, item, ',')) {
        if (item.empty() || item[0] == '-')
            continue;
        try {
            if (item == "#")
                r = new ORpossion();
            else if ((idx = item.find (".")) == item.npos)
                r = new ORstructnum (corp->get_struct(item));
            else {
                string attr (item, idx +1);
                item.erase (idx);
                bool valueonly = item[0] == '=';
                if (valueonly)
                    item.erase (0,1);
                if (attr == "*")
                    r = new ORstructall (corp->get_struct(item));
                else
                    r = new ORstructval (corp->get_struct(item), attr,
                                         corp->conf->find_struct (item)
                                         ->find_attr (attr)["LABEL"], valueonly);
            }
            refs.push_back(r);
        } catch (AttrNotFound &e) {
            if (!ignore_nondef)
                throw e;
            else refs.push_back(new ORdummy());
        } catch (CorpInfoNotFound &e) {
            if (!ignore_nondef)
                throw e;
            else refs.push_back(new ORdummy());
        }
    }
}

void split_attributes (Corpus *corp, const char *astr, PAvec &attrs, 
                       bool ignore_nondef)
{
    istringstream as (astr);
    string attr; 
    while (getline (as, attr, ',')) {
        if (attr.empty())
            continue;
        try {
            attrs.push_back(corp->get_attr(attr));
        } catch (AttrNotFound &e) {
            if (!ignore_nondef)
                throw e;
        }
    }
}


void get_corp_text (PAvec &attrs, string currtags, Position fpos, Position tpos,
                    vector<string> &strs, vector<string> &tags,
                    char posdelim = ' ', char attrdelim = '/')
{
    if (fpos >= tpos || attrs.empty()) return;
    TextIterator *wordi = attrs[0]->textat (fpos);
    //printf ("get_corp_text: currtags=%s, attrs.size=%i, fpos=%i tpos=%i\n", 
    //currtags.c_str(), attrs.size(), fpos, tpos);
    if (attrs.size() == 1) {
        for (; fpos < tpos; fpos++) {
            strs.push_back (wordi->next());
            strs.push_back (" ");
            tags.push_back (currtags);
            tags.push_back (currtags);
        }
    } else {
        string as;
        typedef vector<TextIterator*> VTI;
        VTI tis;
        tis.reserve (attrs.size() -1);
        for (PAvec::iterator i = attrs.begin() +1; i < attrs.end(); i++)
            tis.push_back ((*i)->textat (fpos));
        for (; fpos < tpos; fpos++) {
            strs.push_back (wordi->next());
            tags.push_back (currtags);
            as = "";
            for (VTI::iterator i = tis.begin(); i < tis.end(); i++) {
                as += attrdelim;
                as += (*i)->next();
            }
            strs.push_back (as);
            tags.push_back ("attr");
            strs.push_back (string (1, posdelim));
            tags.push_back (currtags);
        }
        for (VTI::iterator i = tis.begin(); i < tis.end(); i++)
            delete *i;
    }
    delete wordi;
    strs.pop_back();
    tags.pop_back();
}

// -------------------- *_get_* --------------------


class tags_set: public list<string> {
public:
    void insert (string x) {push_back (x);}
    void remove (string x) {
        for (iterator i = begin(); i != end(); i++)
            if (*i == x) {
                erase(i);
                break;
            }
    }

    string join () {
        switch (size()) {
        case 0:
            return "{}";
        case 1:
            return *begin();
        default:
            string ret = "{";
            for (const_iterator i=begin(); i != end(); i++)
                ret += *i + ' ';
            *(ret.end() -1) = '}';
            return ret;
        }
    }
};


static const char *str2tcl (const string &str)
{
    static char *buff = NULL;
    static unsigned buffsize = 0;
    static const char *empty = "{}";

    if (str.empty())
        return empty;

    const char *s = str.c_str();
    if (buffsize <= strlen (s) *2) {
        buffsize = strlen (s) *2 +1;
        buff = (char *)realloc (buff, buffsize);
        //XXX test returned value
    }
    char *p = buff;
    //printf ("str2tcl (%s)\n", s);
    for (; *s; s++, p++) {
        if (*s == ' ' || *s == '"' || *s == '\\' || *s == '{' || *s == '}' 
            || *s == ';' || *s == '[' || *s == ']' || *s == '$')
            *p++ = '\\';
        *p = *s;
    }
    *p = '\0';
    return buff;
}

typedef vector<string>::iterator VSit;


//-------------------- KWICLines --------------------
KWICLines::KWICLines (Corpus *c, RangeStream *r, const char *left,
                      const char *right,  const char *kwica, const char *ctxa,
                      const char *struca, const char *refa, int maxctx,
                      bool ignore_nondef)
    : corp (c), rstream (r), leftctx (prepare_context (c, left, true, maxctx)),
      rightctx (prepare_context (c, right, false, maxctx)),
      inutf8 (corp->get_conf ("ENCODING") == "UTF-8")
{
    split_attributes (corp, kwica, kwicattrs, ignore_nondef);
    if (!ctxa || *ctxa == '\0')
        ctxattrs = kwicattrs;
    else
        split_attributes (corp, ctxa, ctxattrs, ignore_nondef);
    if (struca && *struca != '\0')
        split_structures (corp, struca, structs, ignore_nondef);
    if (refa && *refa != '\0') {
        split_references (corp, refa, refs, ignore_nondef);
        if (refs.empty())
            split_references (corp, corp->get_conf ("SHORTREF").c_str(),
                              refs, ignore_nondef);
    }
}

KWICLines::~KWICLines() 
{
    delete leftctx;
    delete rightctx;
    delete rstream;
    for (ORvec::iterator i=refs.begin(); i != refs.end(); i++)
        delete *i;
    for (OSvec::iterator i=structs.begin(); i != structs.end(); i++)
        delete *i;
}


static void fill_segment (Tokens &partstr, Tokens &parttag, Tokens &output)
{
    if (!parttag.empty()) {
        Tokens::iterator si = partstr.begin();
        Tokens::iterator ti = parttag.begin();
        string currtag = *ti++;
        output.push_back (*si++);
        for (; si != partstr.end(); ++ti, ++si) {
            if (currtag != *ti) {
                output.push_back (currtag);
                output.push_back (*si);
                currtag = *ti;
            } else {
                output.back() += *si;
            }
        }
        output.push_back (currtag);
    }
    parttag.clear();
    partstr.clear();
}

bool less_pos_event (const pos_event &x, const pos_event &y) {
    return x.pos < y.pos || (x.pos == y.pos && x.sub < y.sub);
}

bool KWICLines::nextcontext ()
{
    if (rstream->end())
        return false;
    kwbeg = rstream->peek_beg();
    kwend = rstream->peek_end();
    ctxbeg = min (max (Position(0), leftctx->get (rstream)), corp->size());
    ctxend = min (max (Position(0), rightctx->get (rstream) +1), corp->size());
    rstream->add_labels (lab);
    rstream->next();
    return true;
}

bool KWICLines::skip (ConcIndex count)
{
    while (count-- && rstream->next());
    return nextcontext();
}

bool KWICLines::is_defined ()
{
    return kwbeg != Concordance::kwicnotdef;
}

Position KWICLines::reduce_ctxbeg()
{
    vector<int32_t> lctxlens;
    lctxlens.reserve(kwbeg - ctxbeg);
    TextIterator *lctxwordi = ctxattrs[0]->textat (ctxbeg);
    for(Position p = ctxbeg; p < kwbeg; p++)
        if (inutf8) lctxlens.push_back(utf8len(lctxwordi->next()));
        else lctxlens.push_back(strlen(lctxwordi->next()));
    delete lctxwordi;

    for(int64_t i = lctxlens.size() - 1, clen = 0; i >= 0; i--)
        if ((clen += lctxlens[i]) > leftctx->chars)
            return ctxbeg + i + 1;
    return ctxbeg;
}

Position KWICLines::reduce_ctxend()
{
    TextIterator *rctxwordi = ctxattrs[0]->textat (kwend);
    for(int64_t p = kwend, clen = 0; p < ctxend; p++) {
        if (inutf8) clen += utf8len(rctxwordi->next());
        else clen += strlen(rctxwordi->next());
        if (clen > rightctx->chars) {
            delete rctxwordi;
            return p;
        }
    }

    delete rctxwordi;
    return ctxend;
}

bool KWICLines::nextline () {
    outleft.clear();
    outkwic.clear();
    outright.clear();
    outrefs.clear();
    lab.clear();
    if (! nextcontext ())
        return false;
    if (ctxbeg >= ctxend || !is_defined())
        return true;

    if(leftctx->chars) ctxbeg = reduce_ctxbeg();
    if(rightctx->chars) ctxend = reduce_ctxend();

    vector<pos_event> events;
    events.push_back (pos_event (ev_BEG, ctxbeg, 0));
    events.push_back (pos_event (ev_END, ctxend, 0));
    events.push_back (pos_event (ev_BKW, kwbeg, 1000));
    events.push_back (pos_event (ev_EKW, kwend, kwbeg==kwend ? 2000 : -1000));

    int prior = 1;
    for (OSvec::iterator i = structs.begin(); i < structs.end(); i++, prior++)
        (*i)->add_struct_events (prior * 15, ctxbeg, ctxend, events);

    for (Labels::const_iterator i = lab.upper_bound(0);
         i != lab.lower_bound(100); ++i) {
        Position p = i->second;
        if (p == -1) continue;
        Position pe = lab[-(i->first)];
        events.push_back (pos_event (ev_TEXT, p, 0, " "));
        events.push_back (pos_event (ev_BTAG, p, 1000, "coll"));
        events.push_back (pos_event (ev_ETAG, pe, p==pe ? 2000 : -1000, "coll"));
        events.push_back (pos_event (ev_TEXT, pe, 0, " "));
    }
    ::sort (events.begin(), events.end(), less_pos_event);

    {
        // references
        for (ORvec::iterator i=refs.begin(); i != refs.end(); i++) {
            ostringstream out;
            (*i)->output (out, kwbeg);
            outrefs.push_back (out.str());
        }
    }

    // words & structures
    Tokens partstr, parttag;
    tags_set tags;
    PAvec *currattrs = &ctxattrs;
    bool inside = false;
    bool addspace = false;
    Position a1 = events[0].pos;
    for (vector<pos_event>::iterator e = events.begin(); 
         e != events.end(); e++) {
        Position a2 = e->pos;
        event_t event = e->type;
        //printf ("tcl_get_line for: event=%i, a1=%i, a2=%i\n", event, a1, a2);
        
        if (a1 < a2 && inside) {
            if (addspace) {
                partstr.push_back (" ");
                parttag.push_back (tags.join());
            }
            addspace = true;
            get_corp_text (*currattrs, tags.join(), a1, a2, partstr, parttag);
        }
        a1 = a2;
        switch (event) {
        case ev_BEG:
            inside = true; break;
        case ev_END:
            fill_segment (partstr, parttag, outright);
            break;
        case ev_BKW:
            currattrs = &kwicattrs;
            tags.insert ("col0");
            tags.insert ("coll");
            fill_segment (partstr, parttag, outleft);
            break;
        case ev_EKW:
            fill_segment (partstr, parttag, outkwic);
            currattrs = &ctxattrs;
            tags.remove ("col0");
            tags.remove ("coll");
            break;
        case ev_STRUCT:
            partstr.push_back (e->value);
            parttag.push_back ("strc");
            addspace = false;
            break;
        case ev_BTAG:
            tags.insert (e->value);
            break;
        case ev_ETAG:
            tags.remove (e->value);
            break;
        case ev_TEXT:
            partstr.push_back (e->value);
            parttag.push_back (tags.join());
            addspace = false;
            break;
        default:
            cerr <<"incorrect event type (" << event << ")\n";
        }
        if (event == ev_END)
            break;
    }
    return true;
}

const int KWICLines::get_linegroup()
{
    if (lab.find(Concordance::lngroup_labidx) != lab.end())
        return lab[Concordance::lngroup_labidx];
    else
        return 0;
}

string KWICLines::get_refs()
{
    ostringstream out;
    bool first_ref = true;
    for (Tokens::const_iterator i = outrefs.begin(); i != outrefs.end(); i++) {
        if (!(*i).empty()) {
            if (! first_ref)
                out << ',';
            first_ref = false;
            out << (*i);
        }
    }
    return out.str();
}

void tcl_output_tokens (ostream &out, const Tokens &toks)
{
    out << '\t';
    int i = 0;
    for (Tokens::const_iterator t=toks.begin(); t != toks.end(); ++t, ++i) {
        if (i)
            out << ' ';
        if (i % 2)
            out << *t;
        else
            out << str2tcl (*t);
    }
}

void Concordance::tcl_get (ostream &out, ConcIndex beg, ConcIndex end, 
                           const char *left, const char *right, 
                           const char *ctxa, const char *kwica,
                           const char *struca, const char *refa)
{
    int maxctx = 0;
    if (beg +1 == end) {
        maxctx = atol (corp->conf->opts["MAXDETAIL"].c_str());
    }

    KWICLines klines (this->corp, this->RS (true, beg, end), left, right,
                      kwica, ctxa, struca, refa, maxctx);
    if (beg < end) {
        if (beg < 0) beg = 0;
        if (end > viewsize()) end = viewsize();
        for (; beg < end; beg++) {
            if (!klines.get_ref_list().empty())
                out << str2tcl (klines.get_refs()) << " strc";
            tcl_output_tokens (out, klines.get_left());
            tcl_output_tokens (out, klines.get_kwic());
            if (klines.get_linegroup())
                out << " (" << klines.get_linegroup() << ") grp";
            tcl_output_tokens (out, klines.get_right());
            out << '\n';
            klines.nextline ();
        }
    } else {
        if (end < 0) end = 0;
        if (beg > viewsize()) beg = viewsize();
        for (beg--; end <= beg; beg--) {
            if (!klines.get_ref_list().empty())
                out << str2tcl (klines.get_refs()) << " strc";
            tcl_output_tokens (out, klines.get_left());
            tcl_output_tokens (out, klines.get_kwic());
            if (klines.get_linegroup())
                out << '(' << klines.get_linegroup() << ") grp";
            tcl_output_tokens (out, klines.get_right());
            out << '\n';
            klines.nextline ();
        }
    }
}

void Concordance::tcl_get_reflist (ostream &out, ConcIndex idx, const char *reflist)
{
    if (idx < 0 || idx >= viewsize())
        return;
    Position pos = beg_at (view ? (*view)[idx] : idx);
    ORvec refs;
    split_references (corp, reflist, refs, true);
    for (ORvec::iterator i=refs.begin(); i != refs.end(); i++)
        if ((*i)->output (out, pos))
            out << '\n';
}

void Concordance::poss_of_selected_lines (ostream &out, const char *rngs)
{
    ConcIndex fline, tline;
    NumOfPos csize = viewsize();
    istringstream rs (rngs);
    while (rs >> fline >> tline) {
        if (tline > csize) tline = csize;
        if (fline < 0) fline = 0;
        while (fline < tline) {
            ConcIndex idx = view ? (*view)[fline] : fline;
            Position pos = beg_at (idx);
            out << pos << ' ' << end_at (idx) - pos << '\n';
            ++fline;
        }
    }
}

//-------------------- CorpRegion --------------------

CorpRegion::CorpRegion (Corpus *corp, const char *attra, const char *struca,
                        bool ignore_nondef)
    : corp (corp), ignore_nondef (ignore_nondef)
{
    split_attributes (corp, attra, attrs, ignore_nondef);
    if (struca && *struca != '\0')
        split_structures (corp, struca, structs, ignore_nondef);
}

const Tokens &CorpRegion::region (Position frompos, Position topos,
                                  char posdelim, char attrdelim)
{
    output.clear();
    vector<pos_event> events;
    Position ctxbeg = max (Position(0), frompos);
    Position ctxend = min (topos, corp->size());
    events.push_back (pos_event (ev_BEG, ctxbeg, 0));
    events.push_back (pos_event (ev_END, ctxend, 0));

    int prior = 1;
    for (OSvec::iterator i = structs.begin(); i < structs.end(); i++, prior++)
        (*i)->add_struct_events (prior * 15, ctxbeg, ctxend, events);

    ::sort (events.begin(), events.end(), less_pos_event);

    // words & structures
    Tokens partstr, parttag;
    tags_set tags;
    bool inside = false;
    bool addspace = false;
    Position a1 = events[0].pos;
    for (vector<pos_event>::iterator e = events.begin(); 
         e != events.end(); e++) {
        Position a2 = e->pos;
        event_t event = e->type;
        
        if (a1 < a2 && inside) {
            if (addspace) {
                partstr.push_back (" ");
                parttag.push_back (tags.join());
            }
            addspace = true;
            get_corp_text (attrs, tags.join(), a1, a2, partstr, parttag,
                           posdelim, attrdelim);
        }
        a1 = a2;
        switch (event) {
        case ev_BEG:
            inside = true; break;
        case ev_END:
            fill_segment (partstr, parttag, output);
            break;
        case ev_STRUCT:
            partstr.push_back (e->value);
            parttag.push_back ("strc");
            addspace = false;
            break;
        case ev_BTAG:
            tags.insert (e->value);
            break;
        case ev_ETAG:
            tags.remove (e->value);
            break;
        case ev_TEXT:
            partstr.push_back (e->value);
            parttag.push_back (tags.join());
            addspace = false;
            break;
        default:
            cerr <<"incorrent event type (" << event << ")\n";
        }
        if (event == ev_END)
            break;
    }
    return output;
}

CorpRegion::~CorpRegion()
{
    for (OSvec::iterator i=structs.begin(); i != structs.end(); i++)
        delete *i;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
