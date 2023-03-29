//  Copyright (c) 1999-2016  Pavel Rychly, Milos Jakubicek

#ifndef CORPCONF_HH
#define CORPCONF_HH

#include "excep.hh"
#include <map>
#include <vector>
#include <exception>

class CorpInfo {
    bool no_defaults;
public:
    typedef enum {Unset_type, Corpus_type, Attr_type, Struct_type, 
                  Proc_type} type_t;
    type_t type;
    typedef std::map<std::string,std::string> MSS;
    typedef std::vector<std::pair<std::string,CorpInfo*> > VSC;
protected:
    CorpInfo* find_sub (const std::string &val, VSC &v);
public:
    MSS opts;
    VSC attrs;
    VSC structs;
    VSC procs;
    std::string conffile;

    CorpInfo (type_t t=Unset_type, bool no_defaults=false);
    CorpInfo (CorpInfo *x);
    ~CorpInfo ();
    std::string dump (int indent=0);
    void set_defaults(CorpInfo *global, type_t d_type);
    MSS& find_attr (const std::string &attr);
    CorpInfo* find_struct (const std::string &attr)
        {return find_sub (attr, structs);}
    CorpInfo* add_attr (const std::string &path);
    CorpInfo* add_struct (const std::string &path);
    void remove_attr (const std::string &path);
    void remove_struct (const std::string &struc);
    void set_opt (const std::string &path, const std::string &val);
    std::string &find_opt (const std::string &path);
    static bool str2bool (const std::string &str);

};


CorpInfo *loadCorpInfo (const std::string &corp_name_or_path,
                        bool no_defaults = false);
CorpInfo *loadCorpInfoDefaults (const std::string &corp_name_or_path);
CorpInfo *loadCorpInfoNoDefaults (const std::string &corp_name_or_path);

void languages (std::vector<std::string> &out);

#endif

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
