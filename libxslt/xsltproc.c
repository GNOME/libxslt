/*
 * xsltproc.c: user program for the XSL Transformation 1.0 engine
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
 */

#include <string.h>
#include <libxml/xmlversion.h>
#include <libxml/xmlmemory.h>
#include <libxml/debugXML.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>

static int debug = 0;

int
main(int argc, char **argv) {
    int i;
    xsltStylesheetPtr cur;
    xmlDocPtr doc, res;

    LIBXML_TEST_VERSION
    for (i = 1; i < argc ; i++) {
	if ((!strcmp(argv[i], "-debug")) || (!strcmp(argv[i], "--debug")))
	    debug++;
    }
    xmlSubstituteEntitiesDefault(1);
    for (i = 1; i < argc ; i++) {
	if ((argv[i][0] != '-') || (strcmp(argv[i], "-") == 0)) {
	    cur = xsltParseStylesheetFile((const xmlChar *)argv[i]);
	    i++;
	    break;
	}
    }
    for (;i < argc ; i++) {
	doc = xmlParseFile(argv[i]);
	if (doc == NULL) {
	    fprintf(stderr, "unable to parse %s\n", argv[i]);
	    continue;
	}
	res = xsltApplyStylesheet(cur, doc);
	xmlFreeDoc(doc);
	if (res == NULL) {
	    fprintf(stderr, "no result for %s\n", argv[i]);
	    continue;
	}
#ifdef LIBXML_DEBUG_ENABLED
	if (debug)
            xmlDebugDumpDocument(stdout, res);
	else
#endif
	    xmlDocDump(stdout, res);

	xmlFreeDoc(res);
    }
    if (cur != NULL)
	xsltFreeStylesheet(cur);
    xmlCleanupParser();
    xmlMemoryDump();
    return(0);
}

