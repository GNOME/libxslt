## Process this file with automake to produce Makefile.in

$(top_builddir)/libxslt/xsltproc:
	@(cd ../../libxslt ; make xsltproc)

EXTRA_DIST = bredfort.css index.xml system.xml \
             bredfort.xsl menu.xml worklog.xml \
	     result.xhtml


all: test

test tests: $(top_builddir)/libxslt/xsltproc
	@(rm -f .memdump ; touch .memdump)
	@($(top_builddir)/libxslt/xsltproc bredfort.xsl index.xml > result ; \
	diff result.xhtml result ; \
	grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0";\
	rm -f result)

