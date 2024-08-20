typedef const unsigned char *arc_pointer;

typedef struct {
  arc_pointer   dict, start;
  unsigned char gtl, entryl, epsilon, *candidate;
#ifdef NO_MMAP
  size_t        max_len;
#else
  size_t        max_len, mmap_size;
  void *        mmap_start;
#endif
  size_t        count;
  } fsa;

typedef struct {
  ssize_t pref_beg, pref_end;
  arc_pointer node;
} fsa_res;

int fsa_init(fsa * const aut, const char * const dict_file_name);
fsa_res find_number(const fsa * const aut, const unsigned char * word);
const unsigned char * find_word(const fsa * const aut, const size_t word_no);
void fsa_destroy(fsa * const aut);
const char *fsa_strerror (int errno);
