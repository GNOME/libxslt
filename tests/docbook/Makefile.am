## Process this file with automake to produce Makefile.in

SUBDIRS = common html dtd lib result test

all: # Avoid doing the test in normal pass

$(top_builddir)/libxslt/xsltproc:
	@(cd ../../libxslt ; make xsltproc)

EXTRA_DIST = README VERSION

tests: html

html: ../../libxslt/xsltproc
	@(for i in test/*.xml ; do \
	  echo $$i ; \
	  out=result/html/`basename $$i .xml`.tst; \
	  html=result/html/`basename $$i .xml`.html; \
	  ../../libxslt/xsltproc html/docbook.xsl $$i > $$out ; \
	  if [ -f $$html ] ; then diff $$html $$out ; \
	  else mv $$out $$html ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0";\
	  rm -f $$out ; done )
