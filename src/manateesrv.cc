//  Copyright (c) 1999-2013  Pavel Rychly, Milos Jakubicek

#include "regpref.hh"
#include "config.hh"
#include "subcorp.hh"
#include "concord.hh"
#include "cqpeval.hh"

#ifdef SKETCH_ENGINE
#include "wmap.hh"
#include "frsop.hh"
#include "fsop.hh"
#endif

#include <cstdlib>
#include <sstream>
#include <fstream>
#include <cstring>

using namespace std;

#ifdef SKETCH_ENGINE
void output_ws (ostream &out, PosAttr *attr, WMap *ws, const char *word, 
                int minfreq=3, int maxitems=20);
void output_ws_diff (ostream &out, PosAttr *attr, WMap *ws, const char *word1,
                     const char *word2, int minfreq, int maxitems);
void output_thes (vector<pair<string,double> > &out, PosAttr *attr, 
                  const string &base, const char *word, int maxitems=30);
#endif


class CorpNotFound: public exception {
    const string _what;
public:
    const string name;
    CorpNotFound (const string &name)
        :_what ("Corpus `" + name + "' not found"), name (name) {}
    virtual const char* what () const throw () {return _what.c_str();}
    virtual ~CorpNotFound() throw () {}
};


class CorpusManager {
private:
    bool allow_unknown_corp;

protected:
    struct CIC {
        CIC (const string ci="", Corpus *co=NULL): conf (ci), corp(co) {}
        string conf;
        Corpus *corp;
    };
    typedef map<string,CIC> MSCr;
    typedef map<string,Concordance*> MSCn;
    typedef map<string,pair<string,string> > MSPSS;
    MSCr corpora;
    MSCn concord;
    MSPSS subcorps;
    const char *subcorpdir;
public:
    CorpusManager (istream &corplist, bool allow_unknown=false);
    ~CorpusManager();
    Corpus *get_corp (istream &in) {
        string corpname;
        in >> corpname;
        return get_corp (corpname);
    }
    Corpus *get_corp (const string &corpname) {
        if (!allow_unknown_corp && corpora.find (corpname) == corpora.end())
                throw CorpNotFound (corpname);
        CIC &cic = corpora [corpname];
        if (!cic.corp) {
            MSPSS::const_iterator i = subcorps.find (corpname);
            if (i != subcorps.end()) {
                cic.corp = new SubCorpus (get_corp ((*i).second.first),
                                          (*i).second.second);
            } else {
                cic.corp = new Corpus (cic.conf.empty() ? corpname : cic.conf);
            }
            if (!cic.corp)
                throw CorpNotFound (corpname);
        }
        return cic.corp;
    }
    Concordance *get_conc (istream &in) {
        string concname;
        in >> concname;
        return get_conc (concname);
    }
    Concordance *get_conc (const string &concname) {
        Concordance *c = concord [concname];
        if (!c) {
            concord.erase (concname);
            throw ConcNotFound (concname);
        }
        return c;
    }

    void run (istream &in, ostream &out);
    bool canrebuild (const string &corpname) {
        Corpus *c = get_corp (corpname);
        char *username = getenv ("USER");
        if (username == NULL) 
            return true;
        string rebuser = c->get_conf ("REBUILDUSER");
        if (rebuser == "*")
            return true;
        istringstream users (rebuser);
        string oneuser;
        while (getline (users, oneuser, ',')) {
            if (oneuser == username)
                return true;
        }
        return false;
    }
};

CorpusManager::CorpusManager (istream &corplist, bool allow_unknown)
    :allow_unknown_corp (allow_unknown), subcorpdir (getenv ("SUBCORPDIR"))
{
    string name;
    while (corplist >> name) {
        CIC cic;
        cic.conf = name;
        cic.corp = NULL;

        int slash = name.rfind ('/');
        if (slash >= 0) {
            string base (name, slash +1);
            name = base;
        }
        corpora [name] = cic;
    }

    // setup corpus if only one allowed
    if (!allow_unknown && corpora.size() == 1) {
        get_corp (name);
    }
    
    // locate subcorpora
    if (subcorpdir) {
        find_subcorpora (subcorpdir, subcorps);
        for (MSPSS::const_iterator i = subcorps.begin(); 
             i != subcorps.end(); i++)
            if (corpora.find ((*i).second.first) != corpora.end())
                corpora [(*i).first];
    }
}

CorpusManager::~CorpusManager()
{
    for (MSCn::iterator m=concord.begin(); m != concord.end(); m++)
        delete (*m).second;
    for (MSCr::iterator m=corpora.begin(); m != corpora.end(); m++)
        if ((*m).second.corp)
            delete (*m).second.corp;
}

template <class T>
istream& operator>> (istream& is, vector<T> &args)
{
    for (typename vector<T>::iterator i=args.begin(); i != args.end(); i++)
        is >> (*i);
    return is;
};

template<class container>
void copy_first (const container &c, ostream &out, const string &prefix="",
                 char delim='\n')
{
    for (typename container::const_iterator i=c.begin(); i != c.end(); i++)
        out << prefix << (*i).first << delim;
};

#define ISSPACE(x) (strchr (" \t\n\r", (x)) != 0)

void trimspaces (string &val)
{
    while (!val.empty() && ISSPACE (val[0]))
        val.erase (val.begin());
    while (!val.empty() && ISSPACE (*(val.end()-1)))
        val.erase (val.end()-1);
}

void tcloutput_text (ostream &out, const string &text)
{
    for (string::const_iterator i=text.begin(); i != text.end(); i++) {
        switch (*i) {
        case '\t': out << "\\t"; break;
        case '\v': out << "\\v"; break;
        case '\n': out << "\\n"; break;
        default: out << *i;
        }
    }
}

void output_posrange (ostream &out, PosAttr *a, Position from, Position to)
{
    TextIterator *ti = a->textat (from);
    while (from++ < to)
        out << ti->next() << ' ';
    delete ti;
}

void output_aligned_part (ostream &out, Corpus *c, int align_idx, 
                          int kwicbeg, int kwicend,
                          const string &corp_name="")
{
    Structure *s = c->get_struct ("align");
    int bb = s->rng->beg_at (align_idx);
    int ee = s->rng->end_at (align_idx);
    out << bb << ' ' << ee << ' ' << corp_name << '\n';
    if (kwicend == 0 && kwicbeg == 0)
        output_posrange (out, c->get_attr("-"), bb, ee);
    else {
        output_posrange (out, c->get_attr("-"), bb, kwicbeg);
        out << '\t';
        output_posrange (out, c->get_attr("-"), kwicbeg, kwicend);
        out << '\t';
        output_posrange (out, c->get_attr("-"), kwicend, ee);
    }
    out << '\n';
}

void CorpusManager::run (istream &in, ostream &out)
{
    string cmd, restlinestr;
    string lasterr;
    while (in >> cmd) {
        try {
            getline (in, restlinestr);
            cerr << "cmd=(" << cmd << ") args=(" 
                 << restlinestr << ')' << endl;
            istringstream args (restlinestr.c_str());
            if (cmd == "get") {
                Concordance *c = get_conc (args);
                vector<string> a (6);
                int beg, end;
                args >> beg >> end >> a;
                c->tcl_get (out, beg, end, a[0].c_str(), a[1].c_str(), 
                            a[2].c_str(), a[3].c_str(), a[4].c_str(), 
                            a[5].c_str());
            } else if (cmd == "len") {
                try {
                    Concordance *c = get_conc (args);
                    unsigned s = c->viewsize();
                    out << s << ' ';
                    if (c->finished())
                        out << s << '\n';
                    else
                        out << "UNKNOWN\n";
                } catch (ConcNotFound&) {
                    out << "0 0\n";
                }
            } else if (cmd == "pend") {
                out << int (!get_conc (args)->finished());
            } else if (cmd == "sync") {
                get_conc (args)->sync();
            } else if (cmd == "err") {
                if (!lasterr.empty()) {
                    out << lasterr << '\n';
                    lasterr = "";
                }
            } else if (cmd == "set") {
                string conc, query; 
                args >> conc;
                Corpus *c = get_corp (args);
                getline (args, query);
                query += ';';
                if (concord [conc])
                    delete concord [conc];
                concord.erase (conc);
                try {
                    concord [conc] 
                        = new Concordance (c, c->filter_query 
                                           (eval_cqpquery (query.c_str(), c)));
                    lasterr = "";
                } catch (exception &e) {
                    lasterr = e.what();
                    throw e;
                }
            } else if (cmd == "linegroup") {
                Concordance *c = get_conc (args);
                int linenum, group;
                args >> linenum >> group;
                c->set_linegroup (linenum, group);
            } else if (cmd == "coloc") {
                Concordance *c = get_conc (args);
                string lctx, rctx, query; 
                int collnum, rank;
                args >> collnum >> lctx >> rctx >> rank;
                getline (args, query);
                query += ';';
                c->set_collocation (collnum, query, lctx.c_str(), rctx.c_str(),
                                    rank);
            } else if (cmd == "lscorp") {
                copy_first (corpora, out);
            } else if (cmd == "list") {
                copy_first (concord, out);
            } else if (cmd == "encoding") {
                out << get_corp (args)->get_conf ("ENCODING");
            } else if (cmd == "lsdpos") {
                copy_first (get_corp (args)->conf->attrs, out);
            } else if (cmd == "lsdstr") {
                copy_first (get_corp (args)->conf->structs, out);
            } else if (cmd == "lsdstrattr") {
                Corpus *c = get_corp (args);
                CorpInfo *ci = c->conf;
                for (CorpInfo::VSC::iterator i = ci->structs.begin(); 
                     i != ci->structs.end(); i++)
                    copy_first ((*i).second->attrs, out, 
                                (*i).first + '.');
            } else if (cmd == "lsdstrattrname") {
                Corpus *c = get_corp (args);
                for (CorpInfo::VSC::iterator i = c->conf->structs.begin(); 
                     i != c->conf->structs.end(); i++) {
                    string pref = (*i).first + '.';
                    for (CorpInfo::VSC::iterator j = (*i).second->attrs.begin(); 
                         j != (*i).second->attrs.end(); j++) {
                        out << pref << (*j).first;
                        string &label = (*j).second->opts["NAME"];
                        if (!label.empty())
                            out << '\t' << label;
                        out << '\n';
                    }
                }
            } else if (cmd == "corpsize") {
                Corpus *c = get_corp (args);
                out << c->size();
            } else if (cmd == "info") {
                Corpus *c = get_corp (args);
                out << "info\t";
                tcloutput_text (out, c->get_info());
                out << '\n';
                out << "size\t" << c->size() << '\n';
                out << "search_size\t" << c->search_size() << '\n';
                CorpInfo *ci = c->conf;
                for (CorpInfo::VSC::iterator i = ci->attrs.begin();
                     i != ci->attrs.end(); i++)
                    out << "a-" << (*i).first << '\t' 
                        << c->get_attr((*i).first)->id_range() << '\n';
                for (CorpInfo::VSC::iterator i = ci->structs.begin(); 
                     i != ci->structs.end(); i++)
                    out << "s-" << (*i).first << '\t' 
                        << c->get_struct((*i).first)->size() << '\n';
                
            } else if (cmd == "corpconf") {
                Corpus *c = get_corp (args);
                string item;
                args >> item;
                out << c->get_conf (item) << '\n';
            } else if (cmd == "setdefaultattr") {
                Corpus *c = get_corp (args);
                string attrname;
                args >> attrname;
                c->set_default_attr (attrname);
            } else if (cmd == "redfreq") {
                out << get_conc (args)->redfreq() << '\n';
            } else if (cmd == "arf") {
                out << get_conc (args)->compute_ARF() << '\n';
            } else if (cmd == "freq") {
                Corpus *c = get_corp (args);
                string attr, val;
                args >> attr;
                PosAttr *a = c->get_attr (attr);
                getline (args, val);
                trimspaces (val);
                int id;
                if (a && (id = a->str2id (val.c_str())) >= 0)
                    out << a->freq(id) << '\n';
                else
                    out << 0 << '\n';
            } else if (cmd == "count") {
                Corpus *c = get_corp (args);
                string query;
                getline (args, query);
                query += ';';
                unsigned int cnt = 0;
                RangeStream *s = eval_cqpquery (query.c_str(), c);
                if (s && s->peek_beg() != s->final()) {
                    cnt++;
                    while (s->next())
                        cnt++;
                }
                if (s)
                    delete s;
                out << cnt << '\n';
            } else if (cmd == "distrib") {
                Concordance *c = get_conc (args);
                int x,y;
                bool show_begs;
                args >> x >> y >> show_begs;
                vector<int> dis(x);
                vector<int> begs(x);
                out << c->distribution (dis, begs, y) << ' ';
                if (show_begs) {
                    vector<int>::iterator bi = begs.begin();
                    for (vector<int>::iterator i= dis.begin();
                         i < dis.end(); i++, bi++)
                        out << *i << ' ' << *bi << ' ';
                } else {
                    for (vector<int>::iterator i= dis.begin(); 
                         i < dis.end(); i++)
                        out << *i << ' ';
                }
                out << '\n';
            } else if (cmd == "cluster") {              
                Concordance *c = get_conc (args);
                c->make_grouping();
            } else if (cmd == "findcoll") {
                Concordance *c = get_conc (args);
                string attr, cmpr;
                int minfreq, minbgr, cfrom, cto, maxlines;
                args >> attr >> cmpr >> minfreq >> minbgr >> cfrom >> cto 
                     >> maxlines;
                c->find_coll (out, attr, cmpr, minfreq, minbgr, cfrom, cto, 
                              maxlines, "mtrf");
            } else if (cmd == "concref") {
                Concordance *c = get_conc (args);
                int idx;
                string reflist;
                args >> idx >> reflist;
                c->tcl_get_reflist (out, idx, reflist.c_str());
            } else if (cmd == "rename") {
                string c1, c2;
                args >> c1 >> c2;
                if (concord [c2])
                    delete concord [c2];
                concord [c2] = concord [c1];
                concord.erase (c1);
            } else if (cmd == "clone") {
                string c1, c2;
                args >> c1 >> c2;
                try {
                    delete get_conc(c2);
                } catch (ConcNotFound &e){}
                concord [c2] = new Concordance (*(get_conc (c1)));
            } else if (cmd == "erase") {
                string conc;
                args >> conc;
                delete get_conc (conc);
                concord.erase (conc); 
                get_conc (args)->sync();
            } else if (cmd == "sort") {
                Concordance *c = get_conc (args);
                string crit; 
                bool uniq;
                args >> uniq;
                getline (args, crit);
                c->sort (crit.c_str(), uniq);
            } else if (cmd == "countsort") {
                Concordance *c = get_conc (args);
                string attr, lctx, rctx; 
                bool words_only;
                args >> attr >> words_only >> lctx >> rctx;
                c->count_sort (lctx.c_str(), rctx.c_str(), attr, words_only);
            } else if (cmd == "fdist") {
                Concordance *c = get_conc (args);
                string crit; 
                int limit;
                args >> limit;
                getline (args, crit);
                c->corp->freq_dist (c->RS(), out, crit.c_str(), limit);
            } else if (cmd == "delpnf") {
                Concordance *c = get_conc (args);
                int collnum, positive;
                args >> collnum >> positive;
                c->delete_pnfilter (collnum, positive);
            } else if (cmd == "reduce") {
                string concname, crit;
                args >> concname >> crit;
                Concordance *c = get_conc (concname);
                c->reduce_lines (crit.c_str());
            } else if (cmd == "swapkwic") {
                Concordance *c = get_conc (args);
                int collnum;
                args >> collnum;
                c->swap_kwic_coll (collnum);
            } else if (cmd == "linegroupstat") {
                Concordance *c = get_conc (args);
                map<short int,int> lgs;
                c->get_linegroup_stat (lgs);
                for (map<short int,int>::iterator i=lgs.begin(); 
                     i!=lgs.end(); i++)
                    out << int((*i).first) << '\t' << (*i).second << '\n';
            } else if (cmd == "dellinegroups") {
                Concordance *c = get_conc (args);
                string grps;
                getline (args, grps);
                c->delete_linegroups (grps.c_str(), false);
            } else if (cmd == "subcorp") {
                if (subcorpdir) {
                    string subcname, corpname, struc, query;
                    args >> subcname >> corpname >> struc;
                    getline (args, query);
                    Corpus *c = get_corp(corpname);
                    string spath = string(subcorpdir) +"/"+ subcname + ".subc";
                    try {
                        if (!create_subcorpus (spath.c_str(), c,
                                               struc.c_str(), query.c_str()))
                            lasterr = "empty subcorpus";
                        else {
                            SubCorpus *sub = new SubCorpus (c, spath);
                            corpora [corpname + ':' + subcname] = CIC ("", sub);
                            lasterr = "";
                        }
                    } catch (exception &e) {
                        lasterr = e.what();
                        throw e;
                    }
                } else {
                    lasterr = "you are not allowed to create subcorpora";
                }
            } else if (cmd == "removesubc") {
                if (subcorpdir) {
                    string subcorpname;
                    args >> subcorpname;
                    MSCr::iterator ci = corpora.find (subcorpname);
                    if (ci != corpora.end()) {
                        if ((*ci).second.corp)
                            delete (*ci).second.corp;
                        corpora.erase (ci);
                    }
                    unlink (subcorps [subcorpname].second.c_str());
                    MSPSS::iterator si = subcorps.find (subcorpname);
                    if (si != subcorps.end())
                        subcorps.erase (si);
                }
            } else if (cmd == "wordlist") {
                string attrname, regpat;
                int minfreq, maxlines;
                bool ignorecase;
                Corpus *c = get_corp (args);
                args >> attrname >> minfreq >> maxlines >> ignorecase;
                getline (args, regpat);
                trimspaces (regpat);
                PosAttr *a = c->get_attr (attrname, true);
                Generator<int> *ids = a->regexp2ids (regpat.c_str(), 
                                                     ignorecase);
                while (!ids->end() && maxlines > 0) {
                    int id = ids->next();
                    int frq;
                    if ((frq = a->freq (id)) >= minfreq) {
                        out << a->id2str (id) << '\t' << frq << '\n';
                        maxlines--;
                    }
                }
            } else if (cmd == "positions") {
                Concordance *c = get_conc (args);
                string rngs;
                getline (args, rngs);
                c->poss_of_selected_lines (out, rngs.c_str());
            } else if (cmd == "at") {
                Concordance *c = get_conc (args);
                int idx;
                args >> idx;
                if (idx == -1) {
                    for (idx=0; idx < c->size(); idx++) {
                        out << idx << ": <" << c->beg_at(idx) 
                            << ',' << c->end_at(idx) << '>';
                        for (int i=0; i < c->numofcolls(); i++)
                            out << ' ' << i << ": <" 
                                << c->coll_beg_at (i+1, idx) << ',' 
                                << c->coll_end_at (i+1, idx) << '>';
                        out << '\n';
                    }
                } else {
                    out << idx << ": <" << c->beg_at(idx) 
                        << ',' << c->end_at(idx) << ">\n";
                    for (int i=0; i < c->numofcolls(); i++)
                        out << "coll " << i << ": <" 
                            << c->coll_beg_at (i+1, idx) << ',' 
                            << c->coll_end_at (i+1, idx) << ">\n";
                }
            } else if (cmd == "align") {
                Concordance *c = get_conc (args);
                int line;
                args >> line;
                string acname = c->corp->get_conf ("ALIGNED");
                Corpus *ac;
                if (acname.length() && (ac = get_corp (acname))) {
                    int p = c->beg_at (line);
                    Structure *s = c->corp->get_struct ("align");
                    int n = s->rng->num_at_pos (p);
                    if (n >= 0) {
                        output_aligned_part (out, c->corp, n, p,
                                             c->end_at (line));
                        output_aligned_part (out, ac, n, 0, 0, acname);
                    }
                }
            } else if (cmd == "alignpos") {
                string cname;
                int pos;
                args >> cname >> pos;
                Corpus *c = get_corp (cname);
                string acname = c->get_conf ("ALIGNED");
                Corpus *ac;
                if (acname.length() && (ac = get_corp (acname))) {
                    Structure *s = c->get_struct ("align");
                    int n = s->rng->num_at_pos (pos);
                    if (n >= 0) {
                        output_aligned_part (out, c, n, 0, 0, cname);
                        output_aligned_part (out, ac, n, 0, 0, acname);
                    }
                }
#ifdef SKETCH_ENGINE
            } else if (cmd == "showws") {
                Corpus *c = get_corp (args);
                string word;
                int minfreq, maxitems;
                args >> minfreq >> maxitems >> word;
                WMap *ws = new_WMap (c->get_conf ("WSBASE"));
                out.setf (ios::fixed);
                out.precision(1);
                output_ws (out, c->get_attr (c->get_conf ("WSATTR")), ws,
                           word.c_str(), minfreq, maxitems);
                delete ws;
            } else if (cmd == "showwsdiff") {
                Corpus *c = get_corp (args);
                string word1, word2;
                int minfreq, maxitems;
                args >> minfreq >> maxitems >> word1 >> word2;
                WMap *ws = new_WMap (c->get_conf ("WSBASE"));
                out.setf (ios::fixed);
                out.precision(1);
                output_ws_diff (out, c->get_attr (c->get_conf ("WSATTR")), ws,
                                word1.c_str(), word2.c_str(), 
                                minfreq, maxitems);
                delete ws;
            } else if (cmd == "ws2conc") {
                string conc; 
                args >> conc;
                Corpus *c = get_corp (args);
                int seek;
                args >> seek;
                WMap *ws = new_WMap (c->get_conf ("WSBASE"), "", 2);
                if (!ws->seek (seek)) {
                    lasterr = "out of range";
                } else {
                    if (concord [conc])
                        delete concord [conc];
                    concord.erase (conc);
                    try {
                        FastStream *f = ws->poss();
                        while (args >> seek) {
                            if (ws->seek (seek))
                                f = new QOrNode (f, ws->poss());
                        }
                        concord [conc]
                            = new Concordance (c, new Pos2Range (f));
                        concord [conc]->sync();
                        lasterr = "";
                    } catch (exception &e) {
                        lasterr = e.what();
                        throw e;
                    }
                }
                delete ws;
            } else if (cmd == "thes") {
                Corpus *c = get_corp (args);
                string word;
                int maxitems;
                args >> maxitems >> word;
                vector<pair<string, double> > words;
                output_thes (words, c->get_attr (c->get_conf ("WSATTR")),
                             c->get_conf ("WSTHES"), word.c_str(), maxitems);
                for (vector<pair<string,double> >::const_iterator 
                         i = words.begin(); 
                     i != words.end(); i++)
                    out << (*i).first << '\t' << (*i).second << '\n';
#endif
            } else if (cmd == "load") {
                string concname, filename;
                args >> concname;
                Corpus *c = get_corp (args);
                args >> filename;
                concord [concname] = new Concordance (c, filename.c_str());
            } else if (cmd == "save") {
                Concordance *c = get_conc (args);
                string filename;
                args >> filename;
                c->save (filename.c_str());
            } else if (cmd == "passwd") {
                string newp, oldp;
                args >> newp >> oldp;
                char *chusercmd = getenv ("CHUSERCMD");
                //XXX CHUSERCMD nastavit na "/.../chuser -f /.../users"
                char *username = getenv ("USER");
                if (chusercmd && username) {
                    string cmd = (chusercmd + (" -p '" + newp + 
                                          "' -o '" + oldp + "' ") 
                                  + username + " 2>&1");
                    //cerr << "volam chuser: >>" << cmd << "<<\n";
                    if (system (cmd.c_str()) != 0)
                                cerr << "error: failed to execute: " << cmd.c_str() << "\n";
                }
            } else if (cmd == "canrebuild") {
                string cname;
                args >> cname;
                out << (canrebuild (cname) ? "OK" : "NO");
            } else if (cmd == "rebuild") {
                string cname;
                args >> cname;
                if (canrebuild (cname)) {
                    CIC &cic = corpora [cname];
                    if (cic.corp) {
                        // delete concordances
                        for (MSCn::iterator i = concord.begin();
                             i != concord.end(); ) {
                            MSCn::iterator di = i;
                            i++;
                            if ((*di).second->corp == cic.corp) {
                                delete (*di).second;
                                concord.erase (di);
                            }
                        }
                        // free corpus
                        delete cic.corp;
                        cic.corp = NULL;
                    }
                    // free subcorpora
                    for (MSPSS::const_iterator i = subcorps.begin(); 
                         i != subcorps.end(); i++)
                        if ((*i).second.first == cname &&
                            (cic = corpora [(*i).first]).corp) {
                            delete cic.corp;
                            cic.corp = NULL;
                        }
                    //string cpath = get_corp (cname)->get_conf ("PATH");
                    //remove ((cname + ".org").c_str());
                    //rename (cname.c_str(), (cname + ".org").c_str());
                    if (system (("encodevert -c " + cname + " 2>&1").c_str()) != 0)
                                cerr << "error: encodevert failed!\n";

                }
            } else if (cmd == "version") {
                out << VERSION_STR << '\n';
            } else if (cmd == "exit") {
                out << "\v\n" << flush;
                return;
            } else {
                cerr << "unknown command (" << cmd << ")\n";
            }
        } catch (exception &ex) {
            cerr << "error: " << ex.what() << '\n';
        }
        out << "\v\n" << flush;
        cerr.flush();
    }
}

int main (int argc, char **argv)
{
    const char *corps = "susanne";
    bool allow_unknown = true;
    const char *input = 0;
    if (argc > 1) {
        const char *o = argv[1];
        if (*(o++) == '-' && (*o == 'v' || *o == 'V' || 
                              (*o == '-' && (o[1] == 'v' || o[1] == 'V')))) {
            cout << VERSION_STR << '\n';
            return 2;
        }
        if (*argv[1] == '+')
            input = argv[1] +1;
        else {
            corps = argv[1];
            allow_unknown = false;
        }
    }
    try {
        istringstream corpnames (corps);
        CorpusManager cm (corpnames, allow_unknown);
        if (input) {
            ifstream f(input);
            cm.run (f, cout);
        } else {
            cm.run (cin, cout);
        }
    } catch (exception &ex) {
        cerr << "Unhandled exception: " << ex.what() << '\n';
        return 1;
    }
    cerr << "manateesrv finished.\n";
    return 0;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
