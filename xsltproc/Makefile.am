INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/libxslt -I$(top_srcdir)/libexslt \
           -I$(top_builddir) -I$(top_builddir)/libxslt \
	   -I$(top_builddir)/libexslt $(LIBXML_CFLAGS) $(CFLAGS)

if DV_STATIC_LINK
#
# Try to bypass the braindead libtool processing of shared libs
#
all: xsltproc

if WITH_DEBUGGER
xsltproc$(EXEEXT): xsltproc.o ../libxslt/.libs/libxslt.a ../libexslt/.libs/libexslt.a ../breakpoint/.libs/libxsltbreakpoint.a ../../XML/.libs/libxml2.a
	gcc -g -O -o xsltproc$(EXEEXT) xsltproc.o ../libxslt/.libs/libxslt.a ../breakpoint/.libs/libxsltbreakpoint.a ../libexslt/.libs/libexslt.a ../../XML/.libs/libxml2.a -lz -lm
else
xsltproc$(EXEEXT): xsltproc.o ../libxslt/.libs/libxslt.a ../libexslt/.libs/libexslt.a ../../XML/.libs/libxml2.a
	gcc -g -O -o xsltproc$(EXEEXT) xsltproc.o ../libxslt/.libs/libxslt.a ../libexslt/.libs/libexslt.a ../../XML/.libs/libxml2.a -lz -lm
endif

programs=
else
programs=xsltproc
endif

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
