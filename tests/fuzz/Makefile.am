LIBXSLT_LIBS = $(top_builddir)/libxslt/libxslt.la \
               $(top_builddir)/libexslt/libexslt.la

EXTRA_PROGRAMS = xpath xslt
EXTRA_DIST = xpath.dict xpath.xml xslt.dict xslt.xml seed
CLEANFILES = $(EXTRA_PROGRAMS)
AM_CPPFLAGS = -I$(top_srcdir)
AM_CFLAGS = $(LIBXML_CFLAGS)
AM_LDFLAGS = -fsanitize=fuzzer
DEPENDENCIES = $(LIBXSLT_LIBS)
LDADD = $(LIBXSLT_LIBS) \
        $(LIBGCRYPT_LIBS) $(LIBXML_LIBS) $(EXTRA_LIBS) $(M_LIBS)

$(top_builddir)/libxslt/libxslt.la:
	cd $(top_builddir)/libxslt && $(MAKE) libxslt.la

$(top_builddir)/libexslt/libexslt.la: $(top_builddir)/libxslt/libxslt.la
	cd $(top_builddir)/libexslt && $(MAKE) libexslt.la

.PHONY: fuzz-xpath fuzz-xslt

fuzz-xpath: xpath$(EXEEXT)
	@mkdir -p corpus/xpath
	./xpath$(EXEEXT) \
            -max_len=256 \
            -dict=$(srcdir)/xpath.dict \
	    corpus/xpath $(srcdir)/seed/xpath

fuzz-xslt: xslt$(EXEEXT)
	@mkdir -p corpus/xslt
	./xslt$(EXEEXT) \
            -dict=$(srcdir)/xslt.dict \
	    corpus/xslt $(srcdir)/seed/xslt

