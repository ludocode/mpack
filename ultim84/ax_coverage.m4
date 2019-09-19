AC_DEFUN([AX_COVERAGE],
[
  AC_ARG_VAR([GCOV], [Coverage testing command])
  if test "x$GCOV" = "x"; then
    AC_PATH_PROG(GCOV, gcov, no)
  else
    AC_PATH_PROG(GCOV, $GCOV, no)
  fi

  AC_PATH_PROG(LCOV, lcov, no)
  AC_PATH_PROG(GENHTML, genhtml)
  AC_CHECK_PROG(HAVE_CPPFILT, c++filt, yes, no)
  if test "x$HAVE_CPPFILT" = "xyes"; then
    GENHTML_OPTIONS="--demangle-cpp"
  else
    GENHTML_OPTIONS=""
  fi


  AC_MSG_CHECKING([for clang])

  AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([], [[
#ifndef __clang__
  not clang
#endif
]])],
[CLANG=yes], [CLANG=no])

  AC_MSG_RESULT([$CLANG])

  COVERAGE_CFLAGS="-fprofile-arcs -ftest-coverage"
  COVERAGE_CXXFLAGS="-fprofile-arcs -ftest-coverage"
  COVERAGE_OPTFLAGS="-O0"
  COVERAGE_LDFLAGS="-fprofile-arcs -ftest-coverage"
  if test "x$GCC" = "xyes" -a "x$CLANG" = "xno"; then
    COVERAGE_LIBS="-lgcov"
  else
    COVERAGE_LIBS=""
  fi

  if test "x$GCOV" != "xno" -a "x$LCOV" != "xno"; then
    COVERAGE_SUPPORT="yes"
  else
    COVERAGE_SUPPORT="no"
  fi

  AC_SUBST([COVERAGE_SUPPORT])
  AC_SUBST([GCOV])
  AC_SUBST([LCOV])
  AC_SUBST([GENHTML])
  AC_SUBST([GENHTML_OPTIONS])
  AC_SUBST([COVERAGE_CFLAGS])
  AC_SUBST([COVERAGE_CXXFLAGS])
  AC_SUBST([COVERAGE_OPTFLAGS])
  AC_SUBST([COVERAGE_LDFLAGS])
  AC_SUBST([COVERAGE_LIBS])
])
