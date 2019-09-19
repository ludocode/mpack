# SYNOPSIS
#
#   AX_SUPERWARN
#
# DESCRIPTION
#
#   Adds the ultimate list of warnings to improve quality of C code
#
# LICENSE
#
#   Copyright (c) 2019 Martin Schröder <mkschreder.uk@gmail.com>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved.  This file is offered as-is, without any
#   warranty.
AC_DEFUN([AX_SUPERWARN],
	[AX_APPEND_FLAG([\
		-pedantic \
		-Wall\
		-Wextra\
		-Werror\
		-Wno-unused-parameter\
		-Wchar-subscripts\
		-Wno-strict-overflow\
		-Wformat\
		-Wformat-nonliteral\
		-Wformat-security\
		-Wmissing-braces\
		-Wparentheses\
		-Wsequence-point\
		-Wswitch\
		-Wtrigraphs\
		-Wno-unused-function\
		-Wunused-label\
		-Wno-unused-parameter\
		-Wunused-variable\
		-Wunused-value\
		-Wuninitialized\
		-Wdiv-by-zero\
		-Wfloat-equal\
		-Wdouble-promotion\
		-fsingle-precision-constant\
		-Wshadow\
		-Wpointer-arith\
		-Wwrite-strings\
		-Wno-conversion\
		-Wno-redundant-decls\
		-Wunreachable-code\
		-Winline\
		-Wenum-compare \
		-Wlong-long\
		-Wchar-subscripts
	])
])
