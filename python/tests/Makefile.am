if WITH_PYTHON
TESTSPY =	\
    basic.py	\
    exslt.py	\
    extelem.py	\
    extfunc.py

exampledir = $(docdir)/python/examples
dist_example_DATA = test.xml test.xsl pyxsltproc.py setup_test.py $(TESTSPY)

check-local: $(TESTSPY)
	@(export PYTHONPATH="..:../.libs:$(srcdir)/..:$$PYTHONPATH"; \
	  export LD_LIBRARY_PATH="$(top_builddir)/libxslt/.libs:$(top_builddir)/libexslt/.libs:$$LD_LIBRARY_PATH"; \
	  export DYLD_LIBRARY_PATH="$(top_builddir)/libxslt/.libs:$(top_builddir)/libexslt/.libs:$$DYLD_LIBRARY_PATH"; \
	  export PATH="$(top_builddir)/libxslt/.libs:$(top_builddir)/libexslt/.libs:$$PATH" ; \
	  if test "x$(LIBXML_SRC)" != "x" ; then \
            PYTHONPATH="$(LIBXML_SRC)/python:$(LIBXML_SRC)/python/.libs:$$PYTHONPATH"; \
	    LD_LIBRARY_PATH="$(LIBXML_SRC)/.libs:$$LD_LIBRARY_PATH"; \
	    DYLD_LIBRARY_PATH="$(LIBXML_SRC)/.libs:$$DYLD_LIBRARY_PATH"; \
	    PATH="$(LIBXML_SRC)/.libs:$$PATH" ; \
          fi; \
	  for test in $(TESTSPY) ; do \
	    echo "-- $$test" ; \
	    LIBXML_SRC="$(LIBXML_SRC)" $(PYTHON) $(srcdir)/$$test; \
	  done)
endif

CLEANFILES = *.pyc core
