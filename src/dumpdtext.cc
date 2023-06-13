// Copyright (c) 2006-2016  Pavel Rychly

#include <config.hh>
#include "lexicon.hh"
#include "binfile.hh"
#include "bitio.hh"
#include "fromtof.hh"
#include <iostream>
#include <cstdlib>

using namespace std;

void usage()
{
    cerr << "usage: dumpdtext [-r] DELTATEXT+LEX_PATH\n"
	 << "         -r     revidx dump\n"
	 << "         -i     id dump\n"
	;
}


int main (int argc, char **argv) 
{
    //const char *progname = argv[0];
    string path;
    bool revidxdump = false;
    bool iddump = false;
    {
	// process options
	int c;
	while ((c = getopt (argc, argv, "?ri")) != -1)
	    switch (c) {
	    case '?':
		usage();
		return 2;
	    case 'r':
		revidxdump = true;
		break;
	    case 'i':
		iddump = true;
		break;
	    default:
		cerr << "unknown option (" << c << ")\n";
		usage();
		return 2;
	    }
	
	if (optind +1 == argc) {
	    path = argv [optind];
	} else {
	    usage();
	    return 2;
	}
    }

    try {
	lexicon *lex = new_lexicon (path);
	BinCachedFile<unsigned char> text (path + ".text");
	typedef read_bits<BinCachedFile<unsigned char>::const_iterator,
	    uint8_t, uint64_t> bits_t;
	int64_t text_size;
	{
	    bits_t bits (text.at (16));
	    bits.delta(); // skip seg_size
	    text_size = bits.delta() -1;
	}
	bits_t bits (text.at (32));
	
	if (revidxdump) {
	    ToFile<int64_t> out(stdout);
	    int64_t curr = 0;
	    while (text_size--) {
		out.put(bits.delta() -1);
		out.put(curr++);
	    }
	} else if (iddump) {
	    while (text_size--)
		cout << (bits.delta() -1) << '\n';
	} else {
	    while (text_size--)
		cout << lex->id2str (bits.delta() -1) << '\n';
	}
	delete lex;
    } catch (exception &e) {
	cerr << e.what() << endl;
	return 1;
    }
    return 0;
}
