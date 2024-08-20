// Copyright (c) 2004-2016  Pavel Rychly, Milos Jakubicek

#include "fromtof.hh"
#include "config.hh"
#include "subcorp.hh"
#include <iostream>
#include <cstdlib>
#include <unordered_map>

using namespace std;

typedef const pair<Position,Position> onerange;

namespace std {
    template <> struct hash<onerange> {
        size_t operator()(const onerange &r) const {
            return ph(r.first)*3 xor ph(r.second);
        }
    private: hash<Position> ph;
    };
}

int main (int argc, char **argv) 
{
    if (argc < 3) {
        cerr << "usage: mknorms CORPUS STRUCT [NORM_STRUCTATTR SUBCORPFILE]\n"
             << "(use - as NORM_STRUCTATTR to use structure size)\n"
             << "example: mknorms mycorp doc wordcount\n";
        return 2;
    }
    try {
        Corpus corp (argv[1]);
        Structure *st = corp.get_struct (argv[2]);
        unordered_map<onerange,uint64_t> norms;
        string normattrname;
        
        bool token_counts = not (argc >= 4 && argv[3][0] != '-');

        // init norms hash
        if (token_counts) {
            cerr << "preparing token counts ..." << endl;
            for (NumOfPos i = 0; i < st->size(); i++) {
                Position beg = st->rng->beg_at(i);
                Position end = st->rng->end_at(i);
                norms[onerange(beg,end)] += end - beg;
            }
        } else {
            cerr << "preparing norm counts ..." << endl;
            normattrname = argv[3];
            PosAttr *na = st->get_attr (normattrname);
            TextIterator *it = na->textat (0);
            for (NumOfPos i = 0; i < st->size(); i++)
                norms[onerange(st->rng->beg_at(i),st->rng->end_at(i))] = atoi (it->next());
            delete it;
        }
        SubCorpus *subc = NULL;
        string path = st->conf->opts["PATH"];
        if (argc == 5) {
            subc = new SubCorpus (&corp, argv[4]);
            path = string (argv[4], strlen (argv[4]) - 4) + st->name + '.';
        }

        // save norms
        CorpInfo::VSC::const_iterator sattr = st->conf->attrs.begin();
        for (; sattr != st->conf->attrs.end(); ++sattr) {
            const string &aname = sattr->first;
            string normf_path;
            if (token_counts) {
                cerr << "saving .token for " << aname << " ..." << endl;
                normf_path = path + aname + ".token";
            } else {
                cerr << "saving norms for " << aname << " ..." << endl;
                normf_path = path + aname + ".norm";
            }
            ToFile<int64_t> normf (normf_path);
            PosAttr *at = st->get_attr (aname);
            int id, id_range = at->id_range();
            for (id = 0; id < id_range; id++) {
                NumOfPos ntotal = 0;
                FastStream *pos = at->id2poss (id);
                RangeStream *rs = st->rng->part (pos);
                if (subc)
                    rs = subc->filter_query (rs);
                while (!rs->end()) {
                    ntotal += norms[onerange(rs->peek_beg(),rs->peek_end())];
                    rs->next();
                }
                delete pos;
                normf.put (ntotal);
            }
        }
    } catch (exception &e) {
        cerr << "mknorms error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
