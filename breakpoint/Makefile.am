INCLUDES = -I$(top_srcdir) -I$(top_builddir)/libxslt -I$(top_srcdir)/libxslt $(LIBXML_CFLAGS)

lib_LTLIBRARIES = libxsltbreakpoint.la

xsltbreakpointincdir = $(includedir)/breakpoint

xsltbreakpointinc_HEADERS = 	\
	breakpoint.h

libxsltbreakpoint_la_SOURCES = 	\
	dbgbreakpoint.c	\
	dbgcallstack.c 	\
	dbgsearch.c 	\
	dbgmain.c	


libxsltbreakpoint_la_LIBADD = @LIBXML_LIBS@
libxsltbreakpoint_la_LDFLAGS = -version-info @LIBXSLT_VERSION_INFO@

man_MANS = #breakpoint.4

EXTRA_DIST = $(man_MANS)

clean:
	rm -f *.lo
	rm -f *.o
	rm -rf .libs
	rm -f *.la 


