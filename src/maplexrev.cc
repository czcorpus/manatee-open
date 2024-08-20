// Copyright (c) 1999-2020  Pavel Rychly, Milos Jakubicek

#include "binfile.hh"
#include <config.hh>
#include <finlib/fromtof.hh>
#include <finlib/lexicon.hh>
#include <iostream>

using namespace std;


static void usage (const char *progname) {
    cerr << "usage: " << progname << " [-c] PATH_TO_lex.isrt\n"
         << "    -c    input contains one collocation\n";
}


int main (int argc, char **argv)
{
    bool colls = false;
    const char *progname = argv[0];
    const char *lexpath = NULL;
    {
        // process options
        int c;
        while ((c = getopt (argc, argv, "h?c")) != -1)
            switch (c) {
            case 'c':
                colls = true;
                break;
            default:
                cerr << "unknown option (" << c << ")\n";
                usage (progname);
                return 2;
            }

        if (optind < argc)
            lexpath = argv [optind++];
        else {
            usage (progname);
            return 2;
        }
    }

    try {
        FromFile<int64_t> ci (stdin);
        ToFile<int64_t> co (stdout);
        MapBinFile<lexpos> isrt (lexpath);
        int64_t id, pos, coll;
        if (colls) {
            while (!ci) {
                id = *ci;
                ++ci;
                pos = *ci;
                ++ci;
                coll = *ci;
                ++ci;
                co(isrt[id]);
                co(pos);
                co(coll);
            }
        } else {
            while (!ci) {
                id = *ci;
                ++ci;
                pos = *ci;
                ++ci;
                co(isrt[id]);
                co(pos);
            }
        }
    } catch (exception &e) {
        cerr << e.what() << endl;
        return 1;
    }
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
