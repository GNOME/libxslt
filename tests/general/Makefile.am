## Process this file with automake to produce Makefile.in

$(top_builddir)/xsltproc/xsltproc:
	@(cd ../../xsltproc ; $(MAKE) xsltproc)

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
    bug-36-.out bug-36-.xsl \
    bug-36-inc.out bug-36-inc.xsl \
    bug-37-.out bug-37-.xsl \
    bug-37-inc.out bug-37-inc.xsl \
    array.out array.xsl \
    bug-38-.out bug-38-.xsl \
    bug-39-.out bug-39-.xsl \
    bug-40-.out bug-40-.xsl \
    bug-41-.out bug-41-.xsl \
    bug-42-.out bug-42-.xsl \
    bug-43-.out bug-43-.xsl \
    bug-44-.out bug-44-.xsl \
    bug-45-.out bug-45-.xsl \
    bug-46-.out bug-46-.xsl \
    bug-47-.out bug-47-.xsl \
    bug-48-.out bug-48-.xsl \
    bug-49-.out bug-49-.xsl \
    bug-50-.out bug-50-.xsl \
    bug-52.out bug-52.xsl \
    bug-53.out bug-53.xsl \
    bug-54.out bug-54.xsl \
    bug-55.out bug-55.xsl \
    bug-56.out bug-56.xsl \
    bug-57.out bug-57.xsl \
    bug-59.out bug-59.xsl \
    bug-60.out bug-60.xsl \
    bug-61.out bug-61.xsl \
    bug-62-inc.out bug-62-inc.xsl \
    bug-62.out bug-62.xsl \
    bug-63.out bug-63.xsl \
    bug-64.out bug-64.xsl \
    bug-65.ent bug-65.out bug-65.xsl \
    bug-66.out bug-66.xsl \
    bug-68.out bug-68.xsl \
    bug-69.out bug-69.xsl \
    bug-70.out bug-70.xsl \
    bug-71.out bug-71.xsl \
    bug-72.out bug-72.xsl \
    bug-73.out bug-73.xsl \
    bug-74.out bug-74.xsl \
    bug-75.out bug-75.xsl \
    bug-76.out bug-76.xsl \
    character.out character.xsl \
    character2.out character2.xsl \
    itemschoose.out itemschoose.xsl \
    inner.xsl


all:

test tests: $(top_builddir)/xsltproc/xsltproc
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
	  $(top_builddir)/xsltproc/xsltproc $$j $$i > result.$$name;\
	  if [ ! -f $$out ] ; then cp result.$$name $$out ; \
	  else diff $$out result.$$name; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true;\
	  rm -f result.$$name ; \
	  done ; done)

