lib_LTLIBRARIES = libplang.la

libplang_la_SOURCES = \
	context.c \
	context-priv.h \
	database.c \
	database-priv.h \
	term.c \
	term-priv.h

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include -I. -I$(srcdir)

libplang_la_LDFLAGS = -version-info $(PLANG_VERSION) -no-undefined -lm

CLEANFILES = *.gcov *.gcda *.gcno
