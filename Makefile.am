SUBDIRS = \
	libxslt \
	libexslt \
	xsltproc \
	tests \
	doc

confexecdir=$(libdir)
confexec_DATA = xsltConf.sh

bin_SCRIPTS = xslt-config

dist-hook: libxslt.spec
	(cd $(srcdir) ; tar -cf - --exclude CVS win32) | (cd $(distdir); tar xf -)

EXTRA_DIST = xsltConf.sh.in xslt-config.in libxslt.spec libxslt.spec.in \
             FEATURES TODO COPYING.LIB Copyright IPR \
	     win32/libxslt/libxslt.def win32/libxslt/libxslt.dsw \
	     win32/libxslt/libxslt_so.dsp win32/libxslt/xsltproc.dsp

## We create xsltConf.sh here and not from configure because we want
## to get the paths expanded correctly.  Macros like srcdir are given
## the value NONE in configure if the user doesn't specify them (this
## is an autoconf feature, not a bug).

xsltConf.sh: xsltConf.sh.in Makefile
## Use sed and then mv to avoid problems if the user interrupts.
	sed -e 's?\@XSLT_LIBDIR\@?$(XSLT_LIBDIR)?g' \
	    -e 's?\@XSLT_INCLUDEDIR\@?$(XSLT_INCLUDEDIR)?g' \
	    -e 's?\@VERSION\@?$(VERSION)?g' \
	    -e 's?\@XSLT_LIBS\@?$(XSLT_LIBS)?g' \
	       < $(srcdir)/xsltConf.sh.in > xsltConf.tmp \
	&& mv xsltConf.tmp xsltConf.sh

check-local: tests

dummy:

tests: dummy
	@(cd tests ; make test)

cleantar:
	@(rm -f libxslt*.tar.gz)

rpm: cleantar dist
	rpm -ta libxslt*.tar.gz

