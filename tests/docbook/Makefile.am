## Process this file with automake to produce Makefile.in

all: # Avoid doing the test in normal pass

$(top_builddir)/libxslt/xsltproc:
	@(cd ../../libxslt ; make xsltproc)

EXTRA_DIST = README VERSION

tests: html

html: $(top_builddir)/libxslt/xsltproc
	@(for i in test/*.xml ; do \
	  echo $$i ; \
	  out=result/html/`basename $$i .xml`.tst; \
	  html=result/html/`basename $$i .xml`.html; \
	  $(top_builddir)/libxslt/xsltproc html/docbook.xsl $$i > $$out ; \
	  if [ -f $$html ] ; then \
	      grep -v id < $$html > $$html.noid ; \
	      grep -v id < $$out > $$out.noid ; \
	      diff $$html.noid $$out.noid ; \
	      rm -f $$html.noid $$out.noid ; \
	  else mv $$out $$html ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0";\
	  rm -f $$out ; done )

dist-hook:
	@(cd $(srcdir) ; tar -cf - --exclude CVS common html dtd lib result test) | (cd $(distdir); tar xf -)
