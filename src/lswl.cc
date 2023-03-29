//  Copyright (c) 2016-2020  Milos Jakubicek

#include "config.hh"
#include "keyword.hh"
#include <iostream>
#ifdef SKETCH_ENGINE
#include "sketch/wmap.hh"
#include "stat/ngram.hh"
#endif

void usage()
{
    cerr << "Usage: lswl [OPTIONS] CORPUS[:SUBCPATH] ATTR\n"
#ifdef SKETCH_ENGINE
         << "ATTR may have special values 'WSCOLLOC' and 'TERM' use 'ATTR.ngr' for ngrams\n"
#endif
         << "OPTIONS:\n"
         << "-l          number of items to return, defaults to 100\n"
         << "-i          minimum frequency, defaults to 5\n"
         << "-a          maximum frequency, defaults to 0=disabled\n"
         << "-b          path to a word-per-line formatted blacklist file\n"
         << "-w          path to a word-per-line formatted whitelist file\n"
         << "-f          comma-separated additional frequencies, one of arf, alfd, docf, frq or custom (type:format)\n"
         << "-s          sort type, one of arf, aldf, docf, frq (default) or custom (type:format)\n"
         << "-r          regex pattern\n"
         << "-xREGEXP    filter nonwords, argument is filtering regexp, otherwise NONWORDRE corpus directive is used\n"
         << "OUTPUT is a list of lines containing string and frequencies\n";
}

int main (int argc, char **argv)
{
    if (argc < 3) {
        usage();
        return 2;
    }
    NumOfPos minfreq = 5;
    NumOfPos maxfreq = 0;
    const char *blacklist = NULL;
    const char *whitelist = NULL;
    const char *frqtype = "";
    const char *sorttype = "frq";
    const char *nonwordre = NULL;
    string wlpat(".*");
    bool include_nonwords = true;
    int items = 100;
    int c;
    while ((c = getopt (argc, argv, "l:r:i:s:a:b:w:f:x::")) != -1) {
        switch (c) {
        case 'l':
            items = atoll (optarg);
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
            sorttype = optarg;
            break;
        case 'r':
            wlpat = optarg;
            break;
        case 'f':
            frqtype = optarg;
            break;
        case 'x':
            include_nonwords = false;
            nonwordre = optarg;
            break;
        case '?':
        case 'h':
            usage ();
            return 2;
        default:
          abort ();
        }
    }
    if (optind + 2 > argc) {
        usage();
        return 2;
    }

    try {
        string corpus (argv[optind]);
        const char *wl = argv[optind + 1];
        Corpus *c;
        size_t colon = corpus.find(':');
        if (colon != string::npos) {
            c = new Corpus (corpus.substr(0, colon));
            c = new SubCorpus (c, corpus.substr (colon + 1));
        } else
            c = new Corpus (corpus);
        WordList *l;
    #ifdef SKETCH_ENGINE
        if (!strcmp(wl, "WSCOLLOC")) {
            WMap *wm = new_WMap (c->get_conf("WSBASE"), c);
            l = wm->get_wordlist();
        } else if (!strcmp(wl, "TERM")) {
            l = c->get_wordlist (c->get_conf("TERMBASE"));
        } else if (strlen(wl) > 4 && !strcmp(wl + strlen(wl) - 4, ".ngr")) {
            string fpath = "";
            if (c->get_conf("SUBCPATH") != "") {
                fpath = c->get_conf("SUBCPATH") + wl;
            }
            NGram *ngr = new NGram (c->get_conf("PATH") + "/" + wl, fpath);
            l = ngr->get_wordlist ();
        } else
    #endif
            l = c->get_attr (wl);

        vector<Frequency*> addfreqs;
        char *frq = strtok((char*) frqtype, ",");
        while (frq) {
            addfreqs.push_back(l->get_stat(frq));
            frq = strtok(NULL, ",");
        }

        vector<string> blist, wlist;
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

        if (!include_nonwords && !nonwordre)
            nonwordre = c->get_conf("NONWORDRE").c_str();

        vector<string> out_list;
        NumOfPos out_count, out_count_total;
        wordlist (l, wlpat.c_str(), addfreqs, l->get_stat(sorttype), wlist, blist, minfreq, maxfreq, items, nonwordre, out_list, out_count, out_count_total);

        for (auto i = out_list.begin(); i != out_list.end(); i++) {
            char *item = strtok((char*) (*i).c_str(), "\v");
            cout << item;
            while (item) {
                item = strtok(NULL, "\v");
                if (item)
                    cout << "\t" << item;
            }
            cout << "\n";
        }
    } catch (exception &e) {
        cerr << e.what() << endl;
        return 1;
    }
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
