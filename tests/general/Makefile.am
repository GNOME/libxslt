## Process this file with automake to produce Makefile.in

$(top_builddir)/libxslt/xsltproc:
	@(cd ../../libxslt ; make xsltproc)

EXTRA_DIST = \
    itemschoose.out  itemschoose.xsl


all: test

test tests: $(top_builddir)/libxslt/xsltproc
	@(rm -f .memdump ; touch .memdump)
	@(for i in $(srcdir)/../docs/*.xml ; do \
	  if [ -d $$i ] ; then continue ; fi ; \
	  doc=`basename $$i .xml` ; \
	  for j in $(srcdir)/$$doc*.xsl ; do \
	  if [ ! -f $$j ] ; then continue ; fi ; \
	  if [ -d $$j ] ; then continue ; fi ; \
	  name=`basename $$j .xsl`; \
	  out=$(srcdir)/"$$name".out; \
	  echo Running $$j on $$i ; \
	  $(top_builddir)/libxslt/xsltproc $$j $$i > result.$$name;\
	  if [ ! -f $$out ] ; then cp result.$$name $$out ; \
	  else diff $$out result.$$name ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0";\
	  rm -f result.$$name ; \
	  done ; done)

