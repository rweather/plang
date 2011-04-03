
lib_LTLIBRARIES = libplang_wordnet.la

libplang_wordnet_la_SOURCES = \
	words.c

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include -I. -I$(srcdir)

libplang_wordnet_la_LDFLAGS = -version-info $(PLANG_VERSION) -no-undefined $(WORDS_LIBS)

CLEANFILES = *.gcov *.gcda *.gcno
