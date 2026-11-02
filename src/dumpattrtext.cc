#include <corpus.hh>
#include <posattr.hh>
#include "fromtof.hh"
#include <iostream>
#include <stdint.h>
#include <vector>

int main (int argc, char **argv) {
    bool sets = argc > 1 && !strcmp (argv[1], "-s");
    if (sets) {
        argc--;
        argv++;
    }
    if (argc < 3) {
        cerr << "Usage: dumpattrtext [-s] CORPUS ATTR\n"
             << "Dumps text of (virtual) corpus attribute\n"
             << "-s        dump the set of ids at every position (count\n"
             << "          followed by the ids), for TYPE BITSET attributes\n"
             << "CORPUS    corpus configuration file\n"
             << "ATTR      attribute name\n";
        return 1;
    }
    Corpus c (argv[1]);
    PosAttr *pa;
    NumOfPos max_pos = c.size();
    char *dot = strchr (argv[2], '.');
    if (dot) {
        *dot++ = '\0';
        Structure *struc = c.get_struct (argv[2]);
        max_pos = struc->size();
        pa = struc->get_attr (dot);
    } else
        pa = c.get_attr (argv[2]);
    ToFile <int32_t> out (stdout);
    if (sets) {
        std::vector<int> ids;
        for (NumOfPos p = 0; p < max_pos; p++) {
            pa->posset (p, ids);
            out.put (ids.size());
            for (size_t i = 0; i < ids.size(); i++)
                out.put (ids[i]);
        }
        return 0;
    }
    IDIterator *it = pa->posat(0);
    for (NumOfPos p = 0; p < max_pos; p++)
        out.put (it->next());
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
