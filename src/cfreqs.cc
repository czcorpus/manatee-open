// Copyright (c) 2021  Pavel Rychly, Milos Jakubicek

#include "cqpeval.hh"
#include "subcorp.hh"
#include <iostream>

using namespace std;

int main (int argc, char **argv) 
{
	Corpus *c = new Corpus(argv[1]);

	if (argc > 5)
		c = new SubCorpus (c, argv[5]);
	RangeStream *result = c->filter_query (eval_cqpquery((string(argv[2]) + ';').c_str(), c));

	string context = "word -1";
	if (argc > 3)
		context = argv[3];

	int limit = 0;
	if (argc > 4)
		limit = atoi (argv[4]);
    
	vector<string> words;
	vector<NumOfPos> freqs;
	vector<NumOfPos> norms;
	c->freq_dist (result, context.c_str(), limit, words, freqs, norms);

	for (size_t i = 0; i < words.size(); i++)
		cout << words[i] << '\t' << freqs[i] << '\t' << norms[i] << '\n';
}
