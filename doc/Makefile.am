## Process this file with automake to produce Makefile.in

# The name of the module.
DOC_MODULE=libxslt-$(VERSION)

# The top-level SGML file.
DOC_MAIN_SGML_FILE=libxslt.sgml

# The directory containing the source code (if it contains documentation).
DOC_SOURCE_DIR=..

HTML_DIR=$(prefix)/doc

TARGET_DIR=$(HTML_DIR)/$(DOC_MODULE)/html

# htmldir = $(prefix)/html
# html_DATA = gnome-dev-info.html

scan:
	gtkdoc-scan --module=libxslt --source-dir=$(DOC_SOURCE_DIR) --ignore-headers="acconfig.h config.h"

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
	-(cd $(DESTDIR); gtkdoc-fixxref --module=libxslt --html-dir=$(HTML_DIR))

dist-hook:
	(cd $(srcdir) ; tar cvf - *.html *.gif html/*.html html/*.sgml) | (cd $(distdir); tar xf -)

.PHONY : html sgml templates scan
