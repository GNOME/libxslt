## Process this file with automake to produce Makefile.in

$(top_builddir)/libxslt/xsltproc:
	@(cd ../../libxslt ; make xsltproc)

EXTRA_DIST = format-number.xsl format-number.xml format-number.out

all: test

test tests: $(top_builddir)/libxslt/xsltproc
	@(echo > .memdump)
	@($(top_builddir)/libxslt/xsltproc $(srcdir)/format-number.xsl $(srcdir)/format-number.xml > format-number.res ; \
	diff $(srcdir)/format-number.out format-number.res ; \
	grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0";\
	rm -f format-number.res)

