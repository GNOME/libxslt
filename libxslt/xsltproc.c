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
static int novalid = 0;
static int noout = 0;

int
main(int argc, char **argv) {
    int i;
    xsltStylesheetPtr cur = NULL;
    xmlDocPtr doc, res;
    struct timeval begin, end;

    if (argc <= 1) {
	printf("Usage: %s [options] stylesheet file [file ...]\n", argv[0]);
	printf("   Options:\n");
	printf("      --verbose or -v: show logs of what's happening\n");
	printf("      --timing: display the time used\n");
	printf("      --repeat: run the transformation 20 times\n");
	printf("      --debug: dump the tree of the result instead\n");
	printf("      --novalid: skip the Dtd loading phase\n");
	printf("      --noout: do not dump the result\n");
	printf("      --maxdepth val : increase the maximum depth\n");
	return(0);
    }
    /* --repeat : repeat 20 times, for timing or profiling */
    LIBXML_TEST_VERSION
    for (i = 1; i < argc ; i++) {
#ifdef LIBXML_DEBUG_ENABLED
	if ((!strcmp(argv[i], "-debug")) || (!strcmp(argv[i], "--debug"))) {
	    debug++;
	} else 
#endif
	if ((!strcmp(argv[i], "-v")) ||
		   (!strcmp(argv[i], "-verbose")) ||
		   (!strcmp(argv[i], "--verbose"))) {
	    xsltSetGenericDebugFunc(stderr, NULL);
	} else if ((!strcmp(argv[i], "-repeat")) ||
		   (!strcmp(argv[i], "--repeat"))) {
	    repeat++;
	} else if ((!strcmp(argv[i], "-novalid")) ||
		   (!strcmp(argv[i], "--novalid"))) {
	    novalid++;
	} else if ((!strcmp(argv[i], "-noout")) ||
		   (!strcmp(argv[i], "--noout"))) {
	    noout++;
	} else if ((!strcmp(argv[i], "-timing")) ||
		   (!strcmp(argv[i], "--timing"))) {
	    timing++;
	} else if ((!strcmp(argv[i], "-maxdepth")) ||
		   (!strcmp(argv[i], "--maxdepth"))) {
	    int value;
	    i++;
	    if (sscanf(argv[i], "%d", &value) == 1) {
		if (value > 0)
		    xsltMaxDepth = value;
	    }
	}
    }
    xmlSubstituteEntitiesDefault(1);
    if (novalid == 0)
	xmlLoadExtDtdDefaultValue = 1;
    else
	xmlLoadExtDtdDefaultValue = 0;
    for (i = 1; i < argc ; i++) {
	if ((!strcmp(argv[i], "-maxdepth")) ||
	    (!strcmp(argv[i], "--maxdepth"))) {
	    i++;
	    continue;
	}
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
	    if (timing)
		gettimeofday(&begin, NULL);
	    if (repeat) {
		int j;
		for (j = 0;j < 19; j++) {
		    res = xsltApplyStylesheet(cur, doc);
		    xmlFreeDoc(res);
		    xmlFreeDoc(doc);
		    doc = xmlParseFile(argv[i]);
		}
	    }
	    res = xsltApplyStylesheet(cur, doc);
	    if (timing) {
		long msec;
		gettimeofday(&end, NULL);
		msec = end.tv_sec - begin.tv_sec;
		msec *= 1000;
		msec += (end.tv_usec - begin.tv_usec) / 1000;
		if (repeat)
		    fprintf(stderr,
			    "Applying stylesheet 20 times took %ld ms\n",
			    msec);
		else
		    fprintf(stderr, "Applying stylesheet took %ld ms\n",
			    msec);
	    }
	    xmlFreeDoc(doc);
	    if (res == NULL) {
		fprintf(stderr, "no result for %s\n", argv[i]);
		continue;
	    }
	    if (noout) {
		xmlFreeDoc(res);
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

