# Copyright (c) 2003-2022  Pavel Rychly, Milos Jakubicek

noinst_LTLIBRARIES = libfin.la
libfin_la_SOURCES = lexicon.cc range.cc fstream.cc fsstat.cc \
	regpref.cc writelex.cc replacement.cc \
	consumer.cc revcons.cc log.cc regexplex.cc util.cc
pkginclude_HEADERS = bitio.hh bititer.hh binfile.hh \
	revidx.hh compstream.hh text.hh fsop.hh refcount.hh regexplex.hh \
	srtruns.hh bigram.hh excep.hh fingetopt.hh fromtof.hh  \
	generator.hh lexicon.hh range.hh frstream.hh fstream.hh fsstat.hh \
	regpref.hh writelex.hh log.hh \
	consumer.hh seek.hh util.hh # config.hh

AM_CPPFLAGS += -I.. -I$(top_srcdir)/hat-trie # $(top_srcdir) -I$(top_builddir)
libfin_la_LIBADD = $(regexlibs)
# libfin_la_LIBADD = $(REGEX_LDFLAG)
# libfin_la_LDFLAGS = -version-info $(CURRENT):$(REVISION):$(AGE)
