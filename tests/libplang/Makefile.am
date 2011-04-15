check_PROGRAMS = \
	test-builtins \
	test-compiler \
	test-database \
	test-rbtree \
	test-term

test_builtins_SOURCES = test-builtins.c testcase.h
test_builtins_LDADD   = $(top_builddir)/src/libplang/libplang.la

test_compiler_SOURCES = test-compiler.c testcase.h
test_compiler_LDADD   = $(top_builddir)/src/libplang/libplang.la

test_database_SOURCES = test-database.c testcase.h
test_database_LDADD   = $(top_builddir)/src/libplang/libplang.la

test_rbtree_SOURCES = test-rbtree.c testcase.h
test_rbtree_LDADD   = $(top_builddir)/src/libplang/libplang.la

test_term_SOURCES = test-term.c testcase.h
test_term_LDADD   = $(top_builddir)/src/libplang/libplang.la

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include -I. -I$(srcdir) -I$(top_srcdir)/src/libplang -I$(top_builddir)/src/libplang

TESTS = $(check_PROGRAMS)
#TESTS_ENVIRONMENT = P_REPORT_ONLY_FAILURES=1

CLEANFILES = *.gcov *.gcda *.gcno
