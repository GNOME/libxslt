INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/libexslt \
         $(LIBXML_CFLAGS) $(CFLAGS)

lib_LTLIBRARIES = libexslt.la

exsltincdir = $(includedir)/libexslt

exsltinc_HEADERS =                      \
	exslt.h				\
	utils.h

libexslt_la_SOURCES =                   \
	exslt.c				\
	common.c			\
	math.c				\
	sets.c				\
	functions.c			\
	utils.c

libexslt_la_LIBADD = $(EXTRA_LIBS)
libexslt_la_LDFLAGS = -version-info @LIBEXSLT_VERSION_INFO@

DEPS = $(lib_LTLIBRARIES)
LDADDS = -L. $(top_builddir)/libexslt/libexslt.la $(EXTRA_LIBS) \
	 $(top_builddir)/libxslt/libxslt.la
