## Process this file with automake to produce Makefile.in

$(top_builddir)/xsltproc/xsltproc:
	@(cd ../../xsltproc ; $(MAKE) xsltproc)

EXTRA_DIST = \
	tst-1.xml tst-1.xsl tst-1.out tst-1.err \
	tst-2.xml tst-2.xsl tst-2.out tst-2.err \
	undefvar.xml undefvar.xsl undefvar.out undefvar.err \
	cmdlineparams.xml cmdlineparams.xsl cmdlineparams.out \
	cmdlineparams.err \
	recglobparam.xsl recglobvar.xsl reclocparam.xsl reclocvar.xsl \
	recglobparam.xml recglobvar.xml reclocparam.xml reclocvar.xml 


all:

valgrind:
	@echo '## Running the regression tests under Valgrind'
	$(MAKE) CHECKER='valgrind -q' tests

test tests: $(top_builddir)/xsltproc/xsltproc
	@(echo > .memdump)
	@(for i in $(srcdir)/*.xml ; do \
	  if [ -d $$i ] ; then continue ; fi ; \
	  doc=`basename $$i .xml` ; \
	  for j in $(srcdir)/$$doc.xsl ; do \
	  if [ ! -f $$j ] ; then continue ; fi ; \
	  if [ -d $$j ] ; then continue ; fi ; \
	  name=`basename $$j .xsl`; \
	  out=$(srcdir)/"$$name".out; \
	  err=$(srcdir)/"$$name".err; \
	  echo Running $$j on $$i ; \
	  $(CHECKER) $(top_builddir)/xsltproc/xsltproc \
	              --stringparam test passed_value \
		      --stringparam test2 passed_value2 \
	              $$j $$i > result.$$name \
	              2> err.$$name ; \
	  if [ ! -f $$out ] ; \
	  then cp result.$$name $$out; cp err.$$name $$err ; \
	  else diff $$out result.$$name; diff $$err err.$$name; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true;\
	  rm -f result.$$name err.$$name ; \
	  done ; done)

