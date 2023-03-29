# Copyright (c) 2003-2019  Pavel Rychly, Milos Jakubicek

noinst_LTLIBRARIES = libcorp.la
libcorp_la_SOURCES = corpconf.cc corpus.cc dynattr.cc dynfun.cc \
	loadconf.cc posattr.cc struct.cc subcorp.cc bgrstat.cc calign.cc \
	dynattr.hh dynfun.hh virtcorp.cc utf8.cc corpstat.cc \
	pauniq.cc pauniq.hh levels.cc virtstruc.cc regexopt.cc regexopt.hh \
	normattr.cc normattr.hh wordlist.cc wordlist.hh keyword.cc keyword.hh
libcorp_la_LIBADD = -ldl
corp_headers = corpus.hh corpconf.hh posattr.hh bgrstat.hh subcorp.hh \
	virtcorp.hh utf8.hh levels.hh revit.hh

if CXXLIB
pkginclude_HEADERS = $(corp_headers)
else
libcorp_la_SOURCES += $(corp_headers)
endif

EXTRA_DIST = stddynfun.c langcodes.cc corpconf.y regexopt.y regexoptwalker.cc tlds.h

AM_CPPFLAGS += -I$(top_srcdir)/finlib -I$(top_srcdir)/query -I$(top_srcdir)/stat

if MANATEE_REGISTRY_STR
AM_CPPFLAGS += -DMANATEE_REGISTRY_STR='"@MANATEE_REGISTRY@"'
endif

loadconf.cc: corpconf.y
	$(YACC) --output=$@ $<
regexopt.cc: regexopt.y
	$(YACC) --output=$@ $<
