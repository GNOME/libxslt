## Process this file with automake to produce Makefile.in

$(top_builddir)/xsltproc/xsltproc:
	@(cd ../../xsltproc ; $(MAKE) xsltproc)

EXTRA_DIST = \
    module.xml module.xsl module.out \
    list.xml list.xsl list.out


all:

valgrind:
	@echo '## Running the regression tests under Valgrind'
	$(MAKE) CHECKER='valgrind -q' tests

test tests: $(top_builddir)/xsltproc/xsltproc
	@(echo > .memdump)
	@(for i in $(srcdir)/*.xml ; do \
	  if [ -d $$i ] ; then continue ; fi ; \
	  doc=`basename $$i .xml` ; \
	  for j in $(srcdir)/$$doc*.xsl ; do \
	  if [ ! -f $$j ] ; then continue ; fi ; \
	  if [ -d $$j ] ; then continue ; fi ; \
	  name=`basename $$j .xsl`; \
	  out=$(srcdir)/"$$name".out; \
	  echo Running $$j on $$i ; \
	  $(CHECKER) $(top_builddir)/xsltproc/xsltproc $$j $$i > result.$$name;\
	  if [ ! -f $$out ] ; then cp result.$$name $$out ; \
	  else diff $$out result.$$name; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true;\
	  rm -f result.$$name ; \
	  done ; done)

