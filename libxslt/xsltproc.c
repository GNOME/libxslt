/*
 * xsltproc.c: user program for the XSL Transformation 1.0 engine
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
 */

#include <string.h>
#include <libxml/xmlmemory.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>

static int debug = 0;

int
main(int argc, char **argv) {
    int i;
    xsltStylesheetPtr cur;

    LIBXML_TEST_VERSION
    for (i = 1; i < argc ; i++) {
	if ((!strcmp(argv[i], "-debug")) || (!strcmp(argv[i], "--debug")))
	    debug++;
    }
    xmlSubstituteEntitiesDefault(1);
    for (i = 1; i < argc ; i++) {
	if ((argv[i][0] != '-') || (strcmp(argv[i], "-") == 0)) {
	    cur = xsltParseStylesheetFile(argv[i]);
	    xsltFreeStylesheet(cur);
	    break;
	}
    }
    xmlCleanupParser();
    xmlMemoryDump();
    return(0);
}

