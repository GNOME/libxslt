INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/libxslt \
	 $(LIBXML_CFLAGS) $(CFLAGS)

lib_LTLIBRARIES = libxslt.la

xsltincdir = $(includedir)/libxslt

xsltinc_HEADERS = 			\
	xslt.h				\
	xsltutils.h			\
	pattern.h			\
	templates.h			\
	variables.h			\
	keys.h				\
	numbersInternals.h		\
	extensions.h			\
	functions.h			\
	namespaces.h			\
	imports.h			\
	attributes.h			\
	documents.h			\
	transform.h			\
	xsltInternals.h

libxslt_la_SOURCES = 			\
	xslt.c				\
	xsltutils.c			\
	pattern.c			\
	templates.c			\
	variables.c			\
	keys.c				\
	numbers.c			\
	extensions.c			\
	functions.c			\
	namespaces.c			\
	imports.c			\
	attributes.c			\
	documents.c			\
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

