INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/libxslt -I$(top_srcdir)/libexslt \
	   -I$(top_builddir) -I$(top_builddir)/libxslt \
	   -I$(top_builddir)/libexslt $(LIBXML_CFLAGS) $(CFLAGS)

lib_LTLIBRARIES = libexslt.la

exsltincdir = $(includedir)/libexslt

exsltinc_HEADERS =                      \
	exslt.h				\
	exsltconfig.h			\
	exslt.h				\
	exsltconfig.h			\
	exsltexports.h

libexslt_la_SOURCES =                   \
	exslt.c				\
	common.c			\
	math.c				\
	sets.c				\
	functions.c			\
	strings.c			\
	date.c				\
	saxon.c				\
	libexslt.h			\
	dynamic.c

# The following DOES NOT WORK reliably. Sorry no prelinking to uninstalled
# yet libraries.
# libexslt_la_LIBADD = $(top_builddir)/libxslt/libxslt.la $(EXTRA_LIBS)
libexslt_la_LIBADD = $(INSTALLED_XSLT_LIB) $(EXTRA_LIBS)
libexslt_la_LDFLAGS = -version-info @LIBEXSLT_VERSION_INFO@

man_MANS = libexslt.3

EXTRA_DIST = $(man_MANS)
