## Process this file with automake to produce Makefile.in

$(top_builddir)/xsltproc/xsltproc:
	@(cd ../../xsltproc ; $(MAKE) xsltproc)

EXTRA_DIST = \
    tst.xml tst.xsl tst.out \
    tst2.xml tst2.xsl tst2.out \
    tst3.xml tst3.xsl tst3.out \
    extra.out extra.xml extra.xsl \
    extra2.out extra2.xml extra2.xsl

all:

valgrind:
	@echo '## Running the regression tests under Valgrind'
	$(MAKE) CHECKER='valgrind -q' tests

test tests: $(top_builddir)/xsltproc/xsltproc
	@(echo > .memdump)
	@(for i in $(srcdir)/*.xml ; do \
	  doc="`basename $$i .xml`" ; \
	  j=$(srcdir)/"$$doc".xsl; \
	  if [ ! -f $$j ] ; then continue ; fi ; \
	  if [ -d $$j ] ; then continue ; fi ; \
	  name="`basename $$j .xsl`"; \
	  out=$(srcdir)/"$$name".out; \
	  err=$(srcdir)/"$$name".err; \
	  echo $$j ; \
	  $(CHECKER) $(top_builddir)/xsltproc/xsltproc $$j $$i > result.$$name 2>err.$$name;\
	  if [ ! -f $$out ] ; then cp result.$$name $$out ; \
	  else diff $$out result.$$name ; fi ; \
	  if [ ! -f $$err ] ; then cp err.$$name $$err ; \
	  else diff $$err err.$$name ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true;\
	  rm -f result.$$name ; \
	  done)

