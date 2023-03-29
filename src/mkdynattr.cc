// Copyright (c) 2001-2016  Pavel Rychly, Milos Jakubicek

#include "writelex.hh"
#include "consumer.hh"
#include "revidx.hh"
#include "dynfun.hh"
#include "posattr.hh"
#include "corpus.hh"
#include <iostream>
#include <system_error>
#include <stdio.h>


using namespace std;

// XXX dat nekam do finlib
inline void ensuresize (char *&ptr, int &allocsize, int needed, 
                        const char *where)
{
    if (allocsize >= needed) return;
    char *p = (char *)realloc (ptr, needed);
    if (!p)
        throw MemoryAllocationError (where);
    allocsize = needed;
    ptr = p;
}

void store_multival (int id, const char *str, const char *multisep, 
#ifdef LEXICON_USE_FSA3
                     write_fsalex &wl, RevFileConsumer *rev)
#else
                     write_lexicon &wl, RevFileConsumer *rev)
#endif

{
    int did;
    if (!multisep || *multisep == '\0') {
        // multisep = "" --> store characters
        char value[2];
        value[1] = 0;
        while (*str) {
            value[0] = *str;
            did = wl.str2id (value);
            rev->put (did, id);
        }
    } else {
        static int value_len = 0;
        static char *value = NULL;
        ensuresize (value, value_len, strlen (str) +1, "store_multival");
        value[0] = 0;
        strncat (value, str, value_len -1);
        for (char *s = strtok (value, multisep); s;
             s = strtok (NULL, multisep)) {
            did = wl.str2id (s);
            rev->put (did, id);
        }
    }
}


int main (int argc, char **argv) 
{
    if (argc < 3) {
        fputs ("usage: mkdynattr corpus dynattr\n", stderr);
        return 2;
    }
    try {
        Corpus c (argv[1]);
        CorpInfo *ci = c.conf;
        string attr = argv[2];
        string path = ci->opts["PATH"];
        int dotidx = attr.find ('.');
        string struct_name = "";
        string attr_name = attr;
        if (dotidx >= 0) {
            // struct_name.attr_name
            struct_name = string (attr, 0, dotidx);
            path = ci->find_struct (struct_name)->opts["PATH"];
            struct_name += ".";
            attr_name = string (attr, dotidx +1);
        }
        CorpInfo::MSS ao = ci->find_attr (attr);

        FILE *fp = 0;
        DynFun *fun = 0;
        if (ao["DYNLIB"] == "pipe") {
            string cmd = "lsclex " + string(argv[1]) + " " +
                struct_name + ao["FROMATTR"] + "|cut -f2|" + ao["DYNAMIC"];
            fp = popen(cmd.c_str(), "r");
            if(!fp) throw system_error(errno, generic_category());
        } else {
            fun = createDynFun (ao["FUNTYPE"].c_str(),
                        ao["DYNLIB"].c_str(), ao["DYNAMIC"].c_str(),
                        ao["ARG1"].c_str(), ao["ARG2"].c_str());
        }
        path += attr_name;
        PosAttr *at = c.get_attr (struct_name + ao["FROMATTR"], true);
        bool multival = ci->str2bool (ao["MULTIVALUE"]);
        char *multisep = NULL;
        if (multival)
            multisep = strdup (ao["MULTISEP"].c_str());

#ifdef LEXICON_USE_FSA3
        write_fsalex wl (path, true, false);
#else
        long lexicon_size = 500000;
        if (!ao["LEXICONSIZE"].empty())
            lexicon_size = atol (ao["LEXICONSIZE"].c_str());
        write_lexicon wl (path, lexicon_size, false);
#endif
        RevFileConsumer *rev = RevFileConsumer::create (path);
        ToFile<lexpos> *ridx = NULL;
        ridx = new ToFile<lexpos>(path + ".lex.ridx");
        
        int id = 0, id_range = at->id_range();
        int did;
        if (ao["DYNLIB"] == "pipe") {
            char *str = 0; size_t strsize = 0; int ret = 0;
            while((ret = getline(&str, &strsize, fp)) > 0) {
                if(id >= id_range) throw runtime_error("error: expected "
                    + to_string(id_range) + " values");
                char *eolpos;
                if((eolpos = strchr(str, '\n'))) *eolpos = 0;
                did = wl.str2id(str);
                if (multival)
                    store_multival (id, str, multisep, wl, rev);
                ridx->put(did);
                rev->put (did, id);
                id += 1;
            }
            if(errno) throw system_error(errno, generic_category());
            if(id < id_range) throw runtime_error("error: expected "
                + to_string(id_range) + " values, got " + to_string(id));
            ret = pclose(fp);
            if(ret == -1) throw system_error(errno, generic_category());
            ret = WEXITSTATUS(ret);
            if(ret) cerr << "warning: the command '" << ao["DYNAMIC"] <<
                "' exited with nonzero status: " << WEXITSTATUS(ret) << endl;
        } else {
            for (id = 0; id < id_range; id++) {
                const char *str = (*fun)(at->id2str (id));
                did = wl.str2id (str);
                rev->put (did, id);
                if (multival)
                    store_multival (id, str, multisep, wl, rev);
                ridx->put(did);
            }
            delete fun;
        }

        delete rev;
        delete ridx;
        if (ao["DYNTYPE"] == "freq") {
            ToFile<int64_t> frqf (path + ".freq");
            map_delta_revidx revf (path);
            for (id = 0; id < revf.maxid(); id++) {
                NumOfPos count = 0;
                FastStream *fs = revf.id2poss (id);
                while (fs->peek() < fs->final())
                    count += at->freq (fs->next());
                delete fs;
                frqf.put (count);
            }
        } else if ((long long) id_range > (long long) 100 * wl.size())
            cerr << "warning: the ratio between the dynamic attribute lexicon"
                 << " and original lexicon is more than 1:100\nconsider"
                 << " changing DYNTYPE to 'freq'\n"
                 << "Source attribute lexicon size: " << id_range << "\n"
                 << "Dynamic attribute lexicon size: " << wl.size() << "\n";
    } catch (exception &e) {
        cerr << "error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:

