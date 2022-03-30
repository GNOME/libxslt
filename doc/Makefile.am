## Process this file with automake to produce Makefile.in

SUBDIRS = devhelp EXSLT/devhelp

# A file in win32 depends upon two of the doc files
WIN32_DIR=$(top_srcdir)/win32

EXTRA_DIST =				\
	EXSLT/libexslt-api.xml		\
	EXSLT/libexslt-refs.xml		\
	api.xsl				\
	apibuild.py			\
	checkapisym.xsl			\
	images/callouts/1.png		\
	images/callouts/10.png		\
	images/callouts/2.png		\
	images/callouts/3.png		\
	images/callouts/4.png		\
	images/callouts/5.png		\
	images/callouts/6.png		\
	images/callouts/7.png		\
	images/callouts/8.png		\
	images/callouts/9.png		\
	libxslt-api.xml			\
	libxslt-refs.xml		\
	symbols.xml			\
	syms.xsl			\
	tutorial/libxslt_tutorial.c	\
	tutorial/libxslttutorial.html	\
	tutorial/libxslttutorial.xml	\
	tutorial2/libxslt_pipes.c	\
	tutorial2/libxslt_pipes.html	\
	tutorial2/libxslt_pipes.xml	\
	xsltproc.1			\
	xsltproc.html			\
	xsltproc.xml

man_MANS = xsltproc.1

if REBUILD_DOCS

all-local: $(man_MANS) xsltproc.html

libxslt-api.xml libxslt-refs.xml EXSLT/libexslt-api.xml \
  EXSLT/libexslt-refs.xml ../libxslt/libxslt.syms : $(srcdir)/apibuild.py $(top_srcdir)/libxslt/*.h \
		$(top_srcdir)/libxslt/*.c $(top_srcdir)/libexslt/*.h \
		$(top_srcdir)/libexslt/*.c
	-(cd $(srcdir) && ./apibuild.py)
	($(XSLTPROC) $(srcdir)/checkapisym.xsl $(srcdir)/libxslt-api.xml)
	($(XSLTPROC) -o ../libxslt/libxslt.syms $(srcdir)/syms.xsl $(srcdir)/symbols.xml)

xsltproc.html: xsltproc.xml
	-@($(XSLTPROC) --nonet -o $@ http://docbook.sourceforge.net/release/xsl/current/html/docbook.xsl xsltproc.xml)

xsltproc.1: $(srcdir)/xsltproc.xml
	-@(if [ -x $(XSLTPROC) ] ; then \
	   echo "Rebuilding the man pages from the xml" ; \
	   $(XSLTPROC) --nonet $(srcdir)/xsltproc.xml ; fi)

$(WIN32_DIR)/libxslt.def.src: libxslt-api.xml
	-@(if [ -x $(XSLTPROC) ] ; then \
	  $(XSLTPROC) -o $(WIN32_DIR)/libxslt.def.src \
	  --nonet $(WIN32_DIR)/defgen.xsl $(srcdir)/libxslt-api.xml ; fi )

$(WIN32_DIR)/libexslt.def.src: EXSLT/libexslt-api.xml
	-@(if [ -x $(XSLTPROC) ] ; then \
	  $(XSLTPROC) -o $(WIN32_DIR)/libexslt.def.src \
	  --nonet $(WIN32_DIR)/defgen.xsl $(srcdir)/EXSLT/libexslt-api.xml ; fi )

rebuild: all

wildcards =			\
	*.1			\
	*.html			\
	*.xsl			\
	images/callouts/*.png	\
	tutorial*/libxslt*

# This target verifies that EXTRA_DIST contains all the files matched by
# the above list of wildcards. Note that this will only work properly when
# srcdir = builddir.
#
check-extra-dist:
	@for a in $(wildcards) ; do \
		if test ! -f "$$a" ; then \
			echo "not found: $$a" ; \
			continue ; \
		fi ; \
		present= ; \
		for b in $(EXTRA_DIST) ; do \
			test "$$a" = "$$b" && present=1 && break ; \
		done ; \
		test -n "$$present" || echo "not in EXTRA_DIST: $$a" ; \
	done

endif

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(HTML_DIR)
	-$(INSTALL) -m 0644 $(srcdir)/xsltproc.html $(DESTDIR)$(HTML_DIR)
	$(MKDIR_P) $(DESTDIR)$(HTML_DIR)/tutorial
	-$(INSTALL) -m 0644 $(srcdir)/tutorial/* $(DESTDIR)$(HTML_DIR)/tutorial
	$(MKDIR_P) $(DESTDIR)$(HTML_DIR)/tutorial2
	-$(INSTALL) -m 0644 $(srcdir)/tutorial2/* $(DESTDIR)$(HTML_DIR)/tutorial2

uninstall-local:
	rm -rf $(DESTDIR)$(HTML_DIR)

.PHONY : rebuild check-extra-dist
