#include <stdio.h>
#include "fsa_op.h"
#include "common.h"

int main(const int argc, const char *argv[]) {
  fsa aut;
  int error;
  search_data data;

  if (argc < 2) { fprintf(stderr, "Usage: dumpfsa FSA [PREFIX]\n"); return 1; }
  if ((error = fsa_init(&aut, argv[1]))) {
    fprintf(stderr, "error initializing FSA: %s: %s", argv[1], fsa_strerror (error));
    return error;
  }
  if (aut.epsilon && (argc <= 2 || *argv[2] == '\0'))
    printf("\n");
  arc_pointer start = NULL;
  const char *prefix = "";
  if (argc > 2 && *argv[2] != '\0') {
    fsa_res r = find_number(&aut, (const unsigned char*) argv[2]);
    start = r.node;
    prefix = argv[2];
    if (r.pref_end == -1) // neither a prefix or a word
      return 0;
    else if (r.pref_end > 0) { // given prefix is a word and is prefix of some other words
      printf("%s\n", prefix);
      if (r.pref_beg == r.pref_end) // given prefix identifies only a single word
        return 0;
    }
  }

  if (aut.start) {
    if ((error = fsa_dump_search_init(&aut, &data, start))) return error;
    while (fsa_dump_get_word(&aut, &data)) printf("%s%s\n", prefix, data.candidate);
    fsa_dump_search_close(&data);
  }

  fsa_destroy(&aut);
  return 0;
}
