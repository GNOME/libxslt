/*
 * imports.c: Implementation of the XSLT imports
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

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_MATH_H
#include <math.h>
#endif
#ifdef HAVE_FLOAT_H
#include <float.h>
#endif
#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#ifdef HAVE_NAN_H
#include <nan.h>
#endif
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#include <libxml/xmlmemory.h>
#include <libxml/tree.h>
#include <libxml/hash.h>
#include <libxml/xmlerror.h>
#include <libxml/uri.h>
#include "xslt.h"
#include "xsltInternals.h"
#include "xsltutils.h"
#include "imports.h"
#include "documents.h"



/************************************************************************
 *									*
 *			Module interfaces				*
 *									*
 ************************************************************************/

/**
 * xsltParseStylesheetImport:
 * @style:  the XSLT stylesheet
 * @template:  the "strip-space" element
 *
 * parse an XSLT stylesheet strip-space element and record
 * elements needing stripping
 */

void
xsltParseStylesheetImport(xsltStylesheetPtr style, xmlNodePtr cur) {
    xmlDocPtr import = NULL;
    xmlChar *base = NULL;
    xmlChar *uriRef = NULL;
    xmlChar *URI = NULL;
    xsltStylesheetPtr res;

    if ((cur == NULL) || (style == NULL))
	return;

    uriRef = xsltGetNsProp(cur, (const xmlChar *)"href", XSLT_NAMESPACE);
    if (uriRef == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsl:import : missing href attribute\n");
	goto error;
    }

    base = xmlNodeGetBase(style->doc, cur);
    URI = xmlBuildURI(uriRef, base);
    if (URI == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsl:import : invalid URI reference %s\n", uriRef);
	goto error;
    }
    import = xmlParseFile((const char *)URI);
    if (import == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsl:import : unable to load %s\n", URI);
	goto error;
    }

    res = xsltParseStylesheetDoc(import);
    if (res != NULL) {
	res->parent = style;
	res->next = style->imports;
	style->imports = res;
    }

error:
    if (uriRef != NULL)
	xmlFree(uriRef);
    if (base != NULL)
	xmlFree(base);
    if (URI != NULL)
	xmlFree(URI);
}

/**
 * xsltParseStylesheetInclude:
 * @style:  the XSLT stylesheet
 * @template:  the "strip-space" element
 *
 * parse an XSLT stylesheet strip-space element and record
 * elements needing stripping
 */

void
xsltParseStylesheetInclude(xsltStylesheetPtr style, xmlNodePtr cur) {
    xmlDocPtr oldDoc;
    xmlChar *base = NULL;
    xmlChar *uriRef = NULL;
    xmlChar *URI = NULL;
    xsltDocumentPtr include;

    if ((cur == NULL) || (style == NULL))
	return;

    uriRef = xsltGetNsProp(cur, (const xmlChar *)"href", XSLT_NAMESPACE);
    if (uriRef == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsl:include : missing href attribute\n");
	goto error;
    }

    base = xmlNodeGetBase(style->doc, cur);
    URI = xmlBuildURI(uriRef, base);
    if (URI == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsl:include : invalid URI reference %s\n", uriRef);
	goto error;
    }

    include = xsltLoadStyleDocument(style, URI);
    if (include == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsl:include : unable to load %s\n", URI);
	goto error;
    }

    oldDoc = style->doc;
    style->doc = include->doc;
    xsltParseStylesheetProcess(style, include->doc);
    style->doc = oldDoc;

error:
    if (uriRef != NULL)
	xmlFree(uriRef);
    if (base != NULL)
	xmlFree(base);
    if (URI != NULL)
	xmlFree(URI);
}

/**
 * xsltNextImport:
 * @cur:  the current XSLT stylesheet
 *
 * Find the next stylesheet in import precedence.
 *
 * Returns the next stylesheet or NULL if it was the last one
 */

xsltStylesheetPtr
xsltNextImport(xsltStylesheetPtr cur) {
    if (cur == NULL)
	return(NULL);
    if (cur->imports != NULL)
	return(cur->imports);
    if (cur->next != NULL)
	return(cur->next) ;
    do {
	cur = cur->parent;
	if (cur == NULL) return(NULL);
	if (cur->next != NULL) return(cur->next);
    } while (cur != NULL);
    return(cur);
}

/**
 * xsltFindElemSpaceHandling:
 * @ctxt:  an XSLT transformation context
 * @node:  an XML node
 *
 * Find strip-space or preserve-space informations for an element
 * respect the import precedence or the wildcards
 *
 * Returns 1 if space should be stripped, 0 if not, and 2 if everything
 *         should be CDTATA wrapped.
 */

int
xsltFindElemSpaceHandling(xsltTransformContextPtr ctxt, xmlNodePtr node) {
    xsltStylesheetPtr style;
    const xmlChar *val;

    if ((ctxt == NULL) || (node == NULL))
	return(0);
    style = ctxt->style;
    while (style != NULL) {
	/* TODO: add namespaces support */
	val = (const xmlChar *)
	      xmlHashLookup(style->stripSpaces, node->name);
	if (val != NULL) {
	    if (xmlStrEqual(val, (xmlChar *) "strip"))
		return(1);
	    if (xmlStrEqual(val, (xmlChar *) "preserve"))
		return(0);
	} 
	if (ctxt->style->stripAll == 1)
	    return(1);
	if (ctxt->style->stripAll == -1)
	    return(0);

	style = xsltNextImport(style);
    }
    return(0);
}

/**
 * xsltFindTemplate:
 * @ctxt:  an XSLT transformation context
 * @name: the template name
 * @nameURI: the template name URI
 *
 * Finds the named template, apply import precedence rule.
 *
 * Returns the xsltTemplatePtr or NULL if not found
 */
xsltTemplatePtr
xsltFindTemplate(xsltTransformContextPtr ctxt, const xmlChar *name,
	         const xmlChar *nameURI) {
    xsltTemplatePtr cur;
    xsltStylesheetPtr style;

    if ((ctxt == NULL) || (name == NULL))
	return(NULL);
    style = ctxt->style;
    while (style != NULL) {
	cur = style->templates;
	while (cur != NULL) {
	    if (xmlStrEqual(name, cur->name)) {
		if (((nameURI == NULL) && (cur->nameURI == NULL)) ||
		    ((nameURI != NULL) && (cur->nameURI != NULL) &&
		     (xmlStrEqual(nameURI, cur->nameURI)))) {
		    return(cur);
		}
	    }
	    cur = cur->next;
	}

	style = xsltNextImport(style);
    }
    return(NULL);
}

