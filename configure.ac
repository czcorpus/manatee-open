# Process this file with autoconf to produce a configure script.
# Copyright (c) 2003-2023  Pavel Rychly, Milos Jakubicek, Ondrej Herman, Jan Michelfeit

AC_COPYRIGHT([Copyright (c) 2003-2020, Pavel Rychly, Milos Jakubicek, Ondrej Herman, Jan Michelfeit])
AC_INIT([manatee],[open-2.223.6],[pary@fi.muni.cz])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR(config)
AC_CONFIG_SRCDIR(corp/corpus.hh)
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([config.hh])

# Checks for programs.
AC_PROG_CXX
AC_LANG([C++])
AX_CXX_COMPILE_STDCXX_11
AX_GCC_BUILTIN(__builtin_expect)
AX_GCC_BUILTIN(__builtin_clzll)
AX_GCC_BUILTIN(__builtin_ctzll)
if test "x$ax_cv_have___builtin_expect" = "xyes"; then
	AC_DEFINE([LIKELY(condition)], [__builtin_expect(static_cast<bool>(condition), 1)], [Defined so that it works in C++])
	AC_DEFINE([UNLIKELY(condition)], [__builtin_expect(static_cast<bool>(condition), 0)], [Defined so that it works in C++])
else
	AC_DEFINE([LIKELY(condition)], [(condition)], [Defined so that it works in C++])
	AC_DEFINE([UNLIKELY(condition)], [(condition)], [Defined so that it works in C++])
fi

# Check for ln -s, fall back to cp -p
AC_PROG_LN_S

# Check for availability of bison
AX_PROG_BISON([],[AC_MSG_ERROR([Bison not found])])
AC_PATH_PROG([BISON],[bison])
AX_PROG_BISON_VERSION([3.0.2],[],[AC_MSG_ERROR([Bison version at least 3.0.2 is required])])
AX_PROG_BISON_VERSION([3.6],[],[AC_DEFINE([BISON_OLD], [1], [Define to 1 if Bison version is less than 3.6])])

AM_CXXFLAGS="-fexceptions -fstack-protector --param=ssp-buffer-size=4"
AM_CPPFLAGS="-Wall -Wp,-D_FORTIFY_SOURCE=2 -Wno-deprecated"

# Checks for opening shared libraries suite.
LT_INIT
AC_CHECK_LIB([dl], [dlinit])

AX_SUBDIRS_CONFIGURE([hat-trie], [--with-pic])
cd hat-trie; ln -sf src hat-trie; cd ..

AC_SUBST([LIBTOOL], ["\$(SHELL) \$(abs_top_builddir)/libtool"])
# use absolute path to libtool so that build in the symlinked hat-trie directory works
AC_DEFINE([LEXICON_USE_FSA3], [1], ["Use FSA3 automata for lexicons"])


AC_ARG_ENABLE(pthread,[AS_HELP_STRING([--disable-pthread],[disable the threading with pthread])],,
		    enable_pthread="yes")
if test "x$enable_pthread" = "xyes" ; then
    AC_DEFINE(HAVE_PTHREAD, 1, [Define to 1 if you have the pthread library])
    PTHREAD_LIBS="-pthread"
fi
AC_SUBST(PTHREAD_LIBS)
AC_SUBST(AM_CXXFLAGS)
AC_SUBST(AM_CPPFLAGS)

# Checks for header files.
AC_CHECK_HEADERS([locale.h stdlib.h string.h unistd.h fsa3/writefsa.h hat-trie/hat-trie.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_SYS_LARGEFILE

# Checks for library functions.
AC_FUNC_STAT
AC_FUNC_MMAP
AC_CHECK_FUNCS([memmove setlocale strchr strdup strerror strpbrk])
AC_REPLACE_FUNCS([getopt])

case $host_os in
    *mingw* )  AM_CONDITIONAL(ADDPCREPOSIX, true) ;;
    *darwin* ) AM_CONDITIONAL(ADDPCREPOSIX, false)
               AC_DEFINE([DARWIN], [1], [MacOS/DARWIN system]) ;;
    * )        AM_CONDITIONAL(ADDPCREPOSIX, false) ;;
esac

# Swig
AX_PKG_SWIG([3.0], [], [ AC_MSG_ERROR([SWIG is required to build.]) ])

# Python
AC_ARG_ENABLE(python,[AS_HELP_STRING([--disable-python],[disable the python API])],,
		    enable_python="yes")
if test "x$enable_python" = "xyes" ; then
    AM_PATH_PYTHON
    AX_SWIG_PYTHON
fi
AM_CONDITIONAL([PYTHON_API_ENABLED], [test "x$enable_python" = "xyes"])

# Java
AC_ARG_ENABLE(java,[AS_HELP_STRING([--enable-java],[enable the java API])],enable_java="yes",)
if test "x$enable_java" = "xyes"; then
    AX_JNI_INCLUDE_DIR
    saved_CPPFLAGS=$CPPFLAGS
    for JNI_INCLUDE_DIR in $JNI_INCLUDE_DIRS ; do
        JAVA_CPPFLAGS="$JAVA_CPPFLAGS -I$JNI_INCLUDE_DIR "
    done
	CPPFLAGS="$CPPFLAGS $JAVA_CPPFLAGS"
    AC_CHECK_HEADER(jni.h, [
		AC_SUBST(JAVA_CPPFLAGS)
		enable_java=yes
	], [
        enable_java=no
	AC_MSG_WARN([cannot find Java include path])])
    CPPFLAGS=$saved_CPPFLAGS

fi
AM_CONDITIONAL([JAVA_API_ENABLED], [test "x$enable_java" = "xyes"])

# Ruby
AC_ARG_ENABLE(ruby,[AS_HELP_STRING([--enable-ruby],[enable the ruby API])],enable_ruby="yes",)
if test "x$enable_ruby" = "xyes"; then
	# locate ruby binary, CPPFLAGS and site directory
	AX_RUBY_DEVEL()

    saved_CPPFLAGS=$CPPFLAGS
    CPPFLAGS="$CPPFLAGS $RUBY_CPPFLAGS"
	AC_CHECK_HEADER(ruby.h, [enable_ruby=yes], [
    enable_ruby=no
	AC_MSG_WARN([cannot find Ruby include path, set RUBY_CPPFLAGS])])
    CPPFLAGS=$saved_CPPFLAGS
fi
AM_CONDITIONAL([RUBY_API_ENABLED], [test "x$enable_ruby" = "xyes"])

# Perl
AC_ARG_ENABLE(perl,[AS_HELP_STRING([--enable-perl],[enable the perl API])],enable_perl="yes",)
if test "x$enable_perl" = "xyes"; then
	# locate perl binary
	AX_WITH_PROG(PERL,perl)

	if test -z "$PERL_CPPFLAGS" ; then
		# locate header directory
	    PERL_INCLUDE_DIR=`$PERL -MConfig -e 'print "$Config{archlib}/CORE"'`
	    PERL_CPPFLAGS="-I$PERL_INCLUDE_DIR"
	fi

	# locate site directory for local modules
	case $prefix in
	NONE|/usr/local|/usr/local/)
		AC_SUBST([PERL_SITE_PKG],[[`$PERL -MConfig -e 'print "$Config{installsitearch}"'`]])
		;;
	*)
		AC_SUBST([PERL_SITE_PKG],[[`$PERL -MConfig -e 'print "$Config{installvendorarch}"' | sed "s,/usr/,$prefix/,"`]])
		;;
	esac

	AC_SUBST(PERL_CPPFLAGS)
    saved_CPPFLAGS=$CPPFLAGS
    CPPFLAGS="$CPPFLAGS $PERL_CPPFLAGS"
    AC_CHECK_HEADER(perl.h,[enable_perl=yes], [enable_perl=no
      AC_MSG_WARN([cannot find Perl include path, set PERL_CPPFLAGS])],
      [#include "EXTERN.h"
      ])
    CPPFLAGS=$saved_CPPFLAGS
fi
AM_CONDITIONAL([PERL_API_ENABLED], [test "x$enable_perl" = "xyes"])

# MANATEE_REGISTRY environment variable
AC_ARG_VAR(MANATEE_REGISTRY, [default MANATEE_REGISTRY path])
AM_CONDITIONAL(MANATEE_REGISTRY_STR, test -n "$MANATEE_REGISTRY")


case $host_os in
    *darwin* ) AC_DEFINE([DARWIN], [1], [MacOS/DARWIN system]) ;;
esac

# Regexp implementation selection (--with-icu, --with-pcre)
regexpimpl=regex
AC_ARG_WITH(icu,AS_HELP_STRING([--with-icu],[Use ICU regexps]),
[
   case "${withval}" in
       yes)
       AC_DEFINE(USE_ICU, 1, [Define to 1 if using ICU regexps])
       AC_MSG_NOTICE([selecting ICU regexps])
       AC_PATH_PROG(icu_config, icu-config, no)
       if test "$icu_config" = no; then
           AC_MSG_ERROR([Unable to find icu-config but ICU regexps were requested.])
       fi
	   CPPFLAGS="$CPPFLAGS `$icu_config --cppflags`"
       regexlibs=`$icu_config --ldflags`
       regexpimpl=icu
    ;;
       no)  ;;
       *) AC_MSG_ERROR(bad value ${withval} for --with-icu) ;;
   esac
])
AC_ARG_WITH(pcre,AS_HELP_STRING([--with-pcre],[Use PCRE regexps]),
[
   case "${withval}" in
       yes)
       AC_DEFINE(USE_PCRE, 1, [Define to 1 if using pcre regexps])
       AC_MSG_NOTICE([selecting pcre regexps])
       if test $regexpimpl = icu; then
          AC_MSG_ERROR([cannot use both ICU and pcre regexps, select one])
       fi
       test $PCRE_VERSION || PCRE_VERSION=`pcre-config --version` || AC_MSG_ERROR([pcre-config not found, export PCRE_VERSION=<pcreversion>])
       AX_COMPARE_VERSION([$PCRE_VERSION],[ge],[8.10],[AC_MSG_NOTICE([Found PCRE version $PCRE_VERSION])],[AC_MSG_ERROR([Need PCRE version >= 8.10])])
       regexpimpl=pcre

       # find pcre library
       if test -f pcre/configure; then
          AC_CONFIG_SUBDIRS([pcre])
          AC_SUBST([PCRE_SUBDIRS],[pcre])
          regexlibs='$(top_builddir)'/pcre/libpcre.la
          CPPFLAGS="$CPPFLAGS -I\$(top_srcdir)/pcre"
       else if test -f ../pcre/configure; then
          regexlibs='$(top_builddir)'/../pcre/libpcre.la
          CPPFLAGS="$CPPFLAGS -I\$(top_srcdir)/../pcre"
       else
          regexlibs=-lpcre
       fi
       fi
       AC_MSG_NOTICE([PCRE from $regexlibs])
    ;;
       no)  ;;
       *) AC_MSG_ERROR(bad value ${withval} for --with-pcre) ;;
   esac
])

if test $regexpimpl = regex; then
     AC_DEFINE(USE_REGEX, 1, [Define to 1 if using standard regexps])
     AC_MSG_NOTICE([selecting standard regexps])
     regexlibs=
fi
AC_SUBST(regexlibs)
AC_MSG_NOTICE(regexlibs is ${regexlibs})


AC_DEFINE_UNQUOTED(VERSION_STR, ["$PACKAGE_VERSION"], [Manatee version number])
AC_DEFINE_UNQUOTED(VERSION_STR, ["$PACKAGE_VERSION"], [Manatee version number])

AC_ARG_ENABLE(cxxlib,[AS_HELP_STRING([--enable-cxxlib],[install C++ libraries])],
[
   case "${enableval}" in
       yes) cxxlib=true 
	AC_DEFINE(CXXLIB, 1, [install C++ libraries])
	AC_MSG_NOTICE([installation will include C++ libraries])
	;;
       no)  cxxlib=false ;;
       *) AC_MSG_ERROR(bad value ${enableval} for --enable-cxxlib) ;;
     esac
],[cxxlib=false])
AM_CONDITIONAL(CXXLIB, test x$cxxlib = xtrue)

AC_ARG_ENABLE(assert,[AS_HELP_STRING([--enable-assert],[compile assertions (use for debugging purposes only)])],
[
   case "${enableval}" in
       yes) assert=true
	AC_DEFINE(DEBUG, 1, [enable assertions])
	AC_MSG_NOTICE([assertions enabled])
	;;
       no)  assert=false ;;
       *) AC_MSG_ERROR(bad value ${enableval} for --enable-assert) ;;
     esac
],[assert=false])

AC_CONFIG_FILES(Makefile finlib/Makefile src/Makefile corp/Makefile concord/Makefile query/Makefile api/Makefile config/dpkg-control fsa3/Makefile)

AC_OUTPUT
