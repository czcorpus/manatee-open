// Copyright (c) 2015 Milos Jakubicek

#include "corpus.hh"
#include <iostream>

int main (int argc, char **argv)
{
    if (argc < 3) {
        cerr << "Usage: lsalsize <CORPUS1> <CORPUS2>\n";
        return 1;
    }
    Corpus c1 (argv[1]);
    Corpus c2 (argv[2]);
    Structure *s = c1.get_struct(c1.get_conf("ALIGNSTRUCT"));
    Structure *als = c2.get_struct (c2.get_conf("ALIGNSTRUCT"));

    FastStream *snums = new SequenceStream(0, als->rng->size() - 1,
                                           als->rng->size());
    if (! c2.get_conf("ALIGNDEF").empty())
        snums = tolevelfs (c2.get_aligned_level (c1.get_conffile()), snums);
    RangeStream *rs = s->rng->part (snums);
    NumOfPos c1_size = 0;
    Position last_end = 0;
    while (!rs->end()) {
        Position end = rs->peek_end();
        if (end > last_end)
            c1_size += end - rs->peek_beg();
        last_end = end;
        rs->next();
    }

    snums = new SequenceStream(0, s->rng->size() - 1, s->rng->size());
    if (! c1.get_conf("ALIGNDEF").empty())
        snums = tolevelfs (c1.get_aligned_level (c2.get_conffile()), snums);
    rs = als->rng->part (snums);
    NumOfPos c2_size = 0;
    last_end = 0;
    while (!rs->end()) {
        Position end = rs->peek_end();
        if (end > last_end)
            c2_size += end - rs->peek_beg();
        last_end = end;
        rs->next();
    }

    cout << c1.get_conffile() << " " << c1_size << " "
         << c2.get_conffile() << " " << c2_size << "\n";
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
