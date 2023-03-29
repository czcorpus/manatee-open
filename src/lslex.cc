// Copyright (c) 2000-2020  Pavel Rychly, Milos Jakubicek

#include <config.hh>
#include "lexicon.hh"
#include "writelex.hh"
#include "fromtof.hh"
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>

using namespace std;

void usage (const char *progname)
{
    cerr << "usage: " << progname <<" [-s|-n|-m] LEX_FILE_PATH" << endl
         << "     -s          str2id" << endl
         << "     -n          id2str" << endl
         << "     -m          make lex.srt file" << endl
         << "     -S          print alphabetically sorted" << endl
        ;
}

int main(int argc, char **argv)
{
    const char *progname = argv[0];
    string lexname;

    bool id2str = false;
    bool str2id = false;
    bool make_lex_srt = false;
    bool sorted = false;
    {
        // process options
        int c;
        while ((c = getopt (argc, argv, "?hnsSm")) != -1)
            switch (c) {
            case 'n':
                id2str = true;
                break;
            case 's':
                str2id = true;
                break;
            case 'm':
                make_lex_srt = true;
                break;
            case 'S':
                sorted = true;
                break;
            case '?':
            case 'h':
                usage (progname);
                return 2;
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
        if (make_lex_srt) {
            struct stat st;
            if (stat ((lexname + ".lex.srt").c_str(), &st) < 0) {
                if (stat ((lexname + ".lex.idx").c_str(), &st) < 0)
                    return 2;
                ToFile<int32_t> tf ((lexname + ".lex.srt").c_str());
                for (int i = 0; i < st.st_size/4; i++)
                    tf(i);
            }
            make_lex_srt_file (lexname);
            return 0;
        }
        lexicon *lex = new_lexicon (lexname);
        if (id2str) {
            int id;
            while (cin >> id) {
                cout << id << '\t' 
                     << (id < lex->size() ? lex->id2str (id) : "*OUT*OF*RANGE*")
                     << endl;
            }
        } else if (str2id) {
            string str;
            while (getline (cin, str)) {
                cout << lex->str2id (str.c_str()) << '\t' << str << endl;
            }
        } else if (sorted) {
            FromFile<int32_t> srt ((lexname + ".lex.srt").c_str());
            for (int id = 0; id < lex->size(); id++) {
                cout << id << '\t' << lex->id2str (srt.get()) << endl;
            }
        } else {
            for (int id = 0; id < lex->size(); id++) {
                cout << id << '\t' << lex->id2str (id) << endl;
            }
        }
        delete lex;
    } catch (exception &e) {
        cerr << progname << '[' << lexname << "]:" << e.what() << endl;
        return 1;
    }
    return 0;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
