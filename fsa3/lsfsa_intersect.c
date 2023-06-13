#include "stdio.h"
#include "fsa.h"
#include "common.h"
#include <stdlib.h>

#define get_char(arc) (*(arc))
#define is_final(arc) ((arc)[goto_offset] & FINALBIT)
#define is_final1(arc) ((arc)[goto_offset] & FINALBIT ? 1 : 0)
#define is_last(arc) ((arc)[goto_offset] & STOPBIT)
#define has_next(arc) ((arc)[goto_offset] & NEXTBIT)

#define get_next_arc(arc, fsa) ((arc) + goto_offset + (fsa)->gtl)
#define get_goto(arc, fsa) (bytes2num((arc) + goto_offset, (fsa)->gtl) >> flag_count)
#define get_node(arc, fsa) (has_next(arc) ? (arc) + goto_offset + 1 : (fsa)->dict + get_goto(arc, fsa))
#define get_node_arcs(arc, fsa) (get_node(arc, fsa) + (fsa)->entryl)
#define words_in_node(arc, fsa) (get_goto(arc, fsa) ? bytes2num(get_node(arc, fsa), (fsa)->entryl) : 0)

#define push(top, a1, a2, w1, w2, l) \
  { ++top; (*(top)).arc1 = a1; (*(top)).arc2 = a2; (*(top)).word_no1 = w1; (*(top)).word_no2 = w2; (*(top)).last = l; }

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

static inline int search_init(const fsa * const fsa1, const fsa * const fsa2, search_data * const data) {
  size_t max_len = fsa1->max_len < fsa2->max_len ? fsa1->max_len : fsa2->max_len;
  if (! (data->candidate = (unsigned char *) malloc(max_len))) return 1;
  if (! (data->stack = (stack_entry *) malloc(max_len * sizeof(stack_entry)))) { free(data->candidate); return 1; }
  data->top = data->stack - 1;
  push(data->top, NULL, NULL, 0, 0, NULL);
  push(data->top, fsa1->start, fsa2->start, fsa1->epsilon, fsa2->epsilon, data->candidate);
  return 0;
}

static inline int get_word(const fsa * const fsa1, const fsa * const fsa2, search_data * const data) {
  while (1) {
    arc_pointer next1, next2, arc1 = (*data->top).arc1, arc2 = (*data->top).arc2;
    size_t word_no1 = (*data->top).word_no1, word_no2 = (*data->top).word_no2;
    unsigned char * last = (*data->top).last;
    data->top--;
    if (! arc1) return 0;

    while (1) {
      unsigned char c1 = get_char(arc1), c2 = get_char(arc2), both_next;
      if (c1 > c2) {
        if (is_last(arc2)) break; else {
          word_no2 += words_in_node(arc2, fsa2) + is_final1(arc2);
          arc2 = get_next_arc(arc2, fsa2);
          }
      } else if (c1 < c2) {
        if (is_last(arc1)) break; else {
          word_no1 += words_in_node(arc1, fsa1) + is_final1(arc1);
          arc1 = get_next_arc(arc1, fsa1);
          }
      } else { /* c1 == c2 */
        unsigned int final1 = is_final1(arc1), final2 = is_final1(arc2);
        *last = c1;
        if (! is_last(arc1) && ! is_last(arc2)) push(data->top, get_next_arc(arc1, fsa1), get_next_arc(arc2, fsa2),
          word_no1 + words_in_node(arc1, fsa1) + final1, word_no2 + words_in_node(arc2, fsa2) + final2, last);
        next1 = get_node_arcs(arc1, fsa1); next2 = get_node_arcs(arc2, fsa2);
        both_next = next1 > fsa1->start && next2 > fsa2->start;
        if (final1 && final2) {
          if (both_next) push(data->top, next1, next2, word_no1 + 1, word_no2 + 1, last + 1);
          data->word_no1 = word_no1; data->word_no2 = word_no2; last[1] = 0; return 1;
          }
        if (! both_next) break;
        word_no1 += final1 && ! final2; word_no2 += final2 && ! final1; arc1 = next1; arc2 = next2; last++;
} } } }

static inline void search_close(search_data * const data) {
  free(data->stack);
  free(data->candidate);
}

int main(const int argc, const char *argv[]) {
  fsa fsa1, fsa2;
  int error;
  search_data data;

  if (argc != 3) { fprintf(stderr, "Wrong number of arguments (%d instead of 2).\n", argc - 1); return 1; }
  if ((error = fsa_init(&fsa1, argv[1]) || fsa_init(&fsa2, argv[2]))) return error;

  if (fsa1.epsilon && fsa2.epsilon) printf("\t0\t0\n");
  if (fsa1.start && fsa2.start) {
    if ((error = search_init(&fsa1, &fsa2, &data))) return error;
    while (get_word(&fsa1, &fsa2, &data)) printf("%s\t%ld\t%ld\n", data.candidate, data.word_no1, data.word_no2);
    search_close(&data);
    }

  fsa_destroy(&fsa1);
  fsa_destroy(&fsa2);
  return 0;
}
