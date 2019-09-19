
AC_DEFUN([AX_PRINT_INFO],
echo "
  $PACKAGE_NAME..     : v$PACKAGE_VERSION
  Prefix.........     : $prefix
  Debug Build....     : $debug
  C Compiler.....     : $CC $CFLAGS $CPPFLAGS
  Linker.........     : $LD $LDFLAGS $LIBS
  Coverage Reports    : $COVERAGE_SUPPORT
	gcov                : $GCOV
	COVERAGE_CFLAGS     : $COVERAGE_CFLAGS
	COVERAGE_CXXFLAGS   : $COVERAGE_CXXFLAGS
	COVERAGE_OPTFLAGS   : $COVERAGE_OPTFLAGS
	COVERAGE_LDFLAGS    : $COVERAGE_LDFLAGS
	COVERAGE_LIBS       : $COVERAGE_LIBS
	lcov                : $LCOV
")
