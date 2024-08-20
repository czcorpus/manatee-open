# Copyright (c) 2003-2022  Pavel Rychly, Milos Jakubicek

bin_PROGRAMS = manateesrv corpinfo encodevert mkdynattr lscbgr lsclex mknorms \
	mkvirt mkalign dumpattrtext dumpattrrev dumpstructrng mkregexattr lsalsize\
	mknormattr filterquery2attr lskw maplexrev lswl mkwc mktokencov filterlexrev
dist_bin_SCRIPTS = filterquery2attr.sh
noinst_PROGRAMS = tstcqpgr dumplevel dumpvirt tstregexopt cfreqs
AM_DEFAULT_SOURCE_EXT=.cc
dumplevel_LDADD = $(top_builddir)/corp/libcorp.la
dumpvirt_LDADD = $(top_builddir)/corp/libcorp.la \
	$(top_builddir)/finlib/libfin.la $(top_builddir)/query/libquery.la $(LDADD)
lskw_LDADD = $(top_builddir)/corp/libcorp.la $(top_builddir)/finlib/libfin.la $(LDADD) \
	$(top_builddir)/query/libquery.la
lswl_LDADD = $(top_builddir)/corp/libcorp.la $(top_builddir)/finlib/libfin.la $(LDADD) \
	$(top_builddir)/query/libquery.la

lib_LTLIBRARIES = libmanatee.la
libmanatee_la_SOURCES = 
libmanatee_la_LIBADD = $(top_builddir)/concord/libconcord.la \
	$(top_builddir)/query/libquery.la $(top_builddir)/corp/libcorp.la \
	$(top_builddir)/finlib/libfin.la $(REGEX_LDFLAG)

noinst_LTLIBRARIES = libbuiltinmanatee.la
libbuiltinmanatee_la_SOURCES = 
libbuiltinmanatee_la_LIBADD = $(top_builddir)/concord/libconcord.la \
	$(top_builddir)/query/libquery.la $(top_builddir)/corp/libcorp.la $(top_builddir)/finlib/libfin.la $(top_builddir)/fsa3/libfsa3.la $(top_builddir)/hat-trie/src/libhat-trie.la
EXTRA_libbuiltinmanatee_la_DEPENDENCIES = $(top_builddir)/hat-trie/src/libhat-trie.la.new
$(top_builddir)/hat-trie/src/libhat-trie.la.new: $(top_builddir)/hat-trie/src/libhat-trie.la
	sed "s/^libdir=.*/libdir=''/" $(top_builddir)/hat-trie/src/libhat-trie.la > $(top_builddir)/hat-trie/src/libhat-trie.la.new
	touch -r $(top_builddir)/hat-trie/src/libhat-trie.la $(top_builddir)/hat-trie/src/libhat-trie.la.new
	mv $(top_builddir)/hat-trie/src/libhat-trie.la.new $(top_builddir)/hat-trie/src/libhat-trie.la
	cp $(top_builddir)/hat-trie/src/libhat-trie.la $(top_builddir)/hat-trie/src/libhat-trie.la.new

AM_CPPFLAGS += -I$(top_srcdir)/corp -I$(top_srcdir)/query -I$(top_srcdir)/concord -I$(top_srcdir)/hat-trie -I$(top_srcdir)/finlib

LDADD = $(REGEX_LDFLAG) libbuiltinmanatee.la

manateesrv_LDADD = $(LDADD) $(PTHREAD_LIBS)
manateesrv_LDFLAGS = $(LDFLAGS) 

tstcqpgr_LDADD = $(top_builddir)/query/libquery.la $(top_builddir)/corp/libcorp.la \
	$(top_builddir)/finlib/libfin.la $(LDADD)

tstregexopt_LDADD = $(top_builddir)/corp/libcorp.la $(top_builddir)/query/libquery.la \
	$(top_builddir)/finlib/libfin.la $(LDADD)

cfreqs_LDADD = $(top_builddir)/corp/libcorp.la $(top_builddir)/query/libquery.la \
	$(top_builddir)/finlib/libfin.la $(LDADD) $(PTHREAD_LIBS)


if ADDPCREPOSIX
encodevert_LDADD = $(LDADD) -lpcreposix $(PCRE_LDFLAG)
else
encodevert_LDADD = $(LDADD)
endif

bin_PROGRAMS += mkdrev mkdtext mkbgr mklex mkfsalex lslex lsbgr dumpdtext dumpdrev dumpbits dumplevel
noinst_HEADERS = message.hh

mkbgr_SOURCES = mkbgr.cc message.cc
lslex_SOURCES = lslex.cc message.cc
lsbgr_SOURCES = lsbgr.cc message.cc

#AM_CPPFLAGS += -I$(top_srcdir) -I$(top_builddir)
#LDADD = $(top_builddir)/finlib/libfin.la
