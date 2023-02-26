/*
 * testTargets.c: Test the fuzz targets
 *
 * See Copyright for the status of this software.
 */

#include <stdio.h>

#include "fuzz.h"
#include <libxml/globals.h>

static int
testXPath(void) {
    xmlXPathObjectPtr obj;
    const char data[] =
        "\0\0\0\0count(//node())\\\n"
        "<d><e><f/></e></d>";
    int ret = 0;

    if (xsltFuzzXPathInit() != 0) {
        xsltFuzzXPathCleanup();
        return 1;
    }

    obj = xsltFuzzXPath(data, sizeof(data) - 1);
    if ((obj == NULL) || (obj->type != XPATH_NUMBER)) {
        fprintf(stderr, "Expression doesn't evaluate to number\n");
        ret = 1;
    } else if (obj->floatval != 3.0) {
        fprintf(stderr, "Expression returned %f, expected %f\n",
                obj->floatval, 3.0);
        ret = 1;
    }

    xsltFuzzXPathFreeObject(obj);
    xsltFuzzXPathCleanup();

    return ret;
}

static int
testXslt(void) {
    xmlChar *result;
    const char fuzzData[] =
        "\0\0\0\0stylesheet.xsl\\\n"
        "<xsl:stylesheet"
        " xmlns:xsl='http://www.w3.org/1999/XSL/Transform'"
        " version='1.0'"
        " extension-element-prefixes='"
        "  exsl exslt crypto date dyn math set str saxon"
        "'>\n"
        "<xsl:output omit-xml-declaration='yes'/>\n"
        "<xsl:template match='/'>\n"
        " <r><xsl:value-of select='count(//node())'/></r>\n"
        "</xsl:template>\n"
        "</xsl:stylesheet>\\\n"
        "document.xml\\\n"
        "<d><e><f/></e></d>";
    int ret = 0;

    if (xsltFuzzXsltInit() != 0) {
        xsltFuzzXsltCleanup();
        return 1;
    }

    result = xsltFuzzXslt(fuzzData, sizeof(fuzzData) - 1);
    if (result == NULL) {
        fprintf(stderr, "Result is NULL\n");
        ret = 1;
    } else if (xmlStrcmp(result, BAD_CAST "<r>3</r>\n") != 0) {
        fprintf(stderr, "Stylesheet returned\n%sexpected \n%s\n",
                result, "<r>3</r>");
        ret = 1;
    }

    xmlFree(result);
    xsltFuzzXsltCleanup();

    return ret;
}

int
main(void) {
    int ret = 0;

    if (testXPath() != 0)
        ret = 1;
    if (testXslt() != 0)
        ret = 1;

    return ret;
}
