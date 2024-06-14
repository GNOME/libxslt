## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir)

SUBDIRS = xmlspec multiple xinclude XSLTMark docbook fuzz

DEPENDENCIES = $(top_builddir)/libxslt/libxslt.la \
               $(top_builddir)/libexslt/libexslt.la

LDADD = $(top_builddir)/libxslt/libxslt.la \
        $(top_builddir)/libexslt/libexslt.la \
        $(LIBXML_LIBS)

check_PROGRAMS = runtest

runtest_SOURCES = runtest.c

if WITH_MODULES

check_LTLIBRARIES = xmlsoft_org_xslt_testplugin.la

# our rpath is a rather unorthodox location as we
# don't want to pollute $(DESTDIR) with the test plugin

plugindir = $(abs_builddir)/.libs

xmlsoft_org_xslt_testplugin_la_CFLAGS = $(AM_CFLAGS) -DMODULE_COMPILE
xmlsoft_org_xslt_testplugin_la_SOURCES = testplugin.c
xmlsoft_org_xslt_testplugin_la_LIBADD = $(top_builddir)/libxslt/libxslt.la $(LIBXML_LIBS) $(EXTRA_LIBS)
xmlsoft_org_xslt_testplugin_la_LDFLAGS = \
	$(AM_LDFLAGS) -no-undefined \
	-module -avoid-version -rpath $(plugindir)

endif

all:

check-local:
	cd $(srcdir) && LIBXSLT_PLUGINS_PATH=$(plugindir) $(abs_builddir)/runtest

docbook_tests:
	@(cd docbook ; $(MAKE) full)

dist-hook:
	cp -a $(srcdir)/REC $(distdir)
	cp -a $(srcdir)/REC2 $(distdir)
	cp -a $(srcdir)/documents $(distdir)
	cp -a $(srcdir)/encoding $(distdir)
	cp -a $(srcdir)/exslt $(distdir)
	cp -a $(srcdir)/extensions $(distdir)
	cp -a $(srcdir)/general $(distdir)
	cp -a $(srcdir)/keys $(distdir)
	cp -a $(srcdir)/namespaces $(distdir)
	cp -a $(srcdir)/numbers $(distdir)
	cp -a $(srcdir)/plugins $(distdir)
	cp -a $(srcdir)/reports $(distdir)
