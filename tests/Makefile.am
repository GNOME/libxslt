## Process this file with automake to produce Makefile.in

SUBDIRS=docs REC1 REC2 REC general namespaces keys numbers documents \
        extensions reports xmlspec multiple XSLTMark docbook exslt

all:

test tests:
	@(cur=`pwd` ; for dir in $(SUBDIRS) ; do cd $$dir ; $(MAKE) CHECKER='$(CHECKER)' tests ; cd $$cur ; done)

valgrind:
	@echo '## Running the regression tests under Valgrind'
	@echo '## Go get a cup of coffee it is gonna take a while ...'
	$(MAKE) CHECKER='valgrind -q' tests

full: tests docbook_tests

docbook_tests:
	@(cd docbook ; $(MAKE) full)

