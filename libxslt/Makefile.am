INCLUDES = -I$(top_srcdir)/libxslt \
	 $(LIBXML_CFLAGS) -Wall -ansi

lib_LTLIBRARIES = libxslt.la

libxslt_la_SOURCES = 			\
	xslt.c				\
	xslt.h				\
	xsltutils.c			\
	xsltutils.h			\
	pattern.c			\
	pattern.h			\
	transform.c			\
	transform.h			\
	xsltInternals.h



# what is this include dir ?
# libxsltincdir = $(includedir)/libxslt

noinst_PROGRAMS = xsltproc

DEPS = $(top_builddir)/libxslt/libxslt.la
LDADDS = -L. $(top_builddir)/libxslt/libxslt.la $(LIBXML_LIBS)

xsltproc_SOURCES = xsltproc.c
xsltproc_LDFLAGS =
xsltproc_DEPENDENCIES = $(DEPS)
xsltproc_LDADD = $(LDADDS)

