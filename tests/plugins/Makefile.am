## Process this file with automake to produce Makefile.in

# somewhat unconventional pkglibdir, but noinst_LTLIBRARIES
# never build DSOs (afaik). NOTE: must be defined outside the AM_CONDITIONAL
pkglibdir=$(shell pwd)/plugin

$(top_builddir)/xsltproc/xsltproc:
	@(cd ../../../xsltproc ; $(MAKE) xsltproc)

EXTRA_DIST = plugin.out	plugin.xml plugin.xsl

all:

if WITH_MODULES
pkglib_LTLIBRARIES = xmlsoft_org_xslt_testplugin.la

xmlsoft_org_xslt_testplugin_la_CFLAGS = -DMODULE_COMPILE $(LIBXML_CFLAGS) $(LIBXSLT_CFLAGS)
xmlsoft_org_xslt_testplugin_la_SOURCES = testplugin.c
xmlsoft_org_xslt_testplugin_la_LDFLAGS = -module -avoid-version $(LIBXML_LIBS) $(XSLT_LIBS)

test-logall:
	@echo '## Running plugin tests'
	@echo '##  Note: installing xmlsoft_org_xslt_testplugin.so'
	@echo '##        to $(pkglibdir)'

test tests: $(top_builddir)/xsltproc/xsltproc test-logall install-pkglibLTLIBRARIES
	@LIBXSLT_PLUGINS_PATH=./plugin $(top_builddir)/xsltproc/xsltproc plugin.xsl plugin.xml > plugin.res
	@diff plugin.out plugin.res
	@rm plugin.res

else

test tests:
	@echo Skipping the plugin tests.

endif

