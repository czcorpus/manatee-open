// Copyright (c) 2013-2020  Pavel Rychly, Milos Jakubicek

#include "revidx.hh"
#include <iostream>
#include <cstdlib>

using namespace std;
void usage()
{
    cerr << "usage: dumpdrev [-c] REV_PATH [ID]\n"
         << "       -c print collocation from CollDeltaRev\n"
         << "       -s only print number of items for each ID\n"
         << "       for ID -1 (same as not given) print rev for all IDs in binary, unless -s\n"
         << "       for ID != -1 output is always text\n";
}

int main (int argc, char **argv)
{
    bool coll = false;
    bool only_counts = false;
    int c;
    const char *revpath = NULL;
    int id = -1;
    while ((c = getopt (argc, argv, "?hsc")) != -1)
        switch (c) {
        case 'c':
            coll = true;
            break;
        case 's':
            only_counts = true;
            break;
        case 'h':
        case '?':
            usage();
            return 2;
        default:
            cerr << "dumpdrev: unknown option (" << c << ")" << endl;
            usage();
            return 2;
        }

    if (optind < argc) {
        revpath = argv [optind];
        if (optind + 1 < argc)
            id = atoi(argv[optind + 1]);
    } else {
        usage();
        return 2;
    }

    try {
        if (coll) {
            map_colldelta_revidx rev (revpath);
            if (id == -1) {
                ToFile<int64_t> out (stdout);
                for (int i = 0; i < rev.maxid(); i++) {
                    NumOfPos cnt = rev.count (i);
                    if (only_counts) {
                        cout << cnt << "\n";
                        continue;
                    }
                    FastStream *fs = rev.id2poss (i);
                    Labels lab;
                    while (cnt--) {
                        fs->add_labels (lab);
                        out.put (i);
                        out.put (fs->next());
                        out.put (lab[1]);
                    }
                    delete fs;
                }
            } else {
                NumOfPos cnt = rev.count (id);
                if (only_counts) {
                    cout << cnt << "\n";
                } else {
                    FastStream *fs = rev.id2poss (id);
                    Labels lab;
                    while (cnt--) {
                        fs->add_labels (lab);
                        cout << fs->next() << '\t' << lab[1] << '\n';
                    }
                    delete fs;
                }
            }
        } else {
            map_delta_revidx rev (revpath);
            if (id == -1) {
                ToFile<int64_t> out (stdout);
                for (int i = 0; i < rev.maxid(); i++) {
                    NumOfPos cnt = rev.count (i);
                    if (only_counts) {
                        cout << cnt << "\n";
                        continue;
                    }
                    FastStream *fs = rev.id2poss (i);
                    while (cnt--) {
                        out.put (i);
                        out.put (fs->next());
                    }
                    delete fs;
                }
            } else {
                NumOfPos cnt = rev.count (id);
                if (only_counts) {
                    cout << cnt << "\n";
                } else {
                    FastStream *fs = rev.id2poss (id);
                    while (cnt--)
                        cout << fs->next() << '\n';
                    delete fs;
                }
            }
        }
    } catch (exception &e) {
        cerr << e.what() << endl;
        return 1;
    }
    return 0;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
