# ===========================================================================
#      https://www.gnu.org/software/autoconf-archive/ax_append_flag.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_CHECK_STDC
#
# DESCRIPTION
#
#   Checks first for gnu11 then c11 then c99 and aborts if c99 not supported
#
# LICENSE
#
#   Copyright (c) 2019 Martin Schröder <mkschreder.uk@gmail.com>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved.  This file is offered as-is, without any
#   warranty.

#serial 8
AC_DEFUN([AX_CHECK_STDC],
[AX_CHECK_COMPILE_FLAG([-std=gnu11],
	[AX_APPEND_FLAG([-std=gnu11])],
	[AX_CHECK_COMPILE_FLAG([-std=c11],
		[AX_APPEND_FLAG([-std=c11])],
		[AX_CHECK_COMPILE_FLAG([-std=c99],
			[AX_APPEND_FLAG([-std=c99])],
			[AC_MSG_ERROR([C compiled does not support at least C99!])])
		])
	])
])
