## Process this file with automake to produce Makefile.in

$(top_builddir)/xsltproc/xsltproc:
	@(cd ../../xsltproc ; $(MAKE) xsltproc)

EXTRA_DIST = bredfort.css index.xml system.xml \
             bredfort.xsl menu.xml worklog.xml \
	     result.xhtml docfile.xml doc_file.xml \
	     test.xml test.xsl test.result \
	     message.xml message.xsl message.result


all:

test tests: $(top_builddir)/xsltproc/xsltproc
	@(echo > .memdump)
	@($(top_builddir)/xsltproc/xsltproc $(srcdir)/bredfort.xsl $(srcdir)/index.xml > result ; \
	diff $(srcdir)/result.xhtml result; \
	grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true;\
	rm -f result)
	@($(top_builddir)/xsltproc/xsltproc $(srcdir)/test.xsl $(srcdir)/test.xml > result ; \
	diff $(srcdir)/test.result result; \
	grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true;\
	rm -f result)
	@($(top_builddir)/xsltproc/xsltproc $(srcdir)/message.xsl $(srcdir)/message.xml > result 2>&1 ; \
	diff $(srcdir)/message.result result; \
	grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true;\
	rm -f result)

