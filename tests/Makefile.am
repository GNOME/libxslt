## Process this file with automake to produce Makefile.in

INCLUDES = -I$(srcdir) -I$(top_srcdir)/libxslt \
	$(LIBXML_CFLAGS) -Wall -ansi

noinst_PROGRAMS = # testxslt testevents

DEPS = $(top_builddir)/libxslt/libxslt.la
LDADDS = $(top_builddir)/libxslt/libxslt.la $(LIBXML_LIBS)

#testxslt_SOURCES = test-xslt.c
#testxslt_LDFLAGS =
#testxslt_DEPENDENCIES = $(DEPS)
#testxslt_LDADD = $(LDADDS)
#
#testevents_SOURCES = test-events.c
#testevents_LDFLAGS =
#testevents_DEPENDENCIES = $(DEPS)
#testevents_LDADD = $(LDADDS)
