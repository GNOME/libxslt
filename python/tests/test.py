#!/usr/bin/python -u
import libxml2
import libxslt

styledoc = libxml2.parseFile("test.xsl")
style = libxslt.parseStylesheetDoc(styledoc)
doc = libxml2.parseFile("test.xml")
result = style.applyStylesheet(doc, None)
result.saveFile("-")
