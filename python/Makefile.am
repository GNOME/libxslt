# Makefile for libxml2 python library
AUTOMAKE_OPTIONS = 1.4 foreign

SUBDIRS= . tests

INCLUDES = \
	-I/usr/include/python$(PYTHON_VERSION) \
	-I$(PYTHON_INCLUDES) \
	 $(LIBXML_CFLAGS) \
	-I$(top_srcdir)

DOCS_DIR = $(prefix)/share/doc/libxslt-python-$(LIBXML_VERSION)
DOCS = TODO libxsltclass.txt

EXTRA_DIST = 			\
	libxslt.c		\
	types.c			\
	generator.py		\
	libxml_wrap.h		\
	libxslt_wrap.h		\
	libxsl.py		\
	libxslt-python-api.xml	\
	$(DOCS)

if WITH_PYTHON
mylibs = \
	$(top_builddir)/libxslt/libxslt.la \
	$(top_builddir)/libexslt/libexslt.la

all: libxsltmod.so libxslt.py $(PYTHONSODV)

LDADD = -lxslt -lexslt
CFLAGS = -Wall -g

pythondir = $(prefix)/lib/python${PYTHON_VERSION}/site-packages
python_PROGRAMS = libxsltmod.so

libxsltmod_so_SOURCES =
libxsltmod_so_LDFLAGS = $(mylibs) $(LIBS) -shared -Wl,-soname,libxsltmod.so

noinst_LTLIBRARIES = libxsltmodule.la
libxsltmodule_la_SOURCES = libxslt.c types.c libxslt-py.c

libxsltmod.so: $(libxsltmodule_la_OBJECTS)
	-@rm -f .libs/libxsltmod.so
	$(LINK) -o $@ $(libxsltmodule_la_OBJECTS) $(libxsltmod_so_LDFLAGS)
	-@(if [ -r .libs/libxsltmod.so ] ; then cp .libs/libxsltmod.so . ; fi)


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

else
all: 
endif
tests test: all
	cd tests && $(MAKE) tests

clean:
	rm -f $(GENERATED) *.o libxslt.so *.pyc libxslt.py


