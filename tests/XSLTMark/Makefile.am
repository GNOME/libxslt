## Process this file with automake to produce Makefile.in
%.out : %

EXTRA_DIST = \
  dbgen.pl \
  alphabetize.out alphabetize.ref alphabetize.xsl \
  attsets.out attsets.ref attsets.xsl \
  avts.out avts.ref avts.xsl \
  axis.out axis.ref axis.xml axis.xsl \
  backwards.out backwards.ref backwards.xsl \
  bottles.out bottles.ref bottles.xml bottles.xsl \
  breadth.out breadth.ref breadth.xml \
  brutal.out brutal.ref brutal.xml brutal.xsl \
  chart.out chart.ref chart.xml chart.xsl \
  creation.out creation.ref creation.xsl \
  current.out current.ref current.xml current.xsl \
  dbonerow.out dbonerow.ref dbonerow.xsl \
  dbtail.out dbtail.ref dbtail.xsl \
  decoy.out decoy.xsl \
  depth.out depth.ref depth.xml \
  encrypt.out encrypt.ref encrypt.xsl \
  find.xsl \
  functions.out functions.ref functions.xsl \
  game.out game.ref game.xml game.xsl \
  gettysburg.xml \
  html.out html.ref html.xml html.xsl \
  identity.out identity.ref identity.xml identity.xsl \
  inventory.out inventory.ref inventory.xml inventory.xsl \
  metric.out metric.ref metric.xml metric.xsl \
  number.out number.xml number.xsl \
  oddtemplate.out oddtemplate.ref oddtemplate.xml oddtemplate.xsl \
  patterns.out patterns.ref patterns.xsl \
  prettyprint.out prettyprint.ref prettyprint.xsl \
  priority.out priority.ref priority.xml priority.xsl \
  products.out products.ref products.xml products.xsl \
  queens.out queens.ref queens.xml queens.xsl \
  reverser.out reverser.ref reverser.xsl \
  stringsort.out stringsort.ref stringsort.xsl \
  summarize.out summarize.ref summarize.xsl \
  total.out total.ref total.xsl \
  tower.out tower.ref tower.xml tower.xsl tower2.xsl \
  trend.out trend.ref trend.xml trend.xsl \
  union.out union.ref union.xml union.xsl \
  xpath.out xpath.ref xpath.xml xpath.xsl \
  xslbench1.out xslbench1.xml xslbench1.xsl \
  xslbench2.out xslbench2.xsl \
  xslbench3.out xslbench3.xsl \
  xslbenchdream.xml

$(top_builddir)/xsltproc/xsltproc:
	@(cd ../../xsltproc ; $(MAKE) xsltproc)

all:

tests test: \
  alphabetize attsets avts axis backwards bottles breadth brutal chart \
  creation current dbonerow dbtail decoy depth encrypt functions game \
  html identity inventory metric number oddtemplate patterns prettyprint \
  priority products queens reverser stringsort summarize total tower trend \
  union xpath xslbench1 xslbench2 xslbench3
	rm -f *.tmp

valgrind:
	@echo '## Running the regression tests under Valgrind'
	$(MAKE) CHECKER='valgrind -q' tests

clean:
	rm -f *.tmp *~ core db100.xml db1000.xml db10000.xml


if WITH_PERL
db100.xml: $(srcdir)/dbgen.pl
	$(PERL) $(srcdir)/dbgen.pl 100 > db100.xml

db1000.xml: $(srcdir)/dbgen.pl
	$(PERL) $(srcdir)/dbgen.pl 1000 > db1000.xml

db10000.xml: $(srcdir)/dbgen.pl
	$(PERL) $(srcdir)/dbgen.pl 10000 > db10000.xml

alphabetize: db100.xml
	@(echo > .memdump)
	@(echo alphabetize)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/alphabetize.xsl db100.xml > alphabetize.tmp; \
	  if [ ! -f $(srcdir)/alphabetize.out ] ; then cp alphabetize.tmp $(srcdir)/alphabetize.out ; \
	  else diff $(srcdir)/alphabetize.out alphabetize.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )


# reference=alphabetize.ref

avts: db100.xml
	@(echo > .memdump)
	@(echo avts)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/avts.xsl db100.xml > avts.tmp; \
	  if [ ! -f $(srcdir)/avts.out ] ; then cp avts.tmp $(srcdir)/avts.out ; \
	  else diff $(srcdir)/avts.out avts.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=avts.ref

creation: db100.xml
	@(echo > .memdump)
	@(echo creation)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/creation.xsl db100.xml > creation.tmp; \
	  if [ ! -f $(srcdir)/creation.out ] ; then cp creation.tmp $(srcdir)/creation.out ; \
	  else diff $(srcdir)/creation.out creation.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=creation.ref

dbonerow: db10000.xml
	@(echo > .memdump)
	@(echo dbonerow)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/dbonerow.xsl db10000.xml > dbonerow.tmp; \
	  if [ ! -f $(srcdir)/dbonerow.out ] ; then cp dbonerow.tmp $(srcdir)/dbonerow.out ; \
	  else diff $(srcdir)/dbonerow.out dbonerow.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=dbonerow.ref

dbtail: db100.xml
	@(echo > .memdump)
	@(echo dbtail)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/dbtail.xsl db100.xml > dbtail.tmp; \
	  if [ ! -f $(srcdir)/dbtail.out ] ; then cp dbtail.tmp $(srcdir)/dbtail.out ; \
	  else diff $(srcdir)/dbtail.out dbtail.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=dbtail.ref

decoy: db100.xml
	@(echo > .memdump)
	@(echo decoy)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/decoy.xsl db100.xml > decoy.tmp; \
	  if [ ! -f $(srcdir)/decoy.out ] ; then cp decoy.tmp $(srcdir)/decoy.out ; \
	  else diff $(srcdir)/decoy.out decoy.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=patterns.ref

encrypt: db100.xml
	@(echo > .memdump)
	@(echo encrypt)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/encrypt.xsl db100.xml > encrypt.tmp; \
	  if [ ! -f $(srcdir)/encrypt.out ] ; then cp encrypt.tmp $(srcdir)/encrypt.out ; \
	  else diff $(srcdir)/encrypt.out encrypt.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=encrypt.ref

functions: db100.xml
	@(echo > .memdump)
	@(echo functions)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/functions.xsl db100.xml > functions.tmp; \
	  if [ ! -f $(srcdir)/functions.out ] ; then cp functions.tmp $(srcdir)/functions.out ; \
	  else diff $(srcdir)/functions.out functions.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=functions.ref

identity: db1000.xml
	@(echo > .memdump)
	@(echo identity)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/identity.xsl db1000.xml > identity.tmp; \
	  if [ ! -f $(srcdir)/identity.out ] ; then cp identity.tmp $(srcdir)/identity.out ; \
	  else diff $(srcdir)/identity.out identity.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=identity.ref

inventory:
	@(echo > .memdump)
	@(echo inventory)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/inventory.xsl $(srcdir)/inventory.xml > inventory.tmp; \
	  if [ ! -f $(srcdir)/inventory.out ] ; then cp inventory.tmp $(srcdir)/inventory.out ; \
	  else diff $(srcdir)/inventory.out inventory.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=inventory.ref

patterns: db100.xml
	@(echo > .memdump)
	@(echo patterns)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/patterns.xsl db100.xml > patterns.tmp; \
	  if [ ! -f $(srcdir)/patterns.out ] ; then cp patterns.tmp $(srcdir)/patterns.out ; \
	  else diff $(srcdir)/patterns.out patterns.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=patterns.ref

prettyprint: db100.xml
	@(echo > .memdump)
	@(echo prettyprint)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/prettyprint.xsl db100.xml > prettyprint.tmp; \
	  if [ ! -f $(srcdir)/prettyprint.out ] ; then cp prettyprint.tmp $(srcdir)/prettyprint.out ; \
	  else diff $(srcdir)/prettyprint.out prettyprint.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=prettyprint.ref

stringsort: db1000.xml
	@(echo > .memdump)
	@(echo stringsort)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/stringsort.xsl db1000.xml > stringsort.tmp; \
	  if [ ! -f $(srcdir)/stringsort.out ] ; then cp stringsort.tmp $(srcdir)/stringsort.out ; \
	  else diff $(srcdir)/stringsort.out stringsort.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=stringsort.ref

endif

attsets:
	@(echo > .memdump)
	@(echo attsets)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/attsets.xsl $(srcdir)/chart.xml > attsets.tmp; \
	  if [ ! -f $(srcdir)/attsets.out ] ; then cp attsets.tmp $(srcdir)/attsets.out ; \
	  else diff $(srcdir)/attsets.out attsets.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=attsets.ref

axis:
	@(echo > .memdump)
	@(echo axis)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/axis.xsl $(srcdir)/axis.xml > axis.tmp; \
	  if [ ! -f $(srcdir)/axis.out ] ; then cp axis.tmp $(srcdir)/axis.out ; \
	  else diff $(srcdir)/axis.out axis.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=axis.ref

backwards:
	@(echo > .memdump)
	@(echo backwards)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/backwards.xsl $(srcdir)/game.xml > backwards.tmp; \
	  if [ ! -f $(srcdir)/backwards.out ] ; then cp backwards.tmp $(srcdir)/backwards.out ; \
	  else diff $(srcdir)/backwards.out backwards.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=backwards.ref

bottles:
	@(echo > .memdump)
	@(echo bottles)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/bottles.xsl $(srcdir)/bottles.xml > bottles.tmp; \
	  if [ ! -f $(srcdir)/bottles.out ] ; then cp bottles.tmp $(srcdir)/bottles.out ; \
	  else diff $(srcdir)/bottles.out bottles.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=bottles.ref

breadth:
	@(echo > .memdump)
	@(echo breadth)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/find.xsl $(srcdir)/breadth.xml > breadth.tmp; \
	  if [ ! -f $(srcdir)/breadth.out ] ; then cp breadth.tmp $(srcdir)/breadth.out ; \
	  else diff $(srcdir)/breadth.out breadth.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=breadth.ref

brutal:
	@(echo > .memdump)
	@(echo brutal)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/brutal.xsl $(srcdir)/brutal.xml > brutal.tmp; \
	  if [ ! -f $(srcdir)/brutal.out ] ; then cp brutal.tmp $(srcdir)/brutal.out ; \
	  else diff $(srcdir)/brutal.out brutal.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=brutal.ref

chart:
	@(echo > .memdump)
	@(echo chart)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/chart.xsl $(srcdir)/chart.xml > chart.tmp; \
	  if [ ! -f $(srcdir)/chart.out ] ; then cp chart.tmp $(srcdir)/chart.out ; \
	  else diff $(srcdir)/chart.out chart.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=chart.ref

current:
	@(echo > .memdump)
	@(echo current)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/current.xsl $(srcdir)/current.xml > current.tmp; \
	  if [ ! -f $(srcdir)/current.out ] ; then cp current.tmp $(srcdir)/current.out ; \
	  else diff $(srcdir)/current.out current.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=current.ref

depth:
	@(echo > .memdump)
	@(echo depth)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/find.xsl $(srcdir)/depth.xml > depth.tmp; \
	  if [ ! -f $(srcdir)/depth.out ] ; then cp depth.tmp $(srcdir)/depth.out ; \
	  else diff $(srcdir)/depth.out depth.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=depth.ref

game:
	@(echo > .memdump)
	@(echo game)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/game.xsl $(srcdir)/game.xml > game.tmp; \
	  if [ ! -f $(srcdir)/game.out ] ; then cp game.tmp $(srcdir)/game.out ; \
	  else diff $(srcdir)/game.out game.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=game.ref

html:
	@(echo > .memdump)
	@(echo html)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/html.xsl $(srcdir)/html.xml > html.tmp; \
	  if [ ! -f $(srcdir)/html.out ] ; then cp html.tmp $(srcdir)/html.out ; \
	  else diff $(srcdir)/html.out html.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=html.ref

metric:
	@(echo > .memdump)
	@(echo metric)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/metric.xsl $(srcdir)/metric.xml > metric.tmp; \
	  if [ ! -f $(srcdir)/metric.out ] ; then cp metric.tmp $(srcdir)/metric.out ; \
	  else diff $(srcdir)/metric.out metric.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=metric.ref

number:
	@(echo > .memdump)
	@(echo number)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/number.xsl $(srcdir)/number.xml > number.tmp; \
	  if [ ! -f $(srcdir)/number.out ] ; then cp number.tmp $(srcdir)/number.out ; \
	  else diff $(srcdir)/number.out number.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )
iterations=100
skipdriver=ExternalDriver:txdriver

oddtemplate:
	@(echo > .memdump)
	@(echo oddtemplate)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/oddtemplate.xsl $(srcdir)/oddtemplate.xml > oddtemplate.tmp; \
	  if [ ! -f $(srcdir)/oddtemplate.out ] ; then cp oddtemplate.tmp $(srcdir)/oddtemplate.out ; \
	  else diff $(srcdir)/oddtemplate.out oddtemplate.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=oddtemplate.ref

priority:
	@(echo > .memdump)
	@(echo priority)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/priority.xsl $(srcdir)/priority.xml > priority.tmp; \
	  if [ ! -f $(srcdir)/priority.out ] ; then cp priority.tmp $(srcdir)/priority.out ; \
	  else diff $(srcdir)/priority.out priority.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=priority.ref

products:
	@(echo > .memdump)
	@(echo products)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/products.xsl $(srcdir)/products.xml > products.tmp; \
	  if [ ! -f $(srcdir)/products.out ] ; then cp products.tmp $(srcdir)/products.out ; \
	  else diff $(srcdir)/products.out products.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=products.ref

queens:
	@(echo > .memdump)
	@(echo queens)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/queens.xsl $(srcdir)/queens.xml > queens.tmp; \
	  if [ ! -f $(srcdir)/queens.out ] ; then cp queens.tmp $(srcdir)/queens.out ; \
	  else diff $(srcdir)/queens.out queens.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=queens.ref

reverser:
	@(echo > .memdump)
	@(echo reverser)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/reverser.xsl $(srcdir)/gettysburg.xml > reverser.tmp; \
	  if [ ! -f $(srcdir)/reverser.out ] ; then cp reverser.tmp $(srcdir)/reverser.out ; \
	  else diff $(srcdir)/reverser.out reverser.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=reverser.ref

summarize:
	@(echo > .memdump)
	@(echo summarize)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/summarize.xsl $(srcdir)/queens.xsl > summarize.tmp; \
	  if [ ! -f $(srcdir)/summarize.out ] ; then cp summarize.tmp $(srcdir)/summarize.out ; \
	  else diff $(srcdir)/summarize.out summarize.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=summarize.ref

total:
	@(echo > .memdump)
	@(echo total)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/total.xsl $(srcdir)/chart.xml > total.tmp; \
	  if [ ! -f $(srcdir)/total.out ] ; then cp total.tmp $(srcdir)/total.out ; \
	  else diff $(srcdir)/total.out total.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=total.ref

tower:
	@(echo > .memdump)
	@(echo tower)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/tower.xsl $(srcdir)/tower.xml > tower.tmp; \
	  if [ ! -f $(srcdir)/tower.out ] ; then cp tower.tmp $(srcdir)/tower.out ; \
	  else diff $(srcdir)/tower.out tower.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=tower.ref

trend:
	@(echo > .memdump)
	@(echo trend)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/trend.xsl $(srcdir)/trend.xml > trend.tmp; \
	  if [ ! -f $(srcdir)/trend.out ] ; then cp trend.tmp $(srcdir)/trend.out ; \
	  else diff $(srcdir)/trend.out trend.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=trend.ref

union:
	@(echo > .memdump)
	@(echo union)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/union.xsl $(srcdir)/union.xml > union.tmp; \
	  if [ ! -f $(srcdir)/union.out ] ; then cp union.tmp $(srcdir)/union.out ; \
	  else diff $(srcdir)/union.out union.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=union.ref

xpath:
	@(echo > .memdump)
	@(echo xpath)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/xpath.xsl $(srcdir)/xpath.xml > xpath.tmp; \
	  if [ ! -f $(srcdir)/xpath.out ] ; then cp xpath.tmp $(srcdir)/xpath.out ; \
	  else diff $(srcdir)/xpath.out xpath.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

# reference=xpath.ref

xslbench1:
	@(echo > .memdump)
	@(echo xslbench1)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/xslbench1.xsl $(srcdir)/xslbench1.xml > xslbench1.tmp; \
	  if [ ! -f $(srcdir)/xslbench1.out ] ; then cp xslbench1.tmp $(srcdir)/xslbench1.out ; \
	  else diff $(srcdir)/xslbench1.out xslbench1.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

xslbench2:
	@(echo > .memdump)
	@(echo xslbench2)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/xslbench2.xsl $(srcdir)/xslbenchdream.xml > xslbench2.tmp; \
	  if [ ! -f $(srcdir)/xslbench2.out ] ; then cp xslbench2.tmp $(srcdir)/xslbench2.out ; \
	  else diff $(srcdir)/xslbench2.out xslbench2.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )

xslbench3:
	@(echo > .memdump)
	@(echo xslbench3)
	@($(CHECKER) $(top_builddir)/xsltproc/xsltproc $(srcdir)/xslbench3.xsl $(srcdir)/xslbenchdream.xml > xslbench3.tmp; \
	  if [ ! -f $(srcdir)/xslbench3.out ] ; then cp xslbench3.tmp $(srcdir)/xslbench3.out ; \
	  else diff $(srcdir)/xslbench3.out xslbench3.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" || true )



