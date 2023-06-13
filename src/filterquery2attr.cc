// Copyright (c) 2019  Milos Jakubicek

#include <config.hh>
#include "consumer.hh"
#include "writelex.hh"
#include "revidx.hh"
#include <corpus.hh>
#include <cqpeval.hh>
#include <revit.hh>
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <string>
#include <assert.h>

using namespace std;


static void usage (const char *progname) {
    cerr << "usage: " << progname << " CORPUS SRCATTR DSTATTR\n";
}

// strip trailing white spaces
void stripend (string &str)
{
    unsigned int i;
    for (i = str.length(); i > 0 && (str[i-1] == ' ' || str[i-1] == '\t' ||
                                     str[i-1] == '\r'); i--)
        ;
    if (i < str.length())
        str.erase (i);
}

int main (int argc, char **argv) 
{
    if (argc < 4) {
        usage(argv[0]);
        return 1;
    }
    const char *corpname = argv[1];
    const char *srcattr = argv[2];
    const char *dstattr = argv[3];

    try {
        Corpus *corp = new Corpus(corpname);
        string path = corp->get_conf("PATH");
        string outname = path + "/" + dstattr;
        write_lexicon *wlex = new write_lexicon(outname, 50000);
        RevFileConsumer *drevcons = RevFileConsumer::create (outname + "_tmp", 50000000, 1, false, true);

        // evaluate queries, write new items to lexicon and rev
        string line;
        while (getline (cin, line)) {
            stripend(line);
            size_t tab = line.find('\t');
            if (tab == string::npos)
                continue;
            string str = line.substr(0, tab);
            string query = line.substr(tab + 1);
            int id = wlex->str2id(str.c_str());
            RangeStream *rs = eval_cqpquery ((query + ';').c_str(), corp);
            while (!rs->end()) {
                drevcons->put (id, rs->peek_beg());
                rs->next();
            }
            delete rs;
        }
        delete drevcons;
        delete wlex;

        // filter old rev and write filtered items
        map_delta_revidx rev (outname + "_tmp");
        drevcons = RevFileConsumer::create (outname, 50000000);

        // simulate new text
        IDPosIterator *new_text = new IDPosIteratorFromRevs<map_delta_revidx> (&rev, 0);
        PosAttr *pa = corp->get_attr(srcattr);
        IDIterator *old_text = pa->posat(0);
        Position pos = 0, size = corp->size();
        float fsize = float(size);
        ToFile<int> tf(stdout);
        unsigned prog_step = (1 << 24) - 1; // every 16M
        while (pos < size) {
            if (pos & prog_step)
                fprintf(stderr, "\r%.2f%%", 100 * pos/fsize);
            Position next_new_pos = new_text->peek_pos();
            bool new_finished = new_text->end();

            // insert to new rev from old text where positions should be kept
            while (pos < size && (new_finished || pos < next_new_pos)) {
                int id = old_text->next();
                assert(id >= 0);
                tf.put(id);
                drevcons->put (id, pos++);
            }

            // insert to new rev from new text where positions differ
            if (!new_finished) {
                int new_id = new_text->peek_id();
                tf.put(new_id);
                drevcons->put(new_id, next_new_pos);
                new_text->next();
                old_text->next(); // skip position in old text
                pos++;
            }
        }
        fprintf(stderr, "\n");
        assert(new_text->end());
        delete new_text;
        delete old_text;
        delete drevcons;
        delete corp;

    } catch (exception &e) {
        cerr << e.what() << endl;
        return 1;
    }
    return 0;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
