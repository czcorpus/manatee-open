//  Copyright (c) 2016-2020  Milos Jakubicek

#include "config.hh"
#include "keyword.hh"
#include <iostream>
#ifdef SKETCH_ENGINE
#include "sketch/wmap.hh"
#endif

void usage()
{
    cerr << "Usage: lskw [OPTIONS] CORPUS1[:SUBCPATH1] CORPUS2[:SUBCPATH2] ATTR1 ATTR2\n"
#ifdef SKETCH_ENGINE
         << "ATTR may have special values 'WSCOLLOC' and 'TERM'\n"
#endif
         << "OPTIONS:\n"
         << "-l    number of items to return, defaults to 100\n"
         << "-n    simplemath N parameter, defaults to 1\n"
         << "-i    minimum frequency, defaults to 5\n"
         << "-a    maximum frequency, defaults to 0=disabled\n"
         << "-b    path to a word-per-line formatted blacklist file\n"
         << "-w    path to a word-per-line formatted whitelist file\n"
         << "-s    scoring frequency type (default is frq)\n"
         << "-f    comma-separated list of additional frequency types\n"
         << "-x    include nonwords (see NONWORDRE corpus directive)\n"
         << "-r    $-separated list of regex filters\n"
         << "OUTPUT is a list of lines containing string, score, ID1, ID2, freq1, freq2, freq-per-million1, freq-per-million2, and any additional frequency pairs requested by -f\n";
}

int main (int argc, char **argv)
{
    if (argc < 5) {
        usage();
        return 2;
    }
    float sm_param = 1.0;
    NumOfPos minfreq = 5;
    NumOfPos maxfreq = 0;
    const char *blacklist = NULL;
    const char *whitelist = NULL;
    const char *frqtype = "frq";
    bool include_nonwords = false;
    vector<string> regexes, addfrq;
    int items = 100;
    int c;
    char *re = NULL;
    while ((c = getopt (argc, argv, "l:n:i:a:b:w:s:f:r:x")) != -1) {
        switch (c) {
        case 'l':
            items = atoll (optarg);
            break;
        case 'n':
            sm_param = atof (optarg);
            break;
        case 'i':
            minfreq = max(atoll (optarg), 1LL);
            break;
        case 'a':
            maxfreq = atoll (optarg);
            break;
        case 'b':
            blacklist = optarg;
            break;
        case 'w':
            whitelist = optarg;
            break;
        case 's':
            frqtype = optarg;
            break;
        case 'f':
            re = strtok(optarg, ",");
            while (re) {
                addfrq.push_back(re);
                re = strtok(NULL, ",");
            }
            break;
        case 'x':
            include_nonwords = true;
            break;
        case 'r':
            re = strtok(optarg, "$");
            while (re) {
                regexes.push_back(re);
                re = strtok(NULL, "$");
            }
            break;
        case '?':
        case 'h':
            usage ();
            return 2;
        default:
          abort ();
        }
    }
    if (optind + 5 < argc) {
        usage();
        return 2;
    }

    try {
        string corpus1 (argv[optind]);
        string corpus2 (argv[optind + 1]);
        const char *wl1 = argv[optind + 2];
        const char *wl2 = argv[optind + 3];
        Corpus *c1, *c2;
        size_t colon = corpus1.find(':');
        if (colon != string::npos) {
            c1 = new Corpus (corpus1.substr(0, colon));
            c1 = new SubCorpus (c1, corpus1.substr (colon + 1));
        } else
            c1 = new Corpus (corpus1);
        colon = corpus2.find(':');
        if (colon != string::npos) {
            c2 = new Corpus (corpus2.substr(0, colon));
            c2 = new SubCorpus (c2, corpus2.substr (colon + 1));
        } else
            c2 = new Corpus (corpus2);
        unordered_set<string> blist, wlist;
        if (blacklist) {
            ifstream f (blacklist);
            copy(istream_iterator<string>(f), istream_iterator<string>(),
                 inserter(blist, blist.end()));
        }
        if (whitelist) {
            ifstream f (whitelist);
            copy(istream_iterator<string>(f), istream_iterator<string>(),
                 inserter(wlist, wlist.end()));
        }
        WordList *l1, *l2;
    #ifdef SKETCH_ENGINE
        if (!strcmp(wl1, "WSCOLLOC")) {
            WMap *wm = new_WMap (c1->get_conf("WSBASE"), c1);
            l1 = wm->get_wordlist();
        } else if (!strcmp(wl1, "TERM"))
            l1 = c1->get_wordlist (c1->get_conf("TERMBASE"));
        else
    #endif
            l1 = c1->get_attr (wl1);
    #ifdef SKETCH_ENGINE
        if (!strcmp(wl2, "WSCOLLOC")) {
            WMap *wm = new_WMap (c2->get_conf("WSBASE"), c2);
            l2 = wm->get_wordlist();
        } else if (!strcmp(wl2, "TERM"))
            l2 = c2->get_wordlist (c2->get_conf("TERMBASE"));
        else
    #endif
            l2 = c2->get_attr (wl2);

        Keyword kw (c1, c2, l1, l2, sm_param, items, minfreq, maxfreq, blist,
                    wlist, frqtype, addfrq, regexes,
                    {include_nonwords?"":c1->get_conf("NONWORDRE")}, stderr);

        cout.precision(2);
        cout << fixed;
        const kwitem *k;
        while ((k = kw.next()) != NULL) {
            cout << k->str << '\t' << k->score << '\t' << k->id1 << '\t' << k->id2 << '\t'
                 << k->freqs[0] << '\t' << k->freqs[1] << '\t' << k->freqs[2] << '\t' << k->freqs[3];
            for (size_t i = 0; i != addfrq.size(); i++)
                cout << '\t' << k->freqs[2*i+4] << '\t' << k->freqs[2*i+5];
            cout << '\n';
        }
    } catch (exception &e) {
        cerr << e.what() << endl;
        return 1;
    }
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
