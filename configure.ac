dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.61)

AC_INIT([plang], [0.0.1], [rhys.weatherley@gmail.com])
AC_CONFIG_SRCDIR([include/plang/term.h])
AC_CONFIG_HEADERS([config.h])

dnl Determine the host system type.
AC_CANONICAL_HOST

dnl Initialize automake.
AM_INIT_AUTOMAKE([-Wall dist-bzip2])

dnl Set the version number for the shared libraries.
AC_SUBST(PLANG_VERSION)
PLANG_VERSION=0:1:0

dnl Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
AC_PROG_YACC
AC_PROG_LEX
AC_PATH_PROG(DOXYGEN, doxygen)

dnl Check for file extensions.
AC_EXEEXT
AC_OBJEXT

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(unistd.h sys/types.h sys/stat.h fcntl.h)
AC_CHECK_HEADERS(sys/mman.h limits.h wchar.h unistd.h dlfcn.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

dnl Check to see if we are using gcc or not.
if test x$GCC = xyes ; then
	CFLAGS="$CFLAGS -Wall"
fi
if test x$GXX = xyes ; then
	CXXFLAGS="$CXXFLAGS -Wall"
fi

dnl Check for building on a multi os system
if test x$GCC = xyes ; then
	multi_os_directory=`$CC -print-multi-os-directory`
	case $multi_os_directory in
	  .) ;; # Avoid trailing /.
	  *) libdir=$libdir/$multi_os_directory ;;
	esac
fi
AC_SUBST(libdir)

dnl Checks for library functions.
AC_CHECK_LIB(m, sin)
AC_CHECK_LIB(dl, dlopen)
AC_FUNC_MEMCMP
AC_CHECK_FUNCS(vsnprintf _vsnprintf strerror strerror_r fstat mmap munmap)
AC_CHECK_FUNCS(fork execvp fmod remainder drem feclearexcept fetestexcept)
AC_CHECK_FUNCS(dlopen)
AC_FUNC_ALLOCA

AC_CHECK_LIB(gc, GC_gcollect)
AC_CHECK_HEADERS(gc.h gc/gc.h fenv.h)

dnl Checks for the presence of the WordNet library.
dnl Only do this if shared libraries are enabled.
AC_SUBST(WORDS_LIBS)
AC_SUBST(WORDS_MODULE)
AC_SUBST(WORDS_TESTCASE)
if test "x$enable_shared" = "xyes" ; then
    AC_CHECK_LIB(WN, in_wn, [WORDS_LIBS=-lWN
                             WORDS_MODULE=words
                             WORDS_TESTCASE=test-words.lp],
        [AC_CHECK_LIB(wordnet, in_wn, [WORDS_LIBS=-lwordnet
                                       WORDS_MODULE=words
                                       WORDS_TESTCASE=test-words.lp])])
fi

AC_CONFIG_FILES([
  Makefile
  doc/Makefile
  include/Makefile
  include/plang/Makefile
  src/Makefile
  src/libplang/Makefile
  src/frontend/Makefile
  src/imports/Makefile
  src/words/Makefile
  tests/Makefile
  tests/libplang/Makefile
  tests/plang/Makefile])
AC_OUTPUT
