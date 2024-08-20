// Copyright (c) 2022  Milos Jakubicek

#include "revidx.hh"
#include <finlib/lexicon.hh>
#include <finlib/writelex.hh>
#include <iostream>
#include <cstdlib>

using namespace std;
void usage()
{
    cerr << "usage: filterlexrev [-c] REVLEX_SRCPATH REVLEX_DSTPATH MINFREQ\n"
         << "       -c rev has collocation from CollDeltaRev\n";
}

int main (int argc, char **argv)
{
    bool coll = false;
    int c;
    const char *srcrevpath = NULL;
    const char *dstrevpath = NULL;
    NumOfPos minfreq = 0;
    while ((c = getopt (argc, argv, "?hsc")) != -1)
        switch (c) {
        case 'c':
            coll = true;
            break;
        case 'h':
        case '?':
            usage();
            return 2;
        default:
            cerr << "filterlexrev: unknown option (" << c << ")" << endl;
            usage();
            return 2;
        }

    if (optind + 2 < argc) {
        srcrevpath = argv [optind++];
        dstrevpath = argv [optind++];
        minfreq = atoll(argv [optind]);
    } else {
        usage();
        return 2;
    }

    FromFile<float> *f_arf = NULL, *f_aldf = NULL;
    ToFile<float> *t_arf = NULL, *t_aldf = NULL;
    FromFile<uint32_t> *f_docf = NULL;
    ToFile<uint32_t> *t_docf = NULL;
    try {
        f_arf = new FromFile<float> (string(srcrevpath) + ".arf");
        t_arf = new ToFile<float> (string(dstrevpath) + ".arf");
    } catch (FileAccessError&) {}
    try {
        f_aldf = new FromFile<float> (string(srcrevpath) + ".aldf");
        t_aldf = new ToFile<float> (string(dstrevpath) + ".aldf");
    } catch (FileAccessError&) {}
    try {
        f_docf = new FromFile<uint32_t> (string(srcrevpath) + ".docf");
        t_docf = new ToFile<uint32_t> (string(dstrevpath) + ".docf");
    } catch (FileAccessError&) {}

    try {
        lexicon *srclex = new_lexicon (srcrevpath);
        write_fsalex *dstlex = new write_fsalex (dstrevpath, false);
        auto *it = srclex->pref2strids("");
        float arf = 0, aldf = 0;
        uint32_t docf = 0;
        if (coll) {
            map_colldelta_revidx rev (srcrevpath);
            ToFile<int64_t> out (stdout);
            for (int i = 0, out_i = 0; i < rev.maxid(); i++) {
                NumOfPos cnt = rev.count (i);
                if (f_arf)
                    arf = f_arf->get();
                if (f_aldf)
                    aldf = f_aldf->get();
                if (f_docf)
                    docf = f_docf->get();
                if (cnt >= minfreq) {
                    dstlex->str2id(it->getStr().c_str());
                    FastStream *fs = rev.id2poss (i);
                    Labels lab;
                    while (cnt--) {
                        fs->add_labels (lab);
                        out.put (out_i);
                        out.put (fs->next());
                        out.put (lab[1]);
                    }
                    delete fs;
                    if (t_arf)
                        t_arf->put(arf);
                    if (t_aldf)
                        t_aldf->put(aldf);
                    if (t_docf)
                        t_docf->put(docf);
                    out_i++;
                }
                it->next();
            }
        } else {
            map_delta_revidx rev (srcrevpath);
            ToFile<int64_t> out (stdout);
            for (int i = 0, out_i = 0; i < rev.maxid(); i++) {
                NumOfPos cnt = rev.count (i);
                if (cnt >= minfreq) {
                    dstlex->str2id(it->getStr().c_str());
                    FastStream *fs = rev.id2poss (i);
                    while (cnt--) {
                        out.put (out_i);
                        out.put (fs->next());
                    }
                    delete fs;
                    out_i++;
                }
                it->next();
            }
        }
        delete srclex;
        delete dstlex;
    } catch (exception &e) {
        cerr << e.what() << endl;
        return 1;
    }
    delete t_arf;
    delete t_aldf;
    delete t_docf;
    return 0;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
