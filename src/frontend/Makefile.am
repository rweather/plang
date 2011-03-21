bin_PROGRAMS = plang

plang_SOURCES = main.c
plang_LDADD   = $(top_builddir)/src/libplang/libplang.la

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include -I. -I$(srcdir) \
	    -I$(top_srcdir)/tests/common -I$(top_builddir)/tests/common

CLEANFILES = *.gcov *.gcda *.gcno
