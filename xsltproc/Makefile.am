INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/libxslt -I$(top_srcdir)/libexslt \
           -I$(top_builddir) -I$(top_builddir)/libxslt \
	   -I$(top_builddir)/libexslt $(LIBXML_CFLAGS) $(CFLAGS)

programs=xsltproc

bin_PROGRAMS = $(programs)

xsltproc_SOURCES = xsltproc.c
xsltproc_LDFLAGS =
xsltproc_DEPENDENCIES = $(DEPS)
xsltproc_LDADD = $(LDADDS)

if WITH_DEBUGGER
DEPS = $(top_builddir)/libxslt/libxslt.la \
	$(top_builddir)/breakpoint/libxsltbreakpoint.la \
	$(top_builddir)/libexslt/libexslt.la 
else
DEPS = $(top_builddir)/libxslt/libxslt.la \
	$(top_builddir)/libexslt/libexslt.la 
endif

if WITH_DEBUGGER
LDADDS = $(top_builddir)/libxslt/libxslt.la \
         $(top_builddir)/libexslt/libexslt.la \
	$(top_builddir)/breakpoint/libxsltbreakpoint.la \
	@LIBXML_LIBS@ $(EXTRA_LIBS)
else
LDADDS = $(top_builddir)/libxslt/libxslt.la \
         $(top_builddir)/libexslt/libexslt.la \
	@LIBXML_LIBS@ $(EXTRA_LIBS)
endif
