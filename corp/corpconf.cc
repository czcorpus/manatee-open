//  Copyright (c) 1999-2016  Pavel Rychly, Milos Jakubicek

#include <config.hh>
#include "corpconf.hh"
#include "langcodes.cc"
#include <libgen.h>
#include <climits>
#include <cstdlib>

CorpInfo::CorpInfo (type_t t, bool no_defaults)
    :no_defaults (no_defaults), type (t)
{
}

CorpInfo::CorpInfo (CorpInfo *x)
    :no_defaults (x->no_defaults), type (x->type), conffile (x->conffile)
{
    for (MSS::iterator i = x->opts.begin(); i != x->opts.end(); i++)
        opts [(*i).first] = (*i).second;
    for (VSC::iterator i = x->attrs.begin(); i != x->attrs.end(); i++)
        attrs.push_back (pair<string,CorpInfo*>((*i).first, 
                                                new CorpInfo ((*i).second)));
    for (VSC::iterator i = x->structs.begin(); i != x->structs.end(); i++)
        structs.push_back (pair<string,CorpInfo*>((*i).first, 
                                                  new CorpInfo ((*i).second)));
    for (VSC::iterator i = x->procs.begin(); i != x->procs.end(); i++)
        procs.push_back (pair<string,CorpInfo*>((*i).first, 
                                                new CorpInfo ((*i).second)));
}

CorpInfo::~CorpInfo () {
    for (VSC::iterator i = attrs.begin(); i != attrs.end(); i++)
        delete (*i).second;
    for (VSC::iterator i = structs.begin(); i != structs.end(); i++)
        delete (*i).second;
    for (VSC::iterator i = procs.begin(); i != procs.end(); i++)
        delete (*i).second;
}

inline void enquote (ostringstream &oss, const string &val)
{
    bool has_quotes = false;
    if (val.find('"') != string::npos)
        has_quotes = true;
    oss << (has_quotes ? " '" : " \"");
    oss << val;
    oss << (has_quotes ? "'\n" : "\"\n");
}

string CorpInfo::dump (int indent) {
    ostringstream oss;
    string space;
    space.resize (indent, ' ');
    for (MSS::iterator i = opts.begin(); i != opts.end(); i++) {
        oss << space << (*i).first;
        enquote (oss, (*i).second);
    }
    for (VSC::iterator i = attrs.begin(); i != attrs.end(); i++)
        oss << space << "ATTRIBUTE \"" << (*i).first << "\" {\n"
            << (*i).second->dump (indent +4) << space << "}\n";
    for (VSC::iterator i = structs.begin(); i != structs.end(); i++)
        oss << space << "STRUCTURE \"" << (*i).first << "\" {\n"
            << (*i).second->dump (indent +4) << space << "}\n";
    for (VSC::iterator i = procs.begin(); i != procs.end(); i++)
        oss << space << "PROCESS \"" << (*i).first << "\" {\n"
            << (*i).second->dump (indent +4) << space << "}\n";
    return oss.str();
}

static void set_defs_from_array (CorpInfo::MSS &opts, 
                                 const char * const *opt_defs)
{
    while (*opt_defs) {
        if (opts [opt_defs[0]] == "")
            opts [opt_defs[0]] = opt_defs[1];
        opt_defs +=2;
    }
}

static void update_relative_path (CorpInfo::MSS &opts, string conffile, const char *opt)
{
    if (!opts[opt].empty() && opts[opt][0] == '.') {
        char registry_realpath[PATH_MAX];
        if (realpath(conffile.c_str(), registry_realpath) == 0)
            throw runtime_error("realpath failed");
        char *registry_dirname = dirname(registry_realpath);

        opts[opt] = string(registry_dirname) + "/" + opts[opt];
    }
}

void CorpInfo::set_defaults (CorpInfo *global, type_t d_type) {
    conffile = global->conffile;
    if (no_defaults)
        return;
    static const char *attr_defs[] 
        = {"ENCODING", "", "MULTIVALUE", "n", "MULTISEP", ",", 
           "TYPE", "default", "DYNTYPE", "index",
           NULL};
    static const char *struct_attr_defs[] = {"TYPE", "MD_MI", NULL};

    const char **opt_defs = NULL;
    if (type == Unset_type)
        type = d_type;
    switch (type) {
    case Corpus_type:
        if (opts["DEFAULTATTR"].empty()) {
            for (unsigned i = 0; i < attrs.size(); i++) {
                if (attrs[i].second->opts["DYNAMIC"].empty()) {
                    opts["DEFAULTATTR"] = attrs[i].first;
                    break;
                }
            }
            if (opts["DEFAULTATTR"].empty())
                opts["DEFAULTATTR"] = "word";
        }
        if (!opts["PATH"].empty()) {
            string path = opts ["PATH"];
            if (*(path.end() -1) != '/') {
                path += '/';
                opts ["PATH"] = path;
            }
        }
        update_relative_path(opts, global->conffile, "PATH");
        if (opts["SUBCBASE"] == "") {
            opts["SUBCBASE"] = opts["PATH"] + "subcorp/";
        }
        update_relative_path(opts, global->conffile, "SUBCBASE");
        if ((opts["ENCODING"] == "utf-8") || 
            !strcasecmp(opts["ENCODING"].c_str(), "utf8")) {
            opts["ENCODING"] = "UTF-8";
        }
        if (opts["DOCSTRUCTURE"] == "") {
            opts["DOCSTRUCTURE"] = "doc";
        }
        if (opts["NONWORDRE"] == "") {
            opts["NONWORDRE"] = "[^[:alpha:]].*";
        }
        if (opts["MAXKWIC"] == "") {
            opts["MAXKWIC"] = "100";
        }
        if (opts["MAXCONTEXT"] == "") {
            opts["MAXCONTEXT"] = "100";
        }
        if (opts["MAXDETAIL"] == "") {
            opts["MAXDETAIL"] = opts["MAXCONTEXT"];
        }
        if (opts["ALIGNSTRUCT"] == "") {
            opts["ALIGNSTRUCT"] = "align";
        }
        if (opts["SIMPLEQUERY"] == "") {
            ostringstream oss;
            oss << '[';
            bool has_lc = false, has_lemma_lc = false,
                 has_word = false, has_lemma = false;
            for (unsigned i = 0; i < attrs.size(); i++) {
                if (attrs[i].first == "lc")
                    has_lc = true;
                else if (attrs[i].first == "lemma_lc")
                    has_lemma_lc = true;
                else if (attrs[i].first == "word")
                    has_word = true;
                else if (attrs[i].first == "lemma")
                    has_lemma = true;
            }
            if (has_lc)
                oss << "lc=\"%s\"";
            else if (has_word)
                oss << "word=\"%s\"";
            else
                oss << opts["DEFAULTATTR"] << "=\"%s\"";
            if (has_lemma_lc)
                oss << " | lemma_lc=\"%s\"";
            else if (has_lemma)
                oss << " | lemma=\"%s\"";
            oss << ']';
            opts["SIMPLEQUERY"] = oss.str();
        }
        if (opts["FREQTTATTRS"].empty()) {
            opts["FREQTTATTRS"] = opts["SUBCORPATTRS"];
        }
        if (opts["BIDICTATTR"].empty()) {
            while (1) {
                try {
                    find_sub ("lemma_lc", attrs);
                    opts["BIDICTATTR"] = "lemma_lc";
                    break;
                } catch (CorpInfoNotFound &e){}
                try {
                    find_sub ("lemma", attrs);
                    opts["BIDICTATTR"] = "lemma";
                    break;
                } catch (CorpInfoNotFound &e){}
                try {
                    find_sub ("lc", attrs);
                    opts["BIDICTATTR"] = "lc";
                    break;
                } catch (CorpInfoNotFound &e){}
                try {
                    find_sub ("word", attrs);
                    opts["BIDICTATTR"] = "word";
                    break;
                } catch (CorpInfoNotFound &e){}
                opts["BIDICTATTR"] = opts["DEFAULTATTR"];
                break;
            }
        }
#ifdef SKETCH_ENGINE
        if (opts["TERMBASE"].empty())
            opts["TERMBASE"] = opts["PATH"] + "terms";
        if (opts["WSMINHITS"].empty())
            opts["WSMINHITS"] = "0";
        if (opts["WSATTR"].empty()) {
            while (1) {
            try {
                find_sub ("lempos_lc", attrs);
                opts["WSATTR"] = "lempos_lc";
                break;
            } catch (CorpInfoNotFound &e){}
            try {
                find_sub ("lempos", attrs);
                opts["WSATTR"] = "lempos";
                break;
            } catch (CorpInfoNotFound &e){}
            try {
                find_sub ("lemma_lc", attrs);
                opts["WSATTR"] = "lemma_lc";
                break;
            } catch (CorpInfoNotFound &e){}
            try {
                find_sub ("lemma", attrs);
                opts["WSATTR"] = "lemma";
                break;
            } catch (CorpInfoNotFound &e){}
            opts["WSATTR"] = opts["DEFAULTATTR"];
            break;
            }
        }
        if (opts["WSBASE"].empty())
            opts["WSBASE"] = opts["PATH"] + opts["WSATTR"] + "-ws";
        if (opts["WSTHES"].empty())
            opts["WSTHES"] = opts["PATH"] + opts["WSATTR"] + "-thes";
        if (opts["WSSTRIP"].empty()) {
            string &wsattr = opts["WSATTR"];
            if (wsattr == "lempos" || wsattr == "lempos_lc")
                opts["WSSTRIP"] = "2";
            else
                opts["WSSTRIP"] = "0";
        }
        if (opts["WSPOSLIST"].empty())
            opts["WSPOSLIST"] = opts["LPOSLIST"];
        if (opts["DIAPOSATTRS"].empty())
            opts["DIAPOSATTRS"] = find_opt("ATTRLIST");
        update_relative_path(opts, global->conffile, "WSDEF");
        update_relative_path(opts, global->conffile, "WSBASE");
        update_relative_path(opts, global->conffile, "WSTHES");
        update_relative_path(opts, global->conffile, "TERMDEF");
        update_relative_path(opts, global->conffile, "TERMBASE");
        update_relative_path(opts, global->conffile, "HISTDEF");
        update_relative_path(opts, global->conffile, "GDEXDEFAULTCONF");
#endif
        update_relative_path(opts, global->conffile, "SUBCDEF");
        update_relative_path(opts, global->conffile, "VERTICAL");

        for (VSC::iterator i = attrs.begin(); i != attrs.end(); i++)
            (*i).second->set_defaults (this, Attr_type);
        for (VSC::iterator i = structs.begin(); i != structs.end(); i++) {
            (*i).second->opts["PATH"] = opts ["PATH"] + (*i).first + '.';
            (*i).second->set_defaults (this, Struct_type);
        }
        for (VSC::iterator i = procs.begin(); i != procs.end(); i++)
            (*i).second->set_defaults (this, Proc_type);

        break;
    case Attr_type:
        opt_defs = attr_defs;
        if (str2bool (global->opts["NESTED"]))
            opts["MULTIVALUE"] = "y";
        if (!opts["DYNLIB"].empty()) {
            if (opts["DYNLIB"] == "pipe") {
                update_relative_path(opts, global->conffile, "DYNAMIC");
            } else {
                update_relative_path(opts, global->conffile, "DYNLIB");
            }
        }
        opts["ENCODING"] = global->opts["ENCODING"];
        if (opts["LOCALE"].empty()) {
            if (!global->opts["DEFAULTLOCALE"].empty()) {
                opts["LOCALE"] = global->opts["DEFAULTLOCALE"];
                break;
            }
            if (!global->opts["LANGUAGE"].empty()) {
                auto langcode = langcodes.find(global->opts["LANGUAGE"]);
                if (langcode != langcodes.end())
                    try {
                        opts["LOCALE"] = langcode->second + "." +
                                         global->opts["ENCODING"];
                        locale(opts["LOCALE"].c_str()); // test availability
                        global->opts["DEFAULTLOCALE"] = opts["LOCALE"];
                        break;
                    } catch (...) {}
            }
            opts["LOCALE"] = global->opts["DEFAULTLOCALE"] = "C";
        }
        break;
    case Struct_type:
        opts["ENCODING"] = global->opts["ENCODING"];
        this->conffile = global->conffile;
        for (VSC::iterator i = attrs.begin(); i != attrs.end(); i++) {
            set_defs_from_array ((*i).second->opts, struct_attr_defs);
            (*i).second->set_defaults (this, Attr_type);
        }
        break;
    case Proc_type:
        /* nothing to do */
        break;
    case Unset_type:
        /* dead code */ break;
    }
    if (opt_defs)
        set_defs_from_array (opts, opt_defs);
}

CorpInfo* CorpInfo::find_sub (const string &val, VSC &v)
{
    for (VSC::iterator i = v.begin(); i != v.end(); i++) {
        if ((*i).first == val)
            return (*i).second;
    }
    throw CorpInfoNotFound (val);
}

CorpInfo::MSS& CorpInfo::find_attr (const string &attr)
{
    int dotidx = attr.find ('.');
    if (dotidx >= 0) {
        // struct_name.attr_name
        string struct_name (attr, 0, dotidx);
        string attr_name (attr, dotidx +1);
        return find_sub (attr_name, 
                         find_sub (struct_name, structs)->attrs)->opts;
    } else 
        return find_sub (attr, attrs)->opts;
}

CorpInfo* CorpInfo::add_attr (const string &path)
{
    int i;
    CorpInfo *ci;
    string attr_name;
    if ((i = path.find ('.')) >= 0) {
        string struct_name (path, 0, i);
        attr_name = string (path, i+1);
        ci = add_struct (struct_name);
        opts["STRUCTATTRLIST"] = "";
    } else {
        attr_name = path;
        ci = this;
        opts["ATTRLIST"] = "";
    }
    try {
        return find_sub (attr_name, ci->attrs);
    } catch (CorpInfoNotFound &e) {}
    CorpInfo* attr_ci = new CorpInfo();
    attr_ci->set_defaults (this, Attr_type);
    ci->attrs.push_back (pair<string,CorpInfo*>(attr_name, attr_ci));
    if (i >= 0)
        find_opt ("STRUCTATTRLIST");
    else
        find_opt ("ATTRLIST");
    return attr_ci;
}

CorpInfo* CorpInfo::add_struct (const string &path)
{
    try {
        return find_sub (path, structs);
    } catch (CorpInfoNotFound &e) {}
    CorpInfo* struct_ci = new CorpInfo();
    structs.push_back (pair<string,CorpInfo*>(path, struct_ci));
    struct_ci->set_defaults (this, Struct_type);
    opts["STRUCTLIST"] = "";
    find_opt ("STRUCTLIST");
    return struct_ci;
}

void CorpInfo::remove_attr (const string &path)
{
    CorpInfo *ci;
    string attr_name;
    int i;
    if ((i = path.find ('.')) >= 0) {
        string struct_name (path, 0, i);
        attr_name = string (path, i+1);
        ci = add_struct (struct_name);
        opts["STRUCTATTRLIST"] = "";
    } else {
        attr_name = path;
        ci = this;
        opts["ATTRLIST"] = "";
    }
    CorpInfo *attr_ci = find_sub (attr_name, ci->attrs);
    for (VSC::iterator it = ci->attrs.begin(); it != ci->attrs.end(); it++) {
        if ((*it).second == attr_ci) {
            ci->attrs.erase (it);
            break;
        }
    }
    if (i >= 0)
        find_opt ("STRUCTATTRLIST");
    else
        find_opt ("ATTRLIST");
    delete attr_ci;
}

void CorpInfo::remove_struct (const string &struc)
{
    CorpInfo *struc_ci = find_sub (struc, structs);
    opts["STRUCTLIST"] = "";
    for (VSC::iterator it = structs.begin(); it != structs.end(); it++) {
        if ((*it).second == struc_ci) {
            structs.erase (it);
            break;
        }
    }
    find_opt ("STRUCTLIST");
    delete struc_ci;
}

void CorpInfo::set_opt (const string &path, const string &val)
{
    int i;
    if ((i = path.find ('.')) >= 0) {
        string first (path, 0, i);
        string rest (path, i+1);
        try {
            find_attr (first)[rest] = val;
            return;
        } catch (CorpInfoNotFound &e) {
            try {
                find_sub (first, procs)->opts [rest] = val;
                return;
            } catch (CorpInfoNotFound &e) {
                return find_struct (first)->set_opt (rest, val);
            }
        }
    }
    opts[path] = val;
}

string &CorpInfo::find_opt (const string &path)
{
    int i;
    if ((i = path.find ('.')) >= 0) {
        string first (path, 0, i);
        string rest (path, i+1);
        try {
            return find_attr (first)[rest];
        } catch (CorpInfoNotFound &e) {
            try {
                return find_sub (first, procs)->opts [rest];
            } catch (CorpInfoNotFound &e) {
                return find_struct (first)->find_opt (rest);
            }
        }
    } else {
        string val = opts[path];
        if (val.empty()) {
            if (path == "FULLREF") {
                val = find_opt ("STRUCTATTRLIST");
                if (val.empty())
                    val = "#";
            } else if (path == "SHORTREF") {
                string doc = find_opt ("DOCSTRUCTURE");
                try {
                    find_attr(doc + ".website");
                    val = '=' + doc + ".website";
                } catch (CorpInfoNotFound&) {
                    try {
                        find_struct(doc);
                        val = doc;
                    } catch (CorpInfoNotFound&) {
                        string refs = find_opt ("STRUCTATTRLIST");
                        if ((i = refs.find (',')) >= 0)
                            refs.erase (i);
                        if (refs.empty())
                            val = "#";
                        else
                            val = '=' + refs;
                    }
                }
            } else if (path == "ATTRLIST") {
                for (VSC::iterator j = attrs.begin(); j != attrs.end(); j++)
                    val += (*j).first + ',';
            } else if (path == "STRUCTLIST") {
                for (VSC::iterator j=structs.begin(); j != structs.end(); j++)
                    val += (*j).first + ',';
            } else if (path == "STRUCTATTRLIST") {
                for (VSC::iterator j=structs.begin(); j != structs.end(); j++)
                    for (VSC::iterator k = (*j).second->attrs.begin(); 
                         k != (*j).second->attrs.end(); k++)
                        val += (*j).first + '.' + (*k).first + ',';
            } else if (path == "PROCLIST") {
                for (VSC::iterator j=procs.begin(); j != procs.end(); j++)
                    val += (*j).first + ',';
            } 
            if (!val.empty() && *(val.end() -1) == ',')
                val.erase (val.length() -1);
        }
        return (opts[path] = val);
    }
}


bool CorpInfo::str2bool (const string &str)
{
    if (str == "y" || str == "yes" || str == "t" || str == "true" || 
        str == "1")
        return true;
    if (str == "n" || str == "no" || str == "f" || str == "false" || 
        str == "0")
        return false;
    //throw xxx;
    return false;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:

