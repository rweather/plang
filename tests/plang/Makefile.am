PLANG_TESTS = \
	test-arith.lp \
	test-assign.lp \
	test-class.lp \
	test-compare.lp \
	test-compose.lp \
	test-dcg.lp \
	test-findall.lp \
	test-type.lp \
	@WORDS_TESTCASE@

TESTS = $(PLANG_TESTS)
TESTS_ENVIRONMENT = $(top_builddir)/src/frontend/plang -I$(top_srcdir)/src/imports -I$(top_srcdir)/src/words -L$(top_builddir)/src/words/.libs

EXTRA_DIST = $(PLANG_TESTS) testcase.lp
