// Copyright (c) 2015-2020  Milos Jakubicek

#include "writelex.hh"
#include <iostream>
#include <unistd.h>

using namespace std;

void usage (const char *progname)
{
    cerr << "usage: " << progname <<" [-a] [-l] [-n] LEX_FILE_PATH" << endl
         << "Adds items to FSA-based lexicon" << endl
         << "\t-l\twrite .lex and .lex.idx" << endl
         << "\t-a\tappend to an existing lexicon instead of overwriting it" << endl
         << "\t-n\toutput the ids of the stored elements" << endl
        ;
}

int main(int argc, char **argv)
{
    const char *progname = argv[0];
    string lexname;
    bool append = false, write_lexfiles = false, write_id = false;
    {
        // process options
        int c;
        while ((c = getopt (argc, argv, "?hlan")) != -1)
            switch (c) {
            case '?':
            case 'h':
                usage (progname);
                return 2;
            case 'l':
                write_lexfiles = true;
                break;
            case 'a':
                append = true;
                break;
            case 'n':
                write_id = true;
                break;
            default:
                cerr << "unknown option (" << c << ")\n";
                usage (progname);
                return 2;
            }

        if (optind < argc) {
            lexname = argv [optind++];
        } else {
            usage (progname);
            return 2;
        }
    }

    try {
		write_fsalex lex (lexname, write_lexfiles, append);
        string str;
        while (getline (cin, str)) {
            int id = lex.str2id (str.c_str());
            if (write_id)
                cout << id << "\n";
        }
    } catch (exception &e) {
        cerr << progname << '[' << lexname << "]:" << e.what() << endl;
        return 1;
    }
    return 0;
}
// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
