INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/libxslt \
	 $(LIBXML_CFLAGS) -Wall -ansi

lib_LTLIBRARIES = libxslt.la

xsltincdir = $(includedir)/libxslt

xsltinc_HEADERS = 			\
	xslt.h				\
	xsltutils.h			\
	pattern.h			\
	templates.h			\
	variables.h			\
	functions.h			\
	namespaces.h			\
	attributes.h			\
	transform.h			\
	xsltInternals.h

libxslt_la_SOURCES = 			\
	xslt.c				\
	xsltutils.c			\
	pattern.c			\
	templates.c			\
	variables.c			\
	functions.c			\
	namespaces.c			\
	attributes.c			\
	transform.c



# what is this include dir ?
# libxsltincdir = $(includedir)/libxslt

bin_PROGRAMS = xsltproc

DEPS = $(top_builddir)/libxslt/libxslt.la
LDADDS = -L. $(top_builddir)/libxslt/libxslt.la $(EXTRA_LIBS)

xsltproc_SOURCES = xsltproc.c
xsltproc_LDFLAGS =
xsltproc_DEPENDENCIES = $(DEPS)
xsltproc_LDADD = $(LDADDS)

