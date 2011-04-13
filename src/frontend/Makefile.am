bin_PROGRAMS = plang
man_MANS     = plang.1

plang_SOURCES = main.c
plang_LDADD   = $(top_builddir)/src/libplang/libplang.la

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include -I. -I$(srcdir)

CLEANFILES = *.gcov *.gcda *.gcno
