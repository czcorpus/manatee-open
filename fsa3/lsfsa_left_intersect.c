#include <stdio.h>
#include "fsa_op.h"

int main(const int argc, const char *argv[]) {
  fsa fsa1, fsa2;
  int error;
  search_data data;

  if (argc != 3) { fprintf(stderr, "Wrong number of arguments (%d instead of 2).\n", argc - 1); return 1; }
  if ((error = fsa_init(&fsa1, argv[1]) || fsa_init(&fsa2, argv[2]))) return error;

  if (fsa1.epsilon && fsa2.epsilon) printf("\t0\t0\n");
  if (fsa1.start && fsa2.start) {
    if ((error = fsa_li_search_init(&fsa1, &fsa2, &data))) return error;
    while (fsa_li_get_word(&fsa1, &fsa2, &data))
	  printf("%s\t%ld\t%ld\n", data.candidate, data.word_no1, (ssize_t) data.word_no2);
    fsa_li_search_close(&data);
  }

  fsa_destroy(&fsa1);
  fsa_destroy(&fsa2);
  return 0;
}
