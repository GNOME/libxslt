## Process this file with automake to produce Makefile.in

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
PAGES=   API.html bugs.html contribs.html docs.html downloads.html \
         help.html index.html intro.html news.html xsltproc2.html FAQ.html

EPAGES=	 EXSLT/bugs.html EXSLT/docs.html EXSLT/downloads.html \
	 EXSLT/help.html EXSLT/index.html EXSLT/intro.html

APIPAGES=APIconstructors.html APIfiles.html \
	 APIfunctions.html APIsymbols.html APIchunk0.html

EAPIPAGES=EXSLT/APIconstructors.html EXSLT/APIfiles.html \
	 EXSLT/APIfunctions.html EXSLT/APIsymbols.html EXSLT/APIchunk0.html

EXTRA_DIST = 			\
	xslt.html		\
	EXSLT/exslt.html	\
	libxslt-api.xml		\
	EXSLT/libexslt-api.xml	\
	xsltproc.xml		\
	apibuild.py		\
	libxslt.xsa		\
	news.xsl

man_MANS = xsltproc.1

all:	web ../NEWS libxslt.xsa $(man_MANS)

api:	libxslt-api.xml libxslt-refs.xml $(APIPAGES) html/index.html \
	$(WIN32_DIR)/libxslt.def.src

eapi:	EXSLT/libexslt-api.xml EXSLT/libexslt-refs.xml $(EAPIPAGES) \
	EXSLT/html/index.html $(WIN32_DIR)/libexslt.def.src

web: $(PAGES) $(EPAGES)

$(PAGES): xslt.html $(srcdir)/site.xsl
	-@(if [ -x $(bindir)/xsltproc ] ; then \
	  echo "Rebuilding the HTML Web pages from xslt.html" ; \
	  $(bindir)/xsltproc --nonet --html $(srcdir)/site.xsl \
	    xslt.html > index.html ; fi )
	-@(if [ -x $(bindir)/xmllint ] ; then \
	  echo "Validating the HTML Web pages" ; \
	  $(bindir)/xmllint --nonet --valid --noout $(PAGES) ; fi );
	  
$(EPAGES): EXSLT/exslt.html $(srcdir)/site.xsl
	 -@(if [ ! -d EXSLT/html ] ; then \
	   mkdir -p EXSLT/html ; fi )
	 -@(if [ -x $(bindir)/xsltproc ] ; then \
	   echo "Rebuilding the HTML Web pages from exslt.html" ; \
	   $(bindir)/xsltproc --nonet --html \
	   	--stringparam dirname EXSLT/ \
		--stringparam libname libexslt \
		--stringparam logo_base ../ \
		$(srcdir)/site.xsl \
		EXSLT/exslt.html > EXSLT/index.html ; fi )
	 -@(if [ -x $(bindir)/xmllint ] ; then \
	   echo "Validating the HTML Web pages" ; \
	   $(bindir)/xmllint --nonet --valid --noout $(EPAGES) ; fi );

../NEWS: $(srcdir)/news.xsl news.html
	-@(if [ -x $(bindir)/xsltproc ] ; then \
	  $(bindir)/xsltproc --nonet $(srcdir)/news.xsl \
	    news.html > ../NEWS ; fi );

libxslt.xsa: $(srcdir)/xsa.xsl news.html
	-@(if [ -x $(bindir)/xsltproc ] ; then \
	  $(bindir)/xsltproc --nonet $(srcdir)/xsa.xsl \
	    news.html > libxslt.xsa ; fi );

$(APIPAGES): libxslt-refs.xml site.xsl api.xsl $(srcdir)/site.xsl \
	     $(srcdir)/api.xsl
	-@(if [ -x $(bindir)/xsltproc ] ; then \
	  echo "Rebuilding the HTML API pages from libxslt-refs.xml" ; \
	  $(bindir)/xsltproc --nonet --html \
			     $(srcdir)/api.xsl \
			     xslt.html ; fi );
	-@(if [ -x $(bindir)/xmllint ] ; then \
	  echo "Validating the HTML API pages" ; \
	  $(bindir)/xmllint --nonet --valid --noout API*.html; fi );

$(EAPIPAGES): EXSLT/libexslt-refs.xml site.xsl api.xsl
	-@(if [ -x $(bindir)/xsltproc ] ; then \
	  echo "Rebuilding the HTML API pages from libexslt-refs.xml" ; \
	  $(bindir)/xsltproc --nonet --html --output EXSLT/ \
			     --stringparam libname libexslt \
			     --stringparam dirname EXSLT/ \
			     --stringparam logo_base ../ \
			     $(srcdir)/api.xsl \
			     EXSLT/exslt.html ; fi );

html/index.html: libxslt-api.xml $(srcdir)/newapi.xsl
	-@(if [ -x $(bindir)/xsltproc ] ; then \
	  echo "Rebuilding the HTML pages from the XSLT API" ; \
	  $(bindir)/xsltproc --nonet \
			     $(srcdir)/newapi.xsl libxslt-api.xml ; fi ); 
	-@(if [ -x $(bindir)/xmllint ] ; then \
	  echo "Validating the resulting XHTML pages" ; \
	  $(bindir)/xmllint --nonet --valid --noout html/*.html ; fi );

EXSLT/html/index.html: EXSLT/libexslt-api.xml \
	$(srcdir)/newapi.xsl
	-@(if [ -x $(bindir)/xsltproc ] ; then \
	  echo "Rebuilding the HTML pages from the EXSLT API" ; \
	  $(bindir)/xsltproc --nonet --output EXSLT/ \
	  		     --stringparam libname libexslt \
			     --stringparam dirname EXSLT/ \
			     --stringparam logo_base ../../ \
			     $(srcdir)/newapi.xsl \
			     EXSLT/libexslt-api.xml ; fi );
	-@(if [ -x $(bindir)/xmllint ] ; then \
	  echo "Validating the resulting XHTML pages" ; \
	  $(bindir)/xmllint --nonet --valid --noout EXSLT/html/*.html ; fi );
			     
	
libxslt-api.xml libxslt-refs.xml EXSLT/libexslt-api.xml \
  EXSLT/libexslt-refs.xml: $(srcdir)/apibuild.py $(top_srcdir)/libxslt/*.h \
		$(top_srcdir)/libxslt/*.c $(top_srcdir)/libexslt/*.h \
		$(top_srcdir)/libexslt/*.c
	-($(srcdir)/apibuild.py)
	
xsltproc.1: xsltproc.xml
	-@(xsltproc --nonet xsltproc.xml)

$(WIN32_DIR)/libxslt.def.src: libxslt-api.xml
	-@(if [ -x $(bindir)/xsltproc ] ; then \
	  $(bindir)/xsltproc -o $(WIN32_DIR)/libxslt.def.src \
	  --nonet $(WIN32_DIR)/defgen.xsl libxslt-api.xml ; fi )

$(WIN32_DIR)/libexslt.def.src: EXSLT/libexslt-api.xml
	-@(if [ -x $(bindir)/xsltproc ] ; then \
	  $(bindir)/xsltproc -o $(WIN32_DIR)/libexslt.def.src \
	  --nonet $(WIN32_DIR)/defgen.xsl EXSLT/libexslt-api.xml ; fi )

clean-local:
	rm -f *~ *.bak *.hierarchy *.signals *-unused.txt

maintainer-clean-local: clean

rebuild: api eapi all

install-data-local: 
	$(mkinstalldirs) $(DESTDIR)$(TARGET_DIR)
	-@INSTALL@ -m 0644 $(srcdir)/*.html $(DESTDIR)$(TARGET_DIR)
	-@INSTALL@ -m 0644 $(srcdir)/*.gif $(DESTDIR)$(TARGET_DIR)
	$(mkinstalldirs) $(DESTDIR)$(TARGET_DIR)/html
	-@INSTALL@ -m 0644 $(srcdir)/html/*.html $(DESTDIR)$(TARGET_DIR)/html
	-@INSTALL@ -m 0644 $(srcdir)/html/*.png $(DESTDIR)$(TARGET_DIR)/html
	-@INSTALL@ -m 0644 $(srcdir)/html/index.sgml $(DESTDIR)$(TARGET_DIR)/html
	$(mkinstalldirs) $(DESTDIR)$(TARGET_DIR)/EXSLT
	-@INSTALL@ -m 0644 $(srcdir)/EXSLT/*.html $(DESTDIR)$(TARGET_DIR)/EXSLT
	$(mkinstalldirs) $(DESTDIR)$(TARGET_DIR)/tutorial
	-@INSTALL@ -m 0644 $(srcdir)/tutorial/* $(DESTDIR)$(TARGET_DIR)/tutorial

dist-hook:
	(cd $(srcdir) ; tar cvf - *.1 *.html site.xsl news.xsl xsa.xsl \
	  *.gif html/*.html html/*.png tutorial/libxslt*) | \
	  (cd $(distdir); tar xf -)

.PHONY : html EXSLT EXSLT/html
