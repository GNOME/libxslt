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

$(top_builddir)/libxslt/xsltproc:
	@(cd ../../libxslt ; make xsltproc)

all: alphabetize attsets avts axis backwards bottles breadth brutal chart \
  creation current dbonerow dbtail decoy depth encrypt functions game \
  html identity inventory metric number oddtemplate patterns prettyprint \
  priority products queens reverser stringsort summarize total tower trend \
  union xpath xslbench1 xslbench2 xslbench3
	rm -f *.tmp

clean:
	rm -f *.tmp *~ core db100.xml db1000.xml db10000.xml

db100.xml: $(srcdir)/dbgen.pl
	$(srcdir)/dbgen.pl 100 > db100.xml

db1000.xml: $(srcdir)/dbgen.pl
	$(srcdir)/dbgen.pl 1000 > db1000.xml

db10000.xml: $(srcdir)/dbgen.pl
	$(srcdir)/dbgen.pl 10000 > db10000.xml

alphabetize: db100.xml
	@(rm -f .memdump ; touch .memdump)
	@(echo alphabetize)
	@(xsltproc alphabetize.xsl db100.xml > alphabetize.tmp; \
	  if [ ! -f $(srcdir)/alphabetize.out ] ; then cp alphabetize.tmp $(srcdir)/alphabetize.out ; \
	  else diff $(srcdir)/alphabetize.out alphabetize.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=alphabetize.ref

attsets:
	@(rm -f .memdump ; touch .memdump)
	@(echo attsets)
	@(xsltproc attsets.xsl chart.xml > attsets.tmp; \
	  if [ ! -f $(srcdir)/attsets.out ] ; then cp attsets.tmp $(srcdir)/attsets.out ; \
	  else diff $(srcdir)/attsets.out attsets.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=attsets.ref

avts: db100.xml
	@(rm -f .memdump ; touch .memdump)
	@(echo avts)
	@(xsltproc avts.xsl db100.xml > avts.tmp; \
	  if [ ! -f $(srcdir)/avts.out ] ; then cp avts.tmp $(srcdir)/avts.out ; \
	  else diff $(srcdir)/avts.out avts.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=avts.ref

axis:
	@(rm -f .memdump ; touch .memdump)
	@(echo axis)
	@(xsltproc axis.xsl axis.xml > axis.tmp; \
	  if [ ! -f $(srcdir)/axis.out ] ; then cp axis.tmp $(srcdir)/axis.out ; \
	  else diff $(srcdir)/axis.out axis.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=axis.ref

backwards:
	@(rm -f .memdump ; touch .memdump)
	@(echo backwards)
	@(xsltproc backwards.xsl game.xml > backwards.tmp; \
	  if [ ! -f $(srcdir)/backwards.out ] ; then cp backwards.tmp $(srcdir)/backwards.out ; \
	  else diff $(srcdir)/backwards.out backwards.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=backwards.ref

bottles:
	@(rm -f .memdump ; touch .memdump)
	@(echo bottles)
	@(xsltproc bottles.xsl bottles.xml > bottles.tmp; \
	  if [ ! -f $(srcdir)/bottles.out ] ; then cp bottles.tmp $(srcdir)/bottles.out ; \
	  else diff $(srcdir)/bottles.out bottles.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=bottles.ref

breadth:
	@(rm -f .memdump ; touch .memdump)
	@(echo breadth)
	@(xsltproc find.xsl breadth.xml > breadth.tmp; \
	  if [ ! -f $(srcdir)/breadth.out ] ; then cp breadth.tmp $(srcdir)/breadth.out ; \
	  else diff $(srcdir)/breadth.out breadth.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=breadth.ref

brutal:
	@(rm -f .memdump ; touch .memdump)
	@(echo brutal)
	@(xsltproc brutal.xsl brutal.xml > brutal.tmp; \
	  if [ ! -f $(srcdir)/brutal.out ] ; then cp brutal.tmp $(srcdir)/brutal.out ; \
	  else diff $(srcdir)/brutal.out brutal.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=brutal.ref

chart:
	@(rm -f .memdump ; touch .memdump)
	@(echo chart)
	@(xsltproc chart.xsl chart.xml > chart.tmp; \
	  if [ ! -f $(srcdir)/chart.out ] ; then cp chart.tmp $(srcdir)/chart.out ; \
	  else diff $(srcdir)/chart.out chart.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=chart.ref

creation: db100.xml
	@(rm -f .memdump ; touch .memdump)
	@(echo creation)
	@(xsltproc creation.xsl db100.xml > creation.tmp; \
	  if [ ! -f $(srcdir)/creation.out ] ; then cp creation.tmp $(srcdir)/creation.out ; \
	  else diff $(srcdir)/creation.out creation.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=creation.ref

current:
	@(rm -f .memdump ; touch .memdump)
	@(echo current)
	@(xsltproc current.xsl current.xml > current.tmp; \
	  if [ ! -f $(srcdir)/current.out ] ; then cp current.tmp $(srcdir)/current.out ; \
	  else diff $(srcdir)/current.out current.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=current.ref

dbonerow: db10000.xml
	@(rm -f .memdump ; touch .memdump)
	@(echo dbonerow)
	@(xsltproc dbonerow.xsl db10000.xml > dbonerow.tmp; \
	  if [ ! -f $(srcdir)/dbonerow.out ] ; then cp dbonerow.tmp $(srcdir)/dbonerow.out ; \
	  else diff $(srcdir)/dbonerow.out dbonerow.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=dbonerow.ref

dbtail: db100.xml
	@(rm -f .memdump ; touch .memdump)
	@(echo dbtail)
	@(xsltproc dbtail.xsl db100.xml > dbtail.tmp; \
	  if [ ! -f $(srcdir)/dbtail.out ] ; then cp dbtail.tmp $(srcdir)/dbtail.out ; \
	  else diff $(srcdir)/dbtail.out dbtail.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=dbtail.ref

decoy: db100.xml
	@(rm -f .memdump ; touch .memdump)
	@(echo decoy)
	@(xsltproc decoy.xsl db100.xml > decoy.tmp; \
	  if [ ! -f $(srcdir)/decoy.out ] ; then cp decoy.tmp $(srcdir)/decoy.out ; \
	  else diff $(srcdir)/decoy.out decoy.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=patterns.ref

depth:
	@(rm -f .memdump ; touch .memdump)
	@(echo depth)
	@(xsltproc find.xsl depth.xml > depth.tmp; \
	  if [ ! -f $(srcdir)/depth.out ] ; then cp depth.tmp $(srcdir)/depth.out ; \
	  else diff $(srcdir)/depth.out depth.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=depth.ref

encrypt: db100.xml
	@(rm -f .memdump ; touch .memdump)
	@(echo encrypt)
	@(xsltproc encrypt.xsl db100.xml > encrypt.tmp; \
	  if [ ! -f $(srcdir)/encrypt.out ] ; then cp encrypt.tmp $(srcdir)/encrypt.out ; \
	  else diff $(srcdir)/encrypt.out encrypt.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=encrypt.ref

functions: db100.xml
	@(rm -f .memdump ; touch .memdump)
	@(echo functions)
	@(xsltproc functions.xsl db100.xml > functions.tmp; \
	  if [ ! -f $(srcdir)/functions.out ] ; then cp functions.tmp $(srcdir)/functions.out ; \
	  else diff $(srcdir)/functions.out functions.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=functions.ref

game:
	@(rm -f .memdump ; touch .memdump)
	@(echo game)
	@(xsltproc game.xsl game.xml > game.tmp; \
	  if [ ! -f $(srcdir)/game.out ] ; then cp game.tmp $(srcdir)/game.out ; \
	  else diff $(srcdir)/game.out game.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=game.ref

html:
	@(rm -f .memdump ; touch .memdump)
	@(echo html)
	@(xsltproc html.xsl html.xml > html.tmp; \
	  if [ ! -f $(srcdir)/html.out ] ; then cp html.tmp $(srcdir)/html.out ; \
	  else diff $(srcdir)/html.out html.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=html.ref

identity: db1000.xml
	@(rm -f .memdump ; touch .memdump)
	@(echo identity)
	@(xsltproc identity.xsl db1000.xml > identity.tmp; \
	  if [ ! -f $(srcdir)/identity.out ] ; then cp identity.tmp $(srcdir)/identity.out ; \
	  else diff $(srcdir)/identity.out identity.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=identity.ref

inventory:
	@(rm -f .memdump ; touch .memdump)
	@(echo inventory)
	@(xsltproc inventory.xsl inventory.xml > inventory.tmp; \
	  if [ ! -f $(srcdir)/inventory.out ] ; then cp inventory.tmp $(srcdir)/inventory.out ; \
	  else diff $(srcdir)/inventory.out inventory.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=inventory.ref

metric:
	@(rm -f .memdump ; touch .memdump)
	@(echo metric)
	@(xsltproc metric.xsl metric.xml > metric.tmp; \
	  if [ ! -f $(srcdir)/metric.out ] ; then cp metric.tmp $(srcdir)/metric.out ; \
	  else diff $(srcdir)/metric.out metric.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=metric.ref

number:
	@(rm -f .memdump ; touch .memdump)
	@(echo number)
	@(xsltproc number.xsl number.xml > number.tmp; \
	  if [ ! -f $(srcdir)/number.out ] ; then cp number.tmp $(srcdir)/number.out ; \
	  else diff $(srcdir)/number.out number.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )
iterations=100
skipdriver=ExternalDriver:txdriver

oddtemplate:
	@(rm -f .memdump ; touch .memdump)
	@(echo oddtemplate)
	@(xsltproc oddtemplate.xsl oddtemplate.xml > oddtemplate.tmp; \
	  if [ ! -f $(srcdir)/oddtemplate.out ] ; then cp oddtemplate.tmp $(srcdir)/oddtemplate.out ; \
	  else diff $(srcdir)/oddtemplate.out oddtemplate.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=oddtemplate.ref

patterns: db100.xml
	@(rm -f .memdump ; touch .memdump)
	@(echo patterns)
	@(xsltproc patterns.xsl db100.xml > patterns.tmp; \
	  if [ ! -f $(srcdir)/patterns.out ] ; then cp patterns.tmp $(srcdir)/patterns.out ; \
	  else diff $(srcdir)/patterns.out patterns.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=patterns.ref

prettyprint: db100.xml
	@(rm -f .memdump ; touch .memdump)
	@(echo prettyprint)
	@(xsltproc prettyprint.xsl db100.xml > prettyprint.tmp; \
	  if [ ! -f $(srcdir)/prettyprint.out ] ; then cp prettyprint.tmp $(srcdir)/prettyprint.out ; \
	  else diff $(srcdir)/prettyprint.out prettyprint.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=prettyprint.ref

priority:
	@(rm -f .memdump ; touch .memdump)
	@(echo priority)
	@(xsltproc priority.xsl priority.xml > priority.tmp; \
	  if [ ! -f $(srcdir)/priority.out ] ; then cp priority.tmp $(srcdir)/priority.out ; \
	  else diff $(srcdir)/priority.out priority.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=priority.ref

products:
	@(rm -f .memdump ; touch .memdump)
	@(echo products)
	@(xsltproc products.xsl products.xml > products.tmp; \
	  if [ ! -f $(srcdir)/products.out ] ; then cp products.tmp $(srcdir)/products.out ; \
	  else diff $(srcdir)/products.out products.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=products.ref

queens:
	@(rm -f .memdump ; touch .memdump)
	@(echo queens)
	@(xsltproc queens.xsl queens.xml > queens.tmp; \
	  if [ ! -f $(srcdir)/queens.out ] ; then cp queens.tmp $(srcdir)/queens.out ; \
	  else diff $(srcdir)/queens.out queens.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=queens.ref

reverser:
	@(rm -f .memdump ; touch .memdump)
	@(echo reverser)
	@(xsltproc reverser.xsl gettysburg.xml > reverser.tmp; \
	  if [ ! -f $(srcdir)/reverser.out ] ; then cp reverser.tmp $(srcdir)/reverser.out ; \
	  else diff $(srcdir)/reverser.out reverser.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=reverser.ref

stringsort: db1000.xml
	@(rm -f .memdump ; touch .memdump)
	@(echo stringsort)
	@(xsltproc stringsort.xsl db1000.xml > stringsort.tmp; \
	  if [ ! -f $(srcdir)/stringsort.out ] ; then cp stringsort.tmp $(srcdir)/stringsort.out ; \
	  else diff $(srcdir)/stringsort.out stringsort.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=stringsort.ref

summarize:
	@(rm -f .memdump ; touch .memdump)
	@(echo summarize)
	@(xsltproc summarize.xsl queens.xsl > summarize.tmp; \
	  if [ ! -f $(srcdir)/summarize.out ] ; then cp summarize.tmp $(srcdir)/summarize.out ; \
	  else diff $(srcdir)/summarize.out summarize.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=summarize.ref

total:
	@(rm -f .memdump ; touch .memdump)
	@(echo total)
	@(xsltproc total.xsl chart.xml > total.tmp; \
	  if [ ! -f $(srcdir)/total.out ] ; then cp total.tmp $(srcdir)/total.out ; \
	  else diff $(srcdir)/total.out total.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=total.ref

tower:
	@(rm -f .memdump ; touch .memdump)
	@(echo tower)
	@(xsltproc tower.xsl tower.xml > tower.tmp; \
	  if [ ! -f $(srcdir)/tower.out ] ; then cp tower.tmp $(srcdir)/tower.out ; \
	  else diff $(srcdir)/tower.out tower.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=tower.ref

trend:
	@(rm -f .memdump ; touch .memdump)
	@(echo trend)
	@(xsltproc trend.xsl trend.xml > trend.tmp; \
	  if [ ! -f $(srcdir)/trend.out ] ; then cp trend.tmp $(srcdir)/trend.out ; \
	  else diff $(srcdir)/trend.out trend.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=trend.ref

union:
	@(rm -f .memdump ; touch .memdump)
	@(echo union)
	@(xsltproc union.xsl union.xml > union.tmp; \
	  if [ ! -f $(srcdir)/union.out ] ; then cp union.tmp $(srcdir)/union.out ; \
	  else diff $(srcdir)/union.out union.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=union.ref

xpath:
	@(rm -f .memdump ; touch .memdump)
	@(echo xpath)
	@(xsltproc xpath.xsl xpath.xml > xpath.tmp; \
	  if [ ! -f $(srcdir)/xpath.out ] ; then cp xpath.tmp $(srcdir)/xpath.out ; \
	  else diff $(srcdir)/xpath.out xpath.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=xpath.ref

xslbench1:
	@(rm -f .memdump ; touch .memdump)
	@(echo xslbench1)
	@(xsltproc xslbench1.xsl xslbench1.xml > xslbench1.tmp; \
	  if [ ! -f $(srcdir)/xslbench1.out ] ; then cp xslbench1.tmp $(srcdir)/xslbench1.out ; \
	  else diff $(srcdir)/xslbench1.out xslbench1.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

xslbench2:
	@(rm -f .memdump ; touch .memdump)
	@(echo xslbench2)
	@(xsltproc xslbench2.xsl xslbenchdream.xml > xslbench2.tmp; \
	  if [ ! -f $(srcdir)/xslbench2.out ] ; then cp xslbench2.tmp $(srcdir)/xslbench2.out ; \
	  else diff $(srcdir)/xslbench2.out xslbench2.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

xslbench3:
	@(rm -f .memdump ; touch .memdump)
	@(echo xslbench3)
	@(xsltproc xslbench3.xsl xslbenchdream.xml > xslbench3.tmp; \
	  if [ ! -f $(srcdir)/xslbench3.out ] ; then cp xslbench3.tmp $(srcdir)/xslbench3.out ; \
	  else diff $(srcdir)/xslbench3.out xslbench3.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )



