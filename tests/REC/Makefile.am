## Process this file with automake to produce Makefile.in

$(top_builddir)/libxslt/xsltproc:
	@(cd ../../libxslt ; make xsltproc)

EXTRA_DIST = 						\
    article.xsl bigfont.xsl gmon.out			\
    test-2.3-1.out test-2.3-1.xml test-2.3-1.xsl	\
    test-2.3-2.out test-2.3-2.xml test-2.3-2.xsl	\
    test-2.5-1.out test-2.5-1.xml test-2.5-1.xsl	\
    test-2.6.2-1.out test-2.6.2-1.xml test-2.6.2-1.xsl	\
    test-5.2-1.out test-5.2-1.xml test-5.2-1.xsl	\
    test-5.2-2.out test-5.2-2.xml test-5.2-2.xsl	\
    test-5.2-3.out test-5.2-3.xml test-5.2-3.xsl	\
    test-5.2-4.out test-5.2-4.xml test-5.2-4.xsl	\
    test-5.2-5.out test-5.2-5.xml test-5.2-5.xsl	\
    test-5.2-6.out test-5.2-6.xml test-5.2-6.xsl	\
    test-5.2-7.out test-5.2-7.xml test-5.2-7.xsl	\
    test-5.2-8.out test-5.2-8.xml test-5.2-8.xsl	\
    test-5.2-9.out test-5.2-9.xml test-5.2-9.xsl	\
    test-5.2-10.out test-5.2-10.xml test-5.2-10.xsl	\
    test-5.2-11.out test-5.2-11.xml test-5.2-11.xsl	\
    test-5.2-12.out test-5.2-12.xml test-5.2-12.xsl	\
    test-5.2-13.out test-5.2-13.xml test-5.2-13.xsl	\
    test-5.2-14.out test-5.2-14.xml test-5.2-14.xsl	\
    test-5.2-15.out test-5.2-15.xml test-5.2-15.xsl	\
    test-5.2-16.out test-5.2-16.xml test-5.2-16.xsl	\
    test-5.2-17.out test-5.2-17.xml test-5.2-17.xsl	\
    test-5.2-18.out test-5.2-18.xml test-5.2-18.xsl	\
    test-5.3.out test-5.3.xml test-5.3.xsl		\
    test-5.4-1.out test-5.4-1.xml test-5.4-1.xsl	\
    test-5.4-2.out test-5.4-2.xml test-5.4-2.xsl	\
    test-5.4-3.out test-5.4-3.xml test-5.4-3.xsl	\
    test-5.4-4.out test-5.4-4.xml test-5.4-4.xsl	\
    test-5.4-5.out test-5.4-5.xml test-5.4-5.xsl	\
    test-5.8.out test-5.8.xml test-5.8.xsl		\
    test-6.out test-6.xml test-6.xsl			\
    test-7.1.1.out test-7.1.1.xml test-7.1.1.xsl	\
    test-7.1.3.out test-7.1.3.xml test-7.1.3.xsl	\
    test-7.1.4.out test-7.1.4.xml test-7.1.4.xsl	\
    test-7.3.out test-7.3.xml test-7.3.xsl		\
    test-7.4.out test-7.4.xml test-7.4.xsl		\
    test-7.5-1.out test-7.5-1.xml test-7.5-1.xsl	\
    test-7.6.1-1.out test-7.6.1-1.xml test-7.6.1-1.xsl	\
    test-7.6.1-2.out test-7.6.1-2.xml test-7.6.1-2.xsl	\
    test-7.6.1-3.out test-7.6.1-3.xml test-7.6.1-3.xsl	\
    test-7.6.2-1.out test-7.6.2-1.xml test-7.6.2-1.xsl	\
    test-7.7-1.out test-7.7-1.xml test-7.7-1.xsl        \
    test-7.7-2.out test-7.7-2.xml test-7.7-2.xsl        \
    test-7.7-3.out test-7.7-3.xml test-7.7-3.xsl        \
    test-7.7-4.out test-7.7-4.xml test-7.7-4.xsl        \
    test-7.7-5.out test-7.7-5.xml test-7.7-5.xsl        \
    test-8-1.out test-8-1.xml test-8-1.xsl		\
    test-9.1-1.out test-9.1-1.xml test-9.1-1.xsl	\
    test-9.1-2.out test-9.1-2.xml test-9.1-2.xsl	\
    test-9.2-1.xsl 					\
    test-10-1.out test-10-1.xml test-10-1.xsl		\
    test-11.2-1.out test-11.2-1.xml test-11.2-1.xsl	\
    test-11.2-2.out test-11.2-2.xml test-11.2-2.xsl	\
    test-11.2-3.out test-11.2-3.xml test-11.2-3.xsl	\
    test-11.2-4.out test-11.2-4.xml test-11.2-4.xsl	\
    test-11.2-5.out test-11.2-5.xml test-11.2-5.xsl	\
    test-11.2-6.out test-11.2-6.xml test-11.2-6.xsl	\
    test-12.2-1.out test-12.2-1.xml test-12.2-1.xsl	\
    test-12.2-2.out test-12.2-2.xml test-12.2-2.xsl

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

