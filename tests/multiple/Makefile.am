## Process this file with automake to produce Makefile.in

$(top_builddir)/libxslt/xsltproc:
	@(cd ../../libxslt ; make xsltproc)

EXTRA_DIST = dict.dtd  dict.xml  dict.xsl result.xml \
    out/lettera.html out/lettera.orig out/letterb.html out/letterb.orig \
    out/letterc.html out/letterc.orig out/letterd.html out/letterd.orig \
    out/lettere.html out/lettere.orig out/letterf.html out/letterf.orig \
    out/letterg.html out/letterg.orig out/letterh.html out/letterh.orig \
    out/letterij.html out/letterij.orig out/letterk.html out/letterk.orig \
    out/letterl.html out/letterl.orig out/letterm.html out/letterm.orig \
    out/lettern.html out/lettern.orig out/lettero.html out/lettero.orig \
    out/letterp.html out/letterp.orig out/letterq.html out/letterq.orig \
    out/letterr.html out/letterr.orig out/letters.html out/letters.orig \
    out/lettert.html out/lettert.orig out/letteruv.html out/letteruv.orig \
    out/letterw.html out/letterw.orig out/letterx.html out/letterx.orig \
    out/lettery.html out/lettery.orig out/letterz.html out/letterz.orig \
    out/titlepage.html out/titlepage.orig

all: test

test tests: $(top_builddir)/libxslt/xsltproc
	@(rm -f .memdump ; touch .memdump)
	@($(top_builddir)/libxslt/xsltproc dict.xsl dict.xml > dict.res ; \
	diff result.xml dict.res ; \
	for i in out/letter*.orig ; do \
	  diff $$i out/`basename $$i .orig`.html ; done ; \
	diff out/titlepage.orig out/titlepage.html ; \
	grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0";\
	rm -f dict.res)


