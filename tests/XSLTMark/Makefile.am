## Process this file with automake to produce Makefile.in
TESTS= \
  alphabetize attsets avts axis backwards bottles breadth brutal chart \
  creation current dbonerow dbtail decoy depth encrypt functions game \
  html identity inventory metric number oddtemplate patterns prettyprint \
  priority products queens reverser stringsort summarize total tower trend \
  union xpath xslbench1 xslbench2 xslbench3

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

all: $(TESTS)
	rm -f *.tmp

clean:
	rm -f *.tmp *~ core db100.xml db1000.xml db10000.xml

db100.xml: dbgen.pl
	./dbgen.pl 100 > db100.xml

db1000.xml: dbgen.pl
	./dbgen.pl 1000 > db1000.xml

db10000.xml: dbgen.pl
	./dbgen.pl 10000 > db10000.xml

alphabetize: db100.xml
	@(rm -f .memdump ; touch .memdump)
	@(echo alphabetize)
	@(xsltproc alphabetize.xsl db100.xml > alphabetize.tmp; \
	  if [ ! -f alphabetize.out ] ; then cp alphabetize.tmp alphabetize.out ; \
	  else diff alphabetize.out alphabetize.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=alphabetize.ref

attsets:
	@(rm -f .memdump ; touch .memdump)
	@(echo attsets)
	@(xsltproc attsets.xsl chart.xml > attsets.tmp; \
	  if [ ! -f attsets.out ] ; then cp attsets.tmp attsets.out ; \
	  else diff attsets.out attsets.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=attsets.ref

avts: db100.xml
	@(rm -f .memdump ; touch .memdump)
	@(echo avts)
	@(xsltproc avts.xsl db100.xml > avts.tmp; \
	  if [ ! -f avts.out ] ; then cp avts.tmp avts.out ; \
	  else diff avts.out avts.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=avts.ref

axis:
	@(rm -f .memdump ; touch .memdump)
	@(echo axis)
	@(xsltproc axis.xsl axis.xml > axis.tmp; \
	  if [ ! -f axis.out ] ; then cp axis.tmp axis.out ; \
	  else diff axis.out axis.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=axis.ref

backwards:
	@(rm -f .memdump ; touch .memdump)
	@(echo backwards)
	@(xsltproc backwards.xsl game.xml > backwards.tmp; \
	  if [ ! -f backwards.out ] ; then cp backwards.tmp backwards.out ; \
	  else diff backwards.out backwards.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=backwards.ref

bottles:
	@(rm -f .memdump ; touch .memdump)
	@(echo bottles)
	@(xsltproc bottles.xsl bottles.xml > bottles.tmp; \
	  if [ ! -f bottles.out ] ; then cp bottles.tmp bottles.out ; \
	  else diff bottles.out bottles.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=bottles.ref

breadth:
	@(rm -f .memdump ; touch .memdump)
	@(echo breadth)
	@(xsltproc find.xsl breadth.xml > breadth.tmp; \
	  if [ ! -f breadth.out ] ; then cp breadth.tmp breadth.out ; \
	  else diff breadth.out breadth.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=breadth.ref

brutal:
	@(rm -f .memdump ; touch .memdump)
	@(echo brutal)
	@(xsltproc brutal.xsl brutal.xml > brutal.tmp; \
	  if [ ! -f brutal.out ] ; then cp brutal.tmp brutal.out ; \
	  else diff brutal.out brutal.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=brutal.ref

chart:
	@(rm -f .memdump ; touch .memdump)
	@(echo chart)
	@(xsltproc chart.xsl chart.xml > chart.tmp; \
	  if [ ! -f chart.out ] ; then cp chart.tmp chart.out ; \
	  else diff chart.out chart.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=chart.ref

creation: db100.xml
	@(rm -f .memdump ; touch .memdump)
	@(echo creation)
	@(xsltproc creation.xsl db100.xml > creation.tmp; \
	  if [ ! -f creation.out ] ; then cp creation.tmp creation.out ; \
	  else diff creation.out creation.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=creation.ref

current:
	@(rm -f .memdump ; touch .memdump)
	@(echo current)
	@(xsltproc current.xsl current.xml > current.tmp; \
	  if [ ! -f current.out ] ; then cp current.tmp current.out ; \
	  else diff current.out current.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=current.ref

dbonerow: db10000.xml
	@(rm -f .memdump ; touch .memdump)
	@(echo dbonerow)
	@(xsltproc dbonerow.xsl db10000.xml > dbonerow.tmp; \
	  if [ ! -f dbonerow.out ] ; then cp dbonerow.tmp dbonerow.out ; \
	  else diff dbonerow.out dbonerow.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=dbonerow.ref

dbtail: db100.xml
	@(rm -f .memdump ; touch .memdump)
	@(echo dbtail)
	@(xsltproc dbtail.xsl db100.xml > dbtail.tmp; \
	  if [ ! -f dbtail.out ] ; then cp dbtail.tmp dbtail.out ; \
	  else diff dbtail.out dbtail.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=dbtail.ref

decoy: db100.xml
	@(rm -f .memdump ; touch .memdump)
	@(echo decoy)
	@(xsltproc decoy.xsl db100.xml > decoy.tmp; \
	  if [ ! -f decoy.out ] ; then cp decoy.tmp decoy.out ; \
	  else diff decoy.out decoy.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=patterns.ref

depth:
	@(rm -f .memdump ; touch .memdump)
	@(echo depth)
	@(xsltproc find.xsl depth.xml > depth.tmp; \
	  if [ ! -f depth.out ] ; then cp depth.tmp depth.out ; \
	  else diff depth.out depth.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=depth.ref

encrypt: db100.xml
	@(rm -f .memdump ; touch .memdump)
	@(echo encrypt)
	@(xsltproc encrypt.xsl db100.xml > encrypt.tmp; \
	  if [ ! -f encrypt.out ] ; then cp encrypt.tmp encrypt.out ; \
	  else diff encrypt.out encrypt.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=encrypt.ref

functions: db100.xml
	@(rm -f .memdump ; touch .memdump)
	@(echo functions)
	@(xsltproc functions.xsl db100.xml > functions.tmp; \
	  if [ ! -f functions.out ] ; then cp functions.tmp functions.out ; \
	  else diff functions.out functions.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=functions.ref

game:
	@(rm -f .memdump ; touch .memdump)
	@(echo game)
	@(xsltproc game.xsl game.xml > game.tmp; \
	  if [ ! -f game.out ] ; then cp game.tmp game.out ; \
	  else diff game.out game.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=game.ref

html:
	@(rm -f .memdump ; touch .memdump)
	@(echo html)
	@(xsltproc html.xsl html.xml > html.tmp; \
	  if [ ! -f html.out ] ; then cp html.tmp html.out ; \
	  else diff html.out html.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=html.ref

identity: db1000.xml
	@(rm -f .memdump ; touch .memdump)
	@(echo identity)
	@(xsltproc identity.xsl db1000.xml > identity.tmp; \
	  if [ ! -f identity.out ] ; then cp identity.tmp identity.out ; \
	  else diff identity.out identity.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=identity.ref

inventory:
	@(rm -f .memdump ; touch .memdump)
	@(echo inventory)
	@(xsltproc inventory.xsl inventory.xml > inventory.tmp; \
	  if [ ! -f inventory.out ] ; then cp inventory.tmp inventory.out ; \
	  else diff inventory.out inventory.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=inventory.ref

metric:
	@(rm -f .memdump ; touch .memdump)
	@(echo metric)
	@(xsltproc metric.xsl metric.xml > metric.tmp; \
	  if [ ! -f metric.out ] ; then cp metric.tmp metric.out ; \
	  else diff metric.out metric.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=metric.ref

number:
	@(rm -f .memdump ; touch .memdump)
	@(echo number)
	@(xsltproc number.xsl number.xml > number.tmp; \
	  if [ ! -f number.out ] ; then cp number.tmp number.out ; \
	  else diff number.out number.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )
iterations=100
skipdriver=ExternalDriver:txdriver

oddtemplate:
	@(rm -f .memdump ; touch .memdump)
	@(echo oddtemplate)
	@(xsltproc oddtemplate.xsl oddtemplate.xml > oddtemplate.tmp; \
	  if [ ! -f oddtemplate.out ] ; then cp oddtemplate.tmp oddtemplate.out ; \
	  else diff oddtemplate.out oddtemplate.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=oddtemplate.ref

patterns: db100.xml
	@(rm -f .memdump ; touch .memdump)
	@(echo patterns)
	@(xsltproc patterns.xsl db100.xml > patterns.tmp; \
	  if [ ! -f patterns.out ] ; then cp patterns.tmp patterns.out ; \
	  else diff patterns.out patterns.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=patterns.ref

prettyprint: db100.xml
	@(rm -f .memdump ; touch .memdump)
	@(echo prettyprint)
	@(xsltproc prettyprint.xsl db100.xml > prettyprint.tmp; \
	  if [ ! -f prettyprint.out ] ; then cp prettyprint.tmp prettyprint.out ; \
	  else diff prettyprint.out prettyprint.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=prettyprint.ref

priority:
	@(rm -f .memdump ; touch .memdump)
	@(echo priority)
	@(xsltproc priority.xsl priority.xml > priority.tmp; \
	  if [ ! -f priority.out ] ; then cp priority.tmp priority.out ; \
	  else diff priority.out priority.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=priority.ref

products:
	@(rm -f .memdump ; touch .memdump)
	@(echo products)
	@(xsltproc products.xsl products.xml > products.tmp; \
	  if [ ! -f products.out ] ; then cp products.tmp products.out ; \
	  else diff products.out products.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=products.ref

queens:
	@(rm -f .memdump ; touch .memdump)
	@(echo queens)
	@(xsltproc queens.xsl queens.xml > queens.tmp; \
	  if [ ! -f queens.out ] ; then cp queens.tmp queens.out ; \
	  else diff queens.out queens.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=queens.ref

reverser:
	@(rm -f .memdump ; touch .memdump)
	@(echo reverser)
	@(xsltproc reverser.xsl gettysburg.xml > reverser.tmp; \
	  if [ ! -f reverser.out ] ; then cp reverser.tmp reverser.out ; \
	  else diff reverser.out reverser.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=reverser.ref

stringsort: db1000.xml
	@(rm -f .memdump ; touch .memdump)
	@(echo stringsort)
	@(xsltproc stringsort.xsl db1000.xml > stringsort.tmp; \
	  if [ ! -f stringsort.out ] ; then cp stringsort.tmp stringsort.out ; \
	  else diff stringsort.out stringsort.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=stringsort.ref

summarize:
	@(rm -f .memdump ; touch .memdump)
	@(echo summarize)
	@(xsltproc summarize.xsl queens.xsl > summarize.tmp; \
	  if [ ! -f summarize.out ] ; then cp summarize.tmp summarize.out ; \
	  else diff summarize.out summarize.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=summarize.ref

total:
	@(rm -f .memdump ; touch .memdump)
	@(echo total)
	@(xsltproc total.xsl chart.xml > total.tmp; \
	  if [ ! -f total.out ] ; then cp total.tmp total.out ; \
	  else diff total.out total.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=total.ref

tower:
	@(rm -f .memdump ; touch .memdump)
	@(echo tower)
	@(xsltproc tower.xsl tower.xml > tower.tmp; \
	  if [ ! -f tower.out ] ; then cp tower.tmp tower.out ; \
	  else diff tower.out tower.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=tower.ref

trend:
	@(rm -f .memdump ; touch .memdump)
	@(echo trend)
	@(xsltproc trend.xsl trend.xml > trend.tmp; \
	  if [ ! -f trend.out ] ; then cp trend.tmp trend.out ; \
	  else diff trend.out trend.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=trend.ref

union:
	@(rm -f .memdump ; touch .memdump)
	@(echo union)
	@(xsltproc union.xsl union.xml > union.tmp; \
	  if [ ! -f union.out ] ; then cp union.tmp union.out ; \
	  else diff union.out union.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=union.ref

xpath:
	@(rm -f .memdump ; touch .memdump)
	@(echo xpath)
	@(xsltproc xpath.xsl xpath.xml > xpath.tmp; \
	  if [ ! -f xpath.out ] ; then cp xpath.tmp xpath.out ; \
	  else diff xpath.out xpath.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

# reference=xpath.ref

xslbench1:
	@(rm -f .memdump ; touch .memdump)
	@(echo xslbench1)
	@(xsltproc xslbench1.xsl xslbench1.xml > xslbench1.tmp; \
	  if [ ! -f xslbench1.out ] ; then cp xslbench1.tmp xslbench1.out ; \
	  else diff xslbench1.out xslbench1.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

xslbench2:
	@(rm -f .memdump ; touch .memdump)
	@(echo xslbench2)
	@(xsltproc xslbench2.xsl xslbenchdream.xml > xslbench2.tmp; \
	  if [ ! -f xslbench2.out ] ; then cp xslbench2.tmp xslbench2.out ; \
	  else diff xslbench2.out xslbench2.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )

xslbench3:
	@(rm -f .memdump ; touch .memdump)
	@(echo xslbench3)
	@(xsltproc xslbench3.xsl xslbenchdream.xml > xslbench3.tmp; \
	  if [ ! -f xslbench3.out ] ; then cp xslbench3.tmp xslbench3.out ; \
	  else diff xslbench3.out xslbench3.tmp ; fi ; \
	  grep "MORY ALLO" .memdump  | grep -v "MEMORY ALLOCATED : 0" ; exit 0 )



