INCLUDES = -I$(top_srcdir) -I$(top_builddir)/libxslt -I$(top_srcdir)/libxslt $(LIBXML_CFLAGS)

lib_LTLIBRARIES = libxsltbreakpoint.la

libxsltbreakpoint_la_SOURCES = 	\
	deprecated.c

libxsltbreakpoint_la_LIBADD =
libxsltbreakpoint_la_LDFLAGS = -version-info @LIBXSLTBREAK_VERSION_INFO@

clean:
	rm -f *.lo
	rm -f *.o
	rm -rf .libs
	rm -f *.la 


