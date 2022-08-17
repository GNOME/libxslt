## Process this file with automake to produce Makefile.in

SUBDIRS = devhelp EXSLT/devhelp

# A file in win32 depends upon two of the doc files
WIN32_DIR=$(top_srcdir)/win32

nobase_dist_doc_DATA =			\
	tutorial/images/callouts/1.png	\
	tutorial/images/callouts/10.png	\
	tutorial/images/callouts/2.png	\
	tutorial/images/callouts/3.png	\
	tutorial/images/callouts/4.png	\
	tutorial/images/callouts/5.png	\
	tutorial/images/callouts/6.png	\
	tutorial/images/callouts/7.png	\
	tutorial/images/callouts/8.png	\
	tutorial/images/callouts/9.png	\
	tutorial/libxslt_tutorial.c	\
	tutorial/libxslttutorial.html	\
	tutorial2/libxslt_pipes.c	\
	tutorial2/libxslt_pipes.html	\
	xsltproc.html

dist_man_MANS = xsltproc.1

EXTRA_DIST =				\
	EXSLT/libexslt-api.xml		\
	EXSLT/libexslt-refs.xml		\
	apibuild.py			\
	checkapisym.xsl			\
	libxslt-api.xml			\
	libxslt-refs.xml		\
	symbols.xml			\
	syms.xsl			\
	tutorial/libxslttutorial.xml	\
	tutorial2/libxslt_pipes.xml	\
	xsltproc.xml

DOCBOOK_HTML = http://docbook.sourceforge.net/release/xsl/current/html/docbook.xsl

rebuild:
	cd $(srcdir) && ./apibuild.py
	cd $(srcdir) && $(XSLTPROC) checkapisym.xsl libxslt-api.xml
	cd $(srcdir) && $(XSLTPROC) -o ../libxslt/libxslt.syms syms.xsl symbols.xml
	cd $(srcdir) && $(XSLTPROC) --nonet xsltproc.xml
	cd $(srcdir) && $(XSLTPROC) --nonet -o xsltproc.html $(DOCBOOK_HTML) xsltproc.xml
	cd $(srcdir) && $(XSLTPROC) -o $(WIN32_DIR)/libxslt.def.src \
	  --nonet $(WIN32_DIR)/defgen.xsl libxslt-api.xml
	cd $(srcdir) && $(XSLTPROC) -o $(WIN32_DIR)/libexslt.def.src \
	  --nonet $(WIN32_DIR)/defgen.xsl EXSLT/libexslt-api.xml
	cd devhelp && $(MAKE) rebuild
	cd EXSLT/devhelp && $(MAKE) rebuild

.PHONY: rebuild
