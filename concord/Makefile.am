# Copyright (c) 2003-2013  Pavel Rychly, Milos Jakubicek

noinst_LTLIBRARIES = libconcord.la
libconcord_la_SOURCES = conccrit.cc concctx.cc concedit.cc concget.cc \
	concgrp.cc concord.cc concsave.cc concsort.cc concstat.cc concfreq.cc \
	concstrm.cc
concord_headers = concord.hh conccrit.hh concget.hh concstat.hh

if CXXLIB
pkginclude_HEADERS = $(concord_headers)
else
libconcord_la_SOURCES += $(concord_headers)
endif

AM_CPPFLAGS += -I${top_srcdir}/finlib -I$(top_srcdir)/corp -I$(top_srcdir)/query $(ZTHREAD_CPPFLAGS)

