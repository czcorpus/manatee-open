#include <stdlib.h>
#include "fsa.h"

typedef struct {
  arc_pointer arc1, arc2;
  size_t word_no1, word_no2;
  unsigned char * last;
} stack_entry;

typedef struct {
  stack_entry * stack, * top;
  size_t word_no1, word_no2;
  unsigned char * candidate;
} search_data;

int fsa_li_search_init(const fsa * const fsa1, const fsa * const fsa2, search_data * const data);
int fsa_li_get_word(const fsa * const fsa1, const fsa * const fsa2, search_data * const data);
void fsa_li_search_close(search_data * const data);

int fsa_dump_search_init(const fsa * const aut, search_data * const data, arc_pointer prefix);
int fsa_dump_get_word(const fsa * const aut, search_data * const data);
void fsa_dump_search_close(search_data * const data);
