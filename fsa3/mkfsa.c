#include "stdio.h"
#include "stdlib.h"
#include "writefsa.h"

int main() {
  fsm aut;
  int error;
  char *word = NULL;
  size_t buffer;
  ssize_t length;

  fsm_init(&aut);
  while ((length = getline(&word, &buffer, stdin)) >= 0) {
    if (length && word[length - 1] == '\n') word[--length] = '\0';
    add_word(&aut, (unsigned char *) word, (size_t) length);
  }
  free(word);

  if ((error = write_fsa(&aut, stdout))) fprintf(stderr, "Could not write the automaton\n");
  return error;
}
