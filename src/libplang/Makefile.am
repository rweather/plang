lib_LTLIBRARIES = libplang.la

libplang_la_SOURCES = \
	arith.c \
	builtins.c \
	compiler.c \
	context.c \
	context-priv.h \
	dcg.c \
	database.c \
	database-priv.h \
	disassembler.c \
	errors.c \
	fuzzy.c \
	inst-priv.h \
	interpreter.c \
	io.c \
	lexer.l \
	parser.y \
	parser-priv.h \
	rbtree.c \
	rbtree-priv.h \
	sort.c \
	term.c \
	term-priv.h

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include -I. -I$(srcdir) -DP_SYSTEM_IMPORT_PATH=\"$(datadir)/plang/imports\" -DP_SYSTEM_LIB_PATH=\"$(pkglibdir)\"
AM_YFLAGS = -d

$(srcdir)/lexer.l: parser.c

libplang_la_LDFLAGS = -version-info $(PLANG_VERSION) -no-undefined -lm

CLEANFILES = *.gcov *.gcda *.gcno \
	parser.h parser.c parser.output \
	parser.tab.h parser.tab.c lexer.c
