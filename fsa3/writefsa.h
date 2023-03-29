typedef struct {
  unsigned char arcs;
  unsigned char * arc;
} active_node;
        
typedef struct {
  size_t	max_len, last_len, word_count, node_count, index_count, index_size, index_mask, index_bytes, allocated;
  unsigned char	*last_word, *nodes, *fsa, *used, *index, epsilon;
  active_node 	*active;
} fsm;

int fsm_init(fsm * const aut);
void add_word(fsm * const aut, const unsigned char * const word, const size_t len);
int write_fsa(fsm * const aut, FILE * outfile);
