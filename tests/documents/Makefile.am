## Process this file with automake to produce Makefile.in

$(top_builddir)/libxslt/xsltproc:
	@(cd ../../libxslt ; make xsltproc)

EXTRA_DIST = bredfort.css index.xml system.xml \
             bredfort.xsl menu.xml worklog.xml \
	     result.xhtml


all: test

test tests: $(top_builddir)/libxslt/xsltproc
	@(echo > .memdump)
	@($(top_builddir)/libxslt/xsltproc $(srcdir)/bredfort.xsl $(srcdir)/index.xml > result ; \
	diff $(srcdir)/result.xhtml result; \
	grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true;\
	rm -f result)

