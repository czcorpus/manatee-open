// Copyright (c) 2016  Milos Jakubicek

#include <config.hh>
#include "binfile.hh"
#include "bitio.hh"
#include "fromtof.hh"
#include <iostream>
#include <cstdlib>

using namespace std;

void usage()
{
    cerr
    << "usage: dumpbits [-r] OFFSET PATTERN PATH\n"
    << "       -r        repeat pattern until the end of file\n"
    << "       OFFSET    is number of bytes to skip\n"
    << "                 if ended with 'b' it is number of bits\n"
    << "       PATTERN   is a string of following letters:\n"
    << "                 b = single bit\n"
    << "                 d = delta\n"
    << "                 g = gamma\n"
    << "       PATH      is a path to the binary file to read\n"
    ;
}


int main (int argc, char **argv) 
{
    bool repeat = false;
    const char *offset, *pattern, *path;
    {
        // process options
        int c;
        while ((c = getopt (argc, argv, "?ri")) != -1)
            switch (c) {
            case '?':
                usage();
                return 2;
            case 'r':
                repeat = true;
                break;
            default:
                cerr << "unknown option (" << c << ")\n";
                usage();
                return 2;
            }
        
        if (optind +3 == argc) {
            offset = argv [optind];
            pattern = argv [optind+1];
            path = argv [optind+2];
        } else {
            usage();
            return 2;
        }
    }

    try {
        BinCachedFile<uint8_t> bitfile (path);
        typedef read_bits<BinCachedFile<uint8_t>::const_iterator,
                          uint8_t, uint64_t> bits_t;
        uint64_t skip_bytes, skip_bits = 0;
        if (offset[strlen(offset)-1] == 'b') {
            skip_bytes = atoll(offset) / 8;
            skip_bits = atoll(offset) % 8;
        } else
            skip_bytes = atoll(offset);
        cerr << "skipping " << skip_bytes << " bytes\n";
        cerr << "skipping " << skip_bits << " bits\n";
        bits_t bits (bitfile.at (skip_bytes));
        while (skip_bits--)
            bits.get_bit();
        do {
            for (const char *p = pattern; *p; p++) {
                int val;
                switch (*p) {
                    case 'b': val = bits.get_bit(); break;
                    case 'd': val = bits.delta(); break;
                    case 'g': val = bits.gamma(); break;
                    default: usage(); return 3;
                }
                printf("%d", val);
                if (*(p+1))
                    printf("\t");
            }
            printf("\n");
        } while (repeat);

    } catch (exception &e) {
        cerr << e.what() << endl;
        return 1;
    }
    return 0;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
