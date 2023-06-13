// Copyright (c) 2000-2009  Pavel Rychly

#include <config.hh>
#include "bigram.hh"
#include "message.hh"
#include <unistd.h>
#include <iostream>
#include <cstdlib>


using namespace std;

void usage (const char *progname)
{
    cerr << "usage: " << progname << " BGR_FILE_PATH [FIRST_ID]\n";
}

int main(int argc, char **argv)
{
    const char *progname = argv[0];
    const char *bgrname;
    int first_id = 0;
    {
	// process options
	int c;
	while ((c = getopt (argc, argv, "?h")) != -1)
	    switch (c) {
	    case 'h':
	    case '?':
		usage (progname);
		return 2;
	    default:
		cerr << "unknown option (" << c << ")\n";
		usage (progname);
		return 2;
	    }

	if (optind < argc) {
	    bgrname = argv [optind++];
	    if (optind < argc)
		first_id = atol (argv [optind]);
	} else {
	    usage (progname);
	    return 2;
	}
    }

    message msg (progname, bgrname);

    try {
	BigramStream bb (bgrname, first_id);
	while (!bb) {
	    BigramItem bi = *bb;
	    cout << bi.first << '\t' << bi.second << '\t' << bi.value << endl;
	    ++bb;
	}
    } catch (exception &e) {
	msg (e.what());
	return 1;
    }
    return 0;
}
