EXAMPLE_DIR = $(datadir)/doc/libxslt-python-$(LIBXSLT_VERSION)/examples

TESTSPY=	\
    basic.py	\
    exslt.py	\
    extelem.py	\
    extfunc.py

XMLS=		\
    test.xml	\
    test.xsl

EXTRAS=		\
    pyxsltproc.py

EXTRA_DIST = $(TESTSPY) $(XMLS) $(EXTRAS)

if WITH_PYTHON
tests: $(TESTSPY)
	-@(PYTHONPATH="..:../.libs:$(srcdir)/.." ; export PYTHONPATH; \
	   for test in $(TESTSPY) ; do echo "-- $$test" ; $(PYTHON) $(srcdir)/$$test ; done)
else
tests:
endif

clean:
	rm -f *.pyc core

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(EXAMPLE_DIR)
	-(for test in $(TESTSPY) $(XMLS) $(EXTRAS); \
	  do @INSTALL@ -m 0644 $$test $(DESTDIR)$(EXAMPLE_DIR) ; done)

