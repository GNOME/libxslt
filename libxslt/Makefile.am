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
	extra.h				\
	functions.h			\
	namespaces.h			\
	imports.h			\
	attributes.h			\
	documents.h			\
	preproc.h			\
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
	extra.c				\
	functions.c			\
	namespaces.c			\
	imports.c			\
	attributes.c			\
	documents.c			\
	preproc.c			\
	transform.c

libxslt_la_LIBADD = $(EXTRA_LIBS)

# what is this include dir ?
# libxsltincdir = $(includedir)/libxslt

bin_PROGRAMS = xsltproc

DEPS = $(top_builddir)/libxslt/libxslt.la
LDADDS = -L. $(top_builddir)/libxslt/libxslt.la $(EXTRA_LIBS)

xsltproc_SOURCES = xsltproc.c
xsltproc_LDFLAGS =
xsltproc_DEPENDENCIES = $(DEPS)
xsltproc_LDADD = $(LDADDS)

