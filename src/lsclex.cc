// Copyright (c) 2000-2015  Pavel Rychly

#include "corpus.hh"
#include "fingetopt.hh"
#include <unistd.h>
#include <iostream>
#include <cstdlib>

using namespace std;

void usage (const char *progname)
{
    cerr << "usage: " << progname <<" [-s|-n|-f] CORPUS ATTR\n"
         << "     -s          str2id\n"
         << "     -n          id2str\n"
         << "     -f          print frequences\n"
        ;
}

inline NumOfPos print_freq(PosAttr *attr, int id)
{
    return id < 0 ? 0 : attr->freq(id);
}

int main(int argc, char **argv)
{
    const char *progname = argv[0];
    string corpname, attrname;

    bool id2str = false;
    bool str2id = false;
    bool showfreqs = false;
    {
        // process options
        int c;
        while ((c = getopt (argc, argv, "?hnsf")) != -1)
            switch (c) {
            case 'n':
                id2str = true;
                break;
            case 's':
                str2id = true;
                break;
            case 'f':
                showfreqs = true;
                break;
            case '?':
            case 'h':
                usage (progname);
                return 2;
            default:
              abort ();
            }

        if (argc - optind == 2) {
            corpname = argv [optind++];
            attrname = argv [optind];
        } else {
            usage (progname);
            return 2;
        }
    }

    try {
        Corpus c (corpname);
        PosAttr *attr = c.get_attr (attrname, true);
        if (id2str) {
            int id;
            while (cin >> id) {
                cout << id << '\t' 
                     << (id < attr->id_range() ? attr->id2str (id) 
                                               : "*OUT*OF*RANGE*");
                if (showfreqs)
                    cout << '\t' << print_freq (attr, id);
                cout << '\n';
            }
        } else if (str2id) {
            string str;
            int id;
            while (getline (cin, str)) {
                id = attr->str2id (str.c_str());
                cout << id << '\t' << str;
                if (showfreqs)
                    cout << '\t' << print_freq (attr, id);
                cout << '\n';
            }
        } else {
            for (int id = 0; id < attr->id_range(); id++) {
                cout << id << '\t' << attr->id2str (id);
                if (showfreqs)
                    cout << '\t' << print_freq (attr, id);
                cout << '\n';
            }
        }
    } catch (exception &e) {
        cerr << progname << '[' << corpname << '-' << attrname << "]:" 
             << e.what() << '\n';
        return 1;
    }
    return 0;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
