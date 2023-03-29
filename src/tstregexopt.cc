// Copyright (c) 2016-2020 Pavel Rychly, Milos Jakubicek

#include "seek.hh"
#include <cstdio>
#include "config.hh"
#include "corpus.hh"
#include "dynattr.hh"
#include "regexopt.hh"
#include <iostream>

using namespace std;

int main (int argc, const char **argv) {
	const char *corpname = "susanne";
    const char *attr = "word";
	if (argc > 1) {
		corpname = argv[1];
        if (argc > 2)
            attr = argv[2];
	}
	string inputQuery;
	try {
		getline(cin, inputQuery);
		Corpus *corp = new Corpus(corpname);
		PosAttr *src = corp->get_attr(attr);
		DynFun *fun = createDynFun ("", "internal", "lowercase"); // lowercase = dummy
        PosAttr *a = createDynAttr ("index", src->attr_path + ".regex",
                                    src->name + ".regex", fun, src, src->locale,
                                    false);
		printf ("Corpus %s found.\n", corpname);
		FastStream *f = optimize_regex (a, inputQuery.c_str(), NULL);
	    puts ("Stream gained.");
        if (f == NULL)
            puts ("Stream is null");
        else if (f->peek() >= f->final()) {
            puts ("Nothing found!");
        } else {
            printf("stream: min=%lli max=%lli\n",f->rest_min(),f->rest_max());
            int maxlines = 45;
            Labels lll;
            int rescount = 0;
            int id;
            while (maxlines-- && (id = f->next()) < f->final()) {
                rescount++;
                printf ("%s\n", src->id2str (id));
            }
            while (f->next() < f->final())
                rescount++;
            printf ("stream: finished %i\n", rescount);
        }
        delete f;
	} catch (exception &e) {
        printf ("exception: %s\n", e.what());
    } catch(RegexOptException *e) {
        printf ("exception: %s\n", e->what());
    }
	return 0;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
