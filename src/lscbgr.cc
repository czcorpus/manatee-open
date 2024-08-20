// Copyright (c) 2000-2015  Pavel Rychly

#include "bigram.hh"
#include "fingetopt.hh"
#include "corpus.hh"
#include "bgrstat.hh"
#include <unistd.h>
#include <iostream>
#include <cstdlib>

using namespace std;

void usage (const char *progname)
{
    cerr << "Lists corpus bigrams\n"
         << "usage: " << progname
         << " [OPTIONS] CORPUS_NAME [FIRST_ID]\n"
         << "     -p ATTR_NAME   corpus positional attribute\n"
         << "     -n BGR_FILE_PATH     path to data files\n"
         << "                          [default CORPPATH/ATTR_NAME.bgr]\n"
         << "     -f                   lists frequencies of both tokens\n"
         << "     -s t|mi|mi3|ll|ms|d  compute statistics:\n"
         << "             t     T score\n"
         << "             mi    MI score\n"
         << "             mi3   MI^3 score\n"
         << "             ll    log likelihood\n"
         << "             ms    minimum sensitivity\n"
         << "             d     logDice\n"
        ;
}


int main(int argc, char **argv)
{
    const char *progname = argv[0];
    const char *bgrname = 0;
    const char *attrname = 0;
    const char *corpname = 0;
    int first_id = 0;
    bool show_freq = false;
    char show_stat[16];
    {
        // process options
        int c;
        char *cstat = show_stat;
        while ((c = getopt (argc, argv, "?hp:n:fs:")) != -1)
            switch (c) {
            case 'p':
                attrname = optarg;
                break;
            case 'n':
                bgrname = optarg;
                break;
            case 'f':
                show_freq = true;
                break;
            case 's':
                if (!strcmp (optarg, "t"))
                    *cstat++ = 't';
                else if (!strcmp (optarg, "mi"))
                    *cstat++ = 'm';
                else if (!strcmp (optarg, "mi3"))
                    *cstat++ = '3';
                else if (!strcmp (optarg, "ll"))
                    *cstat++ = 'l';
                else if (!strcmp (optarg, "ms"))
                    *cstat++ = 's';
                else if (!strcmp (optarg, "d"))
                    *cstat++ = 'd';
                else
                    cerr << "Unknown statistics (" << optarg << ")\n";
                break;
            case '?':
            case 'h':
                usage (progname);
                return 2;
            default:
              abort ();
            }
        *cstat = '\0';

        if (argc - optind > 0) {
            corpname = argv [optind++];
            if (optind < argc)
                first_id = atol (argv [optind]);
        }
        if (!corpname || !attrname) {
            usage (progname);
            return 2;
        }
    }

    try {
        Corpus corp (corpname);
        PosAttr *attr = corp.get_attr (attrname);
        string defaultbgr = corp.conf->opts ["PATH"] + attrname + ".bgr";
        if (!bgrname) {
            bgrname = defaultbgr.c_str();
        }
        BigramStream bb (bgrname, first_id);
        int size = attr->size();
        bigram_fun* *stat_funs = NULL;
        if (*show_stat) {
            stat_funs = new bigram_fun*[strlen (show_stat)];
            bigram_fun **s = stat_funs;
            for (char *c = show_stat; *c; c++, s++)
                *s = code2bigram_fun (*c);
            *s = 0;
        }
        
        while (!bb) {
            BigramItem bi = *bb;
            cout << attr->id2str (bi.first) << '\t' 
                 << attr->id2str (bi.second) << '\t';
            if (show_freq)
                cout << attr->freq (bi.first) << '\t' 
                     << attr->freq (bi.second) << '\t';

            if (stat_funs) {
                int f_A = attr->freq (bi.first);
                int f_B = attr->freq (bi.second);
                int f_AB = bi.value;
                for (bigram_fun **s = stat_funs; *s; s++) {
                    cout << (*s)(f_AB, f_A, f_B, size) << '\t';
                }
            }
            cout << bi.value << '\n';
            ++bb;
        }
    } catch (exception &e) {
        cerr << "lscbgr error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
