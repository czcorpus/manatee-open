#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "writefsa.h"
#include "common.h"

static const unsigned char build_flag_count = 3, flag_bits = 7, DEADBIT = 4, SAFEBIT = 0x80, addr_len = 5;
static const size_t NODES_REALLOC_STEP = 1 << 20, INITIAL_INDEX_SIZE = 1 << 20;

#define arc_size (addr_len + 1U)
#define active_node_size (1 + 255 * arc_size)

int fsm_init(fsm * const aut) {
  aut->last_len = aut->max_len = aut->epsilon = aut->word_count = aut->allocated = 0; aut->node_count = 2;
  aut->last_word = NULL; aut->active = NULL; aut->nodes = NULL;
    aut->used = (unsigned char *) sizeof(size_t); /* (used - nodes == 0 is "NULL") */
  aut->index_count = 0; aut->index_size = INITIAL_INDEX_SIZE; aut->index_bytes = aut->index_size * addr_len;
    aut->index_mask = aut->index_size - 1;
  memset((aut->index = (unsigned char *) malloc(aut->index_bytes + sizeof(size_t))), 0, aut->index_bytes + sizeof(size_t));
  return 0;
}

int cmp_nodes(const unsigned char * n1, const unsigned char * n2, const unsigned char arcs) {
  unsigned char i;
  for (i = 0; i < arcs; i++, n1 += arc_size, n2 += arc_size)
    if (bytes2num(n1, addr_len) != bytes2num(n2, addr_len) || n1[addr_len] != n2[addr_len]) return 0;
  return 1;
}

/* 32bit version of murmur3 hash (probably incorrectly! :-) "extended" to size_t */
static inline size_t fmix(size_t h) { h ^= h >> 16; h *= 0x85ebca6b; h ^= h >> 13; h *= 0xc2b2ae35; return h ^ h >> 16; }
static inline size_t rot32(size_t x, unsigned char r) { return (x << r) | (x >> (32 - r)); }
static inline size_t murmur3hash(const unsigned char * n, const size_t len) {
  const size_t blocks = (len >> 2) << 2;
  size_t i, h1 = 0xc062fb4a, c1 = 0xcc9e2d51, c2 = 0x1b873593; /* h1 is "seed" */
  for (i = 0; i < blocks; i += 4) h1 = rot32(h1 ^ rot32(bytes2num(n + i, 4) * c1, 15) * c2, 13) * 5 + 0xe6546b64;
  return fmix(h1 ^ rot32((len & 3 ? bytes2num(n + blocks, len & 3) : 0) * c1, 15) * c2 ^ len);
}

size_t find_or_register(fsm * const aut, const size_t i) {
  active_node const * n = aut->active + i;
  size_t tip, addr, diff, new_mem;

  n->arc[(n->arcs - 1U) * arc_size] |= STOPBIT;
  tip = (murmur3hash(n->arc, n->arcs * arc_size) & aut->index_mask) * addr_len;

  while ((addr = bytes2num(aut->index + tip, addr_len))) {
    if (cmp_nodes(n->arc, aut->nodes + (addr >> build_flag_count), n->arcs)) return addr;
    if ((tip += addr_len) == aut->index_bytes) tip = 0; /* linear probing "overflow" :-) */
  }

  diff = (size_t) (aut->used - aut->nodes);
  new_mem = arc_size * n->arcs; /* add space for the new node if necessary */
  if (diff + new_mem + sizeof(size_t) > aut->allocated)
    aut->used = (aut->nodes = (unsigned char *) realloc(aut->nodes, aut->allocated += NODES_REALLOC_STEP)) + diff;

  addr = (size_t) (aut->used - aut->nodes) << build_flag_count; /* store the new node */
  memcpy(aut->used, n->arc, new_mem);
  aut->used += new_mem;
  aut->node_count++;

  memcpy(aut->index + tip, &addr, addr_len); /* index the new node */

  if (++aut->index_count > 0.75 * aut->index_size) { /* if necessary, double the size of index and rehash */
    size_t tmp_addr, tmp_len;
    unsigned char * mid, * end, * tmp, * tmp_node, * tmp_tip;
    aut->index_mask = (aut->index_size *= 2) - 1;
    aut->index_bytes = aut->index_size * addr_len;
    aut->index = (unsigned char *) realloc(aut->index, aut->index_bytes + sizeof(size_t));
    memset(aut->index + aut->index_bytes / 2, 0, aut->index_bytes / 2);
    mid = aut->index + aut->index_bytes / 2;
    end = aut->index + aut->index_bytes;
    tmp = aut->index;
    for (; (tmp_addr = bytes2num(tmp, addr_len)) || tmp < mid; tmp += addr_len) if (tmp_addr) {
      tmp_node = aut->nodes + (tmp_addr >> build_flag_count);
      tmp_len = 0; while (! (tmp_node[tmp_len] & STOPBIT)) tmp_len += arc_size;
      tmp_tip = aut->index + (murmur3hash(tmp_node, tmp_len + arc_size) & aut->index_mask) * addr_len;
      while (tmp_tip != tmp && bytes2num(tmp_tip, addr_len)) if ((tmp_tip += addr_len) == end) tmp_tip = aut->index;
      if (tmp_tip == tmp) continue;
      memcpy(tmp_tip, &tmp_addr, addr_len);
      memset(tmp, 0, addr_len);
    }
  }
  return addr;
}

/* unlike C++, C does not allow memcpy(dest, &(tmp = value|of|expression), len) */
#define memecpy(dest, src, len) { size_t memecpy_tmp = src; memcpy(dest, &memecpy_tmp, len); }
#define real_addr(address) (aut->nodes + (bytes2num(address, addr_len) >> build_flag_count))

size_t write_arcs(fsm * const aut, unsigned char * n, const size_t gtl, const size_t entryl) {
  unsigned char * xnode, * arc = n;
  size_t start_of_node = (size_t) (aut->used - aut->fsa), stop, entries = 0;

  aut->used += entryl;

  for (stop = 0; ! stop; arc += arc_size) {
    int next_node;
    xnode = real_addr(arc);
    *(aut->used)++ = arc[addr_len]; /* letter */
    next_node = (stop = *arc & STOPBIT) && xnode != aut->nodes && ! (*xnode & DEADBIT);
    if (next_node) *aut->used |= SAFEBIT; /* make sure goto != 0 */
    if (next_node) *aut->used |= NEXTBIT; else *aut->used &= ~NEXTBIT; /* set next bit */
    if (stop) *aut->used |= STOPBIT; else *aut->used &= ~STOPBIT; /* set stop bit */
    if (*arc & FINALBIT) *aut->used |= FINALBIT; else *aut->used &= ~FINALBIT; /* set final bit */
    aut->used += next_node ? 1 : gtl;
  }

  for (arc -= arc_size; arc >= n; arc -= arc_size) {
    unsigned char * start_of_addr =
      aut->fsa + start_of_node + entryl + (goto_offset + gtl) * (size_t) (arc - n) / arc_size + goto_offset;
    unsigned char flags = *start_of_addr & flag_bits;

    if ((xnode = real_addr(arc)) && xnode != aut->nodes) {
      if (! (*xnode & DEADBIT)) {
        memecpy(start_of_addr, ((size_t) (aut->used - aut->fsa) << flag_count) | (*arc & STOPBIT ? SAFEBIT : 0U) | flags, gtl);
        entries += write_arcs(aut, xnode, gtl, entryl);
      } else {
        memecpy(start_of_addr, ((bytes2num(xnode, addr_len) >> build_flag_count) << flag_count) | flags, gtl);
        entries += bytes2num(aut->fsa + (bytes2num(xnode, addr_len) >> build_flag_count), entryl);
      }
    } else memecpy(start_of_addr, flags, gtl);
    entries += *arc & FINALBIT;
  }

  memcpy(aut->fsa + start_of_node, &entries, entryl);
  memecpy(n, start_of_node << build_flag_count | DEADBIT, addr_len);
  return entries;
}

int write_fsa(fsm * const aut, FILE * outfile) {
  size_t i, gtl = 1, entryl, size, root = 0;
  unsigned char header[12] = "\\fsa";

  for (i = aut->last_len; i > 1; i--)
    *(size_t *) (aut->active[i - 2].arc + arc_size * (aut->active[i - 2].arcs - 1U)) |= find_or_register(aut, i - 1);
  if (aut->last_len) root = find_or_register(aut, 0) >> build_flag_count;

  /* now we can free the memory for active nodes and index */
  for (i = 0; i < aut->max_len; i++) free(aut->active[i].arc);
  free(aut->active); free(aut->last_word); free(aut->index);

  aut->word_count += aut->epsilon;
  size = (size_t) (aut->used - aut->nodes);
  entryl = aut->word_count ? (sizeof(aut->word_count) * 8 - (unsigned) __builtin_clzl(aut->word_count) - 1U) / 8 + 1 : 1;
  while ((aut->node_count * entryl + (size / arc_size + 1) * (goto_offset + gtl)) >> ((8 * gtl) - flag_count)) gtl++;

  header[4] = (entryl << 4) | gtl;
  memset(header + 5, 0, 3);
  memecpy(header + 8, aut->word_count ? ++aut->max_len : 0, 4);
  if (! fwrite(header, sizeof(header), 1, outfile)) return ferror(outfile);

  /* memory for the output form (slightly bigger: allocation, not use, is free of charge on Linux) & write the sink node */
  memset((aut->fsa = (unsigned char *) malloc(size + aut->node_count * entryl)), 0, entryl + goto_offset + gtl);
  aut->used = aut->fsa + entryl + goto_offset + gtl;

  memcpy(aut->used, &aut->word_count, entryl); /* write the start node (to retain the format) */
  aut->used += entryl;
  *(aut->used)++ = '^';
  *(aut->used)++ =
    (root ? ((2 * (entryl + goto_offset) + gtl + 1U) << flag_count) | SAFEBIT | NEXTBIT : 0) | (STOPBIT | aut->epsilon);

  if (root) write_arcs(aut, aut->nodes + root, gtl, entryl); /* write the rest of the automaton */
  if(! fwrite(aut->fsa, (size_t) (aut->used - aut->fsa), 1, outfile)) return ferror(outfile);
  free(aut->nodes);
  free(aut->fsa);
  return 0;
}

static inline void fill(unsigned char * c, unsigned char letter, int final)
  { size_t i; *c = final ? FINALBIT : 0; c++; for (i = 0; i + 1 < addr_len; i++, c++) *c = 0; *c = letter; }

void add_word(fsm * const aut, const unsigned char * const word, const size_t len) {
  size_t i, k;
  if (! len) { aut->epsilon = 1; return; }

  if (aut->max_len < len) { /* (re)allocation of fixed space for active nodes */
    aut->active = (active_node *) realloc(aut->active, sizeof(active_node) * len + sizeof(size_t));
    for (i = aut->max_len; i < len; i++) {
      aut->active[i].arcs = 0;
      aut->active[i].arc = (unsigned char *) malloc(active_node_size);
    }
    int do_init = 0;
    if (!aut->last_word) // first word ever
      do_init = 1;
    aut->last_word = (unsigned char *) realloc(aut->last_word, sizeof(unsigned char) * len + sizeof(size_t));
    if (do_init)
      aut->last_word[0] = '\0';
    aut->max_len = len;
  }

  i = 0;
  while (i + 1 < aut->last_len && aut->last_word[i] == word[i]) i++; /* i + 1 == aut->last_len && aut->last_word[i] == word[i] <=> xy after x */
  if (i + 1 == len && aut->last_word[i] == word[i]) return; /* ignore duplicities (it's cheaper than any other action :-) */

  /* The last child and possibly its children of the last node in prefix are to be either registered (if there is no
   * isomorphic subgraph in the automaton), or replaced by pointer to a subgraph already registered. */
  for (k = aut->last_len; k > i + 1; k--)
    *(size_t *) (aut->active[k - 2].arc + arc_size * (aut->active[k - 2].arcs - 1U)) |= find_or_register(aut, k - 1);

  /* {FINALBIT,0,0,0,0} is "NULL address" with final bit set --- any arc from an active node always goes to the next active
   * node thus we do not need any address: either find_or_register will tell us the right number or "NULL" will remain there
   *
   * From the rest of the new word (after prefix; it always exists, as input words are sorted) we make a chain of new nodes.
   *
   * aut->last_len --- only for the first word, where aut->last_word[0] would be undefined (if we would discard binary data,
   * we could set aut->last_word[i] to '\0' and remove aut->last_len test) */
  if (!(aut->last_len && aut->last_word[i] == word[i]))
    fill(aut->active[i].arc + arc_size * aut->active[i].arcs++, word[i], i + 1 == len);

  for (aut->last_word[i] = word[i], ++i; i < len; aut->last_word[i] = word[i], i++) {
    fill(aut->active[i].arc, word[i], i + 1 == len);
    aut->active[i].arcs = 1;
  }

  aut->last_len = len;
  (aut->word_count)++;
}
