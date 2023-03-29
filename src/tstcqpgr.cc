// Copyright (c) 1999-2016 Pavel Rychly, Milos Jakubicek, Radoslav Rabara

#include "seek.hh"
#include <cstdio>
#include "cqpeval.hh"
#include "config.hh"
#include <iostream>

using namespace std;

int main (int argc, const char **argv) {
     if (argc != 3) {
          cerr << "usage: <prog_name> [CORPUS_NAME] [QUERY]" << endl;
          return 1;
     }
     try {
     Corpus* corp = new Corpus(argv[1]);;
     RangeStream* result;
     result = eval_cqpquery(argv[2],corp);
     cout << "Corpus " << argv[1] << " found." << endl;
     cout << "Stream gained." << endl;
     PosAttr* word = corp->get_attr("word");
     const int maxLines = 5;
     int remainingLines = maxLines;
     int resCount = 0;
     Position from, to;
     if (result != NULL) {
          if (!result->end()) {
               cout << "stream: min=" << result->rest_min()
                    << " max=" << result->rest_max() << endl;
          }
          while (!result->end()) {
               if (remainingLines > 0) {
                    resCount++;
                    remainingLines--;
                    from = result->peek_beg();
                    to = result->peek_end();
                    cout << from << "[" << to-from << "]: <";
                    for (; from < to; from++) {
                         cout << word->pos2str(from) << " ";
                    }
                    cout << "> ";
                    for (int i = to; i < to+5;i++) {
                         cout << word->pos2str(i) << " ";
                    }
                    Labels lab;
                    result->add_labels(lab);
                    for (Labels::iterator it=lab.begin(); it != lab.end(); it++) {
                         cout << "<<" << it->first << ":" << it->second << ">>";
                    }
                    cout << endl;
               } else {
                    resCount++;
               }
               result->next();
          }
          if (remainingLines != 5) {
               cout << "rstream: finished " << resCount << endl;
          }
     }
     if (maxLines == remainingLines) {
          cout << "Nothing found!" << endl;
     }

     delete corp;

     } catch (exception &e) {
          cout << "exception: " << e.what() << endl;
     }
     return 0;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
