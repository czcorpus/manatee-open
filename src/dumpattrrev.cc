#include <corpus.hh>
#include <posattr.hh>
#include "fromtof.hh"
#include <iostream>
#include <stdint.h>

int main (int argc, char **argv) {
    if (argc < 3) {
        cerr << "Usage: dumpattrrev CORPUS ATTR\n"
             << "Dumps reversed index of (virtual) corpus attribute\n"
             << "CORPUS    corpus configuration file\n"
             << "ATTR      attribute name\n";
        return 1;
    }
    Corpus c (argv[1]);
    PosAttr *pa;
    char *dot = strchr (argv[2], '.');
    if (dot) {
        *dot++ = '\0';
        Structure *struc = c.get_struct (argv[2]);
        pa = struc->get_attr (dot);
    } else
        pa = c.get_attr (argv[2]);
    ToFile<int64_t> out (stdout);
    for (int id = 0; id < pa->id_range(); id++) {
        FastStream *fs = pa->id2poss (id);
        Position fin = fs->final();
        Position pos;
        while ((pos = fs->peek()) < fin) {
            out.put (id);
            out.put (pos);
            fs->next();
        }
    }
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
