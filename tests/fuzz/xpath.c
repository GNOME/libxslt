/*
 * xpath.c: libFuzzer target for XPath expressions
 *
 * See Copyright for the status of this software.
 *
 * This fuzz target parses and evaluates XPath expressions in an (E)XSLT
 * context using a static XML document. It heavily exercises the libxml2
 * XPath engine (xpath.c), a few other parts of libxml2, and most of
 * libexslt.
 *
 * Some EXSLT functions need the transform context to create RVTs for
 * node-sets. A couple of functions also access the stylesheet. The
 * XPath context from the transform context is used to parse and
 * evaluate expressions.
 *
 * All these objects are created once at startup. After fuzzing each input,
 * they're reset as cheaply as possible.
 *
 * TODO
 *
 * - Some expressions can create lots of temporary node sets (RVTs) which
 *   aren't freed until the whole expression was evaluated, leading to
 *   extensive memory usage. Cleaning them up earlier would require
 *   callbacks from the XPath engine, for example after evaluating a
 *   predicate expression, which doesn't seem feasible. Terminating the
 *   evaluation after creating a certain number of RVTs is a simple
 *   workaround.
 * - Register a custom xsl:decimal-format declaration for format-number().
 * - Some functions add strings to the stylesheet or transform context
 *   dictionary, for example via xsltGetQName, requiring a clean up of the
 *   dicts after fuzzing each input. This behavior seems questionable.
 *   Extension functions shouldn't needlessly modify the transform context
 *   or stylesheet.
 * - Register xsl:keys and fuzz the key() function.
 * - Add a few custom func:functions.
 * - Fuzz the document() function with external documents.
 */

#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxslt/extensions.h>
#include <libxslt/functions.h>
#include <libxslt/security.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libexslt/exslt.h>

static xmlDocPtr doc;
static xsltTransformContextPtr tctxt;
static xmlHashTablePtr saxonExtHash;

static void
xmlFuzzErrorFunc(void *ctx ATTRIBUTE_UNUSED, const char *msg ATTRIBUTE_UNUSED,
                 ...) {
}

int
LLVMFuzzerInitialize(int *argc_p ATTRIBUTE_UNUSED,
                     char ***argv_p ATTRIBUTE_UNUSED) {
    const char *xmlFilename = "xpath.xml";
    const char *dir;
    char *argv0;
    char *xmlPath;
    xsltSecurityPrefsPtr sec;
    xsltStylesheetPtr style;
    xmlXPathContextPtr xpctxt;

    /* Init libxml2 and libexslt */
    xmlInitParser();
    xmlXPathInit();
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
    xmlSetGenericErrorFunc(NULL, xmlFuzzErrorFunc);
    xsltSetGenericErrorFunc(NULL, xmlFuzzErrorFunc);

    style = xsltNewStylesheet();
    tctxt = xsltNewTransformContext(style, doc);

    /* Disallow I/O */
    sec = xsltNewSecurityPrefs();
    xsltSetSecurityPrefs(sec, XSLT_SECPREF_READ_FILE, xsltSecurityForbid);
    xsltSetSecurityPrefs(sec, XSLT_SECPREF_WRITE_FILE, xsltSecurityForbid);
    xsltSetSecurityPrefs(sec, XSLT_SECPREF_CREATE_DIRECTORY, xsltSecurityForbid);
    xsltSetSecurityPrefs(sec, XSLT_SECPREF_READ_NETWORK, xsltSecurityForbid);
    xsltSetSecurityPrefs(sec, XSLT_SECPREF_WRITE_NETWORK, xsltSecurityForbid);
    xsltSetCtxtSecurityPrefs(sec, tctxt);

    /*
     * Some extension functions need the current instruction.
     *
     * - format-number() for namespaces.
     * - document() for the base URL.
     * - maybe others?
     *
     * For fuzzing, it's enough to use the source document's root element.
     */
    tctxt->inst = xmlDocGetRootElement(doc);

    saxonExtHash = (xmlHashTablePtr)
        xsltStyleGetExtData(style, SAXON_NAMESPACE);

    /* Set up XPath context */
    xpctxt = tctxt->xpathCtxt;

    /* Resource limits to avoid timeouts and call stack overflows */
    xpctxt->maxParserDepth = 15;
    xpctxt->maxDepth = 100;
    xpctxt->opLimit = 500000;

    /* Test namespaces used in xpath.xml */
    xmlXPathRegisterNs(xpctxt, BAD_CAST "a", BAD_CAST "a");
    xmlXPathRegisterNs(xpctxt, BAD_CAST "b", BAD_CAST "b");
    xmlXPathRegisterNs(xpctxt, BAD_CAST "c", BAD_CAST "c");

    /* EXSLT namespaces */
    xmlXPathRegisterNs(xpctxt, BAD_CAST "crypto", EXSLT_CRYPTO_NAMESPACE);
    xmlXPathRegisterNs(xpctxt, BAD_CAST "date", EXSLT_DATE_NAMESPACE);
    xmlXPathRegisterNs(xpctxt, BAD_CAST "dyn", EXSLT_DYNAMIC_NAMESPACE);
    xmlXPathRegisterNs(xpctxt, BAD_CAST "exsl", EXSLT_COMMON_NAMESPACE);
    xmlXPathRegisterNs(xpctxt, BAD_CAST "math", EXSLT_MATH_NAMESPACE);
    xmlXPathRegisterNs(xpctxt, BAD_CAST "saxon", SAXON_NAMESPACE);
    xmlXPathRegisterNs(xpctxt, BAD_CAST "set", EXSLT_SETS_NAMESPACE);
    xmlXPathRegisterNs(xpctxt, BAD_CAST "str", EXSLT_STRINGS_NAMESPACE);

    /* Register variables */
    xmlXPathRegisterVariable(xpctxt, BAD_CAST "f", xmlXPathNewFloat(-1.5));
    xmlXPathRegisterVariable(xpctxt, BAD_CAST "b", xmlXPathNewBoolean(1));
    xmlXPathRegisterVariable(xpctxt, BAD_CAST "s",
                             xmlXPathNewString(BAD_CAST "var"));
    xmlXPathRegisterVariable(
            xpctxt, BAD_CAST "n",
            xmlXPathEval(BAD_CAST "//node() | /*/*/namespace::*", xpctxt));

    return 0;
}

int
LLVMFuzzerTestOneInput(const char *data, size_t size) {
    xmlXPathContextPtr xpctxt = tctxt->xpathCtxt;
    xmlChar *xpathExpr;

    /* Null-terminate */
    xpathExpr = malloc(size + 1);
    memcpy(xpathExpr, data, size);
    xpathExpr[size] = 0;

    /*
     * format-number() can still cause memory errors with invalid UTF-8 in
     * prefixes or suffixes. This shouldn't be exploitable in practice, but
     * should be fixed. Check UTF-8 validity for now.
     */
    if (xmlCheckUTF8(xpathExpr) == 0) {
        free(xpathExpr);
        return 0;
    }

    /* Compile and return early if the expression is invalid */
    xmlXPathCompExprPtr compExpr = xmlXPathCtxtCompile(xpctxt, xpathExpr);
    free(xpathExpr);
    if (compExpr == NULL)
        return 0;

    /* Initialize XPath evaluation context and evaluate */
    xpctxt->node = (xmlNodePtr) doc; /* Maybe test different context nodes? */
    xpctxt->contextSize = 1;
    xpctxt->proximityPosition = 1;
    xpctxt->opCount = 0;
    xmlXPathObjectPtr xpathObj = xmlXPathCompiledEval(compExpr, xpctxt);
    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeCompExpr(compExpr);

    /* Some XSLT extension functions create RVTs. */
    xsltFreeRVTs(tctxt);

    /* Clean object cache */
    xmlXPathContextSetCache(xpctxt, 0, 0, 0);
    xmlXPathContextSetCache(xpctxt, 1, -1, 0);

    /* Clean dictionaries */
    if (xmlDictSize(tctxt->dict) > 0) {
        xmlDictFree(tctxt->dict);
        xmlDictFree(tctxt->style->dict);
        tctxt->style->dict = xmlDictCreate();
        tctxt->dict = xmlDictCreateSub(tctxt->style->dict);
    }

    /* Clean saxon:expression cache */
    if (xmlHashSize(saxonExtHash) > 0) {
        /* There doesn't seem to be a cheaper way with the public API. */
        xsltShutdownCtxtExts(tctxt);
        xsltInitCtxtExts(tctxt);
        saxonExtHash = (xmlHashTablePtr)
            xsltStyleGetExtData(tctxt->style, SAXON_NAMESPACE);
    }

    return 0;
}
