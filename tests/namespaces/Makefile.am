## Process this file with automake to produce Makefile.in

$(top_builddir)/libxslt/xsltproc:
	@(cd ../../libxslt ; make xsltproc)

EXTRA_DIST = \
    tst.xml tst.xsl tst.out \
    tst2.xml tst2.xsl tst2.out \
    tst3.xml tst3.xsl tst3.out \
    extra.out extra.xml extra.xsl \
    extra2.out extra2.xml extra2.xsl

all: test

test tests: $(top_builddir)/libxslt/xsltproc
	@(echo > .memdump)
	@(for i in $(srcdir)/*.xml ; do \
	  doc="`basename $$i .xml`" ; \
	  j=$(srcdir)/"$$doc".xsl; \
	  if [ ! -f $$j ] ; then continue ; fi ; \
	  if [ -d $$j ] ; then continue ; fi ; \
	  name="`basename $$j .xsl`"; \
	  out=$(srcdir)/"$$name".out; \
	  echo $$j ; \
	  $(top_builddir)/libxslt/xsltproc $$j $$i > result.$$name;\
	  if [ ! -f $$out ] ; then cp result.$$name $$out ; \
	  else diff $$out result.$$name ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true;\
	  rm -f result.$$name ; \
	  done)

