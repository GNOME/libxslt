## Process this file with automake to produce Makefile.in

$(top_builddir)/libxslt/xsltproc:
	@(cd ../../libxslt ; make xsltproc)

EXTRA_DIST = REC-xml-20001006.xml xmlspec-v21.dtd W3C-REC.css \
             logo-REC xmlspec.xsl

all: test

test tests: $(top_builddir)/libxslt/xsltproc
	@(rm -f .memdump ; touch .memdump)
	@($(top_builddir)/libxslt/xsltproc -timing -v xmlspec.xsl REC-xml-20001006.xml > REC-xml-20001006.html 2> debug ; \
	grep implemented debug | sort | uniq -c ; \
	grep "ms$$" debug ; \
	grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0";\
	rm -f doc.res)

