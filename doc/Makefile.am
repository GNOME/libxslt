## Process this file with automake to produce Makefile.in
EXTRA_DIST = 			\
	libxslt-api.xml		\
	libexslt-api.xml	\
	libxslt-decl.txt	\
	xsltproc.xml		\
	apibuild.py		\
	libxslt.xsa		\
	news.xsl

# The name of the module.
DOC_MODULE=libxslt-$(VERSION)

# The top-level SGML file.
DOC_MAIN_XML_FILE=libxslt.xml

# The directory containing the source code (if it contains documentation).
DOC_SOURCE_DIR=..

HTML_DIR=$(datadir)/doc

# A file in win32 depends upon two of the doc files
WIN32_DIR=$(top_srcdir)/win32

TARGET_DIR=$(HTML_DIR)/$(DOC_MODULE)/html
PAGES= API.html bugs.html contribs.html docs.html downloads.html \
       help.html index.html intro.html news.html xsltproc2.html FAQ.html
APIPAGES=APIconstructors.html APIfiles.html APIfunctions.html \
    APIsymbols.html APIchunk0.html

man_MANS = xsltproc.1

all : $(PAGES) $(top_srcdir)/NEWS libxslt.xsa $(man_MANS)

tests:

web: $(PAGES) $(APIPAGES)

$(PAGES): $(srcdir)/xslt.html $(srcdir)/site.xsl
	-@(if [ -x $(bindir)/xsltproc ] ; then \
	  $(bindir)/xsltproc --nonet --html $(srcdir)/site.xsl $(srcdir)/xslt.html > index.html ; fi )
	-@(if [ -x $(bindir)/xmllint ] ; then \
	  $(bindir)/xmllint --nonet --valid --noout $(PAGES) ; fi );

$(top_srcdir)/NEWS: $(top_srcdir)/doc/news.xsl $(top_srcdir)/doc/news.html
	-@(if [ -x $(bindir)/xsltproc ] ; then \
	  $(bindir)/xsltproc --nonet $(top_srcdir)/doc/news.xsl $(top_srcdir)/doc/news.html > $(top_srcdir)/NEWS ; fi );

libxslt.xsa: $(top_srcdir)/doc/xsa.xsl $(top_srcdir)/doc/news.html
	-@(if [ -x $(bindir)/xsltproc ] ; then \
	  $(bindir)/xsltproc --nonet $(top_srcdir)/doc/xsa.xsl $(top_srcdir)/doc/news.html > libxslt.xsa ; fi );

$(APIPAGES): libxslt-refs.xml site.xsl api.xsl
	-@(if [ -x $(bindir)/xsltproc ] ; then \
	  $(bindir)/xsltproc --nonet --html $(srcdir)/api.xsl $(srcdir)/xslt.html ; fi );

xsltproc.1: xsltproc.xml
	-@(xsltproc --nonet xsltproc.xml)

scan:
	-gtkdoc-scan --module=libxslt --source-dir=$(DOC_SOURCE_DIR)/libxslt --ignore-headers="acconfig.h config.h win32config.h"
#	gtkdoc-scan --module=libexslt --source-dir=$(DOC_SOURCE_DIR)/libexslt --ignore-headers="acconfig.h config.h win32config.h"

templates: scan
	-gtkdoc-mktmpl --module=libxslt
#	gtkdoc-mktmpl --module=libexslt

xml:
	if test -d xml ; then rm xml/* ; else mkdir xml ; fi
	-gtkdoc-mkdb --module=libxslt --source-dir=$(DOC_SOURCE_DIR)/libxslt --output-format=xml --main-sgml-file=$(DOC_MAIN_XML_FILE)

html:
	if ! test -d html ; then mkdir html ; fi
	-cd html && gtkdoc-mkhtml libxslt ../$(DOC_MAIN_XML_FILE)
	   
libxslt-api.xml libexslt-api.xml: apibuild.py ../libxslt/*.h ../libxslt/*.c ../libexslt/*.h ../libexslt/*.c
	-(./apibuild.py)

$(WIN32_DIR)/libxslt.def.src: libxslt-api.xml
	-@(if [ -x $(bindir)/xsltproc ] ; then \
	  $(bindir)/xsltproc -o $(WIN32_DIR)/libxslt.def.src \
	  --nonet $(WIN32_DIR)/defgen.xsl libxslt-api.xml ; fi )

$(WIN32_DIR)/libexslt.def.src: libexslt-api.xml
	-@(if [ -x $(bindir)/xsltproc ] ; then \
	  $(bindir)/xsltproc -o $(WIN32_DIR)/libexslt.def.src \
	  --nonet $(WIN32_DIR)/defgen.xsl libexslt-api.xml ; fi )

api: libxslt-api.xml libxslt-refs.xml $(WIN32_DIR)/libxslt.def.src $(WIN32_DIR)/libexslt.def.src

clean-local:
	rm -f *~ *.bak *.hierarchy *.signals *-unused.txt

maintainer-clean-local: clean
	rm -rf libxslt-decl-list.txt libxslt-decl.txt 

libxslt-decl-list.txt : templates

libxslt-sections.txt : scan
	cp libxslt-decl-list.txt libxslt-sections.txt

rebuild: libxslt-sections.txt templates xml html

install-data-local: 
	$(mkinstalldirs) $(DESTDIR)$(TARGET_DIR)
	-@INSTALL@ -m 0644 $(srcdir)/*.html $(DESTDIR)$(TARGET_DIR)
	-@INSTALL@ -m 0644 $(srcdir)/*.gif $(DESTDIR)$(TARGET_DIR)
	$(mkinstalldirs) $(DESTDIR)$(TARGET_DIR)/html
	-@INSTALL@ -m 0644 $(srcdir)/html/*.html $(DESTDIR)$(TARGET_DIR)/html
	-@INSTALL@ -m 0644 $(srcdir)/html/*.png $(DESTDIR)$(TARGET_DIR)/html
	-@INSTALL@ -m 0644 $(srcdir)/html/index.sgml $(DESTDIR)$(TARGET_DIR)/html
	$(mkinstalldirs) $(DESTDIR)$(TARGET_DIR)/tutorial
	-@INSTALL@ -m 0644 $(srcdir)/tutorial/* $(DESTDIR)$(TARGET_DIR)/tutorial
	-(cd $(DESTDIR); gtkdoc-fixxref --module=libxslt --html-dir=$(HTML_DIR))

dist-hook:
	(cd $(srcdir) ; tar cvf - *.1 *.html site.xsl news.xsl xsa.xsl *.gif html/*.html html/*.png tutorial/libxslt*) | (cd $(distdir); tar xf -)

.PHONY : html xml templates scan
