devhelpdir = $(datadir)/gtk-doc/html/libexslt

dist_devhelp_DATA =			\
	libexslt.devhelp2		\
	$(HTML_FILES)			\
	$(EXTRA_FORMAT)

HTML_FILES =				\
	general.html			\
	index.html			\
	$(HTML_MODULES)

HTML_MODULES=				\
	libexslt-exsltexports.html	\
	libexslt-exslt.html

EXTRA_FORMAT=				\
	home.png			\
	left.png			\
	right.png			\
	up.png				\
	style.css

EXTRA_DIST = devhelp2.xsl html.xsl

rebuild:
	cd $(srcdir) && $(XSLTPROC) --nonet -o libexslt.devhelp2 devhelp2.xsl ../libexslt-api.xml

.PHONY: rebuild
