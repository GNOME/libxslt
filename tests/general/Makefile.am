## Process this file with automake to produce Makefile.in

$(top_builddir)/libxslt/xsltproc:
	@(cd ../../libxslt ; make xsltproc)

EXTRA_DIST = \
    bug-1-.out bug-1-.xsl \
    bug-2-.out bug-2-.xsl \
    bug-3-.out bug-3-.xsl \
    bug-4-.out bug-4-.xsl \
    bug-5-.out bug-5-.xsl \
    bug-6-.out bug-6-.xsl \
    bug-7-.out bug-7-.xsl \
    bug-8-.out bug-8-.xsl \
    bug-9-.out bug-9-.xsl \
    bug-10-.out bug-10-.xsl \
    bug-11-.out bug-11-.xsl \
    bug-12-.out bug-12-.xsl \
    bug-13-.out bug-13-.xsl \
    bug-14-.out bug-14-.xsl \
    bug-15-.out bug-15-.xsl \
    bug-16-.out bug-16-.xsl \
    bug-17-.out bug-17-.xsl \
    bug-18-.out bug-18-.xsl \
    bug-19-.out bug-19-.xsl \
    bug-20-.out bug-20-.xsl \
    bug-21-.out bug-21-.xsl \
    bug-22-.out bug-22-.xsl \
    bug-23-.out bug-23-.xsl \
    bug-24-.out bug-24-.xsl \
    bug-25-.out bug-25-.xsl \
    bug-26-.out bug-26-.xsl \
    bug-27-.out bug-27-.xsl \
    bug-28-.out bug-28-.xsl \
    bug-29-.out bug-29-.xsl \
    bug-30-.out bug-30-.xsl \
    bug-31-.out bug-31-.xsl \
    bug-32-.out bug-32-.xsl \
    bug-33-.out bug-33-.xsl \
    bug-35-.out bug-35-.xsl \
    character.out character.xsl \
    character2.out character2.xsl \
    itemschoose.out itemschoose.xsl \
    inner.xsl


all: test

test tests: $(top_builddir)/libxslt/xsltproc
	@(echo > .memdump)
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
	  else diff $$out result.$$name; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true;\
	  rm -f result.$$name ; \
	  done ; done)

