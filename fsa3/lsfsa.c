#include "stdio.h"
#include "stdlib.h"
#include "fsa.h"

void usage() {
    fprintf(stderr, "Usage: lsfsa -d PATH_TO_FSA DIRECTION\n");
    fprintf(stderr, "DIRECTION is -N for word to number or -W for number to word\n");
}

int main(const int argc, const char *argv[]) {
  fsa hash;
  int state, arg_index;
  const char * dict = NULL;
  const unsigned char * r;
  char direction = 0, * word;
  size_t buff_len = 1024;
  ssize_t length, n;

  for (arg_index = 1; arg_index < argc; arg_index++)
    if (argv[arg_index][1] == 'd' && ++arg_index < argc) dict = argv[arg_index];
    else if (argv[arg_index][1] == 'N') direction = 'N';
    else if (argv[arg_index][1] == 'W') direction = 'W';
  if (dict == NULL) { fprintf(stderr, "No dictionary specified.\n"); usage(); return 1; }
  if (! direction) { fprintf(stderr, "Either -N (words->numbers) or -W (numbers->words) must be specified.\n"); usage(); return 1; }

  if ((state = fsa_init(&hash, dict))) {
    fprintf(stderr, "error initializing FSA: %s: %s\n", dict, fsa_strerror (state));
    return state;
  }

  word = (char *) malloc(buff_len);

  while ((length = getline(&word, &buff_len, stdin)) != -1) {
    if (length && word[length - 1] == '\n') word[--length] = '\0'; /* getline (mostly :-) returns data with \n */
    if (direction == 'N') {
      fsa_res r = find_number(&hash, (unsigned char *) word);
      if (r.pref_end > 0)
        printf("%ld\n", r.pref_beg);
      else
        printf("%ld-%ld\n", r.pref_beg, -r.pref_end);
    }
    else if ((n = atoi(word)) >= 0 && (r = find_word(&hash, (size_t) n))) printf("%s\n", r); else printf("out of range\n");
  }
  fsa_destroy(&hash);
  free(word);
  return 0;
}
