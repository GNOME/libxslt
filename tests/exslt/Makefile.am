## Process this file with automake to produce Makefile.in

SUBDIRS=common functions math sets strings date

all:

test tests:
	@(cur=`pwd` ; for dir in $(SUBDIRS) ; do cd $$dir ; make tests ; cd $$cur ; done)
