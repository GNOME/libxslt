/*
 * xsltproc.c: user program for the XSL Transformation 1.0 engine
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#include "libxslt/libxslt.h"
#include "libexslt/exslt.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#include <libxml/xmlmemory.h>
#include <libxml/debugXML.h>
#include <libxml/HTMLtree.h>
#include <libxml/xmlIO.h>
#ifdef LIBXML_DOCB_ENABLED
#include <libxml/DOCBparser.h>
#endif
#ifdef LIBXML_XINCLUDE_ENABLED
#include <libxml/xinclude.h>
#endif
#ifdef LIBXML_CATALOG_ENABLED
#include <libxml/catalog.h>
#endif
#include <libxml/parserInternals.h>

#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libxslt/extensions.h>

#include <libexslt/exsltconfig.h>

#if defined(WIN32) && !defined (__CYGWIN__)
#ifdef _MSC_VER
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#define gettimeofday(p1,p2)
#define HAVE_TIME_H
#include <time.h>
#define HAVE_STDARG_H
#include <stdarg.h>
#endif /* _MS_VER */
#else /* WIN32 */
#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#elif defined(HAVE_TIME_H)
#include <time.h>
#endif
#endif /* WIN32 */

#ifndef HAVE_STAT
#  ifdef HAVE__STAT
     /* MS C library seems to define stat and _stat. The definition
      *         is identical. Still, mapping them to each other causes a warning. */
#    ifndef _MSC_VER
#      define stat(x,y) _stat(x,y)
#    endif
#    define HAVE_STAT
#  endif
#endif

xmlParserInputPtr xmlNoNetExternalEntityLoader(const char *URL,
	                                       const char *ID,
					       xmlParserCtxtPtr ctxt);

static int debug = 0;
static int repeat = 0;
static int timing = 0;
static int novalid = 0;
static int noout = 0;
#ifdef LIBXML_DOCB_ENABLED
static int docbook = 0;
#endif
#ifdef LIBXML_HTML_ENABLED
static int html = 0;
#endif
#ifdef LIBXML_XINCLUDE_ENABLED
static int xinclude = 0;
#endif
static int profile = 0;

#define MAX_PARAMETERS 64

static const char *params[MAX_PARAMETERS + 1];
static int nbparams = 0;
static xmlChar *strparams[MAX_PARAMETERS + 1];
static int nbstrparams = 0;
static const char *output = NULL;
static int errorno = 0;


/*
 * Internal timing routines to remove the necessity to have unix-specific
 * function calls
 */
#ifndef HAVE_GETTIMEOFDAY 
#ifdef HAVE_SYS_TIMEB_H
#ifdef HAVE_SYS_TIME_H
#ifdef HAVE_FTIME

int
my_gettimeofday(struct timeval *tvp, void *tzp)
{
	struct timeb timebuffer;

	ftime(&timebuffer);
	if (tvp) {
		tvp->tv_sec = timebuffer.time;
		tvp->tv_usec = timebuffer.millitm * 1000L;
	}
	return (0);
}
#define HAVE_GETTIMEOFDAY 1
#define gettimeofday my_gettimeofday

#endif /* HAVE_FTIME */
#endif /* HAVE_SYS_TIME_H */
#endif /* HAVE_SYS_TIMEB_H */
#endif /* !HAVE_GETTIMEOFDAY */

#if defined(HAVE_GETTIMEOFDAY)
static struct timeval begin, endtime;
/*
 * startTimer: call where you want to start timing
 */
static void startTimer(void)
{
    gettimeofday(&begin,NULL);
}
/*
 * endTimer: call where you want to stop timing and to print out a
 *           message about the timing performed; format is a printf
 *           type argument
 */
static void endTimer(const char *format, ...)
{
    long msec;
    va_list ap;

    gettimeofday(&endtime, NULL);
    msec = endtime.tv_sec - begin.tv_sec;
    msec *= 1000;
    msec += (endtime.tv_usec - begin.tv_usec) / 1000;

#ifndef HAVE_STDARG_H
#error "endTimer required stdarg functions"
#endif
    va_start(ap, format);
    vfprintf(stderr,format,ap);
    va_end(ap);

    fprintf(stderr, " took %ld ms\n", msec);
}
#elif defined(HAVE_TIME_H)
/*
 * No gettimeofday function, so we have to make do with calling clock.
 * This is obviously less accurate, but there's little we can do about
 * that.
 */
#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC 100
#endif

clock_t begin, endtime;
static void startTimer(void)
{
    begin=clock();
}
static void endTimer(char *format, ...)
{
    long msec;
    va_list ap;

    endtime=clock();
    msec = ((endtime-begin) * 1000) / CLOCKS_PER_SEC;

#ifndef HAVE_STDARG_H
#error "endTimer required stdarg functions"
#endif
    va_start(ap, format);
    vfprintf(stderr,format,ap);
    va_end(ap);
    fprintf(stderr, " took %ld ms\n", msec);
}
#else
/*
 * We don't have a gettimeofday or time.h, so we just don't do timing
 */
static void startTimer(void)
{
  /*
   * Do nothing
   */
}
static void endTimer(char *format, ...)
{
  /*
   * We cannot do anything because we don't have a timing function
   */
#ifdef HAVE_STDARG_H
    va_start(ap, format);
    vfprintf(stderr,format,ap);
    va_end(ap);
    fprintf(stderr, " was not timed\n", msec);
#else
  /* We don't have gettimeofday, time or stdarg.h, what crazy world is
   * this ?!
   */
#endif
}
#endif

static void
xsltProcess(xmlDocPtr doc, xsltStylesheetPtr cur, const char *filename) {
    xmlDocPtr res;
    xsltTransformContextPtr ctxt;

#ifdef LIBXML_XINCLUDE_ENABLED
    if (xinclude) {
	if (timing)
	    startTimer();
	xmlXIncludeProcess(doc);
	if (timing) {
	    endTimer("XInclude processing %s", filename);
	}
    }
#endif
    if (timing)
        startTimer();
    if (output == NULL) {
	if (repeat) {
	    int j;

	    for (j = 1; j < repeat; j++) {
		res = xsltApplyStylesheet(cur, doc, params);
		xmlFreeDoc(res);
		xmlFreeDoc(doc);
#ifdef LIBXML_HTML_ENABLED
		if (html)
		    doc = htmlParseFile(filename, NULL);
		else
#endif
#ifdef LIBXML_DOCB_ENABLED
		if (docbook)
		    doc = docbParseFile(filename, NULL);
		else
#endif
		    doc = xmlParseFile(filename);
	    }
	}
	ctxt = xsltNewTransformContext(cur, doc);
	if (ctxt == NULL)
	    return;
	if (profile) {
	    res = xsltApplyStylesheetUser(cur, doc, params, NULL,
		                          stderr, ctxt);
	} else {
	    res = xsltApplyStylesheetUser(cur, doc, params, NULL,
		                          NULL, ctxt);
	}
	if (ctxt->state == XSLT_STATE_ERROR)
	    errorno = 9;
	xsltFreeTransformContext(ctxt);
	if (timing) {
	    if (repeat)
		endTimer("Applying stylesheet %d times", repeat);
	    else
		endTimer("Applying stylesheet");
	}
	xmlFreeDoc(doc);
	if (res == NULL) {
	    fprintf(stderr, "no result for %s\n", filename);
	    return;
	}
	if (noout) {
	    xmlFreeDoc(res);
	    return;
	}
#ifdef LIBXML_DEBUG_ENABLED
	if (debug)
	    xmlDebugDumpDocument(stdout, res);
	else {
#endif
	    if (cur->methodURI == NULL) {
		if (timing)
		    startTimer();
		xsltSaveResultToFile(stdout, res, cur);
		if (timing)
		    endTimer("Saving result");
	    } else {
		if (xmlStrEqual
		    (cur->method, (const xmlChar *) "xhtml")) {
		    fprintf(stderr, "non standard output xhtml\n");
		    if (timing)
			startTimer();
		    xsltSaveResultToFile(stdout, res, cur);
		    if (timing)
			endTimer("Saving result");
		} else {
		    fprintf(stderr,
			    "Unsupported non standard output %s\n",
			    cur->method);
		    errorno = 7;
		}
	    }
#ifdef LIBXML_DEBUG_ENABLED
	}
#endif

	xmlFreeDoc(res);
    } else {
	xsltRunStylesheet(cur, doc, params, output, NULL, NULL);
	if (timing)
	    endTimer("Running stylesheet and saving result");
	xmlFreeDoc(doc);
    }
}

static void usage(const char *name) {
    printf("Usage: %s [options] stylesheet file [file ...]\n", name);
    printf("   Options:\n");
    printf("\t--version or -V: show the version of libxml and libxslt used\n");
    printf("\t--verbose or -v: show logs of what's happening\n");
    printf("\t--output file or -o file: save to a given file\n");
    printf("\t--timing: display the time used\n");
    printf("\t--repeat: run the transformation 20 times\n");
    printf("\t--debug: dump the tree of the result instead\n");
    printf("\t--novalid skip the Dtd loading phase\n");
    printf("\t--noout: do not dump the result\n");
    printf("\t--maxdepth val : increase the maximum depth\n");
#ifdef LIBXML_HTML_ENABLED
    printf("\t--html: the input document is(are) an HTML file(s)\n");
#endif
#ifdef LIBXML_DOCB_ENABLED
    printf("\t--docbook: the input document is SGML docbook\n");
#endif
    printf("\t--param name value : pass a (parameter,value) pair\n");
    printf("\t       value is an XPath expression.\n");
    printf("\t       string values must be quoted like \"'string'\"\n or");
    printf("\t       use stringparam to avoid it\n");
    printf("\t--stringparam name value : pass a (parameter,string value) pair\n");
    printf("\t--nonet refuse to fetch DTDs or entities over network\n");
#ifdef LIBXML_CATALOG_ENABLED
    printf("\t--catalogs : use SGML catalogs from $SGML_CATALOG_FILES\n");
    printf("\t             otherwise XML Catalogs starting from \n");
    printf("\t         file:///etc/xml/catalog are activated by default\n");
#endif
#ifdef LIBXML_XINCLUDE_ENABLED
    printf("\t--xinclude : do XInclude processing on document intput\n");
#endif
    printf("\t--profile or --norman : dump profiling informations \n");
    printf("\nProject libxslt home page: http://xmlsoft.org/XSLT/\n");
    printf("To report bugs and get help: http://xmlsoft.org/XSLT/bugs.html\n");
}

int
main(int argc, char **argv)
{
    int i;
    xsltStylesheetPtr cur = NULL;
    xmlDocPtr doc, style;

    if (argc <= 1) {
        usage(argv[0]);
        return (1);
    }

    xmlInitMemory();

    LIBXML_TEST_VERSION

    xmlLineNumbersDefault(1);

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-"))
            break;

        if (argv[i][0] != '-')
            continue;
#ifdef LIBXML_DEBUG_ENABLED
        if ((!strcmp(argv[i], "-debug")) || (!strcmp(argv[i], "--debug"))) {
            debug++;
        } else
#endif
        if ((!strcmp(argv[i], "-v")) ||
                (!strcmp(argv[i], "-verbose")) ||
                (!strcmp(argv[i], "--verbose"))) {
            xsltSetGenericDebugFunc(stderr, NULL);
        } else if ((!strcmp(argv[i], "-o")) ||
                   (!strcmp(argv[i], "-output")) ||
                   (!strcmp(argv[i], "--output"))) {
            i++;
            output = argv[i];
        } else if ((!strcmp(argv[i], "-V")) ||
                   (!strcmp(argv[i], "-version")) ||
                   (!strcmp(argv[i], "--version"))) {
            printf("Using libxml %s, libxslt %s and libexslt %s\n",
                   xmlParserVersion, xsltEngineVersion, exsltLibraryVersion);
            printf
    ("xsltproc was compiled against libxml %d, libxslt %d and libexslt %d\n",
                 LIBXML_VERSION, LIBXSLT_VERSION, LIBEXSLT_VERSION);
            printf("libxslt %d was compiled against libxml %d\n",
                   xsltLibxsltVersion, xsltLibxmlVersion);
            printf("libexslt %d was compiled against libxml %d\n",
                   exsltLibexsltVersion, exsltLibxmlVersion);
        } else if ((!strcmp(argv[i], "-repeat"))
                   || (!strcmp(argv[i], "--repeat"))) {
            if (repeat == 0)
                repeat = 20;
            else
                repeat = 100;
        } else if ((!strcmp(argv[i], "-novalid")) ||
                   (!strcmp(argv[i], "--novalid"))) {
            novalid++;
        } else if ((!strcmp(argv[i], "-noout")) ||
                   (!strcmp(argv[i], "--noout"))) {
            noout++;
#ifdef LIBXML_DOCB_ENABLED
        } else if ((!strcmp(argv[i], "-docbook")) ||
                   (!strcmp(argv[i], "--docbook"))) {
            docbook++;
#endif
#ifdef LIBXML_HTML_ENABLED
        } else if ((!strcmp(argv[i], "-html")) ||
                   (!strcmp(argv[i], "--html"))) {
            html++;
#endif
        } else if ((!strcmp(argv[i], "-timing")) ||
                   (!strcmp(argv[i], "--timing"))) {
            timing++;
        } else if ((!strcmp(argv[i], "-profile")) ||
                   (!strcmp(argv[i], "--profile"))) {
            profile++;
        } else if ((!strcmp(argv[i], "-norman")) ||
                   (!strcmp(argv[i], "--norman"))) {
            profile++;
        } else if ((!strcmp(argv[i], "-nonet")) ||
                   (!strcmp(argv[i], "--nonet"))) {
            xmlSetExternalEntityLoader(xmlNoNetExternalEntityLoader);
#ifdef LIBXML_CATALOG_ENABLED
        } else if ((!strcmp(argv[i], "-catalogs")) ||
                   (!strcmp(argv[i], "--catalogs"))) {
            const char *catalogs;

            catalogs = getenv("SGML_CATALOG_FILES");
            if (catalogs == NULL) {
                fprintf(stderr, "Variable $SGML_CATALOG_FILES not set\n");
            } else {
                xmlLoadCatalogs(catalogs);
            }
#endif
#ifdef LIBXML_XINCLUDE_ENABLED
        } else if ((!strcmp(argv[i], "-xinclude")) ||
                   (!strcmp(argv[i], "--xinclude"))) {
            xinclude++;
            xsltSetXIncludeDefault(1);
#endif
        } else if ((!strcmp(argv[i], "-param")) ||
                   (!strcmp(argv[i], "--param"))) {
            i++;
            params[nbparams++] = argv[i++];
            params[nbparams++] = argv[i];
            if (nbparams >= MAX_PARAMETERS) {
                fprintf(stderr, "too many params increase MAX_PARAMETERS \n");
                return (2);
            }
        } else if ((!strcmp(argv[i], "-stringparam")) ||
                   (!strcmp(argv[i], "--stringparam"))) {
	    const xmlChar *string;
	    xmlChar *value;
	    int len;

            i++;
            params[nbparams++] = argv[i++];
	    string = (const xmlChar *) argv[i];
	    len = xmlStrlen(string);
	    if (xmlStrchr(string, '"')) {
		if (xmlStrchr(string, '\'')) {
		    fprintf(stderr,
		    "stringparam contains both quote and double-quotes !\n");
		    return(8);
		}
		value = xmlStrdup((const xmlChar *)"'");
		value = xmlStrcat(value, string);
		value = xmlStrcat(value, (const xmlChar *)"'");
	    } else {
		value = xmlStrdup((const xmlChar *)"\"");
		value = xmlStrcat(value, string);
		value = xmlStrcat(value, (const xmlChar *)"\"");
	    }

            params[nbparams++] = (const char *) value;
	    strparams[nbstrparams++] = value;
            if (nbparams >= MAX_PARAMETERS) {
                fprintf(stderr, "too many params increase MAX_PARAMETERS \n");
                return (2);
            }
        } else if ((!strcmp(argv[i], "-maxdepth")) ||
                   (!strcmp(argv[i], "--maxdepth"))) {
            int value;

            i++;
            if (sscanf(argv[i], "%d", &value) == 1) {
                if (value > 0)
                    xsltMaxDepth = value;
            }
        } else {
            fprintf(stderr, "Unknown option %s\n", argv[i]);
            usage(argv[0]);
            return (3);
        }
    }
    params[nbparams] = NULL;

    if (novalid == 0)
        xmlLoadExtDtdDefaultValue = XML_DETECT_IDS | XML_COMPLETE_ATTRS;
    else
        xmlLoadExtDtdDefaultValue = 0;


    /*
     * Replace entities with their content.
     */
    xmlSubstituteEntitiesDefault(1);

    /*
     * Register the EXSLT extensions and the test module
     */
    exsltRegisterAll();
    xsltRegisterTestModule();

    for (i = 1; i < argc; i++) {
        if ((!strcmp(argv[i], "-maxdepth")) ||
            (!strcmp(argv[i], "--maxdepth"))) {
            i++;
            continue;
        } else if ((!strcmp(argv[i], "-o")) ||
                   (!strcmp(argv[i], "-output")) ||
                   (!strcmp(argv[i], "--output"))) {
            i++;
	    continue;
	}
        if ((!strcmp(argv[i], "-param")) || (!strcmp(argv[i], "--param"))) {
            i += 2;
            continue;
        }
        if ((!strcmp(argv[i], "-stringparam")) ||
            (!strcmp(argv[i], "--stringparam"))) {
            i += 2;
            continue;
        }
        if ((argv[i][0] != '-') || (strcmp(argv[i], "-") == 0)) {
            if (timing)
                startTimer();
	    style = xmlParseFile((const char *) argv[i]);
            if (timing) 
		endTimer("Parsing stylesheet %s", argv[i]);
	    if (style == NULL) {
		fprintf(stderr,  "cannot parse %s\n", argv[i]);
		cur = NULL;
		errorno = 4;
	    } else {
		cur = xsltLoadStylesheetPI(style);
		if (cur != NULL) {
		    /* it is an embedded stylesheet */
		    xsltProcess(style, cur, argv[i]);
		    xsltFreeStylesheet(cur);
		    goto done;
		}
		cur = xsltParseStylesheetDoc(style);
		if (cur != NULL) {
		    if (cur->indent == 1)
			xmlIndentTreeOutput = 1;
		    else
			xmlIndentTreeOutput = 0;
		    i++;
		} else {
		    xmlFreeDoc(style);
		    errorno = 5;
		    goto done;
		}
	    }
            break;

        }
    }

    /*
     * disable CDATA from being built in the document tree
     */
    xmlDefaultSAXHandlerInit();
    xmlDefaultSAXHandler.cdataBlock = NULL;

    if ((cur != NULL) && (cur->errors == 0)) {
        for (; i < argc; i++) {
	    doc = NULL;
            if (timing)
                startTimer();
#ifdef LIBXML_HTML_ENABLED
            if (html)
                doc = htmlParseFile(argv[i], NULL);
            else
#endif
#ifdef LIBXML_DOCB_ENABLED
            if (docbook)
                doc = docbParseFile(argv[i], NULL);
            else
#endif
                doc = xmlParseFile(argv[i]);
            if (doc == NULL) {
                fprintf(stderr, "unable to parse %s\n", argv[i]);
		errorno = 6;
                continue;
            }
            if (timing)
		endTimer("Parsing document %s", argv[i]);
	    xsltProcess(doc, cur, argv[i]);
        }
    }
    if (cur != NULL)
        xsltFreeStylesheet(cur);
    for (i = 0;i < nbstrparams;i++)
	xmlFree(strparams[i]);
done:
    xsltCleanupGlobals();
    xmlCleanupParser();
#if 0
    xmlMemoryDump();
#endif
    return(errorno);
}

