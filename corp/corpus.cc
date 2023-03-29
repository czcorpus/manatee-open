//  Copyright (c) 1999-2020  Pavel Rychly, Milos Jakubicek

#include "config.hh"
#include "subcorp.hh"
#include "dynattr.hh"
#include "normattr.hh"
#include "binfile.hh"
#include "fromtof.hh"
#include "fsop.hh"
#include <cstdlib>
#include <fstream>
#include <limits>
#include <math.h>

using namespace std;

inline PosAttr *createDynAttr (CorpInfo::MSS &ao, const string &apath, 
                               const string &n, PosAttr *from,
                               bool ownedByCorpus = true)
{
    DynFun *fun = createDynFun (ao["FUNTYPE"].c_str(), 
                                ao["DYNLIB"].c_str(), ao["DYNAMIC"].c_str(),
                                ao["ARG1"].c_str(), ao["ARG2"].c_str());
    PosAttr *da = createDynAttr (ao["DYNTYPE"], apath, n, fun, from, ao["LOCALE"],
                                 CorpInfo::str2bool (ao["TRANSQUERY"]), ownedByCorpus);
    try {
        DynFun *fun2 = createDynFun (ao["FUNTYPE"].c_str(), ao["DYNLIB"].c_str(),
                                     ao["DYNAMIC"].c_str(), ao["ARG1"].c_str(),
                                     ao["ARG2"].c_str());
        // fun2 is needed iff TRANSQUERY
        PosAttr *ro = createDynAttr ("index", apath + ".regex", n + ".regex", fun2, da, ao["LOCALE"],
                                    CorpInfo::str2bool (ao["TRANSQUERY"]));
        da->regex = ro;
    } catch (FileAccessError&) {errno = 0;}
    return da;
}


PosAttr *Corpus::setup_attr (const string &attr_name, bool skip_subcorp)
{
    try {
        PosAttr* pa;
        string name, norm, path = conf->opts ["PATH"];
        size_t at = attr_name.find ('@');
        if (at != string::npos) {
            name = string (attr_name, 0, at);
            norm = string (attr_name, at + 1);
        } else
            name = attr_name;
        CorpInfo::MSS &ao = conf->find_attr (name);
        if (at != string::npos)
            pa = createNormPosAttr (get_attr (name), get_attr (norm));
        else if (ao["DYNAMIC"] != "")
            pa = createDynAttr (ao, path + name, name,
                                setup_attr (ao["FROMATTR"], true));
        else if (virt)
            pa = setup_virtposattr (virt, path + name, name, ao["LOCALE"],
                                    conf->opts["ENCODING"], true,
                                    ao["DEFAULTVALUE"], conf->opts["DOCSTRUCTURE"]);
        else
            pa = createPosAttr (ao["TYPE"], path + name, name, ao["LOCALE"], 
                                conf->opts["ENCODING"]);
        if (!skip_subcorp && !conf->opts ["SUBCPATH"].empty()) {
            string spath = conf->opts ["SUBCPATH"];
            size_t slash = path.rfind('/');
            if (slash != string::npos && slash != path.length() - 1)
                spath += path.substr (slash + 1);
            pa = createSubCorpPosAttr (pa, spath, is_complement());
        }
        attrs.push_back (pair<string,PosAttr*> (name, pa));
        return pa;
    } catch (CorpInfoNotFound&) {
        throw AttrNotFound (attr_name);
    }
}

Structure *Corpus::setup_struct (const string &name)
{
    try {
        CorpInfo *ci = conf->find_struct (name);
        Structure *s;
        if (virt) {
            s = new Structure (ci, name, virt);
        } else {
            string path = conf->opts ["PATH"];
            if (!conf->opts ["SUBCPATH"].empty())
                ci->opts ["SUBCPATH"] = conf->opts ["SUBCPATH"];
            s = new Structure (ci, path + name, name, this);
        }
        structs.push_back (pair<string,Structure*> (name, s));
        s->complement = is_complement();
        s->conf->conffile = conf->conffile;
        return s;
    } catch (CorpInfoNotFound&) {
        throw AttrNotFound (name);
    }
}

void Corpus::init (CorpInfo *ci)
{
    hardcut = atol (ci->opts["HARDCUT"].c_str());
    maxctx = atol (ci->opts["MAXCONTEXT"].c_str());
    if (!ci->opts["ALIGNED"].empty()) {
        istringstream as (ci->opts["ALIGNED"]);
        string cname;
        while (getline (as, cname, ',')) {
            if (cname.empty())
                continue;
            aligned.push_back (AlignedCorpus (cname));
        }
    }
    if (ci->opts["VIRTUAL"] != "")
        virt = setup_virtcorp (ci->opts["VIRTUAL"]);
}

Corpus::Corpus (CorpInfo *ci, type_t t)
    : defaultattr (NULL), virt(NULL), type(t), conf (new CorpInfo (ci))
{
    init (conf);
}

Corpus::Corpus (const string &corp_name)
    : defaultattr (NULL), virt(NULL), type(Corpus_type),
      conf (loadCorpInfo (corp_name))
{
    init (conf);
    //if (!defaultattr)
    // XXX throw (no atributes)
}

Corpus::~Corpus ()
{
    for (VSA::iterator i = attrs.begin(); i != attrs.end(); i++)
        delete (*i).second;
    for (VSS::iterator i = structs.begin(); i != structs.end(); i++)
        delete (*i).second;
    delete conf;
    if (virt)
        delete virt;
    for (unsigned i = 0; i < aligned.size(); i++) {
        delete aligned[i].corp;
        delete_TokenLevel (aligned[i].level);
    }
}

PosAttr* Corpus::get_attr (const string &name, bool struct_attr)
{
    if (name == "-")
        return get_default_attr();

    VSA::iterator i = attrs.begin();
    while (i != attrs.end()) {
        if ((*i).first == name) 
            return (*i).second;
        i++;
    }

    int j;
    if ((j = name.find ('.')) >= 0) {
        string strucname (name, 0, j);
        string attrname (name, j+1);
        if (struct_attr) {
            Structure *s = get_struct (strucname);
            return s->get_attr (attrname);
        } else
            return get_struct_pos_attr (strucname, attrname);
    }
    return setup_attr (name);
}

PosAttr* Corpus::get_default_attr ()
{
    if (defaultattr)
        return defaultattr;
    else
        return defaultattr = get_attr (get_conf ("DEFAULTATTR"));
}

void Corpus::set_default_attr (const string &attname)
{
    conf->opts["DEFAULTATTR"] = attname;
    defaultattr = get_attr (attname);
}

void Corpus::set_reference_corpus (const string &refcorpname)
{
    conf->opts["REFCORPUS"] = refcorpname;
}

Structure* Corpus::get_struct (const string &name)
{
    VSS::iterator i = structs.begin();
    while (i != structs.end()) {
        if ((*i).first == name) 
            return (*i).second;
        i++;
    }
    return setup_struct (name);
}

string Corpus::get_info()
{
    string infostr = get_conf ("INFO");
    if (infostr.empty() || infostr[0] != '@')
        return infostr;
    string s (infostr, 1);
    MapBinFile<char> infofile (get_conf ("PATH") + s);
    s = infofile.at(0);
    return s;
}


const string Corpus::get_sizes()
{
    string sizefile = get_conf ("PATH") + "/sizes";
    ifstream f(sizefile.c_str(), ifstream::in);
    string sizes((istreambuf_iterator<char>(f)),istreambuf_iterator<char>());
    f.close();
    return sizes;
}

WordList *Corpus::get_wordlist (const string &path)
{
    string name = path;
    size_t p = path.rfind('/');
    if (p != string::npos)
        name.erase (0, p);
    p = name.find('.');
    if (p != string::npos)
        name.erase (p);
    return ::get_wordlist (path, "", name, get_conf("LOCALE"), get_conf("ENCODING"));
}

struct freqitem {
    double freq;
    int id;
    string str;
    bool operator< (const freqitem& other) const {
        return freq > other.freq;
    }
};

inline void get_top (vector<freqitem> &out, int id, const string &str, NumOfPos frq, NumOfPos &out_count,
                     NumOfPos &out_total_frq, NumOfPos wlminfreq, NumOfPos wlmaxfreq, NumOfPos wlmaxitems)
{
    if (frq >= wlminfreq && (!wlmaxfreq || frq <= wlmaxfreq)) {
        out_count++;
        out_total_frq += frq;
        if (out.size() < (size_t) wlmaxitems) {
            out.push_back ({double(frq), id, str});
            push_heap (out.begin(), out.end());
        } else if (out.front().freq < frq) {
            pop_heap (out.begin(), out.end());
            out.pop_back();
            out.push_back ({double(frq), id, str});
            push_heap (out.begin(), out.end());
        }
    }
}

void wordlist (WordList *wl, const char *wlpat, vector<Frequency*> &addfreqs, Frequency *sortfreq,
               vector<string> &whitelist, vector<string> &blacklist,
			   NumOfPos wlminfreq, NumOfPos wlmaxfreq, NumOfPos wlmaxitems, const char *nonwordre,
               vector<string> &out_list, NumOfPos &out_count, NumOfPos &out_total_frq)
{
    unordered_set<string> *wlist = NULL;
    unordered_set<int> *blist = NULL;
    if (!whitelist.empty())
        wlist = new unordered_set<string>(whitelist.begin(), whitelist.end());
    NumOfPos maxitems = wlmaxitems;
    if (!blacklist.empty()) {
        maxitems += blacklist.size();
        blist = new unordered_set<int>();
        for (auto it = blacklist.begin(); it != blacklist.end(); it++) {
            int id = wl->str2id((*it).c_str());
            if (id != -1)
                blist->insert(id);
        }
    }
    out_count = 0;
    out_total_frq = 0;

    vector<freqitem> pairs;
    if (wlist) {
        for (auto it = wlist->begin(); it != wlist->end(); it++) {
            const string &str = *it;
            int id = wl->str2id (str.c_str());
            double frq = 0;
            if (id != -1)
                frq = sortfreq->freq (id);
            get_top (pairs, id, str, frq, out_count, out_total_frq, wlminfreq, wlmaxfreq, wlmaxitems);
        }
    } else {
        auto *gen = wl->regexp2strids (wlpat, 0, nonwordre);
        while (!gen->end()) {
            int id = gen->getId();
            get_top (pairs, id, gen->getStr(), sortfreq->freq (id), out_count, out_total_frq, wlminfreq, wlmaxfreq, maxitems);
            gen->next();
        }
        delete gen;
    }
    sort_heap (pairs.begin(), pairs.end());

    for (auto it = pairs.begin(); it != pairs.end(); it++) {
        if (!blist || blist->find((*it).id) == blist->end()) {
            ostringstream oss;
            oss << fixed << (*it).str << '\v' << (*it).freq;
            for (auto f = addfreqs.begin(); f != addfreqs.end(); f++) {
                double frq = 0;
                if ((*it).id != -1)
                    frq = (*f)->freq((*it).id);
                oss << '\v' << frq;
            }
            out_list.push_back (oss.str());
        }
    }
}

const char *manatee_version() {return VERSION_STR;}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
