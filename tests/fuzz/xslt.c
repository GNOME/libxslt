/*
 * xslt.c: libFuzzer target for XSLT stylesheets
 *
 * See Copyright for the status of this software.
 *
 * This is a rather naive fuzz target using a static XML document.
 *
 * TODO
 *
 * - Improve seed corpus
 * - Mutate multiple input documents: source, xsl:import, xsl:include
 * - format-number() with xsl:decimal-format
 * - Better coverage for xsl:key and key() function
 * - EXSLT func:function
 * - xsl:document
 */

#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxslt/security.h>
#include <libxslt/transform.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/xsltutils.h>
#include <libexslt/exslt.h>

static xmlDocPtr doc;
static xsltSecurityPrefsPtr sec;

static void
errorFunc(void *ctx, const char *msg, ...) {
    /* Discard error messages. */
}

int
LLVMFuzzerInitialize(int *argc_p ATTRIBUTE_UNUSED,
                     char ***argv_p ATTRIBUTE_UNUSED) {
    const char *xmlFilename = "xslt.xml";
    const char *dir;
    char *argv0;
    char *xmlPath;

    /* Init libraries */
    xmlInitParser();
    xmlXPathInit();
    xsltInit();
    exsltRegisterAll();

    /* Load XML document */
    argv0 = strdup((*argv_p)[0]);
    dir = dirname(argv0);
    xmlPath = malloc(strlen(dir) + 1 + strlen(xmlFilename) + 1);
    sprintf(xmlPath, "%s/%s", dir, xmlFilename);
    doc = xmlReadFile(xmlPath, NULL, 0);
    free(xmlPath);
    free(argv0);
    if (doc == NULL) {
        fprintf(stderr, "Error: unable to parse file \"%s\"\n", xmlPath);
        return -1;
    }

    /* Suppress error messages */
    xmlSetGenericErrorFunc(NULL, errorFunc);
    xsltSetGenericErrorFunc(NULL, errorFunc);

    /* Disallow I/O */
    sec = xsltNewSecurityPrefs();
    xsltSetSecurityPrefs(sec, XSLT_SECPREF_READ_FILE, xsltSecurityForbid);
    xsltSetSecurityPrefs(sec, XSLT_SECPREF_WRITE_FILE, xsltSecurityForbid);
    xsltSetSecurityPrefs(sec, XSLT_SECPREF_CREATE_DIRECTORY, xsltSecurityForbid);
    xsltSetSecurityPrefs(sec, XSLT_SECPREF_READ_NETWORK, xsltSecurityForbid);
    xsltSetSecurityPrefs(sec, XSLT_SECPREF_WRITE_NETWORK, xsltSecurityForbid);

    return 0;
}

static void
xsltSetXPathResourceLimits(xmlXPathContextPtr ctxt) {
    ctxt->maxParserDepth = 15;
    ctxt->maxDepth = 100;
    ctxt->opLimit = 100000;
}

int
LLVMFuzzerTestOneInput(const char *data, size_t size) {
    xmlDocPtr xsltDoc;
    xmlDocPtr result;
    xmlNodePtr xsltRoot;
    xsltStylesheetPtr sheet;
    xsltTransformContextPtr ctxt;

    xsltDoc = xmlReadMemory(data, size, NULL, NULL, 0);
    if (xsltDoc == NULL)
        return 0;
    xsltRoot = xmlDocGetRootElement(xsltDoc);
    xmlNewNs(xsltRoot, EXSLT_COMMON_NAMESPACE, BAD_CAST "exsl");
    xmlNewNs(xsltRoot, EXSLT_COMMON_NAMESPACE, BAD_CAST "exslt");
    xmlNewNs(xsltRoot, EXSLT_CRYPTO_NAMESPACE, BAD_CAST "crypto");
    xmlNewNs(xsltRoot, EXSLT_DATE_NAMESPACE, BAD_CAST "date");
    xmlNewNs(xsltRoot, EXSLT_DYNAMIC_NAMESPACE, BAD_CAST "dyn");
    xmlNewNs(xsltRoot, EXSLT_MATH_NAMESPACE, BAD_CAST "math");
    xmlNewNs(xsltRoot, EXSLT_SETS_NAMESPACE, BAD_CAST "set");
    xmlNewNs(xsltRoot, EXSLT_STRINGS_NAMESPACE, BAD_CAST "str");
    xmlNewNs(xsltRoot, SAXON_NAMESPACE, BAD_CAST "saxon");

    sheet = xsltNewStylesheet();
    if (sheet == NULL) {
        xmlFreeDoc(xsltDoc);
        return 0;
    }
    xsltSetXPathResourceLimits(sheet->xpathCtxt);
    sheet->xpathCtxt->opCount = 0;
    if (xsltParseStylesheetUser(sheet, xsltDoc) != 0) {
        xsltFreeStylesheet(sheet);
        xmlFreeDoc(xsltDoc);
        return 0;
    }

    ctxt = xsltNewTransformContext(sheet, doc);
    xsltSetCtxtSecurityPrefs(sec, ctxt);
    ctxt->maxTemplateDepth = 100;
    xsltSetXPathResourceLimits(ctxt->xpathCtxt);
    ctxt->xpathCtxt->opCount = sheet->xpathCtxt->opCount;

    result = xsltApplyStylesheetUser(sheet, doc, NULL, NULL, NULL, ctxt);

    xmlFreeDoc(result);
    xsltFreeTransformContext(ctxt);
    xsltFreeStylesheet(sheet);

    return 0;
}

