## Process this file with automake to produce Makefile.in

$(top_builddir)/libxslt/xsltproc:
	@(cd ../../libxslt ; make xsltproc)

EXTRA_DIST = doc.xsl doc.xml doc.dtd result.xml

all: test

test tests: $(top_builddir)/libxslt/xsltproc
	@(echo > .memdump)
	@($(top_builddir)/libxslt/xsltproc $(srcdir)/doc.xsl $(srcdir)/doc.xml > doc.res ; \
	diff $(srcdir)/result.xml doc.res ; \
	grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0";\
	rm -f doc.res)

