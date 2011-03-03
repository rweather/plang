check_PROGRAMS = \
	test-term

test_term_SOURCES = test-term.c
test_term_LDADD   = $(top_builddir)/src/libplang/libplang.la

AM_CFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include -I. -I$(srcdir) \
	    -I$(top_srcdir)/tests/common -I$(top_builddir)/tests/common

TESTS = $(check_PROGRAMS)
#TESTS_ENVIRONMENT = P_REPORT_ONLY_FAILURES=1

CLEANFILES = *.gcov *.gcda *.gcno
