## Process this file with automake to produce Makefile.in

$(top_builddir)/libxslt/xsltproc:
	@(cd ../../libxslt ; make xsltproc)

EXTRA_DIST = data.xml vrml.xsl vrml.xml svg.xsl svg.xml	html.xsl html.xml

all: test

test tests: $(top_builddir)/libxslt/xsltproc
	@(rm -f .memdump ; touch .memdump)
	@($(top_builddir)/libxslt/xsltproc vrml.xsl data.xml > vrml.res ; \
	diff vrml.xml vrml.res ; \
	grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0";\
	rm -f vrml.res)
	@($(top_builddir)/libxslt/xsltproc svg.xsl data.xml > svg.res ; \
	diff svg.xml svg.res ; \
	grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0";\
	rm -f svg.res)
	@($(top_builddir)/libxslt/xsltproc html.xsl data.xml > html.res ; \
	diff html.xml html.res ; \
	grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0";\
	rm -f html.res)

