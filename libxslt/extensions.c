/*
 * extensions.c: Implemetation of the extensions support
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
#include <libxml/tree.h>
#include <libxml/hash.h>
#include <libxml/xmlerror.h>
#include <libxml/parserInternals.h>
#include "xslt.h"
#include "xsltInternals.h"
#include "xsltutils.h"
#include "extensions.h"

#define DEBUG_EXTENSIONS

typedef struct _xsltExtDef xsltExtDef;
typedef xsltExtDef *xsltExtDefPtr;
struct _xsltExtDef {
    struct _xsltExtDef *next;
    xmlChar *prefix;
    xmlChar *URI;
};

const xmlChar *xsltExtMarker = (const xmlChar *)"extension";

/************************************************************************
 * 									*
 * 			Type functions 					*
 * 									*
 ************************************************************************/

/**
 * xsltNewExtDef:
 * @prefix:  the extension prefix
 * @URI:  the namespace URI
 *
 * Create a new XSLT ExtDef
 *
 * Returns the newly allocated xsltExtDefPtr or NULL in case of error
 */
static xsltExtDefPtr
xsltNewExtDef(const xmlChar *prefix, const xmlChar *URI) {
    xsltExtDefPtr cur;

    cur = (xsltExtDefPtr) xmlMalloc(sizeof(xsltExtDef));
    if (cur == NULL) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltNewExtDef : malloc failed\n");
	return(NULL);
    }
    memset(cur, 0, sizeof(xsltExtDef));
    if (prefix != NULL)
	cur->prefix = xmlStrdup(prefix);
    if (URI != NULL)
	cur->URI = xmlStrdup(URI);
    return(cur);
}

/**
 * xsltFreeExtDef:
 * @extensiond:  an XSLT extension definition
 *
 * Free up the memory allocated by @extensiond
 */
static void
xsltFreeExtDef(xsltExtDefPtr extensiond) {
    if (extensiond == NULL)
	return;
    if (extensiond->prefix != NULL)
	xmlFree(extensiond->prefix);
    if (extensiond->URI != NULL)
	xmlFree(extensiond->URI);
    memset(extensiond, -1, sizeof(xsltExtDef));
    xmlFree(extensiond);
}

/**
 * xsltFreeExtDefList:
 * @extensiond:  an XSLT extension definition list
 *
 * Free up the memory allocated by all the elements of @extensiond
 */
static void
xsltFreeExtDefList(xsltExtDefPtr extensiond) {
    xsltExtDefPtr cur;

    while (extensiond != NULL) {
	cur = extensiond;
	extensiond = extensiond->next;
	xsltFreeExtDef(cur);
    }
}


/************************************************************************
 * 									*
 * 		The interpreter for the precompiled patterns		*
 * 									*
 ************************************************************************/


/**
 * xsltFreeExtPrefix:
 * @style: an XSLT stylesheet
 *
 * Free up the memory used by XSLT extensions in a stylesheet
 */
void
xsltFreeExts(xsltStylesheetPtr style) {
    if (style->nsDefs != NULL)
	xsltFreeExtDefList((xsltExtDefPtr) style->nsDefs);
}

/*
 * xsltRegisterExtPrefix:
 * @style: an XSLT stylesheet
 * @prefix: the prefix used
 * @URI: the URI associated to the extension
 *
 * Registers an extension namespace
 *
 * Returns 0 in case of success, -1 in case of failure
 */
int
xsltRegisterExtPrefix(xsltStylesheetPtr style,
		      const xmlChar *prefix, const xmlChar *URI) {
    xsltExtDefPtr def, ret;

    if ((style == NULL) || (prefix == NULL) | (URI == NULL))
	return(-1);

    def = (xsltExtDefPtr) style->nsDefs;
    while (def != NULL) {
	if (xmlStrEqual(prefix, def->prefix))
	    return(-1);
	def = def->next;
    }
    ret = xsltNewExtDef(prefix, URI);
    if (ret == NULL)
	return(-1);
    ret->next = (xsltExtDefPtr) style->nsDefs;
    style->nsDefs = ret;
    return(0);
}

/*
 * xsltRegisterExtFunction:
 * @ctxt: an XSLT transformation context
 * @name: the name of the element
 * @URI: the URI associated to the element
 * @function: the actual implementation which should be called 
 *
 * Registers an extension function
 *
 * Returns 0 in case of success, -1 in case of failure
 */
int
xsltRegisterExtFunction(xsltTransformContextPtr ctxt, const xmlChar *name,
	                const xmlChar *URI, xmlXPathEvalFunc function) {
    if ((ctxt == NULL) || (name == NULL) ||
	(URI == NULL) || (function == NULL))
	return(-1);
    if (ctxt->extFunctions == NULL)
	ctxt->extFunctions = xmlHashCreate(10);
    return(xmlHashAddEntry2(ctxt->extFunctions, name, URI, (void *) function));
}

/*
 * xsltRegisterExtElement:
 * @ctxt: an XSLT transformation context
 * @name: the name of the element
 * @URI: the URI associated to the element
 * @function: the actual implementation which should be called 
 *
 * Registers an extension element
 *
 * Returns 0 in case of success, -1 in case of failure
 */
int	
xsltRegisterExtElement(xsltTransformContextPtr ctxt, const xmlChar *name,
		       const xmlChar *URI, xsltTransformFunction function) {
    if ((ctxt == NULL) || (name == NULL) ||
	(URI == NULL) || (function == NULL))
	return(-1);
    if (ctxt->extElements == NULL)
	ctxt->extElements = xmlHashCreate(10);
    return(xmlHashAddEntry2(ctxt->extElements, name, URI, (void *) function));
}

/*
 * xsltFreeCtxtExts:
 * @ctxt: an XSLT transformation context
 *
 * Free the XSLT extension data
 */
void
xsltFreeCtxtExts(xsltTransformContextPtr ctxt) {
    if (ctxt->extElements != NULL)
	xmlHashFree(ctxt->extElements, NULL);
    if (ctxt->extFunctions != NULL)
	xmlHashFree(ctxt->extFunctions, NULL);
}

/*
 * xsltCheckExtPrefix:
 * @style: the stylesheet
 * @prefix: the namespace prefix (possibly NULL)
 *
 * Check if the given prefix is one of the declared extensions
 *
 * Returns 1 if this is an extension, 0 otherwise
 */
int
xsltCheckExtPrefix(xsltStylesheetPtr style, const xmlChar *prefix) {
    xsltExtDefPtr cur;

    if ((style == NULL) || (style->nsDefs == NULL))
	return(0);

    cur = (xsltExtDefPtr) style->nsDefs;
    while (cur != NULL) {
	if (xmlStrEqual(prefix, cur->prefix))
	    return(1);
	cur = cur->next;
    }
    return(0);
}

