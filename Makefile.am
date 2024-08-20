SUBDIRS = hat-trie fsa3 finlib corp query concord src api
ACLOCAL_AMFLAGS = -I config

doxygen:
	doxygen

changelog:
	wget http://trac.sketchengine.co.uk/wiki/SkE/Manatee?format=txt -O - | dos2unix > ChangeLog

.PHONY:	doxygen changelog

EXTRA_DIST = doc/*.txt doc/manatee.sgml doc/pcre.html doc/ske-stat.tex doc/features doc/inst.readme
