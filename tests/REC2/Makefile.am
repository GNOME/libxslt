## Process this file with automake to produce Makefile.in

$(top_builddir)/libxslt/xsltproc:
	@(cd ../../libxslt ; make xsltproc)

EXTRA_DIST = data.xml vrml.xsl vrml.xml svg.xsl svg.xml	html.xsl html.xml

all: test

test tests: $(top_builddir)/libxslt/xsltproc
	@(echo > .memdump)
	@($(top_builddir)/libxslt/xsltproc $(srcdir)/vrml.xsl $(srcdir)/data.xml > vrml.res ; \
	diff $(srcdir)/vrml.xml vrml.res ; \
	grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true;\
	rm -f vrml.res)
	@($(top_builddir)/libxslt/xsltproc $(srcdir)/svg.xsl $(srcdir)/data.xml > svg.res ; \
	diff $(srcdir)/svg.xml svg.res ; \
	grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true;\
	rm -f svg.res)
	@($(top_builddir)/libxslt/xsltproc $(srcdir)/html.xsl $(srcdir)/data.xml > html.res ; \
	diff $(srcdir)/html.xml html.res ; \
	grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true;\
	rm -f html.res)

