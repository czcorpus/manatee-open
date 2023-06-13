// Copyright (c) 2016  Milos Jakubicek

#include "consumer.hh"
#include "posattr.hh"
#include "corpus.hh"
#include "config.hh"
#include <iostream>
#include <set>

using namespace std;

typedef set <int> id_set;

int main (int argc, char **argv)
{
    if (argc < 3) {
        fputs ("usage: mknormattr corpus srcattr normattr\n", stderr);
        return 2;
    }
    try {
        Corpus c (argv[1]);
        PosAttr *srcattr = c.get_attr (argv[2]);
        PosAttr *normattr = c.get_attr (argv[3]);
        id_set* *sets = new id_set*[srcattr->id_range()]();
        int *id_array = new int[srcattr->id_range()]();
        RevFileConsumer *rev =
            RevFileConsumer::create (srcattr->attr_path + '@' + normattr->name, 20 * 1024 * 1024);
        IDIterator *srcit = srcattr->posat (0);
        IDIterator *normit = normattr->posat (0);
        float size = c.size();
        for (Position pos = 0; pos < c.size(); pos++) {
            if (pos % 1000000 == 0)
                fprintf (stderr, "\r%.2f %%", pos * 100 / size);
            int srcid = srcit->next();
            int normid = normit->next();
            if (!id_array[srcid]) // first occurrence
                id_array[srcid] = normid + 1;
            else { // second and further occurrences
                if (!sets[srcid]) {
                    id_set *new_set = new id_set();
                    new_set->insert (id_array[srcid] - 1);
                    sets[srcid] = new_set;
                }
                sets[srcid]->insert (normid);
            }
        }
        for (int i = 0; i < srcattr->id_range(); i++) {
            if (!sets[i])
                rev->put (i, id_array[i] - 1);
            else {
                for (id_set::const_iterator it = sets[i]->begin(); it != sets[i]->end(); it++)
                    rev->put (i, *it);
            }
        }
        fprintf (stderr, "\r100.00 %%\n");
        delete rev;
        delete srcit;
        delete normit;
        delete[] id_array;
        for (int i = 0; i < srcattr->id_range(); i++)
            delete sets[i];
        delete[] sets;
    } catch (exception &e) {
        cerr << "error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:

