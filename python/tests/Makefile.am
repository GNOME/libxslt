if WITH_PYTHON
TESTSPY =	\
    basic.py	\
    exslt.py	\
    extelem.py	\
    extfunc.py

exampledir = $(docdir)/python/examples
dist_example_DATA = test.xml test.xsl pyxsltproc.py $(TESTSPY)

tests: $(TESTSPY)
	@(PYTHONPATH="`pwd`/..:`pwd`/../.libs:$(srcdir)/.."; \
	  if test "x$(LIBXML_SRC)" != "x" ; then \
            PYTHONPATH="$$PYTHONPATH:$(LIBXML_SRC)/python:$(LIBXML_SRC)/python/.libs"; \
          fi; \
	  export PYTHONPATH; \
	  for test in $(TESTSPY) ; do \
	    echo "-- $$test" ; \
	    (cd $(srcdir) && $(PYTHON) $$test ) || exit 1; \
	  done)
else
tests:
endif

CLEANFILES = *.pyc core
