/*
 * xsltproc.c: user program for the XSL Transformation 1.0 engine
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
 */

#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <libxml/xmlversion.h>
#include <libxml/xmlmemory.h>
#include <libxml/debugXML.h>
#include <libxml/HTMLtree.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

extern int xmlLoadExtDtdDefaultValue;

static int debug = 0;
static int repeat = 0;
static int timing = 0;

int
main(int argc, char **argv) {
    int i;
    xsltStylesheetPtr cur = NULL;
    xmlDocPtr doc, res;
    struct timeval begin, end;

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
	} else if ((!strcmp(argv[i], "-timing")) ||
		   (!strcmp(argv[i], "--timing"))) {
	    timing++;
	}
    }
    xmlSubstituteEntitiesDefault(1);
    xmlLoadExtDtdDefaultValue = 1;
    for (i = 1; i < argc ; i++) {
	if ((argv[i][0] != '-') || (strcmp(argv[i], "-") == 0)) {
	    if (timing)
		gettimeofday(&begin, NULL);
	    cur = xsltParseStylesheetFile((const xmlChar *)argv[i]);
	    if (timing) {
		long msec;
		gettimeofday(&end, NULL);
		msec = end.tv_sec - begin.tv_sec;
		msec *= 1000;
		msec += (end.tv_usec - begin.tv_usec) / 1000;
		fprintf(stderr, "Parsing stylesheet %s took %ld ms\n",
			argv[i], msec);
	    }
	    if (cur != NULL) {
		if (cur->indent == 1)
		    xmlIndentTreeOutput = 1;
		else
		    xmlIndentTreeOutput = 0;
		i++;
	    }
	    break;
		
	}
    }
    if (cur != NULL) {
	for (;i < argc ; i++) {
	    if (timing)
		gettimeofday(&begin, NULL);
	    doc = xmlParseFile(argv[i]);
	    if (doc == NULL) {
		fprintf(stderr, "unable to parse %s\n", argv[i]);
		continue;
	    }
	    if (timing) {
		long msec;
		gettimeofday(&end, NULL);
		msec = end.tv_sec - begin.tv_sec;
		msec *= 1000;
		msec += (end.tv_usec - begin.tv_usec) / 1000;
		fprintf(stderr, "Parsing document %s took %ld ms\n",
			argv[i], msec);
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
	    if (timing)
		gettimeofday(&begin, NULL);
	    res = xsltApplyStylesheet(cur, doc);
	    if (timing) {
		long msec;
		gettimeofday(&end, NULL);
		msec = end.tv_sec - begin.tv_sec;
		msec *= 1000;
		msec += (end.tv_usec - begin.tv_usec) / 1000;
		fprintf(stderr, "Applying stylesheet took %ld ms\n",
			msec);
	    }
	    xmlFreeDoc(doc);
	    if (res == NULL) {
		fprintf(stderr, "no result for %s\n", argv[i]);
		continue;
	    }
#ifdef LIBXML_DEBUG_ENABLED
	    if (debug)
		xmlDebugDumpDocument(stdout, res);
	    else {
#endif
		if (cur->methodURI == NULL) {
		    if (timing)
			gettimeofday(&begin, NULL);
			xsltSaveResultToFile(stdout, res, cur);
		    if (timing) {
			long msec;
			gettimeofday(&end, NULL);
			msec = end.tv_sec - begin.tv_sec;
			msec *= 1000;
			msec += (end.tv_usec - begin.tv_usec) / 1000;
			fprintf(stderr, "Saving result took %ld ms\n",
				msec);
		    }
		} else {
		    if (xmlStrEqual(cur->method, (const xmlChar *)"xhtml")) {
			fprintf(stderr, "non standard output xhtml\n");
			if (timing)
			    gettimeofday(&begin, NULL);
			    xsltSaveResultToFile(stdout, res, cur);
			if (timing) {
			    long msec;
			    gettimeofday(&end, NULL);
			    msec = end.tv_sec - begin.tv_sec;
			    msec *= 1000;
			    msec += (end.tv_usec - begin.tv_usec) / 1000;
			    fprintf(stderr, "Saving result took %ld ms\n",
				    msec);
			}
		    } else {
			fprintf(stderr, "Unsupported non standard output %s\n",
				cur->method);
		    }
		}
#ifdef LIBXML_DEBUG_ENABLED
	    }
#endif

	    xmlFreeDoc(res);
	}
	xsltFreeStylesheet(cur);
    }
    xmlCleanupParser();
    xmlMemoryDump();
    return(0);
}

