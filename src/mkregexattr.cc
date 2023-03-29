// Copyright (c) 2014-2016  Milos Jakubicek

#include "lexicon.hh"
#include "writelex.hh"
#include "consumer.hh"
#include "posattr.hh"
#include "corpus.hh"
#include "utf8.hh"
#include <iostream>

using namespace std;

#define NGRLENGTH 3

inline bool is_utf8char_begin (const unsigned char c)
{
    return (c & 0xc0) != 0x80;
}

int main (int argc, char **argv)
{
    try {
        string attrpath = "";

        switch (argc) {
        default:
            fputs ("usage: mkregexattr corpus attr\n", stderr);
            fputs ("       mkregexattr attrpath\n", stderr);
            return 2;
        case 3:
            attrpath = Corpus(argv[1]).get_conf("PATH") + argv[2];
            break;
        case 2:
            attrpath = argv[1];
        }
        string path = attrpath + ".regex";
        lexicon *lex = new_lexicon (attrpath);

#ifdef LEXICON_USE_FSA3
        write_fsalex wl (path, true, false);
#else
        write_lexicon wl (path, 500000, false);
#endif
        RevFileConsumer *rev = RevFileConsumer::create (path);

        int did, id, id_range = lex->size();
        for (id = 0; id < id_range; id++) {
            string s = string("^").append(lex->id2str(id)).append("$");
            size_t bytes = s.size();
            size_t len = utf8len (s.c_str());
            const char *end = s.c_str() + bytes;
            for (size_t l = 1; l <= NGRLENGTH && l <= len; l++) {
                const char *str = s.c_str();
                for (size_t i = 0; i < len - l + 1; i++) {
                    const char *in = str;
                    const char *next = NULL;
                    size_t n = l + 1;
                    while (in != end && n) {
                        if (is_utf8char_begin(*in)) {
                            n--;
                            if (n == l - 1) // position after first character
                                next = in;
                        }
                        in++;
                    }
                    char *curr_end = (char*) in - (!n);
                    char tmp_end = *curr_end;
                    *curr_end = '\0';
                    did = wl.str2id (str);
                    rev->put (did, id);
                    *curr_end = tmp_end;
                    if (!next)
                        break;
                    else
                        str = next;
                }
            }
        }
        delete rev;
    } catch (exception &e) {
        cerr << "error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:

