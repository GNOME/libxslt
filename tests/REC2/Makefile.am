## Process this file with automake to produce Makefile.in

$(top_builddir)/libxslt/xsltproc:
	@(cd ../../libxslt ; make xsltproc)

test: $(top_builddir)/libxslt/xsltproc
	@(rm -f .memdump ; touch .memdump)
	@($(top_builddir)/libxslt/xsltproc vrml.xsl data.xml > vrml.res ; \
	diff vrml.xml vrml.res ; \
	grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0";\
	rm -f doc.res)


