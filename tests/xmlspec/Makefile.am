## Process this file with automake to produce Makefile.in

$(top_builddir)/xsltproc/xsltproc:
	@(cd ../../xsltproc ; $(MAKE) xsltproc)

EXTRA_DIST = REC-xml-20001006.xml xmlspec-v21.dtd W3C-REC.css \
             logo-REC xmlspec.xsl REC-xml-2e.xsl diffspec.xsl \
	     REC-xml-20001006.html REC-xml-20001006-review.html

all:

valgrind:
	@echo '## Running the regression tests under Valgrind'
	$(MAKE) CHECKER='valgrind -q' tests

test tests: $(top_builddir)/xsltproc/xsltproc
	@(echo > .memdump)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc -timing $(srcdir)/REC-xml-2e.xsl $(srcdir)/REC-xml-20001006.xml > REC-xml-20001006.out 2> debug ; \
	diff $(srcdir)/REC-xml-20001006.html REC-xml-20001006.out | grep -v 'id[0-9]' | grep -v -- '---' | grep -v 100 | grep -v 3866 ; \
	grep implemented debug | sort | uniq -c || true; \
	grep " ms$$" debug || true; \
	grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true;\
	rm -f REC-xml-20001006.out)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc  -timing --param show.diff.markup 1 $(srcdir)/REC-xml-2e.xsl $(srcdir)/REC-xml-20001006.xml > REC-xml-20001006-review.out 2> debug ; \
	diff $(srcdir)/REC-xml-20001006-review.html REC-xml-20001006-review.out | grep -v 'id[0-9]' | grep -v -- '---' | grep -v 117 | grep -v 4066 ; \
	grep implemented debug | sort | uniq -c || true; \
	grep " ms$$" debug || true; \
	grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true;\
	rm -f REC-xml-20001006-review.out)

