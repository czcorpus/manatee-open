#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#ifndef NO_MMAP
#include "sys/mman.h"
#endif
#include "fsa.h"
#include "common.h"

#ifndef NO_MMAP
int str_to_madv_advice (char *adv) {
  if (!strcmp(adv, "MADV_NORMAL"))
    return MADV_NORMAL;
  if (!strcmp(adv, "MADV_RANDOM"))
    return MADV_RANDOM;
  if (!strcmp(adv, "MADV_SEQUENTIAL"))
    return MADV_SEQUENTIAL;
  if (!strcmp(adv, "MADV_WILLNEED"))
    return MADV_WILLNEED;
  if (!strcmp(adv, "MADV_DONTNEED"))
    return MADV_DONTNEED;
  return -1;
}
#endif

const char *fsa_strerror (int errno) {
  switch (errno) {
    case 1: return "cannot open";
    case 2: return "seek to end failed";
    case 3: return "ftell failed";
    case 4: return "seek to start failed";
    case 5: return "cannot read header";
    case 6: return "bad magic number";
    case 7: return "built without -N(umbers)";
    case 8: return "cannot read data";
    case 9: return "cannot mmap data";
    default: return "";
  }
}

int fsa_init(fsa * const aut, const char * const dict_file_name) {
  ssize_t       fsa_size;
  unsigned char header[12];
  FILE *        dict_file;

  if (! (dict_file = fopen(dict_file_name, "rb")))
    return 1;
  if (fseeko(dict_file, 0L, SEEK_END))
    return 2;
  if ((fsa_size = ftello(dict_file) - (ssize_t) sizeof(header)) < 0)
    return 3;
  if (fseeko(dict_file, 0L, SEEK_SET))
    return 4;
  if (! fread(header, sizeof(header), 1, dict_file))
    return 5;
  if (strncmp((char *) header, "\\fsa", (size_t) 4))
    return 6;
  if (! (aut->entryl = (header[4] >> 4) & 0x0f))
    return 7;
#ifdef NO_MMAP
  if (fread((void *) (aut->dict = (unsigned char *) malloc((size_t) fsa_size + sizeof(size_t))),
      (size_t) fsa_size, 1, dict_file) != 1) {
    free((void *) aut->dict);
    return 8;
  }
#else
  aut->mmap_size = sizeof(header) + (size_t) fsa_size + sizeof(size_t);
  if ((aut->mmap_start = mmap(NULL, aut->mmap_size,
      PROT_READ, MAP_SHARED, fileno(dict_file), 0)) == MAP_FAILED)
    return 9;
  char *adv = getenv("FSA3_MADVISE");
  if (adv) {
    if (madvise (aut->mmap_start, aut->mmap_size, str_to_madv_advice (adv)))
      perror ("failed to madvise");
    else
      fprintf(stderr, "succeeded to madvise to %s\n", adv);
  }
  aut->dict = (arc_pointer) aut->mmap_start + sizeof(header);
#endif
  fclose(dict_file);
  aut->gtl = header[4] & 0x0f;
  aut->max_len = 0;
  memcpy(&aut->max_len, header + 8, 4);
  aut->start =
    aut->max_len > 1 ? aut->dict + aut->entryl + goto_offset + aut->gtl + aut->entryl + goto_offset + 1 + aut->entryl : 0;
  aut->epsilon = aut->dict[aut->entryl + goto_offset + aut->gtl + aut->entryl + goto_offset] & FINALBIT;
  aut->candidate = (unsigned char *) malloc(aut->max_len);
  aut->count = bytes2num(aut->dict + aut->entryl + goto_offset + aut->gtl, aut->entryl);
  return 0;
}

#define get_goto(arc) (bytes2num(arc + goto_offset, aut->gtl) >> 3)
#define to_next_node(arc) ((arc[goto_offset] & 4) ? arc + goto_offset + 1 + aut->entryl : aut->dict + aut->entryl + get_goto(arc))
#define words_in_node(arc) (get_goto(arc) ? bytes2num(to_next_node(arc) - aut->entryl, aut->entryl) : 0)

fsa_res find_number(const fsa * const aut, const unsigned char *word) {
  int i, found;
  arc_pointer node = aut->start;
  int word_no = 0;
  ssize_t pref_beg;
  if (aut->epsilon) { if (word[0]) word_no = 1; else return (fsa_res){0,0,NULL}; } /* else: fsa knows epsilon and the user asks for it */
  if (aut->max_len < 2) return (fsa_res){-1,-1,NULL}; /* max_len == 0 => empty fsa, max_len == 1 => fsa knows only epsilon */
  arc_pointer next_node;
  do {
    next_node = node;
    found = 0;
    pref_beg = word_no;
    for (i = 1; i; i = !(next_node[goto_offset] & 2), next_node += goto_offset + aut->gtl) {
      if ((unsigned char) *word == *next_node)
        if (word[1] == '\0' && (next_node[goto_offset] & 1)) return (fsa_res){word_no,word_no + words_in_node(next_node),to_next_node(next_node)}; /* end of the word + the word has been found */
        else {
          if (get_goto(next_node) == 0) return (fsa_res){-1,-1,NULL}; /* end of the string in the automaton */
          node = to_next_node(next_node);
          word++;
          word_no += (next_node[goto_offset] & 1);
          found = 1;
          break;
        }
      else {
        if (next_node[goto_offset] & 1) word_no++;
        word_no += (int) words_in_node(next_node);
      }
    }
  } while (found);
  if (word[0] == '\0')
    // word was not found, but is a prefix of other words
    return (fsa_res){pref_beg,-word_no+1,node};
  else
    return (fsa_res){-1,-1,NULL};
}

const unsigned char * find_word(const fsa * const aut, const size_t word_no) {
  size_t m, n = 0, l = 0;
  int i, found;
  arc_pointer node = aut->start;
  if (aut->epsilon) { if (word_no) n = 1; else { aut->candidate[0] = 0; return aut->candidate; }}
  if (aut->max_len < 2) return NULL; /* see find_number */
  do {
    arc_pointer next_node = node;
    found = 0;
    if (l + 1 > aut->max_len) return NULL; /* input word is longer than the longest word in fsa */
    for (i = 1; i; i = !(next_node[goto_offset] & 2), next_node += goto_offset + aut->gtl) {
      if (next_node[goto_offset] & 1) {
        if (n == word_no) {
          aut->candidate[l] = *next_node;
          aut->candidate[l + 1] = '\0';
          return aut->candidate;
        }
        else n++;
      }
      if ((m = n + words_in_node(next_node)) > word_no) {
        aut->candidate[l] = *next_node;
        l++;
        node = to_next_node(next_node);
        found = 1;
        break;
      }
      else n = m;
    }
  } while (found);
  return NULL;
}

void fsa_destroy(fsa * const aut) {
#ifdef NO_MMAP
  free((void *) aut->dict);
#else
  munmap(aut->mmap_start, aut->mmap_size);
#endif
  free(aut->candidate);
}
