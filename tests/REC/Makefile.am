## Process this file with automake to produce Makefile.in

$(top_builddir)/libxslt/xsltproc:
	@(cd ../../libxslt ; make xsltproc)

EXTRA_DIST = doc.xsl doc.xml result.xml

all: test

test tests: $(top_builddir)/libxslt/xsltproc
	@(rm -f .memdump ; touch .memdump)
	@(for i in *.xsl ; do \
	  name=`basename $$i .xsl` ; \
	  if [ ! -f $$name.xml ] ; then continue ; fi ; \
	  echo $$name.xml ; \
	  $(top_builddir)/libxslt/xsltproc $$name.xsl $$name.xml > $$name.res;\
	  if [ ! -f $$name.out ] ; then cp $$name.res $$name.out ; \
	  else diff $$name.out $$name.res ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0";\
	  rm -f $$name.res ; \
	  done)

