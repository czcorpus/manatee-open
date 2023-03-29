//  Copyright (c) 2001-2016  Pavel Rychly

#include "corpus.hh"
#include "fingetopt.hh"
#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <cstdlib>

using namespace std;

void usage (const char *progname) {
    cerr << "Usage: " << progname 
         << " [OPTIONS] CORPNAME\n"
        "Prints basic information of a given corpus.\n"
        "     -d         dump whole configuration\n"
        "     -p         print corpus directory path\n"
        "     -s         print corpus size\n"
        "     -w         print corpus wordform counts\n"
        "     -c         print path to corpus configuration file\n"
        "     -g OPT     print configuration value of option OPT\n"
        "     -t STRUCT  print structure size\n";
}

unsigned long count_regexp (PosAttr *attr, const char *pat)
{
    unsigned long freq = 0;
    Generator<int> *it = attr->regexp2ids (pat, false);
    while (!it->end()) {
        freq += attr->freq (it->next());
    }
    delete it;
    return freq;
}

void print_wordforms_counts (const char *corpname)
{
    Corpus c(corpname);
    PosAttr *attr = c.get_attr ("-", true);
    unsigned long total = attr->size();
    unsigned long freq;

    cout << "wordforms    " << setw(9)
         << count_regexp (attr, "[[:alpha:]]+") << '\n';
    cout << "numbers      " << setw(9) 
         << count_regexp (attr, "[[:digit:]]+") << '\n';
    freq = count_regexp (attr, "[[:alpha:][:digit:]]+");
    total -= freq;
    cout << "alphanumeric " << setw(9) << freq << '\n';
    freq = count_regexp (attr, "[[:punct:]]+");
    total -= freq;
    cout << "punctuation  " << setw(9)  << freq << '\n';
    if (total) 
        cout << "others       " << setw(9) << total << '\n';
    cout << "total        " << setw(9) << attr->size() << '\n';
}

int main (int argc, char **argv)
{
    const char *progname = argv[0];
    const char *corpname;
    bool opt_d = false;
    bool opt_p = false;
    bool opt_s = false; 
    bool opt_w = false;
    bool opt_c = false;
    const char *opt_g = NULL;
    const char *opt_t = NULL;
    {
        // process options
        int c;
        while ((c = getopt (argc, argv, "dpswcg:t:vh?")) != -1)
            switch (c) {
            case 'd':
                opt_d = true;
                break;
            case 'p':
                opt_p = true;
                break;
            case 's':
                opt_s = true;
                break;
            case 'w':
                opt_w = true;
                break;
            case 'c':
                opt_c = true;
                break;
            case 'g':
                opt_g = optarg;
                break;
            case 't':
                opt_t = optarg;
                break;
            case 'v':
                cout << VERSION_STR << endl;
                return 0;
            case '?':
            case 'h':
                usage (progname);
                return 2;
            default:
              abort ();
            }
        if (optind < argc) {
            corpname = argv [optind++];
            if (optind < argc) {
                usage (progname);
                return 2;
            }
        } else {
            usage (progname);
            return 2;
        }
    }
    
    CorpInfo *ci;
    try {
        ci = loadCorpInfo(corpname);
        if (opt_d)
            cout << ci->dump();
        else if (opt_p)
            cout << ci->opts["PATH"] << '\n';
        else if (opt_s)
            cout << Corpus(corpname).size() << '\n';
        else if (opt_w)
            print_wordforms_counts (corpname);
        else if (opt_c)
            cout << ci->conffile << '\n';
        else if (opt_g)
            cout << ci->find_opt (opt_g) << '\n';
        else if (opt_t)
            cout << Corpus(corpname).get_struct(opt_t)->size() << '\n';
        else {
            Corpus c(corpname);
            cout << "Corpus " << corpname << " exists.\n";
        }
        return 0;
    } catch (exception &e) {
        cerr << progname << ": " << e.what() << '\n';
    }
    return 1;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
