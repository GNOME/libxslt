## Process this file with automake to produce Makefile.in

$(top_builddir)/libxslt/xsltproc:
	@(cd ../../libxslt ; make xsltproc)

EXTRA_DIST = REC-xml-20001006.xml xmlspec-v21.dtd W3C-REC.css \
             logo-REC xmlspec.xsl REC-xml-2e.xsl diffspec.xsl

all: test

test tests: $(top_builddir)/libxslt/xsltproc
	@(rm -f .memdump ; touch .memdump)
	@($(top_builddir)/libxslt/xsltproc -timing REC-xml-2e.xsl REC-xml-20001006.xml > REC-xml-20001006.out 2> debug ; \
	diff $(srcdir)/REC-xml-20001006.html REC-xml-20001006.out | grep -v 'id[0-9]' | grep -v -- '---' | grep -v 158 | grep -v 4031 ; \
	grep implemented debug | sort | uniq -c ; \
	grep " ms$$" debug ; \
	grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0";\
	rm -f REC-xml-20001006.out)
	@($(top_builddir)/libxslt/xsltproc  -timing --param show.diff.markup 1 REC-xml-2e.xsl REC-xml-20001006.xml > REC-xml-20001006-review.out 2> debug ; \
	diff $(srcdir)/REC-xml-20001006-review.html REC-xml-20001006-review.out | grep -v 'id[0-9]' | grep -v -- '---' | grep -v 158 | grep -v 4031 ; \
	grep implemented debug | sort | uniq -c ; \
	grep " ms$$" debug ; \
	grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0";\
	rm -f REC-xml-20001006-review.out)

