# Copyright (c) 2016-2020  Milos Jakubicek

noinst_LTLIBRARIES = libfsa3.la
bin_PROGRAMS = mkhatlex sortuniq mkhatfsa lsfsa lsfsa_intersect lsfsa_left_intersect dumpfsa mkfsa

LDADD = libfsa3.la $(top_builddir)/hat-trie/src/libhat-trie.la

libfsa3_la_SOURCES = fsa.c writefsa.c fsa_op.c
pkginclude_HEADERS = common.h fsa.h fsa_op.h writefsa.h

AM_CPPFLAGS += -I$(top_builddir) -I$(top_builddir)/hat-trie
