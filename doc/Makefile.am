## Process this file with automake to produce Makefile.in

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

man_MANS = xsltproc.1

all : $(PAGES)

$(PAGES): $(srcdir)/xslt.html $(srcdir)/site.xsl
	../xsltproc/xsltproc --html $(srcdir)/site.xsl $(srcdir)/xslt.html > index.html

scan:
	gtkdoc-scan --module=libxslt --source-dir=$(DOC_SOURCE_DIR) --ignore-headers="acconfig.h config.h win32config.h"

templates: scan
	gtkdoc-mktmpl --module=libxslt

sgml:
	gtkdoc-mkdb --module=libxslt --source-dir=$(DOC_SOURCE_DIR)

html:
	if ! test -d html ; then mkdir html ; fi
	-cd html && gtkdoc-mkhtml libxslt ../$(DOC_MAIN_SGML_FILE)

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
	-@INSTALL@ -m 0644 $(srcdir)/xml.html $(DESTDIR)$(TARGET_DIR)
	-@INSTALL@ -m 0644 $(srcdir)/html/*.html $(DESTDIR)$(TARGET_DIR)
	-@INSTALL@ -m 0644 $(srcdir)/html/index.sgml $(DESTDIR)$(TARGET_DIR)
	$(mkinstalldirs) $(DESTDIR)$(TARGET_DIR)/tutorial
	-@INSTALL@ -m 0644 $(srcdir)/tutorial/* $(DESTDIR)$(TARGET_DIR)/tutorial
	-(cd $(DESTDIR); gtkdoc-fixxref --module=libxslt --html-dir=$(HTML_DIR))

dist-hook:
	(cd $(srcdir) ; tar cvf - *.1 *.html site.xsl *.gif html/*.html html/*.sgml tutorial/libxslt*) | (cd $(distdir); tar xf -)

.PHONY : html sgml templates scan
