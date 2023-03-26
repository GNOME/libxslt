LIBXSLT_LIBS = $(top_builddir)/libxslt/libxslt.la \
               $(top_builddir)/libexslt/libexslt.la

EXTRA_PROGRAMS = genSeed xpath xslt
check_PROGRAMS = testTargets
EXTRA_DIST = xpath.dict xslt.dict
CLEANFILES = $(EXTRA_PROGRAMS)
AM_CPPFLAGS = -I$(top_srcdir)
DEPENDENCIES = $(LIBXSLT_LIBS)
LDADD = $(LIBXSLT_LIBS) \
        $(LIBXML_LIBS) $(EXTRA_LIBS) $(LIBM)

testTargets_SOURCES = testTargets.c fuzz.c fuzz.h

$(top_builddir)/libxslt/libxslt.la:
	cd $(top_builddir)/libxslt && $(MAKE) libxslt.la

$(top_builddir)/libexslt/libexslt.la: $(top_builddir)/libxslt/libxslt.la
	cd $(top_builddir)/libexslt && $(MAKE) libexslt.la

.PHONY: fuzz-xpath fuzz-xslt

check-local: $(check_PROGRAMS)
	@echo '## Running fuzz target tests'
	@./testTargets

# Seed corpus

genSeed_SOURCES = genSeed.c fuzz.c

# XPath fuzzer

seed/xpath.stamp: genSeed$(EXEEXT)
	@mkdir -p seed/xpath
	@./genSeed$(EXEEXT) xpath "$(top_srcdir)/tests"
	@touch seed/xpath.stamp

xpath_SOURCES = xpath.c fuzz.c fuzz.h
xpath_LDFLAGS = -fsanitize=fuzzer

fuzz-xpath: xpath$(EXEEXT) seed/xpath.stamp
	@mkdir -p corpus/xpath
	./xpath$(EXEEXT) \
            -dict=$(srcdir)/xpath.dict \
	    corpus/xpath $(srcdir)/seed/xpath

# XSLT fuzzer

seed/xslt.stamp: genSeed$(EXEEXT)
	@mkdir -p seed/xslt
	@./genSeed$(EXEEXT) xslt "$(top_srcdir)/tests"
	@touch seed/xslt.stamp

xslt_SOURCES = xslt.c fuzz.c fuzz.h
xslt_LDFLAGS = -fsanitize=fuzzer

fuzz-xslt: xslt$(EXEEXT) seed/xslt.stamp
	@mkdir -p corpus/xslt
	./xslt$(EXEEXT) \
            -dict=$(srcdir)/xslt.dict \
	    corpus/xslt $(srcdir)/seed/xslt

