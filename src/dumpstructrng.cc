#include <corpus.hh>
#include "fromtof.hh"
#include <iostream>
#include <stdint.h>

template<class SType>
void dump_struct (ranges *rng, const char *outfile) {
    ToFile<SType> *out;
    if (*outfile == '-')
        out = new ToFile<SType> (stdout);
    else
        out = new ToFile<SType> (outfile);
    for (NumOfPos num = 0; num < rng->size(); num++) {
        out->put (rng->beg_at (num));
        out->put (rng->end_at (num));
    }
    delete out;
}

int main (int argc, const char **argv) {
    if (argc < 4) {
        cerr << "Usage: dumpstructrng CORPUS STRUCT OUTFILE\n"
             << "Dumps begin-end ranges of (virtual) corpus structure\n"
             << "CORPUS    corpus configuration file\n"
             << "STRUCT    corpus structure\n"
             << "OUTFILE   output file, use '-' for stdout\n";
        return 1;
    }
    Corpus c (argv[1]);
    Structure *s = c.get_struct (argv[2]);
    string stype = s->get_conf ("TYPE");
    if (!stype.empty() && stype.substr(stype.length() - 2) == "64")
        dump_struct<int64_t> (s->rng, argv[3]);
    else
        dump_struct<int32_t> (s->rng, argv[3]);
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
