//  Copyright (c) 2012-2013  Pavel Rychly

#include "log.hh"
#include "config.hh"
#include "levels.hh"
#include <iostream>

using namespace std;

int main (int argc, const char **argv)
{
    if (argc != 2) {
         cerr << "usage: dumplevel INPUTFILE\n";
        return 1;
    }
    try {
        MLTStream *ls = full_level(new_TokenLevel(argv[1]));
        const char *chdesc [] = {"keep", "concat", "delete", "insert", "morph"};

        while (! ls->end()) {
            cout << chdesc[ls->change_type() -1] << ' ' 
                 << ls->change_size() << '-' << ls->change_newsize() << " <"
                 << ls->orgpos() << ',' << ls->newpos() << ">\n";
            ls->next();
        }
    } catch (exception &e) {
        cerr << "dumplevel error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
