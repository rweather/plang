
pkglib_LTLIBRARIES = libplang_wordnet.la

libplang_wordnet_la_SOURCES = \
	words.c

importsdir = $(datadir)/plang/imports
imports_DATA = words.lp

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include -I. -I$(srcdir)

libplang_wordnet_la_LDFLAGS = -version-info $(PLANG_VERSION) -no-undefined $(top_builddir)/src/libplang/libplang.la $(WORDS_LIBS) -module

CLEANFILES = *.gcov *.gcda *.gcno

EXTRA_DIST = $(imports_DATA)
