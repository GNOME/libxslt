/*
 * namespaces.c: Implementation of the XSLT namespaces handling
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
#include "namespaces.h"
#include "imports.h"



/************************************************************************
 *									*
 *			Module interfaces				*
 *									*
 ************************************************************************/

/**
 * xsltNamespaceAlias:
 * @style:  the XSLT stylesheet
 * @node:  the xsl:namespace-alias node
 *
 * Read the stylesheet-prefix and result-prefix attributes, register
 * them as well as the corresponding namespace.
 */
void
xsltNamespaceAlias(xsltStylesheetPtr style, xmlNodePtr node) {
    xmlChar *sprefix;
    xmlNsPtr sNs;
    xmlChar *rprefix;
    xmlNsPtr rNs;

    sprefix = xmlGetNsProp(node, (const xmlChar *)"stylesheet-prefix",
	                   XSLT_NAMESPACE);
    if (sprefix == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "namespace-alias: stylesheet-prefix attribute missing\n");
	return;
    }
    rprefix = xmlGetNsProp(node, (const xmlChar *)"result-prefix",
	                   XSLT_NAMESPACE);
    if (rprefix == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "namespace-alias: result-prefix attribute missing\n");
	goto error;
    }
    if (xmlStrEqual(sprefix, (const xmlChar *)"#default")) {
	sNs = xmlSearchNs(node->doc, node, NULL);
    } else {
	sNs = xmlSearchNs(node->doc, node, sprefix);
    }
    if ((sNs == NULL) || (sNs->href == NULL)) {
	xsltGenericError(xsltGenericErrorContext,
	    "namespace-alias: prefix %s not bound to any namespace\n",
	                 sprefix);
	goto error;
    }
    if (xmlStrEqual(rprefix, (const xmlChar *)"#default")) {
	rNs = xmlSearchNs(node->doc, node, NULL);
    } else {
	rNs = xmlSearchNs(node->doc, node, rprefix);
    }
    if ((rNs == NULL) || (rNs->href == NULL)) {
	xsltGenericError(xsltGenericErrorContext,
	    "namespace-alias: prefix %s not bound to any namespace\n",
	                 rprefix);
	goto error;
    }
    if (style->nsAliases == NULL)
	style->nsAliases = xmlHashCreate(10);
    if (style->nsAliases == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "namespace-alias: cannot create hash table\n");
	goto error;
    }
    xmlHashAddEntry((xmlHashTablePtr) style->nsAliases,
	            sNs->href, (void *) rNs->href);

error:
    if (sprefix != NULL)
	xmlFree(sprefix);
    if (rprefix != NULL)
	xmlFree(rprefix);
}

/**
 * xsltGetSpecificNamespace:
 * @ctxt:  a transformation context
 * @cur:  the input node
 * @URI:  the namespace URI
 * @prefix:  the suggested prefix
 * @out:  the output node (or its parent)
 *
 * Find the right namespace value for this URI, if needed create
 * and add a new namespace decalaration on the node
 *
 * Returns the namespace node to use or NULL
 */
xmlNsPtr
xsltGetSpecialNamespace(xsltTransformContextPtr ctxt, xmlNodePtr cur,
		const xmlChar *URI, const xmlChar *prefix, xmlNodePtr out) {
    xmlNsPtr ret;
    static int prefixno = 1;
    char nprefix[10];

    if ((ctxt == NULL) || (cur == NULL) || (out == NULL) || (URI == NULL))
	return(NULL);

    if ((out->parent != NULL) &&
	(out->parent->type == XML_ELEMENT_NODE) &&
	(out->parent->ns != NULL) &&
	(xmlStrEqual(out->parent->ns->href, URI)))
	ret = out->parent->ns;
    else
	ret = xmlSearchNsByHref(out->doc, out, URI);

    if (ret == NULL) {
	if (prefix == NULL) {
	    do {
		sprintf(nprefix, "ns%d", prefixno++);
		ret = xmlSearchNs(out->doc, out, (xmlChar *)nprefix);
	    } while (ret != NULL);
	    prefix = (const xmlChar *) &nprefix[0];
	}
	if (out->type == XML_ELEMENT_NODE)
	    ret = xmlNewNs(out, URI, prefix);
    }
    return(ret);
}

/**
 * xsltGetNamespace:
 * @ctxt:  a transformation context
 * @cur:  the input node
 * @ns:  the namespace
 * @out:  the output node (or its parent)
 *
 * Find the right namespace value for this prefix, if needed create
 * and add a new namespace decalaration on the node
 * Handle namespace aliases
 *
 * Returns the namespace node to use or NULL
 */
xmlNsPtr
xsltGetNamespace(xsltTransformContextPtr ctxt, xmlNodePtr cur, xmlNsPtr ns,
	         xmlNodePtr out) {
    xsltStylesheetPtr style;
    xmlNsPtr ret;
    const xmlChar *URI = NULL; /* the replacement URI */

    if ((ctxt == NULL) || (cur == NULL) || (out == NULL) || (ns == NULL))
	return(NULL);

    style = ctxt->style;
    while (style != NULL) {
	if (style->nsAliases != NULL)
	    URI = (const xmlChar *) 
		xmlHashLookup(ctxt->style->nsAliases, ns->href);
	if (URI != NULL)
	    break;

	style = xsltNextImport(style);
    }

    if (URI == NULL)
	URI = ns->href;

    if ((out->parent != NULL) &&
	(out->parent->type == XML_ELEMENT_NODE) &&
	(out->parent->ns != NULL) &&
	(xmlStrEqual(out->parent->ns->href, URI)))
	ret = out->parent->ns;
    else
	ret = xmlSearchNsByHref(out->doc, out, URI);

    if (ret == NULL) {
	if (out->type == XML_ELEMENT_NODE)
	    ret = xmlNewNs(out, URI, ns->prefix);
    }
    return(ret);
}

/**
 * xsltCopyNamespaceList:
 * @ctxt:  a transformation context
 * @node:  the target node
 * @cur:  the first namespace
 *
 * Do a copy of an namespace list. If @node is non-NULL the
 * new namespaces are added automatically. This handles namespaces
 * aliases
 *
 * Returns: a new xmlNsPtr, or NULL in case of error.
 */
xmlNsPtr
xsltCopyNamespaceList(xsltTransformContextPtr ctxt, xmlNodePtr node,
	              xmlNsPtr cur) {
    xmlNsPtr ret = NULL;
    xmlNsPtr p = NULL,q;
    const xmlChar *URI;

    while (cur != NULL) {
	if (!xmlStrEqual(cur->href, XSLT_NAMESPACE)) {
	    /* TODO apply cascading */
	    URI = (const xmlChar *) xmlHashLookup(ctxt->style->nsAliases,
		                                  cur->href);
	    if (URI != NULL) {
		q = xmlNewNs(node, URI, cur->prefix);
	    } else {
		q = xmlNewNs(node, cur->href, cur->prefix);
	    }
	    if (p == NULL) {
		ret = p = q;
	    } else {
		p->next = q;
		p = q;
	    }
	}
	cur = cur->next;
    }
    return(ret);
}


/**
 * xsltFreeNamespaceAliasHashes:
 * @style: an XSLT stylesheet
 *
 * Free up the memory used by namespaces aliases
 */
void
xsltFreeNamespaceAliasHashes(xsltStylesheetPtr style) {
    if (style->nsAliases != NULL)
	xmlHashFree((xmlHashTablePtr) style->nsAliases, NULL);
    style->nsAliases = NULL;
}
