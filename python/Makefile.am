# Makefile for libxml2 python library
AUTOMAKE_OPTIONS = 1.4 foreign

SUBDIRS= . tests

INCLUDES = \
	-I$(PYTHON_INCLUDES) \
	 $(LIBXML_CFLAGS) \
	-I$(top_srcdir)

DOCS_DIR = $(prefix)/share/doc/libxslt-python-$(LIBXML_VERSION)
# libxsltclass.txt is generated
DOCS = TODO

EXTRA_DIST = 			\
	libxslt.c		\
	types.c			\
	generator.py		\
	libxml_wrap.h		\
	libxslt_wrap.h		\
	libxsl.py		\
	libxslt-python-api.xml	\
	$(DOCS)

libxsltmod_la_LDFLAGS = -module -avoid-version

if WITH_PYTHON
mylibs = \
	$(top_builddir)/libxslt/libxslt.la \
	$(top_builddir)/libexslt/libexslt.la

all: libxslt.py

pythondir = $(prefix)/lib/python${PYTHON_VERSION}/site-packages
python_LTLIBRARIES = libxsltmod.la

libxsltmod_la_SOURCES = libxslt.c types.c libxslt-py.c
libxsltmod_la_LIBADD = $(mylibs) 

libxslt.py: $(srcdir)/libxsl.py libxsltclass.py
	cat $(srcdir)/libxsl.py libxsltclass.py > libxslt.py

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(PYTHON_SITE_PACKAGES)
	-@INSTALL@ -m 0644 libxslt.py $(DESTDIR)$(PYTHON_SITE_PACKAGES)
	$(mkinstalldirs) $(DESTDIR)$(DOCS_DIR)
	-@(for doc in $(DOCS) ; \
	   do @INSTALL@ -m 0644 $$doc $(DESTDIR)$(DOCS_DIR) ; done)

GENERATE = generator.py
API_DESC = $(top_srcdir)/doc/libxslt-api.xml $(srcdir)/libxslt-python-api.xml
GENERATED= $(srcdir)/libxsltclass.py \
           $(srcdir)/libxslt-export.c \
	   $(srcdir)/libxslt-py.c \
	   $(srcdir)/libxslt-py.h

$(GENERATED): $(srcdir)/$(GENERATE) $(API_DESC)
	cd $(srcdir) && $(PYTHON) $(GENERATE)

$(libxsltmod_la_OBJECTS): $(GENERATED)

else
all: 
endif
tests test: all
	cd tests && $(MAKE) tests

clean:
	rm -f $(GENERATED) *.o libxslt.so *.pyc libxslt.py

