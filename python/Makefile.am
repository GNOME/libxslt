SUBDIRS= . tests

LIBS=-L../libxslt/.libs -L../libxslt -lxslt -L../libexslt/.libs -L../libexslt -lexslt $(PYTHON_SITE_PACKAGES)/_libxml.so $(LIBXML_LIBS)
INCLUDES=-I/usr/include/python$(PYTHON_VERSION) -I$(PYTHON_INCLUDES) $(LIBXML_CFLAGS) -I$(top_srcdir)
SHCFLAGS=$(INCLUDES) -Wall -fPIC -g
LINK_FLAGS= -shared
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
all: _libxslt.so libxslt.py $(PYTHONSODV)

libxslt.py: $(srcdir)/libxsl.py libxsltclass.py
	cat $(srcdir)/libxsl.py libxsltclass.py > libxslt.py

_libxslt.so: libxslt.o libxslt-py.o types.o
	$(CC) $(LINK_FLAGS) libxslt.o libxslt-py.o types.o $(LIBS) -o _libxslt.so

_libxslt.so.dv: libxslt.o libxslt-py.o types.o
	rm -f _libxslt.so
	$(CC) $(LINK_FLAGS) libxslt.o libxslt-py.o types.o -L../libxslt/.libs -L../libxslt -lxslt -L../libexslt/.libs -L../libexslt -lexslt $(HOME)/XML/python/_libxml.so $(HOME)/XML/.libs/libxml2.a -o _libxslt.so

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(PYTHON_SITE_PACKAGES)
	-@INSTALL@ -m 0644 libxslt.py $(DESTDIR)$(PYTHON_SITE_PACKAGES)
	-@INSTALL@ -m 0755 _libxslt.so $(DESTDIR)$(PYTHON_SITE_PACKAGES)
	$(mkinstalldirs) $(DESTDIR)$(DOCS_DIR)
	-@(for doc in $(DOCS) ; \
	   do @INSTALL@ -m 0644 $$doc $(DESTDIR)$(DOCS_DIR) ; done)
else
all: 
endif

libxslt.o: libxslt.c libxslt-export.c libxslt_wrap.h
	$(CC) $(SHCFLAGS) -c -o libxslt.o $(srcdir)/libxslt.c

types.o: types.c libxslt_wrap.h
	$(CC) $(SHCFLAGS) -c -o types.o $(srcdir)/types.c

libxslt-py.o: libxslt-py.c libxslt-py.h libxslt_wrap.h
	$(CC) $(SHCFLAGS) -c -o libxslt-py.o $(srcdir)/libxslt-py.c

GENERATE = generator.py
API_DESC = $(top_srcdir)/doc/libxslt-api.xml $(srcdir)/libxslt-python-api.xml
GENERATED= $(srcdir)/libxsltclass.py \
           $(srcdir)/libxslt-export.c \
	   $(srcdir)/libxslt-py.c \
	   $(srcdir)/libxslt-py.h

$(GENERATED): $(srcdir)/$(GENERATE) $(API_DESC)
	cd $(srcdir) && $(PYTHON) $(GENERATE)

tests test: all
	cd tests && $(MAKE) tests

clean:
	rm -f $(GENERATED) *.o _libxslt.so *.pyc libxslt.py


