lib_LTLIBRARIES = libmpack.la
libmpack_la_SOURCES = \
	mpack-common.c\
	mpack-expect.c\
	mpack-node.c\
	mpack-platform.c\
	mpack-reader.c\
	mpack-writer.c
libmpack_la_CFLAGS = $(COVERAGE_CFLAGS) -I$(top_srcdir)/include -std=c99
libmpack_la_LDFLAGS = -shared -no-undefined

AUTOMAKE_OPTIONS = subdir-objects
