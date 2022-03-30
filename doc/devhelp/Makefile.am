devhelpdir = $(datadir)/gtk-doc/html/libxslt

dist_devhelp_DATA =			\
	libxslt.devhelp2		\
	$(HTML_FILES)			\
	$(EXTRA_FORMAT)

HTML_FILES =				\
	general.html			\
	index.html			\
	$(HTML_MODULES)

HTML_MODULES=				\
	libxslt-attributes.html		\
	libxslt-documents.html		\
	libxslt-extensions.html		\
	libxslt-extra.html		\
	libxslt-functions.html		\
	libxslt-imports.html		\
	libxslt-keys.html		\
	libxslt-namespaces.html		\
	libxslt-numbersInternals.html	\
	libxslt-pattern.html		\
	libxslt-preproc.html		\
	libxslt-security.html		\
	libxslt-templates.html		\
	libxslt-transform.html		\
	libxslt-variables.html		\
	libxslt-xsltexports.html	\
	libxslt-xslt.html		\
	libxslt-xsltInternals.html	\
	libxslt-xsltlocale.html		\
	libxslt-xsltutils.html

EXTRA_FORMAT=				\
	home.png			\
	left.png			\
	right.png			\
	up.png				\
	style.css

EXTRA_DIST = devhelp2.xsl html.xsl

rebuild:
	cd $(srcdir) && $(XSLTPROC) --nonet -o libxslt.devhelp2 devhelp2.xsl ../libxslt-api.xml

.PHONY: rebuild
