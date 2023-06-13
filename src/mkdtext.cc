// Copyright (c) 1999-2013  Pavel Rychly, Milos Jakubicek

#include <config.hh>
#include "consumer.hh"
#include "fromtof.hh"
#include "bititer.hh"
#include "bitio.hh"
#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <sys/stat.h>

using namespace std;

void usage()
{
    cerr << "usage: mkdtext [-b|-g|-i] [-a] DELTATEXT_PATH [FILENAME]\n"
         << "    -b    create big_delta_text\n"
         << "    -g    create giga_delta_text\n"
         << "    -i    create int_text\n"
         << "    -a    append positions to the end\n"
         << "    -e    only write header according to .off/.seg\n"
        ;
}

int main (int argc, char **argv) 
{
    const char *progname = argv[0];
    TextConsumer::TextType ttype = TextConsumer::tdelta;
    bool append_to_end = false, write_header = false;
    char *outname;
    FILE *input = stdin;
    {
        // process options
        int c;
        while ((c = getopt (argc, argv, "?haebgi")) != -1)
            switch (c) {
            case 'a':
                append_to_end = true;
                break;
            case 'e':
                write_header = true;
                break;
            case 'b':
                ttype = TextConsumer::tdeltabig;
                break;
            case 'g':
                ttype = TextConsumer::tdeltagiga;
                break;
            case 'i':
                ttype = TextConsumer::tint;
                break;
            case 'h':
            case '?':
                usage();
                return 2;
            default:
                cerr << progname << ": unknown option (" << c << ")\n";
                usage();
                return 2;
            }
        
        if (optind < argc) {
            outname = argv [optind++];
            if (optind < argc)
                input = fopen (argv [optind], "r");
        } else {
            usage();
            return 2;
        }
    }

    if (append_to_end) {
        cerr << "Appending not yet implemented, sorry.\n";
        return 2;
    }
    int seg_size = 128;
    if (ttype == TextConsumer::tdeltagiga)
        seg_size = 64;
    try {
        if (write_header) {
            string path = string (outname) + ".text";
            string segpath = path + ".seg";
            string offpath = path + ".off";
            FILE *textf = fopen (path.c_str(), "rb+");
            if (!textf)
                throw FileAccessError (path, "cannot access .text file");
            OutFileBits_tell *deltafile = new OutFileBits_tell(textf);

            struct stat st_buf;
            Position segfile_size, offfile_size, text_size;
            if (stat(segpath.c_str(), &st_buf) == 0)
                segfile_size = st_buf.st_size;
            else
                throw FileAccessError (path, "cannot access .seg file");
            if (stat(offpath.c_str(), &st_buf) == 0)
                offfile_size = st_buf.st_size;
            else
                throw FileAccessError (path, "cannot access .off file");
            text_size = min(segfile_size * 256, offfile_size * 32);

            fseeko (textf, 0, SEEK_SET);
            const char signature[7] = "\243finDT";
            fwrite (signature, sizeof (signature) -1, 1, textf);
            fseeko (textf, 16, SEEK_SET);
            {
                write_bits<OutFileBits_tell&,unsigned char,unsigned char*,Position>
                    write_size(*deltafile);
                write_size.delta (seg_size +1);
                write_size.delta (text_size +1);
            }
            delete deltafile;
            fclose (textf);
            cerr << "Header written: segment size " << seg_size
                 << ", text size " << text_size << "\n";
            return 0;
        }
        TextConsumer *dtextcons = 
            TextConsumer::create (ttype, string (outname));
        FromFile<int> ci (input);
        while (!ci) {
            dtextcons->put (*ci);
            ++ci;
        }
        delete dtextcons;
    } catch (exception &e) {
        cerr << e.what() << endl;
        return 1;
    }
    return 0;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
