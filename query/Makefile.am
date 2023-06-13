# Copyright (c) 2003-2023  Pavel Rychly, Milos Jakubicek

AM_CPPFLAGS += -I$(top_srcdir)/corp -I$(top_builddir)/finlib

noinst_LTLIBRARIES = libquery.la
libquery_la_SOURCES = cqpeval.cc frconcat.cc frstream.cc \
	frsop.hh parsop.hh

query_headers = cqpeval.hh

if CXXLIB
pkginclude_HEADERS = $(query_headers)
else
libquery_la_SOURCES += $(query_headers)
endif

EXTRA_DIST = cqpeval.y

cqpeval.cc: cqpeval.y
	$(BISON) -Werror --output=$@ $<
