/* config.hh.  Generated from config.hh.in by configure.  */
/* config.hh.in.  Generated from configure.ac by autoheader.  */

/* install C++ libraries */
/* #undef CXXLIB */

/* MacOS/DARWIN system */
/* #undef DARWIN */

/* enable assertions */
/* #undef DEBUG */

/* define if the compiler supports basic C++11 syntax */
#define HAVE_CXX11 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <fsa3/writefsa.h> header file. */
/* #undef HAVE_FSA3_WRITEFSA_H */

/* Define to 1 if you have the `getopt' function. */
#define HAVE_GETOPT 1

/* Define to 1 if you have the `getpagesize' function. */
#define HAVE_GETPAGESIZE 1

/* Define to 1 if you have the <hat-trie/hat-trie.h> header file. */
#define HAVE_HAT_TRIE_HAT_TRIE_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `dl' library (-ldl). */
/* #undef HAVE_LIBDL */

/* Define to 1 if you have the <locale.h> header file. */
#define HAVE_LOCALE_H 1

/* Define to 1 if you have the `memmove' function. */
#define HAVE_MEMMOVE 1

/* Define to 1 if you have a working `mmap' system call. */
#define HAVE_MMAP 1

/* Define to 1 if you have the pthread library */
#define HAVE_PTHREAD 1

/* If available, contains the Python version number currently in use. */
#define HAVE_PYTHON "3.10"

/* Define to 1 if you have the `setlocale' function. */
#define HAVE_SETLOCALE 1

/* Define to 1 if `stat' has the bug that it succeeds when given the
   zero-length file name argument. */
/* #undef HAVE_STAT_EMPTY_STRING_BUG */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strchr' function. */
#define HAVE_STRCHR 1

/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strpbrk' function. */
#define HAVE_STRPBRK 1

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if the system has the `__builtin_clzll' built-in function */
#define HAVE___BUILTIN_CLZLL 1

/* Define to 1 if the system has the `__builtin_ctzll' built-in function */
#define HAVE___BUILTIN_CTZLL 1

/* Define to 1 if the system has the `__builtin_expect' built-in function */
#define HAVE___BUILTIN_EXPECT 1

/* "Use FSA3 automata for lexicons" */
#define LEXICON_USE_FSA3 1

/* Defined so that it works in C++ */
#define LIKELY(condition) __builtin_expect(static_cast<bool>(condition), 1)

/* Define to 1 if `lstat' dereferences a symlink specified with a trailing
   slash. */
#define LSTAT_FOLLOWS_SLASHED_SYMLINK 1

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* Name of package */
#define PACKAGE "manatee"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "pary@fi.muni.cz"

/* Define to the full name of this package. */
#define PACKAGE_NAME "manatee"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "manatee open-2.214.1"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "manatee"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "open-2.214.1"

/* Define to 1 if all of the C90 standard headers exist (not just the ones
   required in a freestanding environment). This macro is provided for
   backward compatibility; new code need not use it. */
#define STDC_HEADERS 1

/* Defined so that it works in C++ */
#define UNLIKELY(condition) __builtin_expect(static_cast<bool>(condition), 0)

/* Define to 1 if using ICU regexps */
/* #undef USE_ICU */

/* Define to 1 if using pcre regexps */
#define USE_PCRE 1

/* Define to 1 if using standard regexps */
/* #undef USE_REGEX */

/* Version number of package */
#define VERSION "open-2.214.1"

/* Manatee version number */
#define VERSION_STR "open-2.214.1"

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */
