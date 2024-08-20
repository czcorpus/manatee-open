// Copyright (c) 1999-2019  Pavel Rychly, Milos Jakubicek

#include <config.hh>
#include "consumer.hh"
#include "fromtof.hh"
#include <iostream>
#include <unistd.h>
#include <cstdlib>

using namespace std;


static void usage (const char *progname) {
    cerr << "usage: " << progname << " [-j NUM_OF_FILES] [-b BUFF_SIZE] [-a]"
         << " [-l ALIGNMENT] DELTAREV_PATH [FILENAME]\n"
         << "    -j    join phase only\n"
         << "    -a    append to existing deltarev\n"
         << "    -c    input contains one collocation\n"
         << "    -s    input may not be sorted across runs\n";
}


int main (int argc, char **argv) 
{
    char *outname;
    int buff_size = 1048576;
    int alignmult = 1;
    FILE *input = stdin;

    bool sort_phase = true, append_to = false, colls = false, unsorted = false;
    int num_of_files = 0;
    const char *progname = argv[0];
    {
        // process options
        int c;
        while ((c = getopt (argc, argv, "h?j:b:asl:c")) != -1)
            switch (c) {
            case 'j':
                sort_phase = false; 
                num_of_files = atol (optarg);
                break;
            case 'b':
                buff_size = atol (optarg);
                break;
            case 'l':
                alignmult = atol (optarg);
                break;
            case 'h':
            case '?':
                usage (progname);
                return 2;
            case 'a':
                append_to = true;
                break;
            case 'c':
                colls = true;
                break;
            case 's':
                unsorted = true;
                break;
            default:
                cerr << "unknown option (" << c << ")\n";
                usage (progname);
                return 2;
            }
        
        if (optind < argc) {
            outname = argv [optind++];
            if (optind < argc)
                input = fopen (argv [optind], "r");
        } else {
            usage (progname);
            return 2;
        }
    }

    try {
        if (sort_phase) {
            FromFile<int64_t> ci (input);
            int64_t id, pos, coll;
            if (colls) {
                CollRevFileConsumer *drevcons =
                    CollRevFileConsumer::create (outname, buff_size, alignmult,
                                                 append_to, unsorted);
                while (!ci) {
                    id = *ci;
                    ++ci;
                    pos = *ci;
                    ++ci;
                    coll = *ci;
                    ++ci;
                    drevcons->put (id, pos, coll);
                }
                delete drevcons;
            } else {
                RevFileConsumer *drevcons =
                    RevFileConsumer::create (outname, buff_size, alignmult,
                                             append_to, unsorted);
                while (!ci) {
                    id = *ci;
                    ++ci;
                    pos = *ci;
                    ++ci;
                    drevcons->put (id, pos);
                }
                delete drevcons;
            }
        } else {
            if (colls)
                finish_rev_files <mfile_mfile_delta_revidx> (outname, num_of_files, alignmult, unsorted);
            else
                finish_rev_files <mfile_mfile_delta_collrevidx> (outname, num_of_files, alignmult, unsorted);
        }
    } catch (exception &e) {
        cerr << e.what() << endl;
        return 1;
    }
    return 0;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
