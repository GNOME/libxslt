INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/libxslt -I$(top_srcdir)/libexslt \
           -I$(top_builddir) -I$(top_builddir)/libxslt \
	   -I$(top_builddir)/libexslt $(LIBXML_CFLAGS) $(CFLAGS)

if DV_STATIC_LINK
#
# Try to bypass the braindead libtool processing of shared libs
#
all: xsltproc

xsltproc: xsltproc.o ../libxslt/.libs/libxslt.a ../libexslt/.libs/libexslt.a ../../XML/.libs/libxml2.a
	gcc -g -O -o xsltproc  xsltproc.o ../libxslt/.libs/libxslt.a ../libexslt/.libs/libexslt.a ../../XML/.libs/libxml2.a -lz -lm

programs=
else
programs=xsltproc
endif

bin_PROGRAMS = $(programs)

xsltproc_SOURCES = xsltproc.c
xsltproc_LDFLAGS =
xsltproc_DEPENDENCIES = $(DEPS)
xsltproc_LDADD = $(LDADDS)

DEPS = $(top_builddir)/libxslt/libxslt.la $(top_builddir)/libexslt/libexslt.la
LDADDS = $(top_builddir)/libxslt/libxslt.la \
         $(top_builddir)/libexslt/libexslt.la @LIBXML_LIBS@ $(EXTRA_LIBS)
