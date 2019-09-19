AC_PREREQ([2.61])
AC_INIT([libmpack], [1.0], [mkschreder.uk@gmail.com])
AC_CONFIG_AUX_DIR(aux-dist)
AM_INIT_AUTOMAKE

AC_USE_SYSTEM_EXTENSIONS

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_MACRO_DIR([ultim84])

AC_LANG(C,ASM)

AC_PROG_MAKE_SET
AC_PROG_CC

PKG_PROG_PKG_CONFIG
PKG_INSTALLDIR

AC_CANONICAL_HOST

AX_CHECK_STDC
AX_SUPERWARN

AC_TYPE_SSIZE_T
AC_FUNC_STRNLEN

LT_INIT([shared])

AC_CONFIG_FILES([Makefile src/mpack/Makefile])
AC_CONFIG_FILES([libmpack.pc])

AC_OUTPUT

AC_MSG_RESULT([
Configure Information:
  Host              : $host
  C Compiler        : $CC
    DEFS            :   $DEFS
    CPPFLAGS        :   $CPPFLAGS
    CFLAGS          :   $CFLAGS

  C++ Compiler      : $CXX
    DEFS            :   $DEFS
    CPPFLAGS        :   $CPPFLAGS
    CXXFLAGS        :   $CXXFLAGS

  Linker            : $LD
    LDFLAGS         :   $LDFLAGS
    LIBS            :   $LIBS

  Coverage Reports      : $COVERAGE_SUPPORT
    gcov                : $GCOV
      COVERAGE_CFLAGS   : $COVERAGE_CFLAGS
      COVERAGE_CXXFLAGS : $COVERAGE_CXXFLAGS
      COVERAGE_OPTFLAGS : $COVERAGE_OPTFLAGS
      COVERAGE_LDFLAGS  : $COVERAGE_LDFLAGS
      COVERAGE_LIBS     : $COVERAGE_LIBS
    lcov                : $LCOV
])
