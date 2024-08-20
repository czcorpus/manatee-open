// Copyright (c) 1999-2023 Pavel Rychly, Milos Jakubicek, Radoslav Rabara

#include "seek.hh"
#include <cstdio>
#include "cqpeval.hh"
#include "config.hh"
#include <iostream>

using namespace std;

int main (int argc, const char **argv) {
     if (argc != 2 && argc != 3) {
          cerr << "usage: tstcqpgr CORPUS_NAME [QUERY]" << endl;
          cerr << "if QUERY is not given, it is read from standard input" << endl;
          return 1;
     }
     Corpus *corp = NULL;
     RangeStream *result = NULL;
     try {
     corp = new Corpus(argv[1]);
     string query;
     if (argc == 3)
          query = argv[2];
     else {
          ostringstream oss;
          oss << cin.rdbuf();
          query = oss.str();
          if (query.back() == '\n')
              query.pop_back();
     }
     result = eval_cqpquery(query.c_str(), corp);
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

     } catch (exception &e) {
          cout << "exception: " << e.what() << endl;
     }
     delete result;
     delete corp;
     return 0;
}

// vim: ts=4 sw=4 sta et sts=4 si cindent tw=80:
