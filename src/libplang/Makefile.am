lib_LTLIBRARIES = libplang.la

libplang_la_SOURCES = \
	arith.c \
	builtins.c \
	context.c \
	context-priv.h \
	database.c \
	database-priv.h \
	errors.c \
	lexer.l \
	parser.y \
	term.c \
	term-priv.h

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include -I. -I$(srcdir)
AM_YFLAGS = -d

lexer.l: parser.c

libplang_la_LDFLAGS = -version-info $(PLANG_VERSION) -no-undefined -lm

CLEANFILES = *.gcov *.gcda *.gcno \
	parser.h parser.c parser.output \
	parser.tab.h parser.tab.c lexer.c
