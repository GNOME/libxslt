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
#include <libxml/HTMLtree.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

static int debug = 0;
static int repeat = 0;

int
main(int argc, char **argv) {
    int i;
    xsltStylesheetPtr cur = NULL;
    xmlDocPtr doc, res;

    /* --repeat : repeat 100 times, for timing or profiling */
    LIBXML_TEST_VERSION
    for (i = 1; i < argc ; i++) {
	if ((!strcmp(argv[i], "-debug")) || (!strcmp(argv[i], "--debug"))) {
	    debug++;
	} else if ((!strcmp(argv[i], "-v")) ||
		   (!strcmp(argv[i], "-verbose")) ||
		   (!strcmp(argv[i], "--verbose"))) {
	    xsltSetGenericDebugFunc(stderr, NULL);
	} else if ((!strcmp(argv[i], "-repeat")) ||
		   (!strcmp(argv[i], "--repeat"))) {
	    repeat++;
	}
    }
    xmlSubstituteEntitiesDefault(1);
    for (i = 1; i < argc ; i++) {
	if ((argv[i][0] != '-') || (strcmp(argv[i], "-") == 0)) {
	    cur = xsltParseStylesheetFile((const xmlChar *)argv[i]);
	    if (cur != NULL) {
		if (cur->indent == 1)
		    xmlIndentTreeOutput = 1;
		else
		    xmlIndentTreeOutput = 0;
		i++;
		break;
	    }
	}
    }
    for (;i < argc ; i++) {
	doc = xmlParseFile(argv[i]);
	if (doc == NULL) {
	    fprintf(stderr, "unable to parse %s\n", argv[i]);
	    continue;
	}
	if (repeat) {
	    int j;
	    for (j = 0;j < 99; j++) {
		res = xsltApplyStylesheet(cur, doc);
		xmlFreeDoc(res);
		xmlFreeDoc(doc);
		doc = xmlParseFile(argv[i]);
	    }
	}
	res = xsltApplyStylesheet(cur, doc);
	xmlFreeDoc(doc);
	if (res == NULL) {
	    fprintf(stderr, "no result for %s\n", argv[i]);
	    continue;
	}
	if (cur->methodURI == NULL) {
#ifdef LIBXML_DEBUG_ENABLED
	    if (debug)
		xmlDebugDumpDocument(stdout, res);
	    else
#endif
		xsltSaveResultToFile(stdout, res, cur);
	}

	xmlFreeDoc(res);
    }
    if (cur != NULL)
	xsltFreeStylesheet(cur);
    xmlCleanupParser();
    xmlMemoryDump();
    return(0);
}

