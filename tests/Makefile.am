## Process this file with automake to produce Makefile.in

SUBDIRS=docs REC1 REC2 REC general namespaces numbers documents \
        xmlspec docbook multiple

all:

test tests: all docbook_tests

docbook_tests:
	@(cd docbook ; $(MAKE) tests)

