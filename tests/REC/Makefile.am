## Process this file with automake to produce Makefile.in

$(top_builddir)/libxslt/xsltproc:
	@(cd ../../libxslt ; make xsltproc)

EXTRA_DIST = 							\
    test-10-1.out test-10-1.xml test-10-1.xsl			\
    test-11.2-1.out test-11.2-1.xml test-11.2-1.xsl		\
    test-11.2-2.out test-11.2-2.xml test-11.2-2.xsl		\
    test-11.2-3.out test-11.2-3.xml test-11.2-3.xsl		\
    test-11.2-4.out test-11.2-4.xml test-11.2-4.xsl		\
    test-11.2-5.out test-11.2-5.xml test-11.2-5.xsl		\
    test-11.2-6.out test-11.2-6.xml test-11.2-6.xsl		\
    test-5.3.out test-5.3.xml test-5.3.xsl			\
    test-5.4-1.out test-5.4-1.xml test-5.4-1.xsl		\
    test-5.4-2.out test-5.4-2.xml test-5.4-2.xsl		\
    test-5.4-3.out test-5.4-3.xml test-5.4-3.xsl		\
    test-5.4-4.out test-5.4-4.xml test-5.4-4.xsl		\
    test-5.4-5.out test-5.4-5.xml test-5.4-5.xsl		\
    test-5.8.out test-5.8.xml test-5.8.xsl			\
    test-6.out test-6.xml test-6.xsl				\
    test-7.1.1.out test-7.1.1.xml test-7.1.1.xsl		\
    test-7.1.3.out test-7.1.3.xml test-7.1.3.xsl		\
    test-7.1.4.out test-7.1.4.xml test-7.1.4.xsl		\
    test-7.3.out test-7.3.xml test-7.3.xsl			\
    test-7.4.out test-7.4.xml test-7.4.xsl			\
    test-7.5-1.out test-7.5-1.xml test-7.5-1.xsl		\
    test-7.6.1-1.out test-7.6.1-1.xml test-7.6.1-1.xsl		\
    test-7.6.1-2.out test-7.6.1-2.xml test-7.6.1-2.xsl		\
    test-7.6.1-3.out test-7.6.1-3.xml test-7.6.1-3.xsl		\
    test-7.6.2-1.out test-7.6.2-1.xml test-7.6.2-1.xsl		\
    test-8-1.out test-8-1.xml test-8-1.xsl			\
    test-9.1-1.out test-9.1-1.xml test-9.1-1.xsl		\
    test-9.1-2.out test-9.1-2.xml test-9.1-2.xsl

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

