// Copyright (c) 2015-2020  Milos Jakubicek

#include "writelex.hh"
#include <iostream>
#include <unistd.h>

using namespace std;

void usage (const char *progname, int buff)
{
    cerr << "usage: " << progname <<" [-b BUFF_SIZE] LEX_FILE_PATH" << endl
         << "Adds items to lexicon, appends if lexicon exists" << endl
         << "BUFF_SIZE is in-memory buffer and defaults to " << buff
         << " items" << endl
        ;
}

int main(int argc, char **argv)
{
    const char *progname = argv[0];
    string lexname;
    int buff_size = 50000;
    {
        // process options
        int c;
        while ((c = getopt (argc, argv, "?hb:")) != -1)
            switch (c) {
            case '?':
            case 'h':
                usage (progname, buff_size);
                return 2;
            case 'b':
                buff_size = atol (optarg);
                break;
            default:
                cerr << "unknown option (" << c << ")\n";
                usage (progname, buff_size);
                return 2;
            }

        if (optind < argc) {
            lexname = argv [optind++];
        } else {
            usage (progname, buff_size);
            return 2;
        }
    }

    try {
		write_lexicon lex (lexname, buff_size);
        string str;
        while (getline (cin, str))
            lex.str2id (str.c_str());
    } catch (exception &e) {
        cerr << progname << '[' << lexname << "]:" << e.what() << endl;
        return 1;
    }
    return 0;
}
// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
