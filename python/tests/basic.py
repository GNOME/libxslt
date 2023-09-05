#!/usr/bin/env python
import os
import sys
import setup_test
import libxml2
# Memory debug specific
libxml2.debugMemory(1)
import libxslt

basedir = os.path.dirname(os.path.realpath(__file__))

styledoc = libxml2.parseFile("%s/test.xsl" % basedir)
style = libxslt.parseStylesheetDoc(styledoc)
doc = libxml2.parseFile("%s/test.xml" % basedir)
result = style.applyStylesheet(doc, None)
style.saveResultToFilename("foo", result, 0)
os.remove("foo")
stringval = style.saveResultToString(result)
if (len(stringval) != 68):
  print("Error in saveResultToString")
  sys.exit(255)
style.freeStylesheet()
doc.freeDoc()
result.freeDoc()

# Memory debug specific
libxslt.cleanup()
if libxml2.debugMemory(1) == 0:
    print("OK")
else:
    print("Memory leak %d bytes" % (libxml2.debugMemory(1)))
    libxml2.dumpMemory()
    sys.exit(255)
