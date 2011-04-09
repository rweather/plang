SUBDIRS = include src tests doc

EXTRA_DIST = auto_gen.sh

docs:
	(cd $(srcdir)/doc; $(DOXYGEN))

dist-hook:
	(cd $(distdir)/doc; $(DOXYGEN))
