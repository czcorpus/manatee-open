#include "fsa_op.h"
#include "common.h"

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

#define fsa_li_push(top, a1, a2, w1, w2, l) \
  { ++top; (*(top)).arc1 = a1; (*(top)).arc2 = a2; (*(top)).word_no1 = w1; (*(top)).word_no2 = w2; (*(top)).last = l; }
#define fsa_dump_push(top, a, l) { (top)++; (*(top)).arc1 = a; (*(top)).last = l; }

int fsa_li_search_init(const fsa * const fsa1, const fsa * const fsa2, search_data * const data) {
  if (! (data->candidate = (unsigned char *) malloc(fsa1->max_len))) return 1;
  if (! (data->stack = (stack_entry *) malloc(fsa1->max_len * sizeof(stack_entry)))) { free(data->candidate); return 1; }
  data->top = data->stack - 1;
  fsa_li_push(data->top, NULL, NULL, 0, 0, NULL);
  fsa_li_push(data->top, fsa1->start, fsa2->start, fsa1->epsilon, fsa2->epsilon, data->candidate);
  return 0;
}

int fsa_li_get_word(const fsa * const fsa1, const fsa * const fsa2, search_data * const data) {
  arc_pointer next1, next2, arc1 = (*data->top).arc1, arc2 = (*data->top).arc2;
  size_t word_no1 = (*data->top).word_no1, word_no2 = (*data->top).word_no2;
  unsigned char * last = (*data->top).last;
  data->top--;
  if (! arc1) return 0;

  while (1) {
    unsigned char c1 = *last = get_char(arc1), final1 = is_final1(arc1), equal;
    while (arc2 && c1 > get_char(arc2)) {
      if (! is_last(arc2)) {
        word_no2 += words_in_node(arc2, fsa2) + is_final1(arc2);
        arc2 = get_next_arc(arc2, fsa2);
      } else arc2 = NULL;
    }
    equal = arc2 ? c1 == get_char(arc2) : 0;
    if (! is_last(arc1))
      fsa_li_push(data->top, get_next_arc(arc1, fsa1), arc2, word_no1 + words_in_node(arc1, fsa1) + final1, word_no2, last);
    next1 = get_node_arcs(arc1, fsa1); next2 = arc2 ? get_node_arcs(arc2, fsa2) : NULL;
    if (final1) {
      unsigned int arc2_is_final = arc2 && is_final(arc2) && equal;
      if (next1 > fsa1->start) fsa_li_push(data->top, next1, next2 && equal && next2 > fsa2->start ? next2 : NULL,
        word_no1 + 1, word_no2 + arc2_is_final, last + 1);
      data->word_no1 = word_no1; data->word_no2 = arc2_is_final && c1 == get_char(arc2) ? word_no2 : (size_t) -1;
      last[1] = 0; return 1;
    }
    if (next1 > fsa1->start) { word_no2 += arc2 && is_final(arc2) && ! final1; arc1 = next1;
      arc2 = equal && next2 > fsa2->start ? next2 : NULL; last++; }
  }
}

void fsa_li_search_close(search_data * const data) {
  free(data->stack);
  free(data->candidate);
}

int fsa_dump_search_init(const fsa * const aut, search_data * const data, arc_pointer prefix) {
  if (! (data->candidate = (unsigned char *) malloc(aut->max_len))) return 1;
  if (! (data->stack = (stack_entry *) malloc(aut->max_len * sizeof(stack_entry)))) { free(data->candidate); return 1; }
  data->top = data->stack - 1;
  fsa_dump_push(data->top, NULL, NULL);
  fsa_dump_push(data->top, (prefix ? prefix : aut->start), data->candidate);
  return 0;
}

int fsa_dump_get_word(const fsa * const aut, search_data * const data) {
  arc_pointer next, arc = (*data->top).arc1;
  unsigned char * last = (*data->top).last;
  data->top--;
  if (! arc) return 0;

  while(1) {
    *last = get_char(arc);
    if (! is_last(arc)) fsa_dump_push(data->top, get_next_arc(arc, aut), last);
    next = get_node_arcs(arc, aut);
    if (is_final(arc)) {
      if (next > aut->start) fsa_dump_push(data->top, next, last + 1);
      last[1] = 0; return 1;
    }
    arc = next; last++;
  }
}

void fsa_dump_search_close(search_data * const data) {
  free(data->stack);
  free(data->candidate);
}
