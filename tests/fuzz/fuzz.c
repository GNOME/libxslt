/*
 * fuzz.c: Fuzz targets for libxslt
 *
 * See Copyright for the status of this software.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxslt/extensions.h>
#include <libxslt/functions.h>
#include <libxslt/security.h>
#include <libxslt/transform.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/xsltutils.h>
#include <libexslt/exslt.h>
#include "fuzz.h"

#if defined(_WIN32)
  #define DIR_SEP '\\'
#else
  #define DIR_SEP '/'
#endif

static xsltSecurityPrefsPtr globalSec;
static xsltStylesheetPtr globalStyle;
static xsltTransformContextPtr tctxt;

static void
xsltFuzzXmlErrorFunc(void *vctxt, const char *msg ATTRIBUTE_UNUSED, ...) {
    xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr) vctxt;
    /*
     * Stopping the parser should be slightly faster and might catch some
     * issues related to recent libxml2 changes.
     */
    xmlStopParser(ctxt);
}

static void
xsltFuzzXsltErrorFunc(void *vctxt ATTRIBUTE_UNUSED,
                      const char *msg ATTRIBUTE_UNUSED, ...) {
}

static void
xsltFuzzInit(void) {
    xmlFuzzMemSetup();

    /* Init libxml2, libxslt and libexslt */
    xmlInitParser();
    xsltInit();
    exsltRegisterAll();

    /* Suppress error messages */
    xmlSetGenericErrorFunc(NULL, xsltFuzzXmlErrorFunc);
    xsltSetGenericErrorFunc(NULL, xsltFuzzXsltErrorFunc);

    /* Disallow I/O */
    globalSec = xsltNewSecurityPrefs();
    xsltSetSecurityPrefs(globalSec, XSLT_SECPREF_READ_FILE,
                         xsltSecurityForbid);
    xsltSetSecurityPrefs(globalSec, XSLT_SECPREF_WRITE_FILE,
                         xsltSecurityForbid);
    xsltSetSecurityPrefs(globalSec, XSLT_SECPREF_CREATE_DIRECTORY,
                         xsltSecurityForbid);
    xsltSetSecurityPrefs(globalSec, XSLT_SECPREF_READ_NETWORK,
                         xsltSecurityForbid);
    xsltSetSecurityPrefs(globalSec, XSLT_SECPREF_WRITE_NETWORK,
                         xsltSecurityForbid);
}

/* XPath fuzzer
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

int
xsltFuzzXPathInit(void) {
    xsltFuzzInit();
    globalStyle = xsltNewStylesheet();
    return(0);
}

xmlXPathObjectPtr
xsltFuzzXPath(const char *data, size_t size) {
    xmlXPathContextPtr xpctxt = NULL;
    xmlXPathObjectPtr xpathObj = NULL;
    xmlDocPtr doc;
    xmlNodePtr root;
    const char *xpathExpr, *xml;
    size_t maxAllocs, xmlSize;

    xmlFuzzDataInit(data, size);

    maxAllocs = xmlFuzzReadInt(4) % (size + 1);
    xpathExpr = xmlFuzzReadString(NULL);
    xml = xmlFuzzReadString(&xmlSize);

    /* Recovery mode allows more input to be fuzzed. */
    doc = xmlReadMemory(xml, xmlSize, NULL, NULL, XML_PARSE_RECOVER);
    if (doc == NULL)
        goto error;
    root = xmlDocGetRootElement(doc);
    if (root != NULL) {
        xmlNewNs(root, BAD_CAST "a", BAD_CAST "a");
        xmlNewNs(root, BAD_CAST "b", BAD_CAST "b");
        xmlNewNs(root, BAD_CAST "c", BAD_CAST "c");
    }

    tctxt = xsltNewTransformContext(globalStyle, doc);
    if (tctxt == NULL) {
        xmlFreeDoc(doc);
        goto error;
    }
    xsltSetCtxtSecurityPrefs(globalSec, tctxt);

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

    /* Set up XPath context */
    xpctxt = tctxt->xpathCtxt;

    /* Resource limits to avoid timeouts and call stack overflows */
    xpctxt->opLimit = 500000;

    /* Test namespaces */
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

    /* Compile and return early if the expression is invalid */
    xmlXPathCompExprPtr compExpr = xmlXPathCtxtCompile(xpctxt,
            (const xmlChar *) xpathExpr);
    if (compExpr == NULL)
        goto error;

    /* Initialize XPath evaluation context and evaluate */
    xmlFuzzMemSetLimit(maxAllocs);
    /* Maybe test different context nodes? */
    xpctxt->node = (xmlNodePtr) doc;
    xpctxt->contextSize = 1;
    xpctxt->proximityPosition = 1;
    xpctxt->opCount = 0;
    xpathObj = xmlXPathCompiledEval(compExpr, xpctxt);
    xmlXPathFreeCompExpr(compExpr);

error:
    xmlFuzzMemSetLimit(0);
    xmlXPathRegisteredNsCleanup(xpctxt);
    xmlFuzzDataCleanup();

    return xpathObj;
}

void
xsltFuzzXPathFreeObject(xmlXPathObjectPtr obj) {
    xmlXPathFreeObject(obj);

    if (tctxt != NULL) {
        xmlDocPtr doc = tctxt->document->doc;

        xsltFreeTransformContext(tctxt);
        tctxt = NULL;
        xmlFreeDoc(doc);
    }
}

void
xsltFuzzXPathCleanup(void) {
    xsltFreeSecurityPrefs(globalSec);
    globalSec = NULL;
    xsltFreeStylesheet(globalStyle);
    globalStyle = NULL;
}

/*
 * XSLT fuzzer
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

int
xsltFuzzXsltInit(void) {
    xsltFuzzInit();
    xmlSetExternalEntityLoader(xmlFuzzEntityLoader);
    return(0);
}

xmlChar *
xsltFuzzXslt(const char *data, size_t size) {
    const char *xsltBuffer, *xsltUrl, *docBuffer, *docUrl;
    xmlDocPtr xsltDoc = NULL, doc = NULL;
    xmlDocPtr result = NULL;
    xmlNodePtr root;
    xsltStylesheetPtr sheet = NULL;
    xsltTransformContextPtr ctxt = NULL;
    xmlChar *ret = NULL;
    size_t xsltSize, docSize, maxAllocs;
    int retLen;

    xmlFuzzDataInit(data, size);
    maxAllocs = xmlFuzzReadInt(4) % (size + 1);

    xmlFuzzReadEntities();
    xsltBuffer = xmlFuzzMainEntity(&xsltSize);
    xsltUrl = xmlFuzzMainUrl();
    docBuffer = xmlFuzzSecondaryEntity(&docSize);
    docUrl = xmlFuzzSecondaryUrl();
    if ((xsltBuffer == NULL) || (docBuffer == NULL))
        goto exit;

    doc = xmlReadMemory(docBuffer, docSize, docUrl, NULL, XSLT_PARSE_OPTIONS);
    if (doc == NULL)
        goto exit;

    xsltDoc = xmlReadMemory(xsltBuffer, xsltSize, xsltUrl, NULL,
                            XSLT_PARSE_OPTIONS);
    if (xsltDoc == NULL)
        goto exit;
    root = xmlDocGetRootElement(xsltDoc);
    if (root != NULL) {
        xmlNewNs(root, XSLT_NAMESPACE, BAD_CAST "x");
        xmlNewNs(root, EXSLT_COMMON_NAMESPACE, BAD_CAST "exsl");
        xmlNewNs(root, EXSLT_COMMON_NAMESPACE, BAD_CAST "exslt");
        xmlNewNs(root, EXSLT_CRYPTO_NAMESPACE, BAD_CAST "crypto");
        xmlNewNs(root, EXSLT_DATE_NAMESPACE, BAD_CAST "date");
        xmlNewNs(root, EXSLT_DYNAMIC_NAMESPACE, BAD_CAST "dyn");
        xmlNewNs(root, EXSLT_MATH_NAMESPACE, BAD_CAST "math");
        xmlNewNs(root, EXSLT_SETS_NAMESPACE, BAD_CAST "set");
        xmlNewNs(root, EXSLT_STRINGS_NAMESPACE, BAD_CAST "str");
        xmlNewNs(root, SAXON_NAMESPACE, BAD_CAST "saxon");
    }

    xmlFuzzMemSetLimit(maxAllocs);
    sheet = xsltNewStylesheet();
    if (sheet == NULL)
        goto exit;
    sheet->opLimit = 10000;
    sheet->xpathCtxt->opLimit = 100000;
    sheet->xpathCtxt->opCount = 0;
    if (xsltParseStylesheetUser(sheet, xsltDoc) != 0)
        goto exit;
    xsltDoc = NULL;

    root = xmlDocGetRootElement(doc);
    if (root != NULL) {
        xmlNewNs(root, BAD_CAST "a", BAD_CAST "a");
        xmlNewNs(root, BAD_CAST "b", BAD_CAST "b");
        xmlNewNs(root, BAD_CAST "c", BAD_CAST "c");
    }

    ctxt = xsltNewTransformContext(sheet, doc);
    if (ctxt == NULL)
        goto exit;
    xsltSetCtxtSecurityPrefs(globalSec, ctxt);
    ctxt->maxTemplateDepth = 100;
    ctxt->opLimit = 20000;
    ctxt->xpathCtxt->opLimit = 100000;
    ctxt->xpathCtxt->opCount = sheet->xpathCtxt->opCount;

    result = xsltApplyStylesheetUser(sheet, doc, NULL, NULL, NULL, ctxt);
    if (result != NULL)
        xsltSaveResultToString(&ret, &retLen, result, sheet);

exit:
    xmlFuzzMemSetLimit(0);
    xmlFreeDoc(result);
    xsltFreeTransformContext(ctxt);
    xsltFreeStylesheet(sheet);
    xmlFreeDoc(xsltDoc);
    xmlFreeDoc(doc);
    xmlFuzzDataCleanup();

    return ret;
}

void
xsltFuzzXsltCleanup(void) {
    xsltFreeSecurityPrefs(globalSec);
    globalSec = NULL;
}

/*
 * Utility functions, copied from libxml2
 */

typedef struct {
    const char *data;
    size_t size;
} xmlFuzzEntityInfo;

/* Single static instance for now */
static struct {
    /* Original data */
    const char *data;
    size_t size;

    /* Remaining data */
    const char *ptr;
    size_t remaining;

    /* Buffer for unescaped strings */
    char *outBuf;
    char *outPtr; /* Free space at end of buffer */

    xmlHashTablePtr entities; /* Maps URLs to xmlFuzzEntityInfos */

    /* The first entity is the main entity. */
    const char *mainUrl;
    xmlFuzzEntityInfo *mainEntity;
    const char *secondaryUrl;
    xmlFuzzEntityInfo *secondaryEntity;
} fuzzData;

size_t fuzzNumAllocs;
size_t fuzzMaxAllocs;

/**
 * xmlFuzzErrorFunc:
 *
 * An error function that simply discards all errors.
 */
void
xmlFuzzErrorFunc(void *ctx ATTRIBUTE_UNUSED, const char *msg ATTRIBUTE_UNUSED,
                 ...) {
}

/*
 * Malloc failure injection.
 *
 * Quick tip to debug complicated issues: Increase MALLOC_OFFSET until
 * the crash disappears (or a different issue is triggered). Then set
 * the offset to the highest value that produces a crash and set
 * MALLOC_ABORT to 1 to see which failed memory allocation causes the
 * issue.
 */

#define XML_FUZZ_MALLOC_OFFSET  0
#define XML_FUZZ_MALLOC_ABORT   0

static void *
xmlFuzzMalloc(size_t size) {
    if (fuzzMaxAllocs > 0) {
        if (fuzzNumAllocs >= fuzzMaxAllocs - 1)
#if XML_FUZZ_MALLOC_ABORT
            abort();
#else
            return(NULL);
#endif
        fuzzNumAllocs += 1;
    }
    return malloc(size);
}

static void *
xmlFuzzRealloc(void *ptr, size_t size) {
    if (fuzzMaxAllocs > 0) {
        if (fuzzNumAllocs >= fuzzMaxAllocs - 1)
#if XML_FUZZ_MALLOC_ABORT
            abort();
#else
            return(NULL);
#endif
        fuzzNumAllocs += 1;
    }
    return realloc(ptr, size);
}

void
xmlFuzzMemSetup(void) {
    xmlMemSetup(free, xmlFuzzMalloc, xmlFuzzRealloc, xmlMemStrdup);
}

void
xmlFuzzMemSetLimit(size_t limit) {
    fuzzNumAllocs = 0;
    fuzzMaxAllocs = limit ? limit + XML_FUZZ_MALLOC_OFFSET : 0;
}

/**
 * xmlFuzzDataInit:
 *
 * Initialize fuzz data provider.
 */
void
xmlFuzzDataInit(const char *data, size_t size) {
    fuzzData.data = data;
    fuzzData.size = size;
    fuzzData.ptr = data;
    fuzzData.remaining = size;

    fuzzData.outBuf = xmlMalloc(size + 1);
    fuzzData.outPtr = fuzzData.outBuf;

    fuzzData.entities = xmlHashCreate(8);
    fuzzData.mainUrl = NULL;
    fuzzData.mainEntity = NULL;
    fuzzData.secondaryUrl = NULL;
    fuzzData.secondaryEntity = NULL;
}

/**
 * xmlFuzzDataFree:
 *
 * Cleanup fuzz data provider.
 */
void
xmlFuzzDataCleanup(void) {
    xmlFree(fuzzData.outBuf);
    xmlHashFree(fuzzData.entities, xmlHashDefaultDeallocator);
}

/**
 * xmlFuzzWriteInt:
 * @out:  output file
 * @v:  integer to write
 * @size:  size of integer in bytes
 *
 * Write an integer to the fuzz data.
 */
void
xmlFuzzWriteInt(FILE *out, size_t v, int size) {
    int shift;

    while (size > (int) sizeof(size_t)) {
        putc(0, out);
        size--;
    }

    shift = size * 8;
    while (shift > 0) {
        shift -= 8;
        putc((v >> shift) & 255, out);
    }
}

/**
 * xmlFuzzReadInt:
 * @size:  size of integer in bytes
 *
 * Read an integer from the fuzz data.
 */
size_t
xmlFuzzReadInt(int size) {
    size_t ret = 0;

    while ((size > 0) && (fuzzData.remaining > 0)) {
        unsigned char c = (unsigned char) *fuzzData.ptr++;
        fuzzData.remaining--;
        ret = (ret << 8) | c;
        size--;
    }

    return ret;
}

/**
 * xmlFuzzReadRemaining:
 * @size:  size of string in bytes
 *
 * Read remaining bytes from fuzz data.
 */
const char *
xmlFuzzReadRemaining(size_t *size) {
    const char *ret = fuzzData.ptr;

    *size = fuzzData.remaining;
    fuzzData.ptr += fuzzData.remaining;
    fuzzData.remaining = 0;

    return(ret);
}

/*
 * xmlFuzzWriteString:
 * @out:  output file
 * @str:  string to write
 *
 * Write a random-length string to file in a format similar to
 * FuzzedDataProvider. Backslash followed by newline marks the end of the
 * string. Two backslashes are used to escape a backslash.
 */
void
xmlFuzzWriteString(FILE *out, const char *str) {
    for (; *str; str++) {
        int c = (unsigned char) *str;
        putc(c, out);
        if (c == '\\')
            putc(c, out);
    }
    putc('\\', out);
    putc('\n', out);
}

/**
 * xmlFuzzReadString:
 * @size:  size of string in bytes
 *
 * Read a random-length string from the fuzz data.
 *
 * The format is similar to libFuzzer's FuzzedDataProvider but treats
 * backslash followed by newline as end of string. This makes the fuzz data
 * more readable. A backslash character is escaped with another backslash.
 *
 * Returns a zero-terminated string or NULL if the fuzz data is exhausted.
 */
const char *
xmlFuzzReadString(size_t *size) {
    const char *out = fuzzData.outPtr;

    while (fuzzData.remaining > 0) {
        int c = *fuzzData.ptr++;
        fuzzData.remaining--;

        if ((c == '\\') && (fuzzData.remaining > 0)) {
            int c2 = *fuzzData.ptr;

            if (c2 == '\n') {
                fuzzData.ptr++;
                fuzzData.remaining--;
                if (size != NULL)
                    *size = fuzzData.outPtr - out;
                *fuzzData.outPtr++ = '\0';
                return(out);
            }
            if (c2 == '\\') {
                fuzzData.ptr++;
                fuzzData.remaining--;
            }
        }

        *fuzzData.outPtr++ = c;
    }

    if (fuzzData.outPtr > out) {
        if (size != NULL)
            *size = fuzzData.outPtr - out;
        *fuzzData.outPtr++ = '\0';
        return(out);
    }

    if (size != NULL)
        *size = 0;
    return(NULL);
}

/**
 * xmlFuzzReadEntities:
 *
 * Read entities like the main XML file, external DTDs, external parsed
 * entities from fuzz data.
 */
void
xmlFuzzReadEntities(void) {
    size_t num = 0;

    while (1) {
        const char *url, *entity;
        size_t entitySize;
        xmlFuzzEntityInfo *entityInfo;

        url = xmlFuzzReadString(NULL);
        if (url == NULL) break;

        entity = xmlFuzzReadString(&entitySize);
        if (entity == NULL) break;

        if (xmlHashLookup(fuzzData.entities, (xmlChar *)url) == NULL) {
            entityInfo = xmlMalloc(sizeof(xmlFuzzEntityInfo));
            if (entityInfo == NULL)
                break;
            entityInfo->data = entity;
            entityInfo->size = entitySize;

            xmlHashAddEntry(fuzzData.entities, (xmlChar *)url, entityInfo);

            if (num == 0) {
                fuzzData.mainUrl = url;
                fuzzData.mainEntity = entityInfo;
            } else if (num == 1) {
                fuzzData.secondaryUrl = url;
                fuzzData.secondaryEntity = entityInfo;
            }

            num++;
        }
    }
}

/**
 * xmlFuzzMainUrl:
 *
 * Returns the main URL.
 */
const char *
xmlFuzzMainUrl(void) {
    return(fuzzData.mainUrl);
}

/**
 * xmlFuzzMainEntity:
 * @size:  size of the main entity in bytes
 *
 * Returns the main entity.
 */
const char *
xmlFuzzMainEntity(size_t *size) {
    if (fuzzData.mainEntity == NULL)
        return(NULL);
    *size = fuzzData.mainEntity->size;
    return(fuzzData.mainEntity->data);
}

/**
 * xmlFuzzSecondaryUrl:
 *
 * Returns the secondary URL.
 */
const char *
xmlFuzzSecondaryUrl(void) {
    return(fuzzData.secondaryUrl);
}

/**
 * xmlFuzzSecondaryEntity:
 * @size:  size of the secondary entity in bytes
 *
 * Returns the secondary entity.
 */
const char *
xmlFuzzSecondaryEntity(size_t *size) {
    if (fuzzData.secondaryEntity == NULL)
        return(NULL);
    *size = fuzzData.secondaryEntity->size;
    return(fuzzData.secondaryEntity->data);
}

/**
 * xmlFuzzEntityLoader:
 *
 * The entity loader for fuzz data.
 */
xmlParserInputPtr
xmlFuzzEntityLoader(const char *URL, const char *ID ATTRIBUTE_UNUSED,
                    xmlParserCtxtPtr ctxt) {
    xmlParserInputBufferPtr buf;
    xmlFuzzEntityInfo *entity;

    if (URL == NULL)
        return(NULL);
    entity = xmlHashLookup(fuzzData.entities, (xmlChar *) URL);
    if (entity == NULL)
        return(NULL);

    buf = xmlParserInputBufferCreateMem(entity->data, entity->size,
                                        XML_CHAR_ENCODING_NONE);
    if (buf == NULL)
        return(NULL);

    return(xmlNewIOInputStream(ctxt, buf, XML_CHAR_ENCODING_NONE));
}
