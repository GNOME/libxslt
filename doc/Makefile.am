## Process this file with automake to produce Makefile.in
EXTRA_DIST = 			\
	libxslt-api.xml		\
	libxslt-decl.txt	\
	xsltproc.xml

# The name of the module.
DOC_MODULE=libxslt-$(VERSION)

# The top-level SGML file.
DOC_MAIN_SGML_FILE=libxslt.sgml

# The directory containing the source code (if it contains documentation).
DOC_SOURCE_DIR=..

HTML_DIR=@HTML_DIR@

TARGET_DIR=$(HTML_DIR)/$(DOC_MODULE)/html
PAGES= API.html bugs.html contribs.html docs.html downloads.html \
       help.html index.html intro.html news.html xsltproc2.html FAQ.html
APIPAGES=APIconstructors.html APIfiles.html APIfunctions.html \
    APIsymbols.html APIchunk0.html

man_MANS = xsltproc.1

all : $(PAGES)

web: $(PAGES) $(APIPAGES)

$(PAGES): $(srcdir)/xslt.html $(srcdir)/site.xsl
	-@(if [ -x $(bindir)/xsltproc ] ; then \
	  $(bindir)/xsltproc --html $(srcdir)/site.xsl $(srcdir)/xslt.html > index.html ; fi )

$(APIPAGES): libxslt-refs.xml site.xsl api.xsl
	-@(if [ -x $(bindir)/xsltproc ] ; then \
	  $(bindir)/xsltproc --html $(srcdir)/api.xsl $(srcdir)/xslt.html ; fi );

scan:
	gtkdoc-scan --module=libxslt --source-dir=$(DOC_SOURCE_DIR)/libxslt --ignore-headers="acconfig.h config.h win32config.h"
#	gtkdoc-scan --module=libexslt --source-dir=$(DOC_SOURCE_DIR)/libexslt --ignore-headers="acconfig.h config.h win32config.h"

templates: scan
	gtkdoc-mktmpl --module=libxslt
#	gtkdoc-mktmpl --module=libexslt

sgml:
	gtkdoc-mkdb --module=libxslt --source-dir=$(DOC_SOURCE_DIR)/libxslt
#	gtkdoc-mkdb --module=libxslt --source-dir=$(DOC_SOURCE_DIR)/libexslt

html:
	if ! test -d html ; then mkdir html ; fi
	-cd html && gtkdoc-mkhtml libxslt ../$(DOC_MAIN_SGML_FILE)

xml: sgml
	if test -d xml ; then rm xml/* ; else mkdir xml ; fi
	-@(for s in sgml/*.sgml ; do name=`basename $$s .sgml` ; \
	   echo '<!DOCTYPE refentry [ <!ENTITY nbsp   "&#x00A0;"> ]>' > \
	       xml/"$$name".xml ;\
	   grep -v "^<colspec" $$s >> xml/"$$name".xml ; done)
	   
libxslt-api.xml libxslt-refs.xml: xml parsedecl.py libxslt-decl.txt libxslt-decl-list.txt
	-(./parsedecl.py)

api: libxslt-api.xml libxslt-refs.xml

clean-local:
	rm -f *~ *.bak *.hierarchy *.signals *-unused.txt

maintainer-clean-local: clean
	rm -rf sgml html libxslt-decl-list.txt libxslt-decl.txt 

libxslt-decl-list.txt : templates

libxslt-sections.txt : scan
	cp libxslt-decl-list.txt libxslt-sections.txt

rebuild: libxslt-sections.txt templates sgml html

install-data-local: 
	$(mkinstalldirs) $(DESTDIR)$(TARGET_DIR)
	-@INSTALL@ -m 0644 $(srcdir)/*.html $(DESTDIR)$(TARGET_DIR)
	-@INSTALL@ -m 0644 $(srcdir)/*.gif $(DESTDIR)$(TARGET_DIR)
	-@INSTALL@ -m 0644 $(srcdir)/html/*.html $(DESTDIR)$(TARGET_DIR)
	-@INSTALL@ -m 0644 $(srcdir)/html/index.sgml $(DESTDIR)$(TARGET_DIR)
	$(mkinstalldirs) $(DESTDIR)$(TARGET_DIR)/tutorial
	-@INSTALL@ -m 0644 $(srcdir)/tutorial/* $(DESTDIR)$(TARGET_DIR)/tutorial
	-(cd $(DESTDIR); gtkdoc-fixxref --module=libxslt --html-dir=$(HTML_DIR))

dist-hook:
	(cd $(srcdir) ; tar cvf - *.1 *.html site.xsl *.gif html/*.html html/*.sgml tutorial/libxslt*) | (cd $(distdir); tar xf -)

.PHONY : html sgml templates scan
