/*
 * transform.c: Implemetation of the XSL Transformation 1.0 engine
 *            transform part, i.e. applying a Stylesheet to a document
 *
 * Reference:
 *   http://www.w3.org/TR/1999/REC-xslt-19991116
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
 */

#include "xsltconfig.h"

#include <string.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/valid.h>
#include <libxml/hash.h>
#include <libxml/xmlerror.h>
#include <libxml/xpath.h>
#include <libxml/HTMLtree.h>
#include "xslt.h"
#include "xsltInternals.h"
#include "pattern.h"
#include "transform.h"

#define DEBUG_PROCESS

/*
 * To cleanup
 */
xmlChar *xmlSplitQName2(const xmlChar *name, xmlChar **prefix);

/*
 * There is no XSLT specific error reporting module yet
 */
#define xsltGenericError xmlGenericError
#define xsltGenericErrorContext xmlGenericErrorContext

/*
 * Useful macros
 */

#define TODO 								\
    xsltGenericError(xsltGenericErrorContext,				\
	    "Unimplemented block at %s:%d\n",				\
            __FILE__, __LINE__);

#define STRANGE 							\
    xsltGenericError(xsltGenericErrorContext,				\
	    "Internal error at %s:%d\n",				\
            __FILE__, __LINE__);

/*
 * Types are private:
 */

typedef enum xsltOutputType {
    XSLT_OUTPUT_XML = 0,
    XSLT_OUTPUT_HTML,
    XSLT_OUTPUT_TEXT
} xsltOutputType;

typedef struct _xsltTransformContext xsltTransformContext;
typedef xsltTransformContext *xsltTransformContextPtr;
struct _xsltTransformContext {
    xsltOutputType type;		/* the type of output */
    xmlNodePtr node;			/* the current node */
    xmlNodeSetPtr nodeList;		/* the current node list */

    xmlNodePtr output;			/* output node */

    xmlXPathContextPtr xpathCtxt;	/* the XPath context */
};

/************************************************************************
 *									*
 *			
 *									*
 ************************************************************************/

/**
 * xsltNewTransformContext:
 *
 * Create a new XSLT TransformContext
 *
 * Returns the newly allocated xsltTransformContextPtr or NULL in case of error
 */
xsltTransformContextPtr
xsltNewTransformContext(void) {
    xsltTransformContextPtr cur;

    cur = (xsltTransformContextPtr) xmlMalloc(sizeof(xsltTransformContext));
    if (cur == NULL) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltNewTransformContext : malloc failed\n");
	return(NULL);
    }
    memset(cur, 0, sizeof(xsltTransformContext));
    return(cur);
}

/**
 * xsltFreeTransformContext:
 * @ctxt:  an XSLT parser context
 *
 * Free up the memory allocated by @ctxt
 */
void
xsltFreeTransformContext(xsltTransformContextPtr ctxt) {
    if (ctxt == NULL)
	return;
    memset(ctxt, -1, sizeof(xsltTransformContext));
    xmlFree(ctxt);
}

/************************************************************************
 *									*
 *			
 *									*
 ************************************************************************/

/**
 * xsltApplyStylesheet:
 * @style:  a parsed XSLT stylesheet
 * @doc:  a parsed XML document
 *
 * Apply the stylesheet to the document
 * NOTE: This may lead to a non-wellformed output XML wise !
 *
 * Returns the result document or NULL in case of error
 */
xmlDocPtr
xsltApplyStylesheet(xsltStylesheetPtr style, xmlDocPtr doc) {
    xmlDocPtr res = NULL;
    xsltTransformContextPtr ctxt = NULL;

    if ((style == NULL) || (doc == NULL))
	return(NULL);
    ctxt = xsltNewTransformContext();
    if (ctxt == NULL)
	return(NULL);
    if ((style->method != NULL) &&
	(!xmlStrEqual(style->method, (const xmlChar *) "xml"))) {
	if (xmlStrEqual(style->method, (const xmlChar *) "html")) {
	    ctxt->type = XSLT_OUTPUT_HTML;
	    res = htmlNewDoc(style->doctypePublic, style->doctypeSystem);
	    if (res == NULL)
		goto error;
	} else if (xmlStrEqual(style->method, (const xmlChar *) "text")) {
	    ctxt->type = XSLT_OUTPUT_TEXT;
	    TODO
	    goto error;
	} else {
	    xsltGenericError(xsltGenericErrorContext,
			     "xsltApplyStylesheet: insupported method %s\n",
		             style->method);
	    goto error;
	}
    } else {
	ctxt->type = XSLT_OUTPUT_XML;
	res = xmlNewDoc(style->version);
	if (res == NULL)
	    goto error;
    }
    if (style->encoding != NULL)
	doc->encoding = xmlStrdup(style->encoding);


    /*
	res->intSubset = xmlCreateIntSubset(
     */

    xsltFreeTransformContext(ctxt);
    return(res);

error:
    if (res != NULL)
        xmlFreeDoc(res);
    if (ctxt != NULL)
        xsltFreeTransformContext(ctxt);
    return(NULL);
}

