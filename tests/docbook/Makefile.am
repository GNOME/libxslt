## Process this file with automake to produce Makefile.in

all: # Avoid doing the test in normal pass

$(top_builddir)/libxslt/xsltproc:
	@(cd ../../libxslt ; make xsltproc)

EXTRA_DIST = README VERSION

tests: html

html: $(top_builddir)/libxslt/xsltproc
	@(echo > .memdump)
	@(for i in $(srcdir)/test/*.xml ; do \
	  echo $$i ; \
	  out=$(srcdir)/result/html/`basename $$i .xml`.tst; \
	  html=$(srcdir)/result/html/`basename $$i .xml`.html; \
	  $(top_builddir)/libxslt/xsltproc --nonet $(srcdir)/html/docbook.xsl $$i > $$out ; \
	  if [ -f $$html ] ; then \
	      grep -v id < $$html > $$html.noid ; \
	      grep -v id < $$out > $$out.noid ; \
	      diff $$html.noid $$out.noid ; \
	      rm -f $$html.noid $$out.noid ; \
	  else mv $$out $$html ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true;\
	  rm -f $$out ; done )

dist-hook:
	@(cd $(srcdir) ; tar -cf - --exclude CVS common html dtd lib result test) | (cd $(distdir); tar xf -)
