/*
 * documents.c: Implementation of the documents handling
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
#include "xslt.h"
#include "xsltInternals.h"
#include "xsltutils.h"
#include "documents.h"
#include "keys.h"

#define DEBUG_DOCUMENTS


/************************************************************************
 *									*
 *			Module interfaces				*
 *									*
 ************************************************************************/

/**
 * xsltNewDocument:
 * @ctxt: an XSLT transformation context
 * @doc:  a parsed XML document
 *
 * Register a new document, apply key computations
 */
xsltDocumentPtr	
xsltNewDocument(xsltTransformContextPtr ctxt, xmlDocPtr doc) {
    xsltDocumentPtr cur;

    cur = (xsltDocumentPtr) xmlMalloc(sizeof(xsltDocument));
    if (cur == NULL) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltNewDocument : malloc failed\n");
	return(NULL);
    }
    memset(cur, 0, sizeof(xsltDocument));
    cur->doc = doc;
    cur->next = ctxt->docList;
    ctxt->docList = cur;
    xsltInitCtxtKeys(ctxt, cur);
    return(cur);
}

/**
 * xsltFreeDocuments:
 * @ctxt: an XSLT transformation context
 *
 * Free up all the space used by the loaded documents
 */
void	
xsltFreeDocuments(xsltTransformContextPtr ctxt) {
    xsltDocumentPtr doc, cur;

    cur = ctxt->document;
    while (cur != NULL) {
	doc = cur;
	cur = cur->next;
	xsltFreeDocumentKeys(doc);
	if (!doc->main)
	    xmlFreeDoc(doc->doc);
        xmlFree(doc);
    }
}


/**
 * xsltLoadDocument:
 * @ctxt: an XSLT transformation context
 * @URI:  the computed URI of the document
 *
 * Try to load a document within the XSLT transformation context
 *
 * Returns the new xsltDocumentPtr or NULL in case of error
 */
xsltDocumentPtr	
xsltLoadDocument(xsltTransformContextPtr ctxt, const xmlChar *URI) {
    xsltDocumentPtr ret;
    xmlDocPtr doc;

    if ((ctxt == NULL) || (URI == NULL))
	return(NULL);

    /*
     * Walk the context list to find the document if preparsed
     */
    ret = ctxt->docList;
    while (ret != NULL) {
	if ((ret->doc != NULL) && (ret->doc->URL != NULL) &&
	    (xmlStrEqual(ret->doc->URL, URI)))
	    return(ret);
	ret = ret->next;
    }

    doc = xmlParseFile((const char *) URI);
    if (doc == NULL)
	return(NULL);

    ret = xsltNewDocument(ctxt, doc);
    return(ret);
}

