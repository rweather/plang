PLANG_TESTS = \
	test-arith.lp \
	test-assign.lp \
	test-class.lp \
	test-compare.lp \
	test-compose.lp \
	test-dcg.lp \
	test-dynamic.lp \
	test-findall.lp \
	test-fuzzy.lp \
        test-one-way.lp \
	test-sort.lp \
	test-type.lp \
	@WORDS_TESTCASE@

TESTS = $(PLANG_TESTS)
TESTS_ENVIRONMENT = $(top_builddir)/src/frontend/plang -I$(top_srcdir)/src/imports -I$(top_srcdir)/src/words -L$(top_builddir)/src/words/.libs -mtest::main

EXTRA_DIST = $(PLANG_TESTS)
