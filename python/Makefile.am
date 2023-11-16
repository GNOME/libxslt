# Makefile for libxslt python library

# We use a rule with multiple output files which creates problems with
# parallel builds.
.NOTPARALLEL:

SUBDIRS = . tests

EXTRA_DIST = 			\
	libxslt.c		\
	types.c			\
	generator.py		\
	libxml_wrap.h		\
	libxslt_wrap.h		\
	libxsl.py		\
	libxslt-python-api.xml	\
	pyproject.toml		\
	setup.py.in

if WITH_PYTHON

pyexec_LTLIBRARIES = libxsltmod.la

libxsltmod_la_CPPFLAGS = \
	-I$(top_srcdir)/libxslt \
	-I$(top_srcdir) \
	-I../libexslt
libxsltmod_la_CFLAGS = $(AM_CFLAGS) $(PYTHON_CFLAGS)
libxsltmod_la_SOURCES = libxslt.c types.c
nodist_libxsltmod_la_SOURCES = libxslt-py.c
libxsltmod_la_LIBADD = \
	$(top_builddir)/libxslt/libxslt.la \
	$(top_builddir)/libexslt/libexslt.la \
	$(PYTHON_LIBS) $(LIBXML_LIBS)
libxsltmod_la_LDFLAGS = $(AM_LDFLAGS) $(PYTHON_LDFLAGS) -module -avoid-version

libxslt.py: $(srcdir)/libxsl.py libxsltclass.py
	cat $(srcdir)/libxsl.py libxsltclass.py > $@

nodist_python_PYTHON = libxslt.py

GENERATE = generator.py
API_DESC = $(top_srcdir)/doc/libxslt-api.xml $(srcdir)/libxslt-python-api.xml
GENERATED = libxsltclass.py \
	   libxslt-export.c \
	   libxslt-py.c \
	   libxslt-py.h \
	   libxsltclass.txt

# Generated sources
all-local: libxslt.py

$(GENERATED): gen_prog

gen_prog: $(srcdir)/$(GENERATE) $(API_DESC)
	SRCDIR=$(srcdir) $(PYTHON) $(srcdir)/$(GENERATE)
	touch gen_prog

$(libxsltmod_la_OBJECTS): $(GENERATED)

endif

tests test: all
	cd tests && $(MAKE) tests

CLEANFILES = $(GENERATED) *.pyc libxslt.py gen_prog

clean-local:
	rm -rf __pycache__
