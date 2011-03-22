check_PROGRAMS = \
	test-builtins \
	test-database \
	test-term

test_builtins_SOURCES = test-builtins.c testcase.h
test_builtins_LDADD   = $(top_builddir)/src/libplang/libplang.la

test_database_SOURCES = test-database.c testcase.h
test_database_LDADD   = $(top_builddir)/src/libplang/libplang.la

test_term_SOURCES = test-term.c testcase.h
test_term_LDADD   = $(top_builddir)/src/libplang/libplang.la

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include -I. -I$(srcdir)

TESTS = $(check_PROGRAMS)
#TESTS_ENVIRONMENT = P_REPORT_ONLY_FAILURES=1

CLEANFILES = *.gcov *.gcda *.gcno
