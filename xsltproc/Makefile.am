INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/libxslt -I$(top_srcdir)/libexslt \
           -I$(top_builddir) -I$(top_builddir)/libxslt \
	   -I$(top_builddir)/libexslt $(LIBXML_CFLAGS) $(CFLAGS)

bin_PROGRAMS = xsltproc

DEPS = $(top_builddir)/libxslt/libxslt.la $(top_builddir)/libexslt/libexslt.la
LDADDS = -L. $(top_builddir)/libxslt/libxslt.la \
             $(top_builddir)/libexslt/libexslt.la $(EXTRA_LIBS)

xsltproc_SOURCES = xsltproc.c
xsltproc_LDFLAGS =
xsltproc_DEPENDENCIES = $(DEPS)
xsltproc_LDADD = $(LDADDS)

