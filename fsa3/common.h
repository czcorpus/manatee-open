static const unsigned char goto_offset = 1, flag_count = 3, FINALBIT = 1, STOPBIT = 2, NEXTBIT = 4;

#include <inttypes.h>


#if 0
static inline size_t bytes2num(const unsigned char * const addr, size_t const len) {
  switch(len) { /* up to sizeof(size_t) bytes after addr has to be allocated (otherwise *(size_t *) addr could SIGSEGV) */
    case 5: return *(const size_t *) addr & 0x0ffffffffffU; /* assumes addr_len == 5 (and gtl for really big [4+GB] files) */
    case 4: return *(const size_t *) addr & 0x0ffffffffU; /* most common case in murmur3hash (and gtl for big files) */
    case 3: return *(const size_t *) addr & 0x0ffffffU; /* 0--3: other cases in murmur3hash, gtl for small files, entryl */
    case 2: return *(const size_t *) addr & 0x0ffffU;
    case 1: return *(const size_t *) addr & 0x0ffU;
    /* case 0: return 0U; // not used now (now would default to the last return) */
    case 6: return *(const size_t *) addr & 0x0ffffffffffffU; /* > addr_len not used now, only for the sake of completness */
    case 7: return *(const size_t *) addr & 0x0ffffffffffffffU;
    case 8: return *(const size_t *) addr & 0x0ffffffffffffffffU;
  }
  return 0;
}
#endif

#if 1
/* do not read byte-by-byte and do not access memory regions beyond addr+len
 * performance is the same as the above implementation. but the strict
 * aliasing rule is still violated */
static inline size_t bytes2num(const unsigned char *const addr, const size_t len) {
  switch(len) {
	default: return 0;
	case 1: return addr[0];
	case 2: return ((size_t) *((uint16_t*) addr));
	case 3: return ((size_t) *((uint16_t*) addr)) | (((size_t) addr[2]) << 16);
	case 4: return ((size_t) *((uint32_t*) addr));
	case 5: return ((size_t) *((uint32_t*) addr)) | (((size_t) addr[4]) << 32);
	case 6: return ((size_t) *((uint32_t*) addr)) | (((size_t) *((uint16_t*) (addr+4))) << 32);
	case 7: return ((size_t) *((uint32_t*) addr)) | (((size_t) *((uint16_t*) (addr+4))) << 32) | (((size_t) addr[6]) << 48);
	case 8: return ((size_t) *((uint64_t*) addr));
  }
}
#endif

#if 0
/* should be correct for all sane compilers & targets, but a bit slower (ca 2 % for a whole program in a particular scenario) */
static inline size_t bytes2num(const unsigned char * const addr, size_t const len) {
  union { size_t num; unsigned char bytes[8]; } number = { 0 };
  switch(len) {
    case 8: number.bytes[7] = addr[7];
    case 7: number.bytes[6] = addr[6];
    case 6: number.bytes[5] = addr[5];
    case 5: number.bytes[4] = addr[4];
    case 4: number.bytes[3] = addr[3];
    case 3: number.bytes[2] = addr[2];
    case 2: number.bytes[1] = addr[1];
    case 1: number.bytes[0] = addr[0];
  }
  return number.num;
}
#endif

#if 0
/* correct even for strict reading (one can read only that union member which has been written) of the standard (+ ca 17 %) */
static inline size_t bytes2num(const unsigned char * const addr, size_t const len) {
  size_t num = 0, i;
  for (i = 0; i < len; i++) num |= (size_t) addr[i] << (i * 8);
  return num;
}
#endif
