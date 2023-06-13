# generated automatically by aclocal 1.16.5 -*- Autoconf -*-

# Copyright (C) 1996-2021 Free Software Foundation, Inc.

# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

m4_ifndef([AC_CONFIG_MACRO_DIRS], [m4_defun([_AM_CONFIG_MACRO_DIRS], [])m4_defun([AC_CONFIG_MACRO_DIRS], [_AM_CONFIG_MACRO_DIRS($@)])])
m4_ifndef([AC_AUTOCONF_VERSION],
  [m4_copy([m4_PACKAGE_VERSION], [AC_AUTOCONF_VERSION])])dnl
m4_if(m4_defn([AC_AUTOCONF_VERSION]), [2.71],,
[m4_warning([this file was generated for autoconf 2.71.
You have another version of autoconf.  It may work, but is not guaranteed to.
If you have problems, you may need to regenerate the build system entirely.
To do so, use the procedure documented by the package, typically 'autoreconf'.])])

# ===========================================================================
#    https://www.gnu.org/software/autoconf-archive/ax_compare_version.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_COMPARE_VERSION(VERSION_A, OP, VERSION_B, [ACTION-IF-TRUE], [ACTION-IF-FALSE])
#
# DESCRIPTION
#
#   This macro compares two version strings. Due to the various number of
#   minor-version numbers that can exist, and the fact that string
#   comparisons are not compatible with numeric comparisons, this is not
#   necessarily trivial to do in a autoconf script. This macro makes doing
#   these comparisons easy.
#
#   The six basic comparisons are available, as well as checking equality
#   limited to a certain number of minor-version levels.
#
#   The operator OP determines what type of comparison to do, and can be one
#   of:
#
#    eq  - equal (test A == B)
#    ne  - not equal (test A != B)
#    le  - less than or equal (test A <= B)
#    ge  - greater than or equal (test A >= B)
#    lt  - less than (test A < B)
#    gt  - greater than (test A > B)
#
#   Additionally, the eq and ne operator can have a number after it to limit
#   the test to that number of minor versions.
#
#    eq0 - equal up to the length of the shorter version
#    ne0 - not equal up to the length of the shorter version
#    eqN - equal up to N sub-version levels
#    neN - not equal up to N sub-version levels
#
#   When the condition is true, shell commands ACTION-IF-TRUE are run,
#   otherwise shell commands ACTION-IF-FALSE are run. The environment
#   variable 'ax_compare_version' is always set to either 'true' or 'false'
#   as well.
#
#   Examples:
#
#     AX_COMPARE_VERSION([3.15.7],[lt],[3.15.8])
#     AX_COMPARE_VERSION([3.15],[lt],[3.15.8])
#
#   would both be true.
#
#     AX_COMPARE_VERSION([3.15.7],[eq],[3.15.8])
#     AX_COMPARE_VERSION([3.15],[gt],[3.15.8])
#
#   would both be false.
#
#     AX_COMPARE_VERSION([3.15.7],[eq2],[3.15.8])
#
#   would be true because it is only comparing two minor versions.
#
#     AX_COMPARE_VERSION([3.15.7],[eq0],[3.15])
#
#   would be true because it is only comparing the lesser number of minor
#   versions of the two values.
#
#   Note: The characters that separate the version numbers do not matter. An
#   empty string is the same as version 0. OP is evaluated by autoconf, not
#   configure, so must be a string, not a variable.
#
#   The author would like to acknowledge Guido Draheim whose advice about
#   the m4_case and m4_ifvaln functions make this macro only include the
#   portions necessary to perform the specific comparison specified by the
#   OP argument in the final configure script.
#
# LICENSE
#
#   Copyright (c) 2008 Tim Toolan <toolan@ele.uri.edu>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 13

dnl #########################################################################
AC_DEFUN([AX_COMPARE_VERSION], [
  AC_REQUIRE([AC_PROG_AWK])

  # Used to indicate true or false condition
  ax_compare_version=false

  # Convert the two version strings to be compared into a format that
  # allows a simple string comparison.  The end result is that a version
  # string of the form 1.12.5-r617 will be converted to the form
  # 0001001200050617.  In other words, each number is zero padded to four
  # digits, and non digits are removed.
  AS_VAR_PUSHDEF([A],[ax_compare_version_A])
  A=`echo "$1" | sed -e 's/\([[0-9]]*\)/Z\1Z/g' \
                     -e 's/Z\([[0-9]]\)Z/Z0\1Z/g' \
                     -e 's/Z\([[0-9]][[0-9]]\)Z/Z0\1Z/g' \
                     -e 's/Z\([[0-9]][[0-9]][[0-9]]\)Z/Z0\1Z/g' \
                     -e 's/[[^0-9]]//g'`

  AS_VAR_PUSHDEF([B],[ax_compare_version_B])
  B=`echo "$3" | sed -e 's/\([[0-9]]*\)/Z\1Z/g' \
                     -e 's/Z\([[0-9]]\)Z/Z0\1Z/g' \
                     -e 's/Z\([[0-9]][[0-9]]\)Z/Z0\1Z/g' \
                     -e 's/Z\([[0-9]][[0-9]][[0-9]]\)Z/Z0\1Z/g' \
                     -e 's/[[^0-9]]//g'`

  dnl # In the case of le, ge, lt, and gt, the strings are sorted as necessary
  dnl # then the first line is used to determine if the condition is true.
  dnl # The sed right after the echo is to remove any indented white space.
  m4_case(m4_tolower($2),
  [lt],[
    ax_compare_version=`echo "x$A
x$B" | sed 's/^ *//' | sort -r | sed "s/x${A}/false/;s/x${B}/true/;1q"`
  ],
  [gt],[
    ax_compare_version=`echo "x$A
x$B" | sed 's/^ *//' | sort | sed "s/x${A}/false/;s/x${B}/true/;1q"`
  ],
  [le],[
    ax_compare_version=`echo "x$A
x$B" | sed 's/^ *//' | sort | sed "s/x${A}/true/;s/x${B}/false/;1q"`
  ],
  [ge],[
    ax_compare_version=`echo "x$A
x$B" | sed 's/^ *//' | sort -r | sed "s/x${A}/true/;s/x${B}/false/;1q"`
  ],[
    dnl Split the operator from the subversion count if present.
    m4_bmatch(m4_substr($2,2),
    [0],[
      # A count of zero means use the length of the shorter version.
      # Determine the number of characters in A and B.
      ax_compare_version_len_A=`echo "$A" | $AWK '{print(length)}'`
      ax_compare_version_len_B=`echo "$B" | $AWK '{print(length)}'`

      # Set A to no more than B's length and B to no more than A's length.
      A=`echo "$A" | sed "s/\(.\{$ax_compare_version_len_B\}\).*/\1/"`
      B=`echo "$B" | sed "s/\(.\{$ax_compare_version_len_A\}\).*/\1/"`
    ],
    [[0-9]+],[
      # A count greater than zero means use only that many subversions
      A=`echo "$A" | sed "s/\(\([[0-9]]\{4\}\)\{m4_substr($2,2)\}\).*/\1/"`
      B=`echo "$B" | sed "s/\(\([[0-9]]\{4\}\)\{m4_substr($2,2)\}\).*/\1/"`
    ],
    [.+],[
      AC_WARNING(
        [invalid OP numeric parameter: $2])
    ],[])

    # Pad zeros at end of numbers to make same length.
    ax_compare_version_tmp_A="$A`echo $B | sed 's/./0/g'`"
    B="$B`echo $A | sed 's/./0/g'`"
    A="$ax_compare_version_tmp_A"

    # Check for equality or inequality as necessary.
    m4_case(m4_tolower(m4_substr($2,0,2)),
    [eq],[
      test "x$A" = "x$B" && ax_compare_version=true
    ],
    [ne],[
      test "x$A" != "x$B" && ax_compare_version=true
    ],[
      AC_WARNING([invalid OP parameter: $2])
    ])
  ])

  AS_VAR_POPDEF([A])dnl
  AS_VAR_POPDEF([B])dnl

  dnl # Execute ACTION-IF-TRUE / ACTION-IF-FALSE.
  if test "$ax_compare_version" = "true" ; then
    m4_ifvaln([$4],[$4],[:])dnl
    m4_ifvaln([$5],[else $5])dnl
  fi
]) dnl AX_COMPARE_VERSION

# ===========================================================================
#  https://www.gnu.org/software/autoconf-archive/ax_cxx_compile_stdcxx.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_CXX_COMPILE_STDCXX(VERSION, [ext|noext], [mandatory|optional])
#
# DESCRIPTION
#
#   Check for baseline language coverage in the compiler for the specified
#   version of the C++ standard.  If necessary, add switches to CXX and
#   CXXCPP to enable support.  VERSION may be '11', '14', '17', or '20' for
#   the respective C++ standard version.
#
#   The second argument, if specified, indicates whether you insist on an
#   extended mode (e.g. -std=gnu++11) or a strict conformance mode (e.g.
#   -std=c++11).  If neither is specified, you get whatever works, with
#   preference for no added switch, and then for an extended mode.
#
#   The third argument, if specified 'mandatory' or if left unspecified,
#   indicates that baseline support for the specified C++ standard is
#   required and that the macro should error out if no mode with that
#   support is found.  If specified 'optional', then configuration proceeds
#   regardless, after defining HAVE_CXX${VERSION} if and only if a
#   supporting mode is found.
#
# LICENSE
#
#   Copyright (c) 2008 Benjamin Kosnik <bkoz@redhat.com>
#   Copyright (c) 2012 Zack Weinberg <zackw@panix.com>
#   Copyright (c) 2013 Roy Stogner <roystgnr@ices.utexas.edu>
#   Copyright (c) 2014, 2015 Google Inc.; contributed by Alexey Sokolov <sokolov@google.com>
#   Copyright (c) 2015 Paul Norman <penorman@mac.com>
#   Copyright (c) 2015 Moritz Klammler <moritz@klammler.eu>
#   Copyright (c) 2016, 2018 Krzesimir Nowak <qdlacz@gmail.com>
#   Copyright (c) 2019 Enji Cooper <yaneurabeya@gmail.com>
#   Copyright (c) 2020 Jason Merrill <jason@redhat.com>
#   Copyright (c) 2021 Jörn Heusipp <osmanx@problemloesungsmaschine.de>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved.  This file is offered as-is, without any
#   warranty.

#serial 14

dnl  This macro is based on the code from the AX_CXX_COMPILE_STDCXX_11 macro
dnl  (serial version number 13).

AC_DEFUN([AX_CXX_COMPILE_STDCXX], [dnl
  m4_if([$1], [11], [ax_cxx_compile_alternatives="11 0x"],
        [$1], [14], [ax_cxx_compile_alternatives="14 1y"],
        [$1], [17], [ax_cxx_compile_alternatives="17 1z"],
        [$1], [20], [ax_cxx_compile_alternatives="20"],
        [m4_fatal([invalid first argument `$1' to AX_CXX_COMPILE_STDCXX])])dnl
  m4_if([$2], [], [],
        [$2], [ext], [],
        [$2], [noext], [],
        [m4_fatal([invalid second argument `$2' to AX_CXX_COMPILE_STDCXX])])dnl
  m4_if([$3], [], [ax_cxx_compile_cxx$1_required=true],
        [$3], [mandatory], [ax_cxx_compile_cxx$1_required=true],
        [$3], [optional], [ax_cxx_compile_cxx$1_required=false],
        [m4_fatal([invalid third argument `$3' to AX_CXX_COMPILE_STDCXX])])
  AC_LANG_PUSH([C++])dnl
  ac_success=no

  m4_if([$2], [], [dnl
    AC_CACHE_CHECK(whether $CXX supports C++$1 features by default,
		   ax_cv_cxx_compile_cxx$1,
      [AC_COMPILE_IFELSE([AC_LANG_SOURCE([_AX_CXX_COMPILE_STDCXX_testbody_$1])],
        [ax_cv_cxx_compile_cxx$1=yes],
        [ax_cv_cxx_compile_cxx$1=no])])
    if test x$ax_cv_cxx_compile_cxx$1 = xyes; then
      ac_success=yes
    fi])

  m4_if([$2], [noext], [], [dnl
  if test x$ac_success = xno; then
    for alternative in ${ax_cxx_compile_alternatives}; do
      switch="-std=gnu++${alternative}"
      cachevar=AS_TR_SH([ax_cv_cxx_compile_cxx$1_$switch])
      AC_CACHE_CHECK(whether $CXX supports C++$1 features with $switch,
                     $cachevar,
        [ac_save_CXX="$CXX"
         CXX="$CXX $switch"
         AC_COMPILE_IFELSE([AC_LANG_SOURCE([_AX_CXX_COMPILE_STDCXX_testbody_$1])],
          [eval $cachevar=yes],
          [eval $cachevar=no])
         CXX="$ac_save_CXX"])
      if eval test x\$$cachevar = xyes; then
        CXX="$CXX $switch"
        if test -n "$CXXCPP" ; then
          CXXCPP="$CXXCPP $switch"
        fi
        ac_success=yes
        break
      fi
    done
  fi])

  m4_if([$2], [ext], [], [dnl
  if test x$ac_success = xno; then
    dnl HP's aCC needs +std=c++11 according to:
    dnl http://h21007.www2.hp.com/portal/download/files/unprot/aCxx/PDF_Release_Notes/769149-001.pdf
    dnl Cray's crayCC needs "-h std=c++11"
    for alternative in ${ax_cxx_compile_alternatives}; do
      for switch in -std=c++${alternative} +std=c++${alternative} "-h std=c++${alternative}"; do
        cachevar=AS_TR_SH([ax_cv_cxx_compile_cxx$1_$switch])
        AC_CACHE_CHECK(whether $CXX supports C++$1 features with $switch,
                       $cachevar,
          [ac_save_CXX="$CXX"
           CXX="$CXX $switch"
           AC_COMPILE_IFELSE([AC_LANG_SOURCE([_AX_CXX_COMPILE_STDCXX_testbody_$1])],
            [eval $cachevar=yes],
            [eval $cachevar=no])
           CXX="$ac_save_CXX"])
        if eval test x\$$cachevar = xyes; then
          CXX="$CXX $switch"
          if test -n "$CXXCPP" ; then
            CXXCPP="$CXXCPP $switch"
          fi
          ac_success=yes
          break
        fi
      done
      if test x$ac_success = xyes; then
        break
      fi
    done
  fi])
  AC_LANG_POP([C++])
  if test x$ax_cxx_compile_cxx$1_required = xtrue; then
    if test x$ac_success = xno; then
      AC_MSG_ERROR([*** A compiler with support for C++$1 language features is required.])
    fi
  fi
  if test x$ac_success = xno; then
    HAVE_CXX$1=0
    AC_MSG_NOTICE([No compiler with C++$1 support was found])
  else
    HAVE_CXX$1=1
    AC_DEFINE(HAVE_CXX$1,1,
              [define if the compiler supports basic C++$1 syntax])
  fi
  AC_SUBST(HAVE_CXX$1)
])


dnl  Test body for checking C++11 support

m4_define([_AX_CXX_COMPILE_STDCXX_testbody_11],
  _AX_CXX_COMPILE_STDCXX_testbody_new_in_11
)

dnl  Test body for checking C++14 support

m4_define([_AX_CXX_COMPILE_STDCXX_testbody_14],
  _AX_CXX_COMPILE_STDCXX_testbody_new_in_11
  _AX_CXX_COMPILE_STDCXX_testbody_new_in_14
)

dnl  Test body for checking C++17 support

m4_define([_AX_CXX_COMPILE_STDCXX_testbody_17],
  _AX_CXX_COMPILE_STDCXX_testbody_new_in_11
  _AX_CXX_COMPILE_STDCXX_testbody_new_in_14
  _AX_CXX_COMPILE_STDCXX_testbody_new_in_17
)

dnl  Test body for checking C++20 support

m4_define([_AX_CXX_COMPILE_STDCXX_testbody_20],
  _AX_CXX_COMPILE_STDCXX_testbody_new_in_11
  _AX_CXX_COMPILE_STDCXX_testbody_new_in_14
  _AX_CXX_COMPILE_STDCXX_testbody_new_in_17
  _AX_CXX_COMPILE_STDCXX_testbody_new_in_20
)


dnl  Tests for new features in C++11

m4_define([_AX_CXX_COMPILE_STDCXX_testbody_new_in_11], [[

// If the compiler admits that it is not ready for C++11, why torture it?
// Hopefully, this will speed up the test.

#ifndef __cplusplus

#error "This is not a C++ compiler"

#elif __cplusplus < 201103L

#error "This is not a C++11 compiler"

#else

namespace cxx11
{

  namespace test_static_assert
  {

    template <typename T>
    struct check
    {
      static_assert(sizeof(int) <= sizeof(T), "not big enough");
    };

  }

  namespace test_final_override
  {

    struct Base
    {
      virtual ~Base() {}
      virtual void f() {}
    };

    struct Derived : public Base
    {
      virtual ~Derived() override {}
      virtual void f() override {}
    };

  }

  namespace test_double_right_angle_brackets
  {

    template < typename T >
    struct check {};

    typedef check<void> single_type;
    typedef check<check<void>> double_type;
    typedef check<check<check<void>>> triple_type;
    typedef check<check<check<check<void>>>> quadruple_type;

  }

  namespace test_decltype
  {

    int
    f()
    {
      int a = 1;
      decltype(a) b = 2;
      return a + b;
    }

  }

  namespace test_type_deduction
  {

    template < typename T1, typename T2 >
    struct is_same
    {
      static const bool value = false;
    };

    template < typename T >
    struct is_same<T, T>
    {
      static const bool value = true;
    };

    template < typename T1, typename T2 >
    auto
    add(T1 a1, T2 a2) -> decltype(a1 + a2)
    {
      return a1 + a2;
    }

    int
    test(const int c, volatile int v)
    {
      static_assert(is_same<int, decltype(0)>::value == true, "");
      static_assert(is_same<int, decltype(c)>::value == false, "");
      static_assert(is_same<int, decltype(v)>::value == false, "");
      auto ac = c;
      auto av = v;
      auto sumi = ac + av + 'x';
      auto sumf = ac + av + 1.0;
      static_assert(is_same<int, decltype(ac)>::value == true, "");
      static_assert(is_same<int, decltype(av)>::value == true, "");
      static_assert(is_same<int, decltype(sumi)>::value == true, "");
      static_assert(is_same<int, decltype(sumf)>::value == false, "");
      static_assert(is_same<int, decltype(add(c, v))>::value == true, "");
      return (sumf > 0.0) ? sumi : add(c, v);
    }

  }

  namespace test_noexcept
  {

    int f() { return 0; }
    int g() noexcept { return 0; }

    static_assert(noexcept(f()) == false, "");
    static_assert(noexcept(g()) == true, "");

  }

  namespace test_constexpr
  {

    template < typename CharT >
    unsigned long constexpr
    strlen_c_r(const CharT *const s, const unsigned long acc) noexcept
    {
      return *s ? strlen_c_r(s + 1, acc + 1) : acc;
    }

    template < typename CharT >
    unsigned long constexpr
    strlen_c(const CharT *const s) noexcept
    {
      return strlen_c_r(s, 0UL);
    }

    static_assert(strlen_c("") == 0UL, "");
    static_assert(strlen_c("1") == 1UL, "");
    static_assert(strlen_c("example") == 7UL, "");
    static_assert(strlen_c("another\0example") == 7UL, "");

  }

  namespace test_rvalue_references
  {

    template < int N >
    struct answer
    {
      static constexpr int value = N;
    };

    answer<1> f(int&)       { return answer<1>(); }
    answer<2> f(const int&) { return answer<2>(); }
    answer<3> f(int&&)      { return answer<3>(); }

    void
    test()
    {
      int i = 0;
      const int c = 0;
      static_assert(decltype(f(i))::value == 1, "");
      static_assert(decltype(f(c))::value == 2, "");
      static_assert(decltype(f(0))::value == 3, "");
    }

  }

  namespace test_uniform_initialization
  {

    struct test
    {
      static const int zero {};
      static const int one {1};
    };

    static_assert(test::zero == 0, "");
    static_assert(test::one == 1, "");

  }

  namespace test_lambdas
  {

    void
    test1()
    {
      auto lambda1 = [](){};
      auto lambda2 = lambda1;
      lambda1();
      lambda2();
    }

    int
    test2()
    {
      auto a = [](int i, int j){ return i + j; }(1, 2);
      auto b = []() -> int { return '0'; }();
      auto c = [=](){ return a + b; }();
      auto d = [&](){ return c; }();
      auto e = [a, &b](int x) mutable {
        const auto identity = [](int y){ return y; };
        for (auto i = 0; i < a; ++i)
          a += b--;
        return x + identity(a + b);
      }(0);
      return a + b + c + d + e;
    }

    int
    test3()
    {
      const auto nullary = [](){ return 0; };
      const auto unary = [](int x){ return x; };
      using nullary_t = decltype(nullary);
      using unary_t = decltype(unary);
      const auto higher1st = [](nullary_t f){ return f(); };
      const auto higher2nd = [unary](nullary_t f1){
        return [unary, f1](unary_t f2){ return f2(unary(f1())); };
      };
      return higher1st(nullary) + higher2nd(nullary)(unary);
    }

  }

  namespace test_variadic_templates
  {

    template <int...>
    struct sum;

    template <int N0, int... N1toN>
    struct sum<N0, N1toN...>
    {
      static constexpr auto value = N0 + sum<N1toN...>::value;
    };

    template <>
    struct sum<>
    {
      static constexpr auto value = 0;
    };

    static_assert(sum<>::value == 0, "");
    static_assert(sum<1>::value == 1, "");
    static_assert(sum<23>::value == 23, "");
    static_assert(sum<1, 2>::value == 3, "");
    static_assert(sum<5, 5, 11>::value == 21, "");
    static_assert(sum<2, 3, 5, 7, 11, 13>::value == 41, "");

  }

  // http://stackoverflow.com/questions/13728184/template-aliases-and-sfinae
  // Clang 3.1 fails with headers of libstd++ 4.8.3 when using std::function
  // because of this.
  namespace test_template_alias_sfinae
  {

    struct foo {};

    template<typename T>
    using member = typename T::member_type;

    template<typename T>
    void func(...) {}

    template<typename T>
    void func(member<T>*) {}

    void test();

    void test() { func<foo>(0); }

  }

}  // namespace cxx11

#endif  // __cplusplus >= 201103L

]])


dnl  Tests for new features in C++14

m4_define([_AX_CXX_COMPILE_STDCXX_testbody_new_in_14], [[

// If the compiler admits that it is not ready for C++14, why torture it?
// Hopefully, this will speed up the test.

#ifndef __cplusplus

#error "This is not a C++ compiler"

#elif __cplusplus < 201402L

#error "This is not a C++14 compiler"

#else

namespace cxx14
{

  namespace test_polymorphic_lambdas
  {

    int
    test()
    {
      const auto lambda = [](auto&&... args){
        const auto istiny = [](auto x){
          return (sizeof(x) == 1UL) ? 1 : 0;
        };
        const int aretiny[] = { istiny(args)... };
        return aretiny[0];
      };
      return lambda(1, 1L, 1.0f, '1');
    }

  }

  namespace test_binary_literals
  {

    constexpr auto ivii = 0b0000000000101010;
    static_assert(ivii == 42, "wrong value");

  }

  namespace test_generalized_constexpr
  {

    template < typename CharT >
    constexpr unsigned long
    strlen_c(const CharT *const s) noexcept
    {
      auto length = 0UL;
      for (auto p = s; *p; ++p)
        ++length;
      return length;
    }

    static_assert(strlen_c("") == 0UL, "");
    static_assert(strlen_c("x") == 1UL, "");
    static_assert(strlen_c("test") == 4UL, "");
    static_assert(strlen_c("another\0test") == 7UL, "");

  }

  namespace test_lambda_init_capture
  {

    int
    test()
    {
      auto x = 0;
      const auto lambda1 = [a = x](int b){ return a + b; };
      const auto lambda2 = [a = lambda1(x)](){ return a; };
      return lambda2();
    }

  }

  namespace test_digit_separators
  {

    constexpr auto ten_million = 100'000'000;
    static_assert(ten_million == 100000000, "");

  }

  namespace test_return_type_deduction
  {

    auto f(int& x) { return x; }
    decltype(auto) g(int& x) { return x; }

    template < typename T1, typename T2 >
    struct is_same
    {
      static constexpr auto value = false;
    };

    template < typename T >
    struct is_same<T, T>
    {
      static constexpr auto value = true;
    };

    int
    test()
    {
      auto x = 0;
      static_assert(is_same<int, decltype(f(x))>::value, "");
      static_assert(is_same<int&, decltype(g(x))>::value, "");
      return x;
    }

  }

}  // namespace cxx14

#endif  // __cplusplus >= 201402L

]])


dnl  Tests for new features in C++17

m4_define([_AX_CXX_COMPILE_STDCXX_testbody_new_in_17], [[

// If the compiler admits that it is not ready for C++17, why torture it?
// Hopefully, this will speed up the test.

#ifndef __cplusplus

#error "This is not a C++ compiler"

#elif __cplusplus < 201703L

#error "This is not a C++17 compiler"

#else

#include <initializer_list>
#include <utility>
#include <type_traits>

namespace cxx17
{

  namespace test_constexpr_lambdas
  {

    constexpr int foo = [](){return 42;}();

  }

  namespace test::nested_namespace::definitions
  {

  }

  namespace test_fold_expression
  {

    template<typename... Args>
    int multiply(Args... args)
    {
      return (args * ... * 1);
    }

    template<typename... Args>
    bool all(Args... args)
    {
      return (args && ...);
    }

  }

  namespace test_extended_static_assert
  {

    static_assert (true);

  }

  namespace test_auto_brace_init_list
  {

    auto foo = {5};
    auto bar {5};

    static_assert(std::is_same<std::initializer_list<int>, decltype(foo)>::value);
    static_assert(std::is_same<int, decltype(bar)>::value);
  }

  namespace test_typename_in_template_template_parameter
  {

    template<template<typename> typename X> struct D;

  }

  namespace test_fallthrough_nodiscard_maybe_unused_attributes
  {

    int f1()
    {
      return 42;
    }

    [[nodiscard]] int f2()
    {
      [[maybe_unused]] auto unused = f1();

      switch (f1())
      {
      case 17:
        f1();
        [[fallthrough]];
      case 42:
        f1();
      }
      return f1();
    }

  }

  namespace test_extended_aggregate_initialization
  {

    struct base1
    {
      int b1, b2 = 42;
    };

    struct base2
    {
      base2() {
        b3 = 42;
      }
      int b3;
    };

    struct derived : base1, base2
    {
        int d;
    };

    derived d1 {{1, 2}, {}, 4};  // full initialization
    derived d2 {{}, {}, 4};      // value-initialized bases

  }

  namespace test_general_range_based_for_loop
  {

    struct iter
    {
      int i;

      int& operator* ()
      {
        return i;
      }

      const int& operator* () const
      {
        return i;
      }

      iter& operator++()
      {
        ++i;
        return *this;
      }
    };

    struct sentinel
    {
      int i;
    };

    bool operator== (const iter& i, const sentinel& s)
    {
      return i.i == s.i;
    }

    bool operator!= (const iter& i, const sentinel& s)
    {
      return !(i == s);
    }

    struct range
    {
      iter begin() const
      {
        return {0};
      }

      sentinel end() const
      {
        return {5};
      }
    };

    void f()
    {
      range r {};

      for (auto i : r)
      {
        [[maybe_unused]] auto v = i;
      }
    }

  }

  namespace test_lambda_capture_asterisk_this_by_value
  {

    struct t
    {
      int i;
      int foo()
      {
        return [*this]()
        {
          return i;
        }();
      }
    };

  }

  namespace test_enum_class_construction
  {

    enum class byte : unsigned char
    {};

    byte foo {42};

  }

  namespace test_constexpr_if
  {

    template <bool cond>
    int f ()
    {
      if constexpr(cond)
      {
        return 13;
      }
      else
      {
        return 42;
      }
    }

  }

  namespace test_selection_statement_with_initializer
  {

    int f()
    {
      return 13;
    }

    int f2()
    {
      if (auto i = f(); i > 0)
      {
        return 3;
      }

      switch (auto i = f(); i + 4)
      {
      case 17:
        return 2;

      default:
        return 1;
      }
    }

  }

  namespace test_template_argument_deduction_for_class_templates
  {

    template <typename T1, typename T2>
    struct pair
    {
      pair (T1 p1, T2 p2)
        : m1 {p1},
          m2 {p2}
      {}

      T1 m1;
      T2 m2;
    };

    void f()
    {
      [[maybe_unused]] auto p = pair{13, 42u};
    }

  }

  namespace test_non_type_auto_template_parameters
  {

    template <auto n>
    struct B
    {};

    B<5> b1;
    B<'a'> b2;

  }

  namespace test_structured_bindings
  {

    int arr[2] = { 1, 2 };
    std::pair<int, int> pr = { 1, 2 };

    auto f1() -> int(&)[2]
    {
      return arr;
    }

    auto f2() -> std::pair<int, int>&
    {
      return pr;
    }

    struct S
    {
      int x1 : 2;
      volatile double y1;
    };

    S f3()
    {
      return {};
    }

    auto [ x1, y1 ] = f1();
    auto& [ xr1, yr1 ] = f1();
    auto [ x2, y2 ] = f2();
    auto& [ xr2, yr2 ] = f2();
    const auto [ x3, y3 ] = f3();

  }

  namespace test_exception_spec_type_system
  {

    struct Good {};
    struct Bad {};

    void g1() noexcept;
    void g2();

    template<typename T>
    Bad
    f(T*, T*);

    template<typename T1, typename T2>
    Good
    f(T1*, T2*);

    static_assert (std::is_same_v<Good, decltype(f(g1, g2))>);

  }

  namespace test_inline_variables
  {

    template<class T> void f(T)
    {}

    template<class T> inline T g(T)
    {
      return T{};
    }

    template<> inline void f<>(int)
    {}

    template<> int g<>(int)
    {
      return 5;
    }

  }

}  // namespace cxx17

#endif  // __cplusplus < 201703L

]])


dnl  Tests for new features in C++20

m4_define([_AX_CXX_COMPILE_STDCXX_testbody_new_in_20], [[

#ifndef __cplusplus

#error "This is not a C++ compiler"

#elif __cplusplus < 202002L

#error "This is not a C++20 compiler"

#else

#include <version>

namespace cxx20
{

// As C++20 supports feature test macros in the standard, there is no
// immediate need to actually test for feature availability on the
// Autoconf side.

}  // namespace cxx20

#endif  // __cplusplus < 202002L

]])

# =============================================================================
#  https://www.gnu.org/software/autoconf-archive/ax_cxx_compile_stdcxx_11.html
# =============================================================================
#
# SYNOPSIS
#
#   AX_CXX_COMPILE_STDCXX_11([ext|noext], [mandatory|optional])
#
# DESCRIPTION
#
#   Check for baseline language coverage in the compiler for the C++11
#   standard; if necessary, add switches to CXX and CXXCPP to enable
#   support.
#
#   This macro is a convenience alias for calling the AX_CXX_COMPILE_STDCXX
#   macro with the version set to C++11.  The two optional arguments are
#   forwarded literally as the second and third argument respectively.
#   Please see the documentation for the AX_CXX_COMPILE_STDCXX macro for
#   more information.  If you want to use this macro, you also need to
#   download the ax_cxx_compile_stdcxx.m4 file.
#
# LICENSE
#
#   Copyright (c) 2008 Benjamin Kosnik <bkoz@redhat.com>
#   Copyright (c) 2012 Zack Weinberg <zackw@panix.com>
#   Copyright (c) 2013 Roy Stogner <roystgnr@ices.utexas.edu>
#   Copyright (c) 2014, 2015 Google Inc.; contributed by Alexey Sokolov <sokolov@google.com>
#   Copyright (c) 2015 Paul Norman <penorman@mac.com>
#   Copyright (c) 2015 Moritz Klammler <moritz@klammler.eu>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 18

AX_REQUIRE_DEFINED([AX_CXX_COMPILE_STDCXX])
AC_DEFUN([AX_CXX_COMPILE_STDCXX_11], [AX_CXX_COMPILE_STDCXX([11], [$1], [$2])])

# ===========================================================================
#      https://www.gnu.org/software/autoconf-archive/ax_gcc_builtin.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_GCC_BUILTIN(BUILTIN)
#
# DESCRIPTION
#
#   This macro checks if the compiler supports one of GCC's built-in
#   functions; many other compilers also provide those same built-ins.
#
#   The BUILTIN parameter is the name of the built-in function.
#
#   If BUILTIN is supported define HAVE_<BUILTIN>. Keep in mind that since
#   builtins usually start with two underscores they will be copied over
#   into the HAVE_<BUILTIN> definition (e.g. HAVE___BUILTIN_EXPECT for
#   __builtin_expect()).
#
#   The macro caches its result in the ax_cv_have_<BUILTIN> variable (e.g.
#   ax_cv_have___builtin_expect).
#
#   The macro currently supports the following built-in functions:
#
#    __builtin_assume_aligned
#    __builtin_bswap16
#    __builtin_bswap32
#    __builtin_bswap64
#    __builtin_choose_expr
#    __builtin___clear_cache
#    __builtin_clrsb
#    __builtin_clrsbl
#    __builtin_clrsbll
#    __builtin_clz
#    __builtin_clzl
#    __builtin_clzll
#    __builtin_complex
#    __builtin_constant_p
#    __builtin_cpu_init
#    __builtin_cpu_is
#    __builtin_cpu_supports
#    __builtin_ctz
#    __builtin_ctzl
#    __builtin_ctzll
#    __builtin_expect
#    __builtin_ffs
#    __builtin_ffsl
#    __builtin_ffsll
#    __builtin_fpclassify
#    __builtin_huge_val
#    __builtin_huge_valf
#    __builtin_huge_vall
#    __builtin_inf
#    __builtin_infd128
#    __builtin_infd32
#    __builtin_infd64
#    __builtin_inff
#    __builtin_infl
#    __builtin_isinf_sign
#    __builtin_nan
#    __builtin_nand128
#    __builtin_nand32
#    __builtin_nand64
#    __builtin_nanf
#    __builtin_nanl
#    __builtin_nans
#    __builtin_nansf
#    __builtin_nansl
#    __builtin_object_size
#    __builtin_parity
#    __builtin_parityl
#    __builtin_parityll
#    __builtin_popcount
#    __builtin_popcountl
#    __builtin_popcountll
#    __builtin_powi
#    __builtin_powif
#    __builtin_powil
#    __builtin_prefetch
#    __builtin_trap
#    __builtin_types_compatible_p
#    __builtin_unreachable
#
#   Unsupported built-ins will be tested with an empty parameter set and the
#   result of the check might be wrong or meaningless so use with care.
#
# LICENSE
#
#   Copyright (c) 2013 Gabriele Svelto <gabriele.svelto@gmail.com>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved.  This file is offered as-is, without any
#   warranty.

#serial 7

AC_DEFUN([AX_GCC_BUILTIN], [
    AS_VAR_PUSHDEF([ac_var], [ax_cv_have_$1])

    AC_CACHE_CHECK([for $1], [ac_var], [
        AC_LINK_IFELSE([AC_LANG_PROGRAM([], [
            m4_case([$1],
                [__builtin_assume_aligned], [$1("", 0)],
                [__builtin_bswap16], [$1(0)],
                [__builtin_bswap32], [$1(0)],
                [__builtin_bswap64], [$1(0)],
                [__builtin_choose_expr], [$1(0, 0, 0)],
                [__builtin___clear_cache], [$1("", "")],
                [__builtin_clrsb], [$1(0)],
                [__builtin_clrsbl], [$1(0)],
                [__builtin_clrsbll], [$1(0)],
                [__builtin_clz], [$1(0)],
                [__builtin_clzl], [$1(0)],
                [__builtin_clzll], [$1(0)],
                [__builtin_complex], [$1(0.0, 0.0)],
                [__builtin_constant_p], [$1(0)],
                [__builtin_cpu_init], [$1()],
                [__builtin_cpu_is], [$1("intel")],
                [__builtin_cpu_supports], [$1("sse")],
                [__builtin_ctz], [$1(0)],
                [__builtin_ctzl], [$1(0)],
                [__builtin_ctzll], [$1(0)],
                [__builtin_expect], [$1(0, 0)],
                [__builtin_ffs], [$1(0)],
                [__builtin_ffsl], [$1(0)],
                [__builtin_ffsll], [$1(0)],
                [__builtin_fpclassify], [$1(0, 1, 2, 3, 4, 0.0)],
                [__builtin_huge_val], [$1()],
                [__builtin_huge_valf], [$1()],
                [__builtin_huge_vall], [$1()],
                [__builtin_inf], [$1()],
                [__builtin_infd128], [$1()],
                [__builtin_infd32], [$1()],
                [__builtin_infd64], [$1()],
                [__builtin_inff], [$1()],
                [__builtin_infl], [$1()],
                [__builtin_isinf_sign], [$1(0.0)],
                [__builtin_nan], [$1("")],
                [__builtin_nand128], [$1("")],
                [__builtin_nand32], [$1("")],
                [__builtin_nand64], [$1("")],
                [__builtin_nanf], [$1("")],
                [__builtin_nanl], [$1("")],
                [__builtin_nans], [$1("")],
                [__builtin_nansf], [$1("")],
                [__builtin_nansl], [$1("")],
                [__builtin_object_size], [$1("", 0)],
                [__builtin_parity], [$1(0)],
                [__builtin_parityl], [$1(0)],
                [__builtin_parityll], [$1(0)],
                [__builtin_popcount], [$1(0)],
                [__builtin_popcountl], [$1(0)],
                [__builtin_popcountll], [$1(0)],
                [__builtin_powi], [$1(0, 0)],
                [__builtin_powif], [$1(0, 0)],
                [__builtin_powil], [$1(0, 0)],
                [__builtin_prefetch], [$1("")],
                [__builtin_trap], [$1()],
                [__builtin_types_compatible_p], [$1(int, int)],
                [__builtin_unreachable], [$1()],
                [m4_warn([syntax], [Unsupported built-in $1, the test may fail])
                 $1()]
            )
            ])],
            [AS_VAR_SET([ac_var], [yes])],
            [AS_VAR_SET([ac_var], [no])])
    ])

    AS_IF([test yes = AS_VAR_GET([ac_var])],
        [AC_DEFINE_UNQUOTED(AS_TR_CPP(HAVE_$1), 1,
            [Define to 1 if the system has the `$1' built-in function])], [])

    AS_VAR_POPDEF([ac_var])
])

# ===========================================================================
#    https://www.gnu.org/software/autoconf-archive/ax_jni_include_dir.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_JNI_INCLUDE_DIR
#
# DESCRIPTION
#
#   AX_JNI_INCLUDE_DIR finds include directories needed for compiling
#   programs using the JNI interface.
#
#   JNI include directories are usually in the Java distribution. This is
#   deduced from the value of $JAVA_HOME, $JAVAC, or the path to "javac", in
#   that order. When this macro completes, a list of directories is left in
#   the variable JNI_INCLUDE_DIRS.
#
#   Example usage follows:
#
#     AX_JNI_INCLUDE_DIR
#
#     for JNI_INCLUDE_DIR in $JNI_INCLUDE_DIRS
#     do
#             CPPFLAGS="$CPPFLAGS -I$JNI_INCLUDE_DIR"
#     done
#
#   If you want to force a specific compiler:
#
#   - at the configure.in level, set JAVAC=yourcompiler before calling
#   AX_JNI_INCLUDE_DIR
#
#   - at the configure level, setenv JAVAC
#
#   This macro depends on AC_CANONICAL_HOST which requires that config.guess
#   and config.sub be distributed along with the source code.  See autoconf
#   manual for details.
#
#   Note: This macro can work with the autoconf M4 macros for Java programs.
#   This particular macro is not part of the original set of macros.
#
# LICENSE
#
#   Copyright (c) 2008 Don Anderson <dda@sleepycat.com>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 15

AU_ALIAS([AC_JNI_INCLUDE_DIR], [AX_JNI_INCLUDE_DIR])
AC_DEFUN([AX_JNI_INCLUDE_DIR],[

AC_REQUIRE([AC_CANONICAL_HOST])

JNI_INCLUDE_DIRS=""

if test "x$JAVA_HOME" != x; then
	_JTOPDIR="$JAVA_HOME"
else
	if test "x$JAVAC" = x; then
		JAVAC=javac
	fi
	AC_PATH_PROG([_ACJNI_JAVAC], [$JAVAC], [no])
	if test "x$_ACJNI_JAVAC" = xno; then
		AC_MSG_ERROR([cannot find JDK; try setting \$JAVAC or \$JAVA_HOME])
	fi
	_ACJNI_FOLLOW_SYMLINKS("$_ACJNI_JAVAC")
	_JTOPDIR=`echo "$_ACJNI_FOLLOWED" | sed -e 's://*:/:g' -e 's:/[[^/]]*$::'`
fi

case "$host_os" in
        darwin*)        # Apple Java headers are inside the Xcode bundle.
            macos_version=$(sw_vers -productVersion | sed -n -e 's/^@<:@0-9@:>@*.\(@<:@0-9@:>@*\).@<:@0-9@:>@*/\1/p')
            if @<:@ "$macos_version" -gt "7" @:>@; then
                _JTOPDIR="$(xcrun --show-sdk-path)/System/Library/Frameworks/JavaVM.framework"
                _JINC="$_JTOPDIR/Headers"
            else
                _JTOPDIR="/System/Library/Frameworks/JavaVM.framework"
                _JINC="$_JTOPDIR/Headers"
            fi
            ;;
        *) _JINC="$_JTOPDIR/include";;
esac
_AS_ECHO_LOG([_JTOPDIR=$_JTOPDIR])
_AS_ECHO_LOG([_JINC=$_JINC])

# On Mac OS X 10.6.4, jni.h is a symlink:
# /System/Library/Frameworks/JavaVM.framework/Versions/Current/Headers/jni.h
# -> ../../CurrentJDK/Headers/jni.h.
AC_CACHE_CHECK(jni headers, ac_cv_jni_header_path,
[
  if test -f "$_JINC/jni.h"; then
    ac_cv_jni_header_path="$_JINC"
    JNI_INCLUDE_DIRS="$JNI_INCLUDE_DIRS $ac_cv_jni_header_path"
  else
    _JTOPDIR=`echo "$_JTOPDIR" | sed -e 's:/[[^/]]*$::'`
    if test -f "$_JTOPDIR/include/jni.h"; then
      ac_cv_jni_header_path="$_JTOPDIR/include"
      JNI_INCLUDE_DIRS="$JNI_INCLUDE_DIRS $ac_cv_jni_header_path"
    else
      ac_cv_jni_header_path=none
    fi
  fi
])

# get the likely subdirectories for system specific java includes
case "$host_os" in
bsdi*)          _JNI_INC_SUBDIRS="bsdos";;
freebsd*)       _JNI_INC_SUBDIRS="freebsd";;
darwin*)        _JNI_INC_SUBDIRS="darwin";;
linux*)         _JNI_INC_SUBDIRS="linux genunix";;
osf*)           _JNI_INC_SUBDIRS="alpha";;
solaris*)       _JNI_INC_SUBDIRS="solaris";;
mingw*)		_JNI_INC_SUBDIRS="win32";;
cygwin*)	_JNI_INC_SUBDIRS="win32";;
*)              _JNI_INC_SUBDIRS="genunix";;
esac

if test "x$ac_cv_jni_header_path" != "xnone"; then
  # add any subdirectories that are present
  for JINCSUBDIR in $_JNI_INC_SUBDIRS
  do
    if test -d "$_JTOPDIR/include/$JINCSUBDIR"; then
         JNI_INCLUDE_DIRS="$JNI_INCLUDE_DIRS $_JTOPDIR/include/$JINCSUBDIR"
    fi
  done
fi
])

# _ACJNI_FOLLOW_SYMLINKS <path>
# Follows symbolic links on <path>,
# finally setting variable _ACJNI_FOLLOWED
# ----------------------------------------
AC_DEFUN([_ACJNI_FOLLOW_SYMLINKS],[
# find the include directory relative to the javac executable
_cur="$1"
while ls -ld "$_cur" 2>/dev/null | grep " -> " >/dev/null; do
        AC_MSG_CHECKING([symlink for $_cur])
        _slink=`ls -ld "$_cur" | sed 's/.* -> //'`
        case "$_slink" in
        /*) _cur="$_slink";;
        # 'X' avoids triggering unwanted echo options.
        *) _cur=`echo "X$_cur" | sed -e 's/^X//' -e 's:[[^/]]*$::'`"$_slink";;
        esac
        AC_MSG_RESULT([$_cur])
done
_ACJNI_FOLLOWED="$_cur"
])# _ACJNI

# ===========================================================================
#       https://www.gnu.org/software/autoconf-archive/ax_pkg_swig.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_PKG_SWIG([major.minor.micro], [action-if-found], [action-if-not-found])
#
# DESCRIPTION
#
#   This macro searches for a SWIG installation on your system. If found,
#   then SWIG is AC_SUBST'd; if not found, then $SWIG is empty.  If SWIG is
#   found, then SWIG_LIB is set to the SWIG library path, and AC_SUBST'd.
#
#   You can use the optional first argument to check if the version of the
#   available SWIG is greater than or equal to the value of the argument. It
#   should have the format: N[.N[.N]] (N is a number between 0 and 999. Only
#   the first N is mandatory.) If the version argument is given (e.g.
#   1.3.17), AX_PKG_SWIG checks that the swig package is this version number
#   or higher.
#
#   As usual, action-if-found is executed if SWIG is found, otherwise
#   action-if-not-found is executed.
#
#   In configure.in, use as:
#
#     AX_PKG_SWIG(1.3.17, [], [ AC_MSG_ERROR([SWIG is required to build..]) ])
#     AX_SWIG_ENABLE_CXX
#     AX_SWIG_MULTI_MODULE_SUPPORT
#     AX_SWIG_PYTHON
#
# LICENSE
#
#   Copyright (c) 2008 Sebastian Huber <sebastian-huber@web.de>
#   Copyright (c) 2008 Alan W. Irwin
#   Copyright (c) 2008 Rafael Laboissiere <rafael@laboissiere.net>
#   Copyright (c) 2008 Andrew Collier
#   Copyright (c) 2011 Murray Cumming <murrayc@openismus.com>
#   Copyright (c) 2021 Vincent Danjean <Vincent.Danjean@ens-lyon.org>
#
#   This program is free software; you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by the
#   Free Software Foundation; either version 2 of the License, or (at your
#   option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
#   Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program. If not, see <https://www.gnu.org/licenses/>.
#
#   As a special exception, the respective Autoconf Macro's copyright owner
#   gives unlimited permission to copy, distribute and modify the configure
#   scripts that are the output of Autoconf when processing the Macro. You
#   need not follow the terms of the GNU General Public License when using
#   or distributing such scripts, even though portions of the text of the
#   Macro appear in them. The GNU General Public License (GPL) does govern
#   all other use of the material that constitutes the Autoconf Macro.
#
#   This special exception to the GPL applies to versions of the Autoconf
#   Macro released by the Autoconf Archive. When you make and distribute a
#   modified version of the Autoconf Macro, you may extend this special
#   exception to the GPL to apply to your modified version as well.

#serial 14

AC_DEFUN([AX_PKG_SWIG],[
        # Find path to the "swig" executable.
        AC_PATH_PROGS([SWIG],[swig swig3.0 swig2.0])
        if test -z "$SWIG" ; then
                m4_ifval([$3],[$3],[:])
        elif test -z "$1" ; then
                m4_ifval([$2],[$2],[:])
	else
                AC_MSG_CHECKING([SWIG version])
                [swig_version=`$SWIG -version 2>&1 | grep 'SWIG Version' | sed 's/.*\([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\).*/\1/g'`]
                AC_MSG_RESULT([$swig_version])
                if test -n "$swig_version" ; then
                        # Calculate the required version number components
                        [required=$1]
                        [required_major=`echo $required | sed 's/[^0-9].*//'`]
                        if test -z "$required_major" ; then
                                [required_major=0]
                        fi
                        [required=`echo $required. | sed 's/[0-9]*[^0-9]//'`]
                        [required_minor=`echo $required | sed 's/[^0-9].*//'`]
                        if test -z "$required_minor" ; then
                                [required_minor=0]
                        fi
                        [required=`echo $required. | sed 's/[0-9]*[^0-9]//'`]
                        [required_patch=`echo $required | sed 's/[^0-9].*//'`]
                        if test -z "$required_patch" ; then
                                [required_patch=0]
                        fi
                        # Calculate the available version number components
                        [available=$swig_version]
                        [available_major=`echo $available | sed 's/[^0-9].*//'`]
                        if test -z "$available_major" ; then
                                [available_major=0]
                        fi
                        [available=`echo $available | sed 's/[0-9]*[^0-9]//'`]
                        [available_minor=`echo $available | sed 's/[^0-9].*//'`]
                        if test -z "$available_minor" ; then
                                [available_minor=0]
                        fi
                        [available=`echo $available | sed 's/[0-9]*[^0-9]//'`]
                        [available_patch=`echo $available | sed 's/[^0-9].*//'`]
                        if test -z "$available_patch" ; then
                                [available_patch=0]
                        fi
                        # Convert the version tuple into a single number for easier comparison.
                        # Using base 100 should be safe since SWIG internally uses BCD values
                        # to encode its version number.
                        required_swig_vernum=`expr $required_major \* 10000 \
                            \+ $required_minor \* 100 \+ $required_patch`
                        available_swig_vernum=`expr $available_major \* 10000 \
                            \+ $available_minor \* 100 \+ $available_patch`

                        if test $available_swig_vernum -lt $required_swig_vernum; then
                                AC_MSG_WARN([SWIG version >= $1 is required.  You have $swig_version.])
                                SWIG=''
                                m4_ifval([$3],[$3],[])
                        else
                                AC_MSG_CHECKING([for SWIG library])
                                SWIG_LIB=`$SWIG -swiglib`
                                AC_MSG_RESULT([$SWIG_LIB])
                                m4_ifval([$2],[$2],[])
                        fi
                else
                        AC_MSG_WARN([cannot determine SWIG version])
                        SWIG=''
                        m4_ifval([$3],[$3],[])
                fi
        fi
        AC_SUBST([SWIG_LIB])
])

# ===========================================================================
#      https://www.gnu.org/software/autoconf-archive/ax_prog_bison.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_PROG_BISON(ACTION-IF-TRUE,ACTION-IF-FALSE)
#
# DESCRIPTION
#
#   Check whether bison is the parser generator. Run ACTION-IF-TRUE if
#   successful, ACTION-IF-FALSE otherwise
#
# LICENSE
#
#   Copyright (c) 2009 Francesco Salvestrini <salvestrini@users.sourceforge.net>
#   Copyright (c) 2010 Diego Elio Petteno` <flameeyes@gmail.com>
#
#   This program is free software; you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by the
#   Free Software Foundation; either version 2 of the License, or (at your
#   option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
#   Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program. If not, see <https://www.gnu.org/licenses/>.
#
#   As a special exception, the respective Autoconf Macro's copyright owner
#   gives unlimited permission to copy, distribute and modify the configure
#   scripts that are the output of Autoconf when processing the Macro. You
#   need not follow the terms of the GNU General Public License when using
#   or distributing such scripts, even though portions of the text of the
#   Macro appear in them. The GNU General Public License (GPL) does govern
#   all other use of the material that constitutes the Autoconf Macro.
#
#   This special exception to the GPL applies to versions of the Autoconf
#   Macro released by the Autoconf Archive. When you make and distribute a
#   modified version of the Autoconf Macro, you may extend this special
#   exception to the GPL to apply to your modified version as well.

#serial 10

AC_DEFUN([AX_PROG_BISON], [
  AC_REQUIRE([AC_PROG_YACC])
  AC_REQUIRE([AC_PROG_EGREP])

  AC_CACHE_CHECK([if bison is the parser generator],[ax_cv_prog_bison],[
    AS_IF([$YACC --version 2>/dev/null | $EGREP -q '^bison '],
      [ax_cv_prog_bison=yes], [ax_cv_prog_bison=no])
  ])
  AS_IF([test "$ax_cv_prog_bison" = "yes"], [
      dnl replace the yacc-compatible compiler with the real bison, as
      dnl otherwise autoconf limits us to the POSIX yacc.
      dnl We also change the generated filename to the old one, so that
      dnl automake's ylwrap can deal with it.
      YACC="${YACC% -y} -o y.tab.c"
    ] m4_ifnblank([$1], [[$1]]),
    m4_ifnblank([$2], [[$2]])
  )
])

# ===========================================================================
#  https://www.gnu.org/software/autoconf-archive/ax_prog_bison_version.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_PROG_BISON_VERSION([VERSION],[ACTION-IF-TRUE],[ACTION-IF-FALSE])
#
# DESCRIPTION
#
#   Makes sure that bison version is greater or equal to the version
#   indicated. If true the shell commands in ACTION-IF-TRUE are executed. If
#   not the shell commands in commands in ACTION-IF-TRUE are executed. If
#   not the shell commands in ACTION-IF-FALSE are run. Note if $BISON is not
#   set (for example by running AC_CHECK_PROG or AC_PATH_PROG) the macro
#   will fail.
#
#   Example:
#
#     AC_PATH_PROG([BISON],[bison])
#     AX_PROG_BISON_VERSION([3.0.2],[ ... ],[ ... ])
#
#   This will check to make sure that the bison you have is at least version
#   3.0.2 or greater.
#
#   NOTE: This macro uses the $BISON variable to perform the check.
#
# LICENSE
#
#   Copyright (c) 2015 Jonathan Rajotte-Julien <jonathan.rajotte-julien@efficios.com>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 3

AC_DEFUN([AX_PROG_BISON_VERSION],[
    AC_REQUIRE([AC_PROG_SED])
    AC_REQUIRE([AC_PROG_GREP])

    AS_IF([test -n "$BISON"],[
        ax_bison_version="$1"

        AC_MSG_CHECKING([for bison version])
        changequote(<<,>>)
        bison_version=`$BISON --version 2>&1 \
          | $SED -n -e '/bison (GNU Bison)/b inspect
b
: inspect
s/.* (\{0,1\}\([0-9]*\.[0-9]*\.[0-9]*\))\{0,1\}.*/\1/;p'`
        changequote([,])
        AC_MSG_RESULT($bison_version)

	AC_SUBST([BISON_VERSION],[$bison_version])

        AX_COMPARE_VERSION([$bison_version],[ge],[$ax_bison_version],[
	    :
            $2
        ],[
	    :
            $3
        ])
    ],[
        AC_MSG_WARN([could not find bison])
        $3
    ])
])

# ===========================================================================
#   https://www.gnu.org/software/autoconf-archive/ax_prog_ruby_version.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_PROG_RUBY_VERSION([VERSION],[ACTION-IF-TRUE],[ACTION-IF-FALSE])
#
# DESCRIPTION
#
#   Makes sure that ruby supports the version indicated. If true the shell
#   commands in ACTION-IF-TRUE are executed. If not the shell commands in
#   ACTION-IF-FALSE are run. Note if $RUBY is not set (for example by
#   running AC_CHECK_PROG or AC_PATH_PROG) the macro will fail.
#
#   Example:
#
#     AC_PATH_PROG([RUBY],[ruby])
#     AC_PROG_RUBY_VERSION([1.8.0],[ ... ],[ ... ])
#
#   This will check to make sure that the ruby you have supports at least
#   version 1.6.0.
#
#   NOTE: This macro uses the $RUBY variable to perform the check.
#   AX_WITH_PROG([RUBY],[ruby],[VALUE-IF-NOT-FOUND],[PATH]) can be used to
#   set that variable prior to running this macro. The $RUBY_VERSION
#   variable will be valorized with the detected version.
#
# LICENSE
#
#   Copyright (c) 2009 Francesco Salvestrini <salvestrini@users.sourceforge.net>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 12

AC_DEFUN([AX_PROG_RUBY_VERSION],[
    AC_REQUIRE([AC_PROG_SED])
    AC_REQUIRE([AC_PROG_GREP])

    AS_IF([test -n "$RUBY"],[
        ax_ruby_version="$1"

        AC_MSG_CHECKING([for ruby version])
        changequote(<<,>>)
        ruby_version=`$RUBY --version 2>&1 | $GREP "^ruby " | $SED -e 's/^.* \([0-9]*\.[0-9]*\.[0-9]*\) .*/\1/'`
        changequote([,])
        AC_MSG_RESULT($ruby_version)

	AC_SUBST([RUBY_VERSION],[$ruby_version])

        AX_COMPARE_VERSION([$ax_ruby_version],[le],[$ruby_version],[
	    :
            $2
        ],[
	    :
            $3
        ])
    ],[
        AC_MSG_WARN([could not find the ruby interpreter])
        $3
    ])
])

# ===========================================================================
#     https://www.gnu.org/software/autoconf-archive/ax_python_devel.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_PYTHON_DEVEL([version])
#
# DESCRIPTION
#
#   Note: Defines as a precious variable "PYTHON_VERSION". Don't override it
#   in your configure.ac.
#
#   This macro checks for Python and tries to get the include path to
#   'Python.h'. It provides the $(PYTHON_CPPFLAGS) and $(PYTHON_LIBS) output
#   variables. It also exports $(PYTHON_EXTRA_LIBS) and
#   $(PYTHON_EXTRA_LDFLAGS) for embedding Python in your code.
#
#   You can search for some particular version of Python by passing a
#   parameter to this macro, for example ">= '2.3.1'", or "== '2.4'". Please
#   note that you *have* to pass also an operator along with the version to
#   match, and pay special attention to the single quotes surrounding the
#   version number. Don't use "PYTHON_VERSION" for this: that environment
#   variable is declared as precious and thus reserved for the end-user.
#
#   This macro should work for all versions of Python >= 2.1.0. As an end
#   user, you can disable the check for the python version by setting the
#   PYTHON_NOVERSIONCHECK environment variable to something else than the
#   empty string.
#
#   If you need to use this macro for an older Python version, please
#   contact the authors. We're always open for feedback.
#
# LICENSE
#
#   Copyright (c) 2009 Sebastian Huber <sebastian-huber@web.de>
#   Copyright (c) 2009 Alan W. Irwin
#   Copyright (c) 2009 Rafael Laboissiere <rafael@laboissiere.net>
#   Copyright (c) 2009 Andrew Collier
#   Copyright (c) 2009 Matteo Settenvini <matteo@member.fsf.org>
#   Copyright (c) 2009 Horst Knorr <hk_classes@knoda.org>
#   Copyright (c) 2013 Daniel Mullner <muellner@math.stanford.edu>
#
#   This program is free software: you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by the
#   Free Software Foundation, either version 3 of the License, or (at your
#   option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
#   Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program. If not, see <https://www.gnu.org/licenses/>.
#
#   As a special exception, the respective Autoconf Macro's copyright owner
#   gives unlimited permission to copy, distribute and modify the configure
#   scripts that are the output of Autoconf when processing the Macro. You
#   need not follow the terms of the GNU General Public License when using
#   or distributing such scripts, even though portions of the text of the
#   Macro appear in them. The GNU General Public License (GPL) does govern
#   all other use of the material that constitutes the Autoconf Macro.
#
#   This special exception to the GPL applies to versions of the Autoconf
#   Macro released by the Autoconf Archive. When you make and distribute a
#   modified version of the Autoconf Macro, you may extend this special
#   exception to the GPL to apply to your modified version as well.

#serial 23

AU_ALIAS([AC_PYTHON_DEVEL], [AX_PYTHON_DEVEL])
AC_DEFUN([AX_PYTHON_DEVEL],[
	#
	# Allow the use of a (user set) custom python version
	#
	AC_ARG_VAR([PYTHON_VERSION],[The installed Python
		version to use, for example '2.3'. This string
		will be appended to the Python interpreter
		canonical name.])

	AC_PATH_PROG([PYTHON],[python[$PYTHON_VERSION]])
	if test -z "$PYTHON"; then
	   AC_MSG_ERROR([Cannot find python$PYTHON_VERSION in your system path])
	   PYTHON_VERSION=""
	fi

	#
	# Check for a version of Python >= 2.1.0
	#
	AC_MSG_CHECKING([for a version of Python >= '2.1.0'])
	ac_supports_python_ver=`$PYTHON -c "import sys; \
		ver = sys.version.split ()[[0]]; \
		print (ver >= '2.1.0')"`
	if test "$ac_supports_python_ver" != "True"; then
		if test -z "$PYTHON_NOVERSIONCHECK"; then
			AC_MSG_RESULT([no])
			AC_MSG_FAILURE([
This version of the AC@&t@_PYTHON_DEVEL macro
doesn't work properly with versions of Python before
2.1.0. You may need to re-run configure, setting the
variables PYTHON_CPPFLAGS, PYTHON_LIBS, PYTHON_SITE_PKG,
PYTHON_EXTRA_LIBS and PYTHON_EXTRA_LDFLAGS by hand.
Moreover, to disable this check, set PYTHON_NOVERSIONCHECK
to something else than an empty string.
])
		else
			AC_MSG_RESULT([skip at user request])
		fi
	else
		AC_MSG_RESULT([yes])
	fi

	#
	# if the macro parameter ``version'' is set, honour it
	#
	if test -n "$1"; then
		AC_MSG_CHECKING([for a version of Python $1])
		ac_supports_python_ver=`$PYTHON -c "import sys; \
			ver = sys.version.split ()[[0]]; \
			print (ver $1)"`
		if test "$ac_supports_python_ver" = "True"; then
		   AC_MSG_RESULT([yes])
		else
			AC_MSG_RESULT([no])
			AC_MSG_ERROR([this package requires Python $1.
If you have it installed, but it isn't the default Python
interpreter in your system path, please pass the PYTHON_VERSION
variable to configure. See ``configure --help'' for reference.
])
			PYTHON_VERSION=""
		fi
	fi

	#
	# Check if you have distutils, else fail
	#
	AC_MSG_CHECKING([for the sysconfig Python package])
	ac_sysconfig_result=`$PYTHON -c "import sysconfig" 2>&1`
	if test $? -eq 0; then
		AC_MSG_RESULT([yes])
		IMPORT_SYSCONFIG="import sysconfig"
	else
		AC_MSG_RESULT([no])

		AC_MSG_CHECKING([for the distutils Python package])
		ac_sysconfig_result=`$PYTHON -c "from distutils import sysconfig" 2>&1`
		if test $? -eq 0; then
			AC_MSG_RESULT([yes])
			IMPORT_SYSCONFIG="from distutils import sysconfig"
		else
			AC_MSG_ERROR([cannot import Python module "distutils".
Please check your Python installation. The error was:
$ac_sysconfig_result])
			PYTHON_VERSION=""
		fi
	fi

	#
	# Check for Python include path
	#
	AC_MSG_CHECKING([for Python include path])
	if test -z "$PYTHON_CPPFLAGS"; then
		if test "$IMPORT_SYSCONFIG" = "import sysconfig"; then
			# sysconfig module has different functions
			python_path=`$PYTHON -c "$IMPORT_SYSCONFIG; \
				print (sysconfig.get_path ('include'));"`
			plat_python_path=`$PYTHON -c "$IMPORT_SYSCONFIG; \
				print (sysconfig.get_path ('platinclude'));"`
		else
			# old distutils way
			python_path=`$PYTHON -c "$IMPORT_SYSCONFIG; \
				print (sysconfig.get_python_inc ());"`
			plat_python_path=`$PYTHON -c "$IMPORT_SYSCONFIG; \
				print (sysconfig.get_python_inc (plat_specific=1));"`
		fi
		if test -n "${python_path}"; then
			if test "${plat_python_path}" != "${python_path}"; then
				python_path="-I$python_path -I$plat_python_path"
			else
				python_path="-I$python_path"
			fi
		fi
		PYTHON_CPPFLAGS=$python_path
	fi
	AC_MSG_RESULT([$PYTHON_CPPFLAGS])
	AC_SUBST([PYTHON_CPPFLAGS])

	#
	# Check for Python library path
	#
	AC_MSG_CHECKING([for Python library path])
	if test -z "$PYTHON_LIBS"; then
		# (makes two attempts to ensure we've got a version number
		# from the interpreter)
		ac_python_version=`cat<<EOD | $PYTHON -

# join all versioning strings, on some systems
# major/minor numbers could be in different list elements
from sysconfig import *
e = get_config_var('VERSION')
if e is not None:
	print(e)
EOD`

		if test -z "$ac_python_version"; then
			if test -n "$PYTHON_VERSION"; then
				ac_python_version=$PYTHON_VERSION
			else
				ac_python_version=`$PYTHON -c "import sys; \
					print ("%d.%d" % sys.version_info[[:2]])"`
			fi
		fi

		# Make the versioning information available to the compiler
		AC_DEFINE_UNQUOTED([HAVE_PYTHON], ["$ac_python_version"],
                                   [If available, contains the Python version number currently in use.])

		# First, the library directory:
		ac_python_libdir=`cat<<EOD | $PYTHON -

# There should be only one
$IMPORT_SYSCONFIG
e = sysconfig.get_config_var('LIBDIR')
if e is not None:
	print (e)
EOD`

		# Now, for the library:
		ac_python_library=`cat<<EOD | $PYTHON -

$IMPORT_SYSCONFIG
c = sysconfig.get_config_vars()
if 'LDVERSION' in c:
	print ('python'+c[['LDVERSION']])
else:
	print ('python'+c[['VERSION']])
EOD`

		# This small piece shamelessly adapted from PostgreSQL python macro;
		# credits goes to momjian, I think. I'd like to put the right name
		# in the credits, if someone can point me in the right direction... ?
		#
		if test -n "$ac_python_libdir" -a -n "$ac_python_library"
		then
			# use the official shared library
			ac_python_library=`echo "$ac_python_library" | sed "s/^lib//"`
			PYTHON_LIBS="-L$ac_python_libdir -l$ac_python_library"
		else
			# old way: use libpython from python_configdir
			ac_python_libdir=`$PYTHON -c \
			  "from sysconfig import get_python_lib as f; \
			  import os; \
			  print (os.path.join(f(plat_specific=1, standard_lib=1), 'config'));"`
			PYTHON_LIBS="-L$ac_python_libdir -lpython$ac_python_version"
		fi

		if test -z "PYTHON_LIBS"; then
			AC_MSG_ERROR([
  Cannot determine location of your Python DSO. Please check it was installed with
  dynamic libraries enabled, or try setting PYTHON_LIBS by hand.
			])
		fi
	fi
	AC_MSG_RESULT([$PYTHON_LIBS])
	AC_SUBST([PYTHON_LIBS])

	#
	# Check for site packages
	#
	AC_MSG_CHECKING([for Python site-packages path])
	if test -z "$PYTHON_SITE_PKG"; then
		if test "$IMPORT_SYSCONFIG" = "import sysconfig"; then
			PYTHON_SITE_PKG=`$PYTHON -c "$IMPORT_SYSCONFIG; \
				print (sysconfig.get_path('purelib'));"`
		else
			# distutils.sysconfig way
			PYTHON_SITE_PKG=`$PYTHON -c "$IMPORT_SYSCONFIG; \
				print (sysconfig.get_python_lib(0,0));"`
		fi
	fi
	AC_MSG_RESULT([$PYTHON_SITE_PKG])
	AC_SUBST([PYTHON_SITE_PKG])

	#
	# Check for platform-specific site packages
	#
	AC_MSG_CHECKING([for Python platform specific site-packages path])
	if test -z "$PYTHON_SITE_PKG"; then
		if test "$IMPORT_SYSCONFIG" = "import sysconfig"; then
			PYTHON_PLATFORM_SITE_PKG=`$PYTHON -c "$IMPORT_SYSCONFIG; \
				print (sysconfig.get_path('platlib'));"`
		else
			# distutils.sysconfig way
			PYTHON_PLATFORM_SITE_PKG=`$PYTHON -c "$IMPORT_SYSCONFIG; \
				print (sysconfig.get_python_lib(1,0));"`
		fi
	fi
	AC_MSG_RESULT([$PYTHON_PLATFORM_SITE_PKG])
	AC_SUBST([PYTHON_PLATFORM_SITE_PKG])

	#
	# libraries which must be linked in when embedding
	#
	AC_MSG_CHECKING(python extra libraries)
	if test -z "$PYTHON_EXTRA_LIBS"; then
	   PYTHON_EXTRA_LIBS=`$PYTHON -c "$IMPORT_SYSCONFIG; \
                conf = sysconfig.get_config_var; \
                print (conf('LIBS') + ' ' + conf('SYSLIBS'))"`
	fi
	AC_MSG_RESULT([$PYTHON_EXTRA_LIBS])
	AC_SUBST(PYTHON_EXTRA_LIBS)

	#
	# linking flags needed when embedding
	#
	AC_MSG_CHECKING(python extra linking flags)
	if test -z "$PYTHON_EXTRA_LDFLAGS"; then
		PYTHON_EXTRA_LDFLAGS=`$PYTHON -c "$IMPORT_SYSCONFIG; \
			conf = sysconfig.get_config_var; \
			print (conf('LINKFORSHARED'))"`
	fi
	AC_MSG_RESULT([$PYTHON_EXTRA_LDFLAGS])
	AC_SUBST(PYTHON_EXTRA_LDFLAGS)

	#
	# final check to see if everything compiles alright
	#
	AC_MSG_CHECKING([consistency of all components of python development environment])
	# save current global flags
	ac_save_LIBS="$LIBS"
	ac_save_LDFLAGS="$LDFLAGS"
	ac_save_CPPFLAGS="$CPPFLAGS"
	LIBS="$ac_save_LIBS $PYTHON_LIBS $PYTHON_EXTRA_LIBS $PYTHON_EXTRA_LIBS"
	LDFLAGS="$ac_save_LDFLAGS $PYTHON_EXTRA_LDFLAGS"
	CPPFLAGS="$ac_save_CPPFLAGS $PYTHON_CPPFLAGS"
	AC_LANG_PUSH([C])
	AC_LINK_IFELSE([
		AC_LANG_PROGRAM([[#include <Python.h>]],
				[[Py_Initialize();]])
		],[pythonexists=yes],[pythonexists=no])
	AC_LANG_POP([C])
	# turn back to default flags
	CPPFLAGS="$ac_save_CPPFLAGS"
	LIBS="$ac_save_LIBS"
	LDFLAGS="$ac_save_LDFLAGS"

	AC_MSG_RESULT([$pythonexists])

        if test ! "x$pythonexists" = "xyes"; then
	   AC_MSG_FAILURE([
  Could not link test program to Python. Maybe the main Python library has been
  installed in some non-standard library path. If so, pass it to configure,
  via the LIBS environment variable.
  Example: ./configure LIBS="-L/usr/non-standard-path/python/lib"
  ============================================================================
   ERROR!
   You probably have to install the development version of the Python package
   for your distribution.  The exact name of this package varies among them.
  ============================================================================
	   ])
	  PYTHON_VERSION=""
	fi

	#
	# all done!
	#
])

# ===========================================================================
#      https://www.gnu.org/software/autoconf-archive/ax_ruby_devel.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_RUBY_DEVEL([version])
#
# DESCRIPTION
#
#   This macro checks for Ruby and tries to get the include path to
#   'ruby.h'. It provides the $(RUBY_CPPFLAGS) and $(RUBY_LDFLAGS) output
#   variables. It also exports $(RUBY_EXTRA_LIBS) for embedding Ruby in your
#   code.
#
#   You can search for some particular version of Ruby by passing a
#   parameter to this macro, for example "2.2.1".
#
# LICENSE
#
#   Copyright (c) 2008 Rafal Rzepecki <divided.mind@gmail.com>
#   Copyright (c) 2008 Sebastian Huber <sebastian-huber@web.de>
#   Copyright (c) 2008 Alan W. Irwin
#   Copyright (c) 2008 Rafael Laboissiere <rafael@laboissiere.net>
#   Copyright (c) 2008 Andrew Collier
#   Copyright (c) 2008 Matteo Settenvini <matteo@member.fsf.org>
#   Copyright (c) 2008 Horst Knorr <hk_classes@knoda.org>
#
#   This program is free software: you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by the
#   Free Software Foundation, either version 3 of the License, or (at your
#   option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
#   Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program. If not, see <https://www.gnu.org/licenses/>.
#
#   As a special exception, the respective Autoconf Macro's copyright owner
#   gives unlimited permission to copy, distribute and modify the configure
#   scripts that are the output of Autoconf when processing the Macro. You
#   need not follow the terms of the GNU General Public License when using
#   or distributing such scripts, even though portions of the text of the
#   Macro appear in them. The GNU General Public License (GPL) does govern
#   all other use of the material that constitutes the Autoconf Macro.
#
#   This special exception to the GPL applies to versions of the Autoconf
#   Macro released by the Autoconf Archive. When you make and distribute a
#   modified version of the Autoconf Macro, you may extend this special
#   exception to the GPL to apply to your modified version as well.

#serial 15

AC_DEFUN([AX_RUBY_DEVEL],[
    AX_WITH_PROG(RUBY,ruby)
    AS_IF([test -n "$1"], [
      AX_PROG_RUBY_VERSION([$1],[],[
        AC_MSG_ERROR([this package requires Ruby $1])
      ])
    ])

    #
    # Check if you have rbconfig, else fail
    #
    AC_MSG_CHECKING([for the rbconfig Ruby package])
    ac_rbconfig_result=`$RUBY -rrbconfig -e ";" 2>&1`
    if test -z "$ac_rbconfig_result"; then
        AC_MSG_RESULT([yes])
    else
        AC_MSG_RESULT([no])
        AC_MSG_ERROR([cannot import Ruby module "rbconfig".
Please check your Ruby installation. The error was:
$ac_rbconfig_result])
    fi

    #
    # Check for Ruby include path
    #
    AC_MSG_CHECKING([for Ruby include path])
    if test -z "$RUBY_CPPFLAGS"; then
        ruby_path=`$RUBY -rrbconfig -e 'print RbConfig::CONFIG[["rubyhdrdir"]]'`
        ruby_arch_path=`$RUBY -rrbconfig -e 'print RbConfig::CONFIG[["rubyarchhdrdir"]]'`
        if test -n "${ruby_path}"; then
                ruby_path="-I$ruby_path"
        fi
        if test -n "${ruby_arch_path}"; then
                ruby_arch_path="-I$ruby_arch_path"
        fi
        RUBY_CPPFLAGS="$ruby_path $ruby_arch_path"
    fi
    AC_MSG_RESULT([$RUBY_CPPFLAGS])
    AC_SUBST([RUBY_CPPFLAGS])

    #
    # Check for Ruby library path
    #
    AC_MSG_CHECKING([for Ruby library path])
    if test -z "$RUBY_LDFLAGS"; then
        RUBY_LDFLAGS=`$RUBY -rrbconfig -e 'print RbConfig::CONFIG[["LIBRUBYARG_SHARED"]]'`
    fi
    AC_MSG_RESULT([$RUBY_LDFLAGS])
    AC_SUBST([RUBY_LDFLAGS])

    #
    # Check for site packages
    #
    AC_MSG_CHECKING([for Ruby site-packages path])
    if test -z "$RUBY_SITE_PKG"; then
        RUBY_SITE_PKG=`$RUBY -rrbconfig -e 'print RbConfig::CONFIG[["sitearchdir"]]'`
    fi
    AC_MSG_RESULT([$RUBY_SITE_PKG])
    AC_SUBST([RUBY_SITE_PKG])

    #
    # libraries which must be linked in when embedding
    #
    AC_MSG_CHECKING(ruby extra libraries)
    if test -z "$RUBY_EXTRA_LIBS"; then
       RUBY_EXTRA_LIBS=`$RUBY -rrbconfig -e 'print RbConfig::CONFIG[["SOLIBS"]]'`
    fi
    AC_MSG_RESULT([$RUBY_EXTRA_LIBS])
    AC_SUBST(RUBY_EXTRA_LIBS)

    #
    # linking flags needed when embedding
    # (is it even needed for Ruby?)
    #
    # AC_MSG_CHECKING(ruby extra linking flags)
    # if test -z "$RUBY_EXTRA_LDFLAGS"; then
    # RUBY_EXTRA_LDFLAGS=`$RUBY -rrbconfig -e 'print RbConfig::CONFIG[["LINKFORSHARED"]]'`
    # fi
    # AC_MSG_RESULT([$RUBY_EXTRA_LDFLAGS])
    # AC_SUBST(RUBY_EXTRA_LDFLAGS)

    # this flags breaks ruby.h, and is sometimes defined by KDE m4 macros
    CFLAGS="`echo "$CFLAGS" | sed -e 's/-std=iso9899:1990//g;'`"
    #
    # final check to see if everything compiles alright
    #
    AC_MSG_CHECKING([consistency of all components of ruby development environment])
    AC_LANG_PUSH([C])
    # save current global flags
    ac_save_LIBS="$LIBS"
    LIBS="$ac_save_LIBS $RUBY_LDFLAGS"
    ac_save_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$ac_save_CPPFLAGS $RUBY_CPPFLAGS"
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[
        #include <ruby.h>
    ]], [[
        ruby_init();
    ]])],[rubyexists=yes],[rubyexists=no])

    AC_MSG_RESULT([$rubyexists])

    if test ! "$rubyexists" = "yes"; then
       AC_MSG_ERROR([
  Could not link test program to Ruby. Maybe the main Ruby library has been
  installed in some non-standard library path. If so, pass it to configure,
  via the LDFLAGS environment variable.
  Example: ./configure LDFLAGS="-L/usr/non-standard-path/ruby/lib"
  ============================================================================
   ERROR!
   You probably have to install the development version of the Ruby package
   for your distribution.  The exact name of this package varies among them.
  ============================================================================
       ])
      RUBY_VERSION=""
    fi
    AC_LANG_POP
    # turn back to default flags
    CPPFLAGS="$ac_save_CPPFLAGS"
    LIBS="$ac_save_LIBS"

    #
    # all done!
    #
])

# ===========================================================================
#      https://www.gnu.org/software/autoconf-archive/ax_swig_python.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_SWIG_PYTHON([use-shadow-classes = {no, yes}])
#
# DESCRIPTION
#
#   Checks for Python and provides the $(AX_SWIG_PYTHON_CPPFLAGS), and
#   $(AX_SWIG_PYTHON_OPT) output variables.
#
#   $(AX_SWIG_PYTHON_OPT) contains all necessary SWIG options to generate
#   code for Python. Shadow classes are enabled unless the value of the
#   optional first argument is exactly 'no'. If you need multi module
#   support (provided by the AX_SWIG_MULTI_MODULE_SUPPORT macro) use
#   $(AX_SWIG_PYTHON_LIBS) to link against the appropriate library. It
#   contains the SWIG Python runtime library that is needed by the type
#   check system for example.
#
# LICENSE
#
#   Copyright (c) 2008 Sebastian Huber <sebastian-huber@web.de>
#   Copyright (c) 2008 Alan W. Irwin
#   Copyright (c) 2008 Rafael Laboissiere <rafael@laboissiere.net>
#   Copyright (c) 2008 Andrew Collier
#
#   This program is free software; you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by the
#   Free Software Foundation; either version 2 of the License, or (at your
#   option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
#   Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program. If not, see <https://www.gnu.org/licenses/>.
#
#   As a special exception, the respective Autoconf Macro's copyright owner
#   gives unlimited permission to copy, distribute and modify the configure
#   scripts that are the output of Autoconf when processing the Macro. You
#   need not follow the terms of the GNU General Public License when using
#   or distributing such scripts, even though portions of the text of the
#   Macro appear in them. The GNU General Public License (GPL) does govern
#   all other use of the material that constitutes the Autoconf Macro.
#
#   This special exception to the GPL applies to versions of the Autoconf
#   Macro released by the Autoconf Archive. When you make and distribute a
#   modified version of the Autoconf Macro, you may extend this special
#   exception to the GPL to apply to your modified version as well.

#serial 11

AU_ALIAS([SWIG_PYTHON], [AX_SWIG_PYTHON])
AC_DEFUN([AX_SWIG_PYTHON],[
        AC_REQUIRE([AX_PKG_SWIG])
        AC_REQUIRE([AX_PYTHON_DEVEL])
        test "x$1" != "xno" || swig_shadow=" -noproxy"
        AC_SUBST([AX_SWIG_PYTHON_OPT],[-python$swig_shadow])
        AC_SUBST([AX_SWIG_PYTHON_CPPFLAGS],[$PYTHON_CPPFLAGS])
])

# ===========================================================================
#       https://www.gnu.org/software/autoconf-archive/ax_with_prog.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_WITH_PROG([VARIABLE],[program],[VALUE-IF-NOT-FOUND],[PATH])
#
# DESCRIPTION
#
#   Locates an installed program binary, placing the result in the precious
#   variable VARIABLE. Accepts a present VARIABLE, then --with-program, and
#   failing that searches for program in the given path (which defaults to
#   the system path). If program is found, VARIABLE is set to the full path
#   of the binary; if it is not found VARIABLE is set to VALUE-IF-NOT-FOUND
#   if provided, unchanged otherwise.
#
#   A typical example could be the following one:
#
#     AX_WITH_PROG(PERL,perl)
#
#   NOTE: This macro is based upon the original AX_WITH_PYTHON macro from
#   Dustin J. Mitchell <dustin@cs.uchicago.edu>.
#
# LICENSE
#
#   Copyright (c) 2008 Francesco Salvestrini <salvestrini@users.sourceforge.net>
#   Copyright (c) 2008 Dustin J. Mitchell <dustin@cs.uchicago.edu>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 17

AC_DEFUN([AX_WITH_PROG],[
    AC_PREREQ([2.61])

    pushdef([VARIABLE],$1)
    pushdef([EXECUTABLE],$2)
    pushdef([VALUE_IF_NOT_FOUND],$3)
    pushdef([PATH_PROG],$4)

    AC_ARG_VAR(VARIABLE,Absolute path to EXECUTABLE executable)

    AS_IF(test -z "$VARIABLE",[
        AC_MSG_CHECKING(whether EXECUTABLE executable path has been provided)
        AC_ARG_WITH(EXECUTABLE,AS_HELP_STRING([--with-EXECUTABLE=[[[PATH]]]],absolute path to EXECUTABLE executable), [
            AS_IF([test "$withval" != yes && test "$withval" != no],[
                VARIABLE="$withval"
                AC_MSG_RESULT($VARIABLE)
            ],[
                VARIABLE=""
                AC_MSG_RESULT([no])
                AS_IF([test "$withval" != no], [
                  AC_PATH_PROG([]VARIABLE[],[]EXECUTABLE[],[]VALUE_IF_NOT_FOUND[],[]PATH_PROG[])
                ])
            ])
        ],[
            AC_MSG_RESULT([no])
            AC_PATH_PROG([]VARIABLE[],[]EXECUTABLE[],[]VALUE_IF_NOT_FOUND[],[]PATH_PROG[])
        ])
    ])

    popdef([PATH_PROG])
    popdef([VALUE_IF_NOT_FOUND])
    popdef([EXECUTABLE])
    popdef([VARIABLE])
])

# Copyright (C) 2002-2021 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# AM_AUTOMAKE_VERSION(VERSION)
# ----------------------------
# Automake X.Y traces this macro to ensure aclocal.m4 has been
# generated from the m4 files accompanying Automake X.Y.
# (This private macro should not be called outside this file.)
AC_DEFUN([AM_AUTOMAKE_VERSION],
[am__api_version='1.16'
dnl Some users find AM_AUTOMAKE_VERSION and mistake it for a way to
dnl require some minimum version.  Point them to the right macro.
m4_if([$1], [1.16.5], [],
      [AC_FATAL([Do not call $0, use AM_INIT_AUTOMAKE([$1]).])])dnl
])

# _AM_AUTOCONF_VERSION(VERSION)
# -----------------------------
# aclocal traces this macro to find the Autoconf version.
# This is a private macro too.  Using m4_define simplifies
# the logic in aclocal, which can simply ignore this definition.
m4_define([_AM_AUTOCONF_VERSION], [])

# AM_SET_CURRENT_AUTOMAKE_VERSION
# -------------------------------
# Call AM_AUTOMAKE_VERSION and AM_AUTOMAKE_VERSION so they can be traced.
# This function is AC_REQUIREd by AM_INIT_AUTOMAKE.
AC_DEFUN([AM_SET_CURRENT_AUTOMAKE_VERSION],
[AM_AUTOMAKE_VERSION([1.16.5])dnl
m4_ifndef([AC_AUTOCONF_VERSION],
  [m4_copy([m4_PACKAGE_VERSION], [AC_AUTOCONF_VERSION])])dnl
_AM_AUTOCONF_VERSION(m4_defn([AC_AUTOCONF_VERSION]))])

# AM_AUX_DIR_EXPAND                                         -*- Autoconf -*-

# Copyright (C) 2001-2021 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# For projects using AC_CONFIG_AUX_DIR([foo]), Autoconf sets
# $ac_aux_dir to '$srcdir/foo'.  In other projects, it is set to
# '$srcdir', '$srcdir/..', or '$srcdir/../..'.
#
# Of course, Automake must honor this variable whenever it calls a
# tool from the auxiliary directory.  The problem is that $srcdir (and
# therefore $ac_aux_dir as well) can be either absolute or relative,
# depending on how configure is run.  This is pretty annoying, since
# it makes $ac_aux_dir quite unusable in subdirectories: in the top
# source directory, any form will work fine, but in subdirectories a
# relative path needs to be adjusted first.
#
# $ac_aux_dir/missing
#    fails when called from a subdirectory if $ac_aux_dir is relative
# $top_srcdir/$ac_aux_dir/missing
#    fails if $ac_aux_dir is absolute,
#    fails when called from a subdirectory in a VPATH build with
#          a relative $ac_aux_dir
#
# The reason of the latter failure is that $top_srcdir and $ac_aux_dir
# are both prefixed by $srcdir.  In an in-source build this is usually
# harmless because $srcdir is '.', but things will broke when you
# start a VPATH build or use an absolute $srcdir.
#
# So we could use something similar to $top_srcdir/$ac_aux_dir/missing,
# iff we strip the leading $srcdir from $ac_aux_dir.  That would be:
#   am_aux_dir='\$(top_srcdir)/'`expr "$ac_aux_dir" : "$srcdir//*\(.*\)"`
# and then we would define $MISSING as
#   MISSING="\${SHELL} $am_aux_dir/missing"
# This will work as long as MISSING is not called from configure, because
# unfortunately $(top_srcdir) has no meaning in configure.
# However there are other variables, like CC, which are often used in
# configure, and could therefore not use this "fixed" $ac_aux_dir.
#
# Another solution, used here, is to always expand $ac_aux_dir to an
# absolute PATH.  The drawback is that using absolute paths prevent a
# configured tree to be moved without reconfiguration.

AC_DEFUN([AM_AUX_DIR_EXPAND],
[AC_REQUIRE([AC_CONFIG_AUX_DIR_DEFAULT])dnl
# Expand $ac_aux_dir to an absolute path.
am_aux_dir=`cd "$ac_aux_dir" && pwd`
])

# AM_CONDITIONAL                                            -*- Autoconf -*-

# Copyright (C) 1997-2021 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# AM_CONDITIONAL(NAME, SHELL-CONDITION)
# -------------------------------------
# Define a conditional.
AC_DEFUN([AM_CONDITIONAL],
[AC_PREREQ([2.52])dnl
 m4_if([$1], [TRUE],  [AC_FATAL([$0: invalid condition: $1])],
       [$1], [FALSE], [AC_FATAL([$0: invalid condition: $1])])dnl
AC_SUBST([$1_TRUE])dnl
AC_SUBST([$1_FALSE])dnl
_AM_SUBST_NOTMAKE([$1_TRUE])dnl
_AM_SUBST_NOTMAKE([$1_FALSE])dnl
m4_define([_AM_COND_VALUE_$1], [$2])dnl
if $2; then
  $1_TRUE=
  $1_FALSE='#'
else
  $1_TRUE='#'
  $1_FALSE=
fi
AC_CONFIG_COMMANDS_PRE(
[if test -z "${$1_TRUE}" && test -z "${$1_FALSE}"; then
  AC_MSG_ERROR([[conditional "$1" was never defined.
Usually this means the macro was only invoked conditionally.]])
fi])])

# Copyright (C) 1999-2021 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.


# There are a few dirty hacks below to avoid letting 'AC_PROG_CC' be
# written in clear, in which case automake, when reading aclocal.m4,
# will think it sees a *use*, and therefore will trigger all it's
# C support machinery.  Also note that it means that autoscan, seeing
# CC etc. in the Makefile, will ask for an AC_PROG_CC use...


# _AM_DEPENDENCIES(NAME)
# ----------------------
# See how the compiler implements dependency checking.
# NAME is "CC", "CXX", "OBJC", "OBJCXX", "UPC", or "GJC".
# We try a few techniques and use that to set a single cache variable.
#
# We don't AC_REQUIRE the corresponding AC_PROG_CC since the latter was
# modified to invoke _AM_DEPENDENCIES(CC); we would have a circular
# dependency, and given that the user is not expected to run this macro,
# just rely on AC_PROG_CC.
AC_DEFUN([_AM_DEPENDENCIES],
[AC_REQUIRE([AM_SET_DEPDIR])dnl
AC_REQUIRE([AM_OUTPUT_DEPENDENCY_COMMANDS])dnl
AC_REQUIRE([AM_MAKE_INCLUDE])dnl
AC_REQUIRE([AM_DEP_TRACK])dnl

m4_if([$1], [CC],   [depcc="$CC"   am_compiler_list=],
      [$1], [CXX],  [depcc="$CXX"  am_compiler_list=],
      [$1], [OBJC], [depcc="$OBJC" am_compiler_list='gcc3 gcc'],
      [$1], [OBJCXX], [depcc="$OBJCXX" am_compiler_list='gcc3 gcc'],
      [$1], [UPC],  [depcc="$UPC"  am_compiler_list=],
      [$1], [GCJ],  [depcc="$GCJ"  am_compiler_list='gcc3 gcc'],
                    [depcc="$$1"   am_compiler_list=])

AC_CACHE_CHECK([dependency style of $depcc],
               [am_cv_$1_dependencies_compiler_type],
[if test -z "$AMDEP_TRUE" && test -f "$am_depcomp"; then
  # We make a subdir and do the tests there.  Otherwise we can end up
  # making bogus files that we don't know about and never remove.  For
  # instance it was reported that on HP-UX the gcc test will end up
  # making a dummy file named 'D' -- because '-MD' means "put the output
  # in D".
  rm -rf conftest.dir
  mkdir conftest.dir
  # Copy depcomp to subdir because otherwise we won't find it if we're
  # using a relative directory.
  cp "$am_depcomp" conftest.dir
  cd conftest.dir
  # We will build objects and dependencies in a subdirectory because
  # it helps to detect inapplicable dependency modes.  For instance
  # both Tru64's cc and ICC support -MD to output dependencies as a
  # side effect of compilation, but ICC will put the dependencies in
  # the current directory while Tru64 will put them in the object
  # directory.
  mkdir sub

  am_cv_$1_dependencies_compiler_type=none
  if test "$am_compiler_list" = ""; then
     am_compiler_list=`sed -n ['s/^#*\([a-zA-Z0-9]*\))$/\1/p'] < ./depcomp`
  fi
  am__universal=false
  m4_case([$1], [CC],
    [case " $depcc " in #(
     *\ -arch\ *\ -arch\ *) am__universal=true ;;
     esac],
    [CXX],
    [case " $depcc " in #(
     *\ -arch\ *\ -arch\ *) am__universal=true ;;
     esac])

  for depmode in $am_compiler_list; do
    # Setup a source with many dependencies, because some compilers
    # like to wrap large dependency lists on column 80 (with \), and
    # we should not choose a depcomp mode which is confused by this.
    #
    # We need to recreate these files for each test, as the compiler may
    # overwrite some of them when testing with obscure command lines.
    # This happens at least with the AIX C compiler.
    : > sub/conftest.c
    for i in 1 2 3 4 5 6; do
      echo '#include "conftst'$i'.h"' >> sub/conftest.c
      # Using ": > sub/conftst$i.h" creates only sub/conftst1.h with
      # Solaris 10 /bin/sh.
      echo '/* dummy */' > sub/conftst$i.h
    done
    echo "${am__include} ${am__quote}sub/conftest.Po${am__quote}" > confmf

    # We check with '-c' and '-o' for the sake of the "dashmstdout"
    # mode.  It turns out that the SunPro C++ compiler does not properly
    # handle '-M -o', and we need to detect this.  Also, some Intel
    # versions had trouble with output in subdirs.
    am__obj=sub/conftest.${OBJEXT-o}
    am__minus_obj="-o $am__obj"
    case $depmode in
    gcc)
      # This depmode causes a compiler race in universal mode.
      test "$am__universal" = false || continue
      ;;
    nosideeffect)
      # After this tag, mechanisms are not by side-effect, so they'll
      # only be used when explicitly requested.
      if test "x$enable_dependency_tracking" = xyes; then
	continue
      else
	break
      fi
      ;;
    msvc7 | msvc7msys | msvisualcpp | msvcmsys)
      # This compiler won't grok '-c -o', but also, the minuso test has
      # not run yet.  These depmodes are late enough in the game, and
      # so weak that their functioning should not be impacted.
      am__obj=conftest.${OBJEXT-o}
      am__minus_obj=
      ;;
    none) break ;;
    esac
    if depmode=$depmode \
       source=sub/conftest.c object=$am__obj \
       depfile=sub/conftest.Po tmpdepfile=sub/conftest.TPo \
       $SHELL ./depcomp $depcc -c $am__minus_obj sub/conftest.c \
         >/dev/null 2>conftest.err &&
       grep sub/conftst1.h sub/conftest.Po > /dev/null 2>&1 &&
       grep sub/conftst6.h sub/conftest.Po > /dev/null 2>&1 &&
       grep $am__obj sub/conftest.Po > /dev/null 2>&1 &&
       ${MAKE-make} -s -f confmf > /dev/null 2>&1; then
      # icc doesn't choke on unknown options, it will just issue warnings
      # or remarks (even with -Werror).  So we grep stderr for any message
      # that says an option was ignored or not supported.
      # When given -MP, icc 7.0 and 7.1 complain thusly:
      #   icc: Command line warning: ignoring option '-M'; no argument required
      # The diagnosis changed in icc 8.0:
      #   icc: Command line remark: option '-MP' not supported
      if (grep 'ignoring option' conftest.err ||
          grep 'not supported' conftest.err) >/dev/null 2>&1; then :; else
        am_cv_$1_dependencies_compiler_type=$depmode
        break
      fi
    fi
  done

  cd ..
  rm -rf conftest.dir
else
  am_cv_$1_dependencies_compiler_type=none
fi
])
AC_SUBST([$1DEPMODE], [depmode=$am_cv_$1_dependencies_compiler_type])
AM_CONDITIONAL([am__fastdep$1], [
  test "x$enable_dependency_tracking" != xno \
  && test "$am_cv_$1_dependencies_compiler_type" = gcc3])
])


# AM_SET_DEPDIR
# -------------
# Choose a directory name for dependency files.
# This macro is AC_REQUIREd in _AM_DEPENDENCIES.
AC_DEFUN([AM_SET_DEPDIR],
[AC_REQUIRE([AM_SET_LEADING_DOT])dnl
AC_SUBST([DEPDIR], ["${am__leading_dot}deps"])dnl
])


# AM_DEP_TRACK
# ------------
AC_DEFUN([AM_DEP_TRACK],
[AC_ARG_ENABLE([dependency-tracking], [dnl
AS_HELP_STRING(
  [--enable-dependency-tracking],
  [do not reject slow dependency extractors])
AS_HELP_STRING(
  [--disable-dependency-tracking],
  [speeds up one-time build])])
if test "x$enable_dependency_tracking" != xno; then
  am_depcomp="$ac_aux_dir/depcomp"
  AMDEPBACKSLASH='\'
  am__nodep='_no'
fi
AM_CONDITIONAL([AMDEP], [test "x$enable_dependency_tracking" != xno])
AC_SUBST([AMDEPBACKSLASH])dnl
_AM_SUBST_NOTMAKE([AMDEPBACKSLASH])dnl
AC_SUBST([am__nodep])dnl
_AM_SUBST_NOTMAKE([am__nodep])dnl
])

# Generate code to set up dependency tracking.              -*- Autoconf -*-

# Copyright (C) 1999-2021 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# _AM_OUTPUT_DEPENDENCY_COMMANDS
# ------------------------------
AC_DEFUN([_AM_OUTPUT_DEPENDENCY_COMMANDS],
[{
  # Older Autoconf quotes --file arguments for eval, but not when files
  # are listed without --file.  Let's play safe and only enable the eval
  # if we detect the quoting.
  # TODO: see whether this extra hack can be removed once we start
  # requiring Autoconf 2.70 or later.
  AS_CASE([$CONFIG_FILES],
          [*\'*], [eval set x "$CONFIG_FILES"],
          [*], [set x $CONFIG_FILES])
  shift
  # Used to flag and report bootstrapping failures.
  am_rc=0
  for am_mf
  do
    # Strip MF so we end up with the name of the file.
    am_mf=`AS_ECHO(["$am_mf"]) | sed -e 's/:.*$//'`
    # Check whether this is an Automake generated Makefile which includes
    # dependency-tracking related rules and includes.
    # Grep'ing the whole file directly is not great: AIX grep has a line
    # limit of 2048, but all sed's we know have understand at least 4000.
    sed -n 's,^am--depfiles:.*,X,p' "$am_mf" | grep X >/dev/null 2>&1 \
      || continue
    am_dirpart=`AS_DIRNAME(["$am_mf"])`
    am_filepart=`AS_BASENAME(["$am_mf"])`
    AM_RUN_LOG([cd "$am_dirpart" \
      && sed -e '/# am--include-marker/d' "$am_filepart" \
        | $MAKE -f - am--depfiles]) || am_rc=$?
  done
  if test $am_rc -ne 0; then
    AC_MSG_FAILURE([Something went wrong bootstrapping makefile fragments
    for automatic dependency tracking.  If GNU make was not used, consider
    re-running the configure script with MAKE="gmake" (or whatever is
    necessary).  You can also try re-running configure with the
    '--disable-dependency-tracking' option to at least be able to build
    the package (albeit without support for automatic dependency tracking).])
  fi
  AS_UNSET([am_dirpart])
  AS_UNSET([am_filepart])
  AS_UNSET([am_mf])
  AS_UNSET([am_rc])
  rm -f conftest-deps.mk
}
])# _AM_OUTPUT_DEPENDENCY_COMMANDS


# AM_OUTPUT_DEPENDENCY_COMMANDS
# -----------------------------
# This macro should only be invoked once -- use via AC_REQUIRE.
#
# This code is only required when automatic dependency tracking is enabled.
# This creates each '.Po' and '.Plo' makefile fragment that we'll need in
# order to bootstrap the dependency handling code.
AC_DEFUN([AM_OUTPUT_DEPENDENCY_COMMANDS],
[AC_CONFIG_COMMANDS([depfiles],
     [test x"$AMDEP_TRUE" != x"" || _AM_OUTPUT_DEPENDENCY_COMMANDS],
     [AMDEP_TRUE="$AMDEP_TRUE" MAKE="${MAKE-make}"])])

# Do all the work for Automake.                             -*- Autoconf -*-

# Copyright (C) 1996-2021 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This macro actually does too much.  Some checks are only needed if
# your package does certain things.  But this isn't really a big deal.

dnl Redefine AC_PROG_CC to automatically invoke _AM_PROG_CC_C_O.
m4_define([AC_PROG_CC],
m4_defn([AC_PROG_CC])
[_AM_PROG_CC_C_O
])

# AM_INIT_AUTOMAKE(PACKAGE, VERSION, [NO-DEFINE])
# AM_INIT_AUTOMAKE([OPTIONS])
# -----------------------------------------------
# The call with PACKAGE and VERSION arguments is the old style
# call (pre autoconf-2.50), which is being phased out.  PACKAGE
# and VERSION should now be passed to AC_INIT and removed from
# the call to AM_INIT_AUTOMAKE.
# We support both call styles for the transition.  After
# the next Automake release, Autoconf can make the AC_INIT
# arguments mandatory, and then we can depend on a new Autoconf
# release and drop the old call support.
AC_DEFUN([AM_INIT_AUTOMAKE],
[AC_PREREQ([2.65])dnl
m4_ifdef([_$0_ALREADY_INIT],
  [m4_fatal([$0 expanded multiple times
]m4_defn([_$0_ALREADY_INIT]))],
  [m4_define([_$0_ALREADY_INIT], m4_expansion_stack)])dnl
dnl Autoconf wants to disallow AM_ names.  We explicitly allow
dnl the ones we care about.
m4_pattern_allow([^AM_[A-Z]+FLAGS$])dnl
AC_REQUIRE([AM_SET_CURRENT_AUTOMAKE_VERSION])dnl
AC_REQUIRE([AC_PROG_INSTALL])dnl
if test "`cd $srcdir && pwd`" != "`pwd`"; then
  # Use -I$(srcdir) only when $(srcdir) != ., so that make's output
  # is not polluted with repeated "-I."
  AC_SUBST([am__isrc], [' -I$(srcdir)'])_AM_SUBST_NOTMAKE([am__isrc])dnl
  # test to see if srcdir already configured
  if test -f $srcdir/config.status; then
    AC_MSG_ERROR([source directory already configured; run "make distclean" there first])
  fi
fi

# test whether we have cygpath
if test -z "$CYGPATH_W"; then
  if (cygpath --version) >/dev/null 2>/dev/null; then
    CYGPATH_W='cygpath -w'
  else
    CYGPATH_W=echo
  fi
fi
AC_SUBST([CYGPATH_W])

# Define the identity of the package.
dnl Distinguish between old-style and new-style calls.
m4_ifval([$2],
[AC_DIAGNOSE([obsolete],
             [$0: two- and three-arguments forms are deprecated.])
m4_ifval([$3], [_AM_SET_OPTION([no-define])])dnl
 AC_SUBST([PACKAGE], [$1])dnl
 AC_SUBST([VERSION], [$2])],
[_AM_SET_OPTIONS([$1])dnl
dnl Diagnose old-style AC_INIT with new-style AM_AUTOMAKE_INIT.
m4_if(
  m4_ifset([AC_PACKAGE_NAME], [ok]):m4_ifset([AC_PACKAGE_VERSION], [ok]),
  [ok:ok],,
  [m4_fatal([AC_INIT should be called with package and version arguments])])dnl
 AC_SUBST([PACKAGE], ['AC_PACKAGE_TARNAME'])dnl
 AC_SUBST([VERSION], ['AC_PACKAGE_VERSION'])])dnl

_AM_IF_OPTION([no-define],,
[AC_DEFINE_UNQUOTED([PACKAGE], ["$PACKAGE"], [Name of package])
 AC_DEFINE_UNQUOTED([VERSION], ["$VERSION"], [Version number of package])])dnl

# Some tools Automake needs.
AC_REQUIRE([AM_SANITY_CHECK])dnl
AC_REQUIRE([AC_ARG_PROGRAM])dnl
AM_MISSING_PROG([ACLOCAL], [aclocal-${am__api_version}])
AM_MISSING_PROG([AUTOCONF], [autoconf])
AM_MISSING_PROG([AUTOMAKE], [automake-${am__api_version}])
AM_MISSING_PROG([AUTOHEADER], [autoheader])
AM_MISSING_PROG([MAKEINFO], [makeinfo])
AC_REQUIRE([AM_PROG_INSTALL_SH])dnl
AC_REQUIRE([AM_PROG_INSTALL_STRIP])dnl
AC_REQUIRE([AC_PROG_MKDIR_P])dnl
# For better backward compatibility.  To be removed once Automake 1.9.x
# dies out for good.  For more background, see:
# <https://lists.gnu.org/archive/html/automake/2012-07/msg00001.html>
# <https://lists.gnu.org/archive/html/automake/2012-07/msg00014.html>
AC_SUBST([mkdir_p], ['$(MKDIR_P)'])
# We need awk for the "check" target (and possibly the TAP driver).  The
# system "awk" is bad on some platforms.
AC_REQUIRE([AC_PROG_AWK])dnl
AC_REQUIRE([AC_PROG_MAKE_SET])dnl
AC_REQUIRE([AM_SET_LEADING_DOT])dnl
_AM_IF_OPTION([tar-ustar], [_AM_PROG_TAR([ustar])],
	      [_AM_IF_OPTION([tar-pax], [_AM_PROG_TAR([pax])],
			     [_AM_PROG_TAR([v7])])])
_AM_IF_OPTION([no-dependencies],,
[AC_PROVIDE_IFELSE([AC_PROG_CC],
		  [_AM_DEPENDENCIES([CC])],
		  [m4_define([AC_PROG_CC],
			     m4_defn([AC_PROG_CC])[_AM_DEPENDENCIES([CC])])])dnl
AC_PROVIDE_IFELSE([AC_PROG_CXX],
		  [_AM_DEPENDENCIES([CXX])],
		  [m4_define([AC_PROG_CXX],
			     m4_defn([AC_PROG_CXX])[_AM_DEPENDENCIES([CXX])])])dnl
AC_PROVIDE_IFELSE([AC_PROG_OBJC],
		  [_AM_DEPENDENCIES([OBJC])],
		  [m4_define([AC_PROG_OBJC],
			     m4_defn([AC_PROG_OBJC])[_AM_DEPENDENCIES([OBJC])])])dnl
AC_PROVIDE_IFELSE([AC_PROG_OBJCXX],
		  [_AM_DEPENDENCIES([OBJCXX])],
		  [m4_define([AC_PROG_OBJCXX],
			     m4_defn([AC_PROG_OBJCXX])[_AM_DEPENDENCIES([OBJCXX])])])dnl
])
# Variables for tags utilities; see am/tags.am
if test -z "$CTAGS"; then
  CTAGS=ctags
fi
AC_SUBST([CTAGS])
if test -z "$ETAGS"; then
  ETAGS=etags
fi
AC_SUBST([ETAGS])
if test -z "$CSCOPE"; then
  CSCOPE=cscope
fi
AC_SUBST([CSCOPE])

AC_REQUIRE([AM_SILENT_RULES])dnl
dnl The testsuite driver may need to know about EXEEXT, so add the
dnl 'am__EXEEXT' conditional if _AM_COMPILER_EXEEXT was seen.  This
dnl macro is hooked onto _AC_COMPILER_EXEEXT early, see below.
AC_CONFIG_COMMANDS_PRE(dnl
[m4_provide_if([_AM_COMPILER_EXEEXT],
  [AM_CONDITIONAL([am__EXEEXT], [test -n "$EXEEXT"])])])dnl

# POSIX will say in a future version that running "rm -f" with no argument
# is OK; and we want to be able to make that assumption in our Makefile
# recipes.  So use an aggressive probe to check that the usage we want is
# actually supported "in the wild" to an acceptable degree.
# See automake bug#10828.
# To make any issue more visible, cause the running configure to be aborted
# by default if the 'rm' program in use doesn't match our expectations; the
# user can still override this though.
if rm -f && rm -fr && rm -rf; then : OK; else
  cat >&2 <<'END'
Oops!

Your 'rm' program seems unable to run without file operands specified
on the command line, even when the '-f' option is present.  This is contrary
to the behaviour of most rm programs out there, and not conforming with
the upcoming POSIX standard: <http://austingroupbugs.net/view.php?id=542>

Please tell bug-automake@gnu.org about your system, including the value
of your $PATH and any error possibly output before this message.  This
can help us improve future automake versions.

END
  if test x"$ACCEPT_INFERIOR_RM_PROGRAM" = x"yes"; then
    echo 'Configuration will proceed anyway, since you have set the' >&2
    echo 'ACCEPT_INFERIOR_RM_PROGRAM variable to "yes"' >&2
    echo >&2
  else
    cat >&2 <<'END'
Aborting the configuration process, to ensure you take notice of the issue.

You can download and install GNU coreutils to get an 'rm' implementation
that behaves properly: <https://www.gnu.org/software/coreutils/>.

If you want to complete the configuration process using your problematic
'rm' anyway, export the environment variable ACCEPT_INFERIOR_RM_PROGRAM
to "yes", and re-run configure.

END
    AC_MSG_ERROR([Your 'rm' program is bad, sorry.])
  fi
fi
dnl The trailing newline in this macro's definition is deliberate, for
dnl backward compatibility and to allow trailing 'dnl'-style comments
dnl after the AM_INIT_AUTOMAKE invocation. See automake bug#16841.
])

dnl Hook into '_AC_COMPILER_EXEEXT' early to learn its expansion.  Do not
dnl add the conditional right here, as _AC_COMPILER_EXEEXT may be further
dnl mangled by Autoconf and run in a shell conditional statement.
m4_define([_AC_COMPILER_EXEEXT],
m4_defn([_AC_COMPILER_EXEEXT])[m4_provide([_AM_COMPILER_EXEEXT])])

# When config.status generates a header, we must update the stamp-h file.
# This file resides in the same directory as the config header
# that is generated.  The stamp files are numbered to have different names.

# Autoconf calls _AC_AM_CONFIG_HEADER_HOOK (when defined) in the
# loop where config.status creates the headers, so we can generate
# our stamp files there.
AC_DEFUN([_AC_AM_CONFIG_HEADER_HOOK],
[# Compute $1's index in $config_headers.
_am_arg=$1
_am_stamp_count=1
for _am_header in $config_headers :; do
  case $_am_header in
    $_am_arg | $_am_arg:* )
      break ;;
    * )
      _am_stamp_count=`expr $_am_stamp_count + 1` ;;
  esac
done
echo "timestamp for $_am_arg" >`AS_DIRNAME(["$_am_arg"])`/stamp-h[]$_am_stamp_count])

# Copyright (C) 2001-2021 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# AM_PROG_INSTALL_SH
# ------------------
# Define $install_sh.
AC_DEFUN([AM_PROG_INSTALL_SH],
[AC_REQUIRE([AM_AUX_DIR_EXPAND])dnl
if test x"${install_sh+set}" != xset; then
  case $am_aux_dir in
  *\ * | *\	*)
    install_sh="\${SHELL} '$am_aux_dir/install-sh'" ;;
  *)
    install_sh="\${SHELL} $am_aux_dir/install-sh"
  esac
fi
AC_SUBST([install_sh])])

# Copyright (C) 2003-2021 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Check whether the underlying file-system supports filenames
# with a leading dot.  For instance MS-DOS doesn't.
AC_DEFUN([AM_SET_LEADING_DOT],
[rm -rf .tst 2>/dev/null
mkdir .tst 2>/dev/null
if test -d .tst; then
  am__leading_dot=.
else
  am__leading_dot=_
fi
rmdir .tst 2>/dev/null
AC_SUBST([am__leading_dot])])

# Check to see how 'make' treats includes.	            -*- Autoconf -*-

# Copyright (C) 2001-2021 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# AM_MAKE_INCLUDE()
# -----------------
# Check whether make has an 'include' directive that can support all
# the idioms we need for our automatic dependency tracking code.
AC_DEFUN([AM_MAKE_INCLUDE],
[AC_MSG_CHECKING([whether ${MAKE-make} supports the include directive])
cat > confinc.mk << 'END'
am__doit:
	@echo this is the am__doit target >confinc.out
.PHONY: am__doit
END
am__include="#"
am__quote=
# BSD make does it like this.
echo '.include "confinc.mk" # ignored' > confmf.BSD
# Other make implementations (GNU, Solaris 10, AIX) do it like this.
echo 'include confinc.mk # ignored' > confmf.GNU
_am_result=no
for s in GNU BSD; do
  AM_RUN_LOG([${MAKE-make} -f confmf.$s && cat confinc.out])
  AS_CASE([$?:`cat confinc.out 2>/dev/null`],
      ['0:this is the am__doit target'],
      [AS_CASE([$s],
          [BSD], [am__include='.include' am__quote='"'],
          [am__include='include' am__quote=''])])
  if test "$am__include" != "#"; then
    _am_result="yes ($s style)"
    break
  fi
done
rm -f confinc.* confmf.*
AC_MSG_RESULT([${_am_result}])
AC_SUBST([am__include])])
AC_SUBST([am__quote])])

# Fake the existence of programs that GNU maintainers use.  -*- Autoconf -*-

# Copyright (C) 1997-2021 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# AM_MISSING_PROG(NAME, PROGRAM)
# ------------------------------
AC_DEFUN([AM_MISSING_PROG],
[AC_REQUIRE([AM_MISSING_HAS_RUN])
$1=${$1-"${am_missing_run}$2"}
AC_SUBST($1)])

# AM_MISSING_HAS_RUN
# ------------------
# Define MISSING if not defined so far and test if it is modern enough.
# If it is, set am_missing_run to use it, otherwise, to nothing.
AC_DEFUN([AM_MISSING_HAS_RUN],
[AC_REQUIRE([AM_AUX_DIR_EXPAND])dnl
AC_REQUIRE_AUX_FILE([missing])dnl
if test x"${MISSING+set}" != xset; then
  MISSING="\${SHELL} '$am_aux_dir/missing'"
fi
# Use eval to expand $SHELL
if eval "$MISSING --is-lightweight"; then
  am_missing_run="$MISSING "
else
  am_missing_run=
  AC_MSG_WARN(['missing' script is too old or missing])
fi
])

# Helper functions for option handling.                     -*- Autoconf -*-

# Copyright (C) 2001-2021 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# _AM_MANGLE_OPTION(NAME)
# -----------------------
AC_DEFUN([_AM_MANGLE_OPTION],
[[_AM_OPTION_]m4_bpatsubst($1, [[^a-zA-Z0-9_]], [_])])

# _AM_SET_OPTION(NAME)
# --------------------
# Set option NAME.  Presently that only means defining a flag for this option.
AC_DEFUN([_AM_SET_OPTION],
[m4_define(_AM_MANGLE_OPTION([$1]), [1])])

# _AM_SET_OPTIONS(OPTIONS)
# ------------------------
# OPTIONS is a space-separated list of Automake options.
AC_DEFUN([_AM_SET_OPTIONS],
[m4_foreach_w([_AM_Option], [$1], [_AM_SET_OPTION(_AM_Option)])])

# _AM_IF_OPTION(OPTION, IF-SET, [IF-NOT-SET])
# -------------------------------------------
# Execute IF-SET if OPTION is set, IF-NOT-SET otherwise.
AC_DEFUN([_AM_IF_OPTION],
[m4_ifset(_AM_MANGLE_OPTION([$1]), [$2], [$3])])

# Copyright (C) 1999-2021 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# _AM_PROG_CC_C_O
# ---------------
# Like AC_PROG_CC_C_O, but changed for automake.  We rewrite AC_PROG_CC
# to automatically call this.
AC_DEFUN([_AM_PROG_CC_C_O],
[AC_REQUIRE([AM_AUX_DIR_EXPAND])dnl
AC_REQUIRE_AUX_FILE([compile])dnl
AC_LANG_PUSH([C])dnl
AC_CACHE_CHECK(
  [whether $CC understands -c and -o together],
  [am_cv_prog_cc_c_o],
  [AC_LANG_CONFTEST([AC_LANG_PROGRAM([])])
  # Make sure it works both with $CC and with simple cc.
  # Following AC_PROG_CC_C_O, we do the test twice because some
  # compilers refuse to overwrite an existing .o file with -o,
  # though they will create one.
  am_cv_prog_cc_c_o=yes
  for am_i in 1 2; do
    if AM_RUN_LOG([$CC -c conftest.$ac_ext -o conftest2.$ac_objext]) \
         && test -f conftest2.$ac_objext; then
      : OK
    else
      am_cv_prog_cc_c_o=no
      break
    fi
  done
  rm -f core conftest*
  unset am_i])
if test "$am_cv_prog_cc_c_o" != yes; then
   # Losing compiler, so override with the script.
   # FIXME: It is wrong to rewrite CC.
   # But if we don't then we get into trouble of one sort or another.
   # A longer-term fix would be to have automake use am__CC in this case,
   # and then we could set am__CC="\$(top_srcdir)/compile \$(CC)"
   CC="$am_aux_dir/compile $CC"
fi
AC_LANG_POP([C])])

# For backward compatibility.
AC_DEFUN_ONCE([AM_PROG_CC_C_O], [AC_REQUIRE([AC_PROG_CC])])

# Copyright (C) 1999-2021 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.


# AM_PATH_PYTHON([MINIMUM-VERSION], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# ---------------------------------------------------------------------------
# Adds support for distributing Python modules and packages.  To
# install modules, copy them to $(pythondir), using the python_PYTHON
# automake variable.  To install a package with the same name as the
# automake package, install to $(pkgpythondir), or use the
# pkgpython_PYTHON automake variable.
#
# The variables $(pyexecdir) and $(pkgpyexecdir) are provided as
# locations to install python extension modules (shared libraries).
# Another macro is required to find the appropriate flags to compile
# extension modules.
#
# If your package is configured with a different prefix to python,
# users will have to add the install directory to the PYTHONPATH
# environment variable, or create a .pth file (see the python
# documentation for details).
#
# If the MINIMUM-VERSION argument is passed, AM_PATH_PYTHON will
# cause an error if the version of python installed on the system
# doesn't meet the requirement.  MINIMUM-VERSION should consist of
# numbers and dots only.
AC_DEFUN([AM_PATH_PYTHON],
 [
  dnl Find a Python interpreter.  Python versions prior to 2.0 are not
  dnl supported. (2.0 was released on October 16, 2000).
  m4_define_default([_AM_PYTHON_INTERPRETER_LIST],
[python python2 python3 dnl
 python3.9 python3.8 python3.7 python3.6 python3.5 python3.4 python3.3 dnl
 python3.2 python3.1 python3.0 dnl
 python2.7 python2.6 python2.5 python2.4 python2.3 python2.2 python2.1 dnl
 python2.0])

  AC_ARG_VAR([PYTHON], [the Python interpreter])

  m4_if([$1],[],[
    dnl No version check is needed.
    # Find any Python interpreter.
    if test -z "$PYTHON"; then
      AC_PATH_PROGS([PYTHON], _AM_PYTHON_INTERPRETER_LIST, :)
    fi
    am_display_PYTHON=python
  ], [
    dnl A version check is needed.
    if test -n "$PYTHON"; then
      # If the user set $PYTHON, use it and don't search something else.
      AC_MSG_CHECKING([whether $PYTHON version is >= $1])
      AM_PYTHON_CHECK_VERSION([$PYTHON], [$1],
			      [AC_MSG_RESULT([yes])],
			      [AC_MSG_RESULT([no])
			       AC_MSG_ERROR([Python interpreter is too old])])
      am_display_PYTHON=$PYTHON
    else
      # Otherwise, try each interpreter until we find one that satisfies
      # VERSION.
      AC_CACHE_CHECK([for a Python interpreter with version >= $1],
	[am_cv_pathless_PYTHON],[
	for am_cv_pathless_PYTHON in _AM_PYTHON_INTERPRETER_LIST none; do
	  test "$am_cv_pathless_PYTHON" = none && break
	  AM_PYTHON_CHECK_VERSION([$am_cv_pathless_PYTHON], [$1], [break])
	done])
      # Set $PYTHON to the absolute path of $am_cv_pathless_PYTHON.
      if test "$am_cv_pathless_PYTHON" = none; then
	PYTHON=:
      else
        AC_PATH_PROG([PYTHON], [$am_cv_pathless_PYTHON])
      fi
      am_display_PYTHON=$am_cv_pathless_PYTHON
    fi
  ])

  if test "$PYTHON" = :; then
    dnl Run any user-specified action, or abort.
    m4_default([$3], [AC_MSG_ERROR([no suitable Python interpreter found])])
  else

  dnl Query Python for its version number.  Although site.py simply uses
  dnl sys.version[:3], printing that failed with Python 3.10, since the
  dnl trailing zero was eliminated. So now we output just the major
  dnl and minor version numbers, as numbers. Apparently the tertiary
  dnl version is not of interest.
  dnl
  AC_CACHE_CHECK([for $am_display_PYTHON version], [am_cv_python_version],
    [am_cv_python_version=`$PYTHON -c "import sys; print ('%u.%u' % sys.version_info[[:2]])"`])
  AC_SUBST([PYTHON_VERSION], [$am_cv_python_version])

  dnl At times, e.g., when building shared libraries, you may want
  dnl to know which OS platform Python thinks this is.
  dnl
  AC_CACHE_CHECK([for $am_display_PYTHON platform], [am_cv_python_platform],
    [am_cv_python_platform=`$PYTHON -c "import sys; sys.stdout.write(sys.platform)"`])
  AC_SUBST([PYTHON_PLATFORM], [$am_cv_python_platform])

  dnl emacs-page
  dnl If --with-python-sys-prefix is given, use the values of sys.prefix
  dnl and sys.exec_prefix for the corresponding values of PYTHON_PREFIX
  dnl and PYTHON_EXEC_PREFIX. Otherwise, use the GNU ${prefix} and
  dnl ${exec_prefix} variables.
  dnl
  dnl The two are made distinct variables so they can be overridden if
  dnl need be, although general consensus is that you shouldn't need
  dnl this separation.
  dnl
  dnl Also allow directly setting the prefixes via configure options,
  dnl overriding any default.
  dnl
  if test "x$prefix" = xNONE; then
    am__usable_prefix=$ac_default_prefix
  else
    am__usable_prefix=$prefix
  fi

  # Allow user to request using sys.* values from Python,
  # instead of the GNU $prefix values.
  AC_ARG_WITH([python-sys-prefix],
  [AS_HELP_STRING([--with-python-sys-prefix],
                  [use Python's sys.prefix and sys.exec_prefix values])],
  [am_use_python_sys=:],
  [am_use_python_sys=false])

  # Allow user to override whatever the default Python prefix is.
  AC_ARG_WITH([python_prefix],
  [AS_HELP_STRING([--with-python_prefix],
                  [override the default PYTHON_PREFIX])],
  [am_python_prefix_subst=$withval
   am_cv_python_prefix=$withval
   AC_MSG_CHECKING([for explicit $am_display_PYTHON prefix])
   AC_MSG_RESULT([$am_cv_python_prefix])],
  [
   if $am_use_python_sys; then
     # using python sys.prefix value, not GNU
     AC_CACHE_CHECK([for python default $am_display_PYTHON prefix],
     [am_cv_python_prefix],
     [am_cv_python_prefix=`$PYTHON -c "import sys; sys.stdout.write(sys.prefix)"`])

     dnl If sys.prefix is a subdir of $prefix, replace the literal value of
     dnl $prefix with a variable reference so it can be overridden.
     case $am_cv_python_prefix in
     $am__usable_prefix*)
       am__strip_prefix=`echo "$am__usable_prefix" | sed 's|.|.|g'`
       am_python_prefix_subst=`echo "$am_cv_python_prefix" | sed "s,^$am__strip_prefix,\\${prefix},"`
       ;;
     *)
       am_python_prefix_subst=$am_cv_python_prefix
       ;;
     esac
   else # using GNU prefix value, not python sys.prefix
     am_python_prefix_subst='${prefix}'
     am_python_prefix=$am_python_prefix_subst
     AC_MSG_CHECKING([for GNU default $am_display_PYTHON prefix])
     AC_MSG_RESULT([$am_python_prefix])
   fi])
  # Substituting python_prefix_subst value.
  AC_SUBST([PYTHON_PREFIX], [$am_python_prefix_subst])

  # emacs-page Now do it all over again for Python exec_prefix, but with yet
  # another conditional: fall back to regular prefix if that was specified.
  AC_ARG_WITH([python_exec_prefix],
  [AS_HELP_STRING([--with-python_exec_prefix],
                  [override the default PYTHON_EXEC_PREFIX])],
  [am_python_exec_prefix_subst=$withval
   am_cv_python_exec_prefix=$withval
   AC_MSG_CHECKING([for explicit $am_display_PYTHON exec_prefix])
   AC_MSG_RESULT([$am_cv_python_exec_prefix])],
  [
   # no explicit --with-python_exec_prefix, but if
   # --with-python_prefix was given, use its value for python_exec_prefix too.
   AS_IF([test -n "$with_python_prefix"],
   [am_python_exec_prefix_subst=$with_python_prefix
    am_cv_python_exec_prefix=$with_python_prefix
    AC_MSG_CHECKING([for python_prefix-given $am_display_PYTHON exec_prefix])
    AC_MSG_RESULT([$am_cv_python_exec_prefix])],
   [
    # Set am__usable_exec_prefix whether using GNU or Python values,
    # since we use that variable for pyexecdir.
    if test "x$exec_prefix" = xNONE; then
      am__usable_exec_prefix=$am__usable_prefix
    else
      am__usable_exec_prefix=$exec_prefix
    fi
    #
    if $am_use_python_sys; then # using python sys.exec_prefix, not GNU
      AC_CACHE_CHECK([for python default $am_display_PYTHON exec_prefix],
      [am_cv_python_exec_prefix],
      [am_cv_python_exec_prefix=`$PYTHON -c "import sys; sys.stdout.write(sys.exec_prefix)"`])
      dnl If sys.exec_prefix is a subdir of $exec_prefix, replace the
      dnl literal value of $exec_prefix with a variable reference so it can
      dnl be overridden.
      case $am_cv_python_exec_prefix in
      $am__usable_exec_prefix*)
        am__strip_prefix=`echo "$am__usable_exec_prefix" | sed 's|.|.|g'`
        am_python_exec_prefix_subst=`echo "$am_cv_python_exec_prefix" | sed "s,^$am__strip_prefix,\\${exec_prefix},"`
        ;;
      *)
        am_python_exec_prefix_subst=$am_cv_python_exec_prefix
        ;;
     esac
   else # using GNU $exec_prefix, not python sys.exec_prefix
     am_python_exec_prefix_subst='${exec_prefix}'
     am_python_exec_prefix=$am_python_exec_prefix_subst
     AC_MSG_CHECKING([for GNU default $am_display_PYTHON exec_prefix])
     AC_MSG_RESULT([$am_python_exec_prefix])
   fi])])
  # Substituting python_exec_prefix_subst.
  AC_SUBST([PYTHON_EXEC_PREFIX], [$am_python_exec_prefix_subst])

  # Factor out some code duplication into this shell variable.
  am_python_setup_sysconfig="\
import sys
# Prefer sysconfig over distutils.sysconfig, for better compatibility
# with python 3.x.  See automake bug#10227.
try:
    import sysconfig
except ImportError:
    can_use_sysconfig = 0
else:
    can_use_sysconfig = 1
# Can't use sysconfig in CPython 2.7, since it's broken in virtualenvs:
# <https://github.com/pypa/virtualenv/issues/118>
try:
    from platform import python_implementation
    if python_implementation() == 'CPython' and sys.version[[:3]] == '2.7':
        can_use_sysconfig = 0
except ImportError:
    pass"

  dnl emacs-page Set up 4 directories:

  dnl 1. pythondir: where to install python scripts.  This is the
  dnl    site-packages directory, not the python standard library
  dnl    directory like in previous automake betas.  This behavior
  dnl    is more consistent with lispdir.m4 for example.
  dnl Query distutils for this directory.
  dnl
  AC_CACHE_CHECK([for $am_display_PYTHON script directory (pythondir)],
  [am_cv_python_pythondir],
  [if test "x$am_cv_python_prefix" = x; then
     am_py_prefix=$am__usable_prefix
   else
     am_py_prefix=$am_cv_python_prefix
   fi
   am_cv_python_pythondir=`$PYTHON -c "
$am_python_setup_sysconfig
if can_use_sysconfig:
  sitedir = sysconfig.get_path('purelib', vars={'base':'$am_py_prefix'})
else:
  from distutils import sysconfig
  sitedir = sysconfig.get_python_lib(0, 0, prefix='$am_py_prefix')
sys.stdout.write(sitedir)"`
   #
   case $am_cv_python_pythondir in
   $am_py_prefix*)
     am__strip_prefix=`echo "$am_py_prefix" | sed 's|.|.|g'`
     am_cv_python_pythondir=`echo "$am_cv_python_pythondir" | sed "s,^$am__strip_prefix,\\${PYTHON_PREFIX},"`
     ;;
   *)
     case $am_py_prefix in
       /usr|/System*) ;;
       *) am_cv_python_pythondir="\${PYTHON_PREFIX}/lib/python$PYTHON_VERSION/site-packages"
          ;;
     esac
     ;;
   esac
  ])
  AC_SUBST([pythondir], [$am_cv_python_pythondir])

  dnl 2. pkgpythondir: $PACKAGE directory under pythondir.  Was
  dnl    PYTHON_SITE_PACKAGE in previous betas, but this naming is
  dnl    more consistent with the rest of automake.
  dnl
  AC_SUBST([pkgpythondir], [\${pythondir}/$PACKAGE])

  dnl 3. pyexecdir: directory for installing python extension modules
  dnl    (shared libraries).
  dnl Query distutils for this directory.
  dnl
  AC_CACHE_CHECK([for $am_display_PYTHON extension module directory (pyexecdir)],
  [am_cv_python_pyexecdir],
  [if test "x$am_cv_python_exec_prefix" = x; then
     am_py_exec_prefix=$am__usable_exec_prefix
   else
     am_py_exec_prefix=$am_cv_python_exec_prefix
   fi
   am_cv_python_pyexecdir=`$PYTHON -c "
$am_python_setup_sysconfig
if can_use_sysconfig:
  sitedir = sysconfig.get_path('platlib', vars={'platbase':'$am_py_exec_prefix'})
else:
  from distutils import sysconfig
  sitedir = sysconfig.get_python_lib(1, 0, prefix='$am_py_exec_prefix')
sys.stdout.write(sitedir)"`
   #
   case $am_cv_python_pyexecdir in
   $am_py_exec_prefix*)
     am__strip_prefix=`echo "$am_py_exec_prefix" | sed 's|.|.|g'`
     am_cv_python_pyexecdir=`echo "$am_cv_python_pyexecdir" | sed "s,^$am__strip_prefix,\\${PYTHON_EXEC_PREFIX},"`
     ;;
   *)
     case $am_py_exec_prefix in
       /usr|/System*) ;;
       *) am_cv_python_pyexecdir="\${PYTHON_EXEC_PREFIX}/lib/python$PYTHON_VERSION/site-packages"
          ;;
     esac
     ;;
   esac
  ])
  AC_SUBST([pyexecdir], [$am_cv_python_pyexecdir])

  dnl 4. pkgpyexecdir: $(pyexecdir)/$(PACKAGE)
  dnl
  AC_SUBST([pkgpyexecdir], [\${pyexecdir}/$PACKAGE])

  dnl Run any user-specified action.
  $2
  fi
])


# AM_PYTHON_CHECK_VERSION(PROG, VERSION, [ACTION-IF-TRUE], [ACTION-IF-FALSE])
# ---------------------------------------------------------------------------
# Run ACTION-IF-TRUE if the Python interpreter PROG has version >= VERSION.
# Run ACTION-IF-FALSE otherwise.
# This test uses sys.hexversion instead of the string equivalent (first
# word of sys.version), in order to cope with versions such as 2.2c1.
# This supports Python 2.0 or higher. (2.0 was released on October 16, 2000).
AC_DEFUN([AM_PYTHON_CHECK_VERSION],
 [prog="import sys
# split strings by '.' and convert to numeric.  Append some zeros
# because we need at least 4 digits for the hex conversion.
# map returns an iterator in Python 3.0 and a list in 2.x
minver = list(map(int, '$2'.split('.'))) + [[0, 0, 0]]
minverhex = 0
# xrange is not present in Python 3.0 and range returns an iterator
for i in list(range(0, 4)): minverhex = (minverhex << 8) + minver[[i]]
sys.exit(sys.hexversion < minverhex)"
  AS_IF([AM_RUN_LOG([$1 -c "$prog"])], [$3], [$4])])

# Copyright (C) 2001-2021 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# AM_RUN_LOG(COMMAND)
# -------------------
# Run COMMAND, save the exit status in ac_status, and log it.
# (This has been adapted from Autoconf's _AC_RUN_LOG macro.)
AC_DEFUN([AM_RUN_LOG],
[{ echo "$as_me:$LINENO: $1" >&AS_MESSAGE_LOG_FD
   ($1) >&AS_MESSAGE_LOG_FD 2>&AS_MESSAGE_LOG_FD
   ac_status=$?
   echo "$as_me:$LINENO: \$? = $ac_status" >&AS_MESSAGE_LOG_FD
   (exit $ac_status); }])

# Check to make sure that the build environment is sane.    -*- Autoconf -*-

# Copyright (C) 1996-2021 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# AM_SANITY_CHECK
# ---------------
AC_DEFUN([AM_SANITY_CHECK],
[AC_MSG_CHECKING([whether build environment is sane])
# Reject unsafe characters in $srcdir or the absolute working directory
# name.  Accept space and tab only in the latter.
am_lf='
'
case `pwd` in
  *[[\\\"\#\$\&\'\`$am_lf]]*)
    AC_MSG_ERROR([unsafe absolute working directory name]);;
esac
case $srcdir in
  *[[\\\"\#\$\&\'\`$am_lf\ \	]]*)
    AC_MSG_ERROR([unsafe srcdir value: '$srcdir']);;
esac

# Do 'set' in a subshell so we don't clobber the current shell's
# arguments.  Must try -L first in case configure is actually a
# symlink; some systems play weird games with the mod time of symlinks
# (eg FreeBSD returns the mod time of the symlink's containing
# directory).
if (
   am_has_slept=no
   for am_try in 1 2; do
     echo "timestamp, slept: $am_has_slept" > conftest.file
     set X `ls -Lt "$srcdir/configure" conftest.file 2> /dev/null`
     if test "$[*]" = "X"; then
	# -L didn't work.
	set X `ls -t "$srcdir/configure" conftest.file`
     fi
     if test "$[*]" != "X $srcdir/configure conftest.file" \
	&& test "$[*]" != "X conftest.file $srcdir/configure"; then

	# If neither matched, then we have a broken ls.  This can happen
	# if, for instance, CONFIG_SHELL is bash and it inherits a
	# broken ls alias from the environment.  This has actually
	# happened.  Such a system could not be considered "sane".
	AC_MSG_ERROR([ls -t appears to fail.  Make sure there is not a broken
  alias in your environment])
     fi
     if test "$[2]" = conftest.file || test $am_try -eq 2; then
       break
     fi
     # Just in case.
     sleep 1
     am_has_slept=yes
   done
   test "$[2]" = conftest.file
   )
then
   # Ok.
   :
else
   AC_MSG_ERROR([newly created file is older than distributed files!
Check your system clock])
fi
AC_MSG_RESULT([yes])
# If we didn't sleep, we still need to ensure time stamps of config.status and
# generated files are strictly newer.
am_sleep_pid=
if grep 'slept: no' conftest.file >/dev/null 2>&1; then
  ( sleep 1 ) &
  am_sleep_pid=$!
fi
AC_CONFIG_COMMANDS_PRE(
  [AC_MSG_CHECKING([that generated files are newer than configure])
   if test -n "$am_sleep_pid"; then
     # Hide warnings about reused PIDs.
     wait $am_sleep_pid 2>/dev/null
   fi
   AC_MSG_RESULT([done])])
rm -f conftest.file
])

# Copyright (C) 2009-2021 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# AM_SILENT_RULES([DEFAULT])
# --------------------------
# Enable less verbose build rules; with the default set to DEFAULT
# ("yes" being less verbose, "no" or empty being verbose).
AC_DEFUN([AM_SILENT_RULES],
[AC_ARG_ENABLE([silent-rules], [dnl
AS_HELP_STRING(
  [--enable-silent-rules],
  [less verbose build output (undo: "make V=1")])
AS_HELP_STRING(
  [--disable-silent-rules],
  [verbose build output (undo: "make V=0")])dnl
])
case $enable_silent_rules in @%:@ (((
  yes) AM_DEFAULT_VERBOSITY=0;;
   no) AM_DEFAULT_VERBOSITY=1;;
    *) AM_DEFAULT_VERBOSITY=m4_if([$1], [yes], [0], [1]);;
esac
dnl
dnl A few 'make' implementations (e.g., NonStop OS and NextStep)
dnl do not support nested variable expansions.
dnl See automake bug#9928 and bug#10237.
am_make=${MAKE-make}
AC_CACHE_CHECK([whether $am_make supports nested variables],
   [am_cv_make_support_nested_variables],
   [if AS_ECHO([['TRUE=$(BAR$(V))
BAR0=false
BAR1=true
V=1
am__doit:
	@$(TRUE)
.PHONY: am__doit']]) | $am_make -f - >/dev/null 2>&1; then
  am_cv_make_support_nested_variables=yes
else
  am_cv_make_support_nested_variables=no
fi])
if test $am_cv_make_support_nested_variables = yes; then
  dnl Using '$V' instead of '$(V)' breaks IRIX make.
  AM_V='$(V)'
  AM_DEFAULT_V='$(AM_DEFAULT_VERBOSITY)'
else
  AM_V=$AM_DEFAULT_VERBOSITY
  AM_DEFAULT_V=$AM_DEFAULT_VERBOSITY
fi
AC_SUBST([AM_V])dnl
AM_SUBST_NOTMAKE([AM_V])dnl
AC_SUBST([AM_DEFAULT_V])dnl
AM_SUBST_NOTMAKE([AM_DEFAULT_V])dnl
AC_SUBST([AM_DEFAULT_VERBOSITY])dnl
AM_BACKSLASH='\'
AC_SUBST([AM_BACKSLASH])dnl
_AM_SUBST_NOTMAKE([AM_BACKSLASH])dnl
])

# Copyright (C) 2001-2021 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# AM_PROG_INSTALL_STRIP
# ---------------------
# One issue with vendor 'install' (even GNU) is that you can't
# specify the program used to strip binaries.  This is especially
# annoying in cross-compiling environments, where the build's strip
# is unlikely to handle the host's binaries.
# Fortunately install-sh will honor a STRIPPROG variable, so we
# always use install-sh in "make install-strip", and initialize
# STRIPPROG with the value of the STRIP variable (set by the user).
AC_DEFUN([AM_PROG_INSTALL_STRIP],
[AC_REQUIRE([AM_PROG_INSTALL_SH])dnl
# Installed binaries are usually stripped using 'strip' when the user
# run "make install-strip".  However 'strip' might not be the right
# tool to use in cross-compilation environments, therefore Automake
# will honor the 'STRIP' environment variable to overrule this program.
dnl Don't test for $cross_compiling = yes, because it might be 'maybe'.
if test "$cross_compiling" != no; then
  AC_CHECK_TOOL([STRIP], [strip], :)
fi
INSTALL_STRIP_PROGRAM="\$(install_sh) -c -s"
AC_SUBST([INSTALL_STRIP_PROGRAM])])

# Copyright (C) 2006-2021 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# _AM_SUBST_NOTMAKE(VARIABLE)
# ---------------------------
# Prevent Automake from outputting VARIABLE = @VARIABLE@ in Makefile.in.
# This macro is traced by Automake.
AC_DEFUN([_AM_SUBST_NOTMAKE])

# AM_SUBST_NOTMAKE(VARIABLE)
# --------------------------
# Public sister of _AM_SUBST_NOTMAKE.
AC_DEFUN([AM_SUBST_NOTMAKE], [_AM_SUBST_NOTMAKE($@)])

# Check how to create a tarball.                            -*- Autoconf -*-

# Copyright (C) 2004-2021 Free Software Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# _AM_PROG_TAR(FORMAT)
# --------------------
# Check how to create a tarball in format FORMAT.
# FORMAT should be one of 'v7', 'ustar', or 'pax'.
#
# Substitute a variable $(am__tar) that is a command
# writing to stdout a FORMAT-tarball containing the directory
# $tardir.
#     tardir=directory && $(am__tar) > result.tar
#
# Substitute a variable $(am__untar) that extract such
# a tarball read from stdin.
#     $(am__untar) < result.tar
#
AC_DEFUN([_AM_PROG_TAR],
[# Always define AMTAR for backward compatibility.  Yes, it's still used
# in the wild :-(  We should find a proper way to deprecate it ...
AC_SUBST([AMTAR], ['$${TAR-tar}'])

# We'll loop over all known methods to create a tar archive until one works.
_am_tools='gnutar m4_if([$1], [ustar], [plaintar]) pax cpio none'

m4_if([$1], [v7],
  [am__tar='$${TAR-tar} chof - "$$tardir"' am__untar='$${TAR-tar} xf -'],

  [m4_case([$1],
    [ustar],
     [# The POSIX 1988 'ustar' format is defined with fixed-size fields.
      # There is notably a 21 bits limit for the UID and the GID.  In fact,
      # the 'pax' utility can hang on bigger UID/GID (see automake bug#8343
      # and bug#13588).
      am_max_uid=2097151 # 2^21 - 1
      am_max_gid=$am_max_uid
      # The $UID and $GID variables are not portable, so we need to resort
      # to the POSIX-mandated id(1) utility.  Errors in the 'id' calls
      # below are definitely unexpected, so allow the users to see them
      # (that is, avoid stderr redirection).
      am_uid=`id -u || echo unknown`
      am_gid=`id -g || echo unknown`
      AC_MSG_CHECKING([whether UID '$am_uid' is supported by ustar format])
      if test $am_uid -le $am_max_uid; then
         AC_MSG_RESULT([yes])
      else
         AC_MSG_RESULT([no])
         _am_tools=none
      fi
      AC_MSG_CHECKING([whether GID '$am_gid' is supported by ustar format])
      if test $am_gid -le $am_max_gid; then
         AC_MSG_RESULT([yes])
      else
        AC_MSG_RESULT([no])
        _am_tools=none
      fi],

  [pax],
    [],

  [m4_fatal([Unknown tar format])])

  AC_MSG_CHECKING([how to create a $1 tar archive])

  # Go ahead even if we have the value already cached.  We do so because we
  # need to set the values for the 'am__tar' and 'am__untar' variables.
  _am_tools=${am_cv_prog_tar_$1-$_am_tools}

  for _am_tool in $_am_tools; do
    case $_am_tool in
    gnutar)
      for _am_tar in tar gnutar gtar; do
        AM_RUN_LOG([$_am_tar --version]) && break
      done
      am__tar="$_am_tar --format=m4_if([$1], [pax], [posix], [$1]) -chf - "'"$$tardir"'
      am__tar_="$_am_tar --format=m4_if([$1], [pax], [posix], [$1]) -chf - "'"$tardir"'
      am__untar="$_am_tar -xf -"
      ;;
    plaintar)
      # Must skip GNU tar: if it does not support --format= it doesn't create
      # ustar tarball either.
      (tar --version) >/dev/null 2>&1 && continue
      am__tar='tar chf - "$$tardir"'
      am__tar_='tar chf - "$tardir"'
      am__untar='tar xf -'
      ;;
    pax)
      am__tar='pax -L -x $1 -w "$$tardir"'
      am__tar_='pax -L -x $1 -w "$tardir"'
      am__untar='pax -r'
      ;;
    cpio)
      am__tar='find "$$tardir" -print | cpio -o -H $1 -L'
      am__tar_='find "$tardir" -print | cpio -o -H $1 -L'
      am__untar='cpio -i -H $1 -d'
      ;;
    none)
      am__tar=false
      am__tar_=false
      am__untar=false
      ;;
    esac

    # If the value was cached, stop now.  We just wanted to have am__tar
    # and am__untar set.
    test -n "${am_cv_prog_tar_$1}" && break

    # tar/untar a dummy directory, and stop if the command works.
    rm -rf conftest.dir
    mkdir conftest.dir
    echo GrepMe > conftest.dir/file
    AM_RUN_LOG([tardir=conftest.dir && eval $am__tar_ >conftest.tar])
    rm -rf conftest.dir
    if test -s conftest.tar; then
      AM_RUN_LOG([$am__untar <conftest.tar])
      AM_RUN_LOG([cat conftest.dir/file])
      grep GrepMe conftest.dir/file >/dev/null 2>&1 && break
    fi
  done
  rm -rf conftest.dir

  AC_CACHE_VAL([am_cv_prog_tar_$1], [am_cv_prog_tar_$1=$_am_tool])
  AC_MSG_RESULT([$am_cv_prog_tar_$1])])

AC_SUBST([am__tar])
AC_SUBST([am__untar])
]) # _AM_PROG_TAR

m4_include([config/ax_subdirs_configure.m4])
m4_include([config/libtool.m4])
m4_include([config/ltoptions.m4])
m4_include([config/ltsugar.m4])
m4_include([config/ltversion.m4])
m4_include([config/lt~obsolete.m4])
