#!/usr/bin/python -u
import libxml2
import libxslt

# Memory debug specific
libxml2.debugMemory(1)


styledoc = libxml2.parseFile("test.xsl")
style = libxslt.parseStylesheetDoc(styledoc)
doc = libxml2.parseFile("test.xml")
result = style.applyStylesheet(doc, None)
style.saveResultToFilename("foo", result, 0)
style.freeStylesheet()
doc.freeDoc()
result.freeDoc()

# Memory debug specific
libxslt.cleanup()
if libxml2.debugMemory(1) == 0:
    print "OK"
else:
    print "Memory leak %d bytes" % (libxml2.debugMemory(1))
    libxml2.dumpMemory()
