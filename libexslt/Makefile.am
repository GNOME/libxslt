INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/libxslt -I$(top_srcdir)/libexslt \
	   -I$(top_builddir) -I$(top_builddir)/libxslt \
	   -I$(top_builddir)/libexslt $(LIBXML_CFLAGS) $(CFLAGS)

lib_LTLIBRARIES = libexslt.la

exsltincdir = $(includedir)/libexslt

exsltinc_HEADERS =                      \
	exslt.h				\
	exsltconfig.h			\
	exslt.h				\
	exsltconfig.h

libexslt_la_SOURCES =                   \
	exslt.c				\
	common.c			\
	math.c				\
	sets.c				\
	functions.c

libexslt_la_LIBADD = $(EXTRA_LIBS)
libexslt_la_LDFLAGS = -version-info @LIBEXSLT_VERSION_INFO@

