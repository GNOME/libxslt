/*
 * attributes.c: Implementation of the XSLT attributes handling
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
#include "attributes.h"
#include "namespaces.h"
#include "templates.h"

/*
 * Useful macros
 */

#define IS_BLANK(c) (((c) == 0x20) || ((c) == 0x09) || ((c) == 0xA) ||	\
                     ((c) == 0x0D))

#define IS_BLANK_NODE(n)						\
    (((n)->type == XML_TEXT_NODE) && (xsltIsBlank((n)->content)))


/*
 * The in-memory structure corresponding to an XSLT Attribute in
 * an attribute set
 */

typedef struct _xsltAttrElem xsltAttrElem;
typedef xsltAttrElem *xsltAttrElemPtr;
struct _xsltAttrElem {
    struct _xsltAttrElem *next;/* chained list */
    xmlNodePtr attr;	/* the xsl:attribute definition */
};

/************************************************************************
 *									*
 *			XSLT Attribute handling				*
 *									*
 ************************************************************************/

/**
 * xsltNewAttrElem:
 * @attr:  the new xsl:attribute node
 *
 * Create a new XSLT AttrElem
 *
 * Returns the newly allocated xsltAttrElemPtr or NULL in case of error
 */
xsltAttrElemPtr
xsltNewAttrElem(xmlNodePtr attr) {
    xsltAttrElemPtr cur;

    cur = (xsltAttrElemPtr) xmlMalloc(sizeof(xsltAttrElem));
    if (cur == NULL) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltNewAttrElem : malloc failed\n");
	return(NULL);
    }
    memset(cur, 0, sizeof(xsltAttrElem));
    cur->attr = attr;
    return(cur);
}

/**
 * xsltFreeAttrElem:
 * @attr:  an XSLT AttrElem
 *
 * Free up the memory allocated by @attr
 */
void
xsltFreeAttrElem(xsltAttrElemPtr attr) {
    memset(attr, -1, sizeof(xsltAttrElem));
    xmlFree(attr);
}

/**
 * xsltFreeAttrElemList:
 * @list:  an XSLT AttrElem list
 *
 * Free up the memory allocated by @list
 */
void
xsltFreeAttrElemList(xsltAttrElemPtr list) {
    xsltAttrElemPtr next;
    
    while (list != NULL) {
	next = list->next;
	xsltFreeAttrElem(list);
	list = next;
    }
}

/**
 * xsltAddAttrElemList:
 * @list:  an XSLT AttrElem list
 * @attr:  the new xsl:attribute node
 *
 * Add the new attribute to the list.
 *
 * Returns the new list pointer
 */
xsltAttrElemPtr
xsltAddAttrElemList(xsltAttrElemPtr list, xmlNodePtr attr) {
    xsltAttrElemPtr next;

    if (attr == NULL)
	return(list);
    while (list != NULL) {
	next = list->next;
	if (list->attr == attr)
	    return(list);
	if (next == NULL) {
	    list->next = xsltNewAttrElem(attr);
	    return(list);
	}
	list = next;
    }
    return(xsltNewAttrElem(attr));
}
/************************************************************************
 *									*
 *			Module interfaces				*
 *									*
 ************************************************************************/

/**
 * xsltParseStylesheetAttributeSet:
 * @style:  the XSLT stylesheet
 * @template:  the "preserve-space" element
 *
 * parse an XSLT stylesheet preserve-space element and record
 * elements needing preserving
 */

void
xsltParseStylesheetAttributeSet(xsltStylesheetPtr style, xmlNodePtr cur) {
    xmlChar *prop = NULL;
    xmlChar *ncname = NULL;
    xmlChar *prefix = NULL;
    xmlChar *attributes;
    xmlChar *attribute, *end;
    xmlNodePtr list, delete;
    xsltAttrElemPtr values;

    if ((cur == NULL) || (style == NULL))
	return;

    prop = xmlGetNsProp(cur, (const xmlChar *)"name", XSLT_NAMESPACE);
    if (prop == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:attribute-set : name is missing\n");
	goto error;
    }

    ncname = xmlSplitQName2(prop, &prefix);
    if (ncname == NULL) {
	ncname = prop;
	prop = NULL;
	prefix = NULL;
    }

    if (style->attributeSets == NULL)
	style->attributeSets = xmlHashCreate(10);
    if (style->attributeSets == NULL)
	goto error;

    values = xmlHashLookup2(style->attributeSets, ncname, prefix);

    /*
     * check the children list
     */
    list = cur->children;
    delete = NULL;
    while (list != NULL) {
	if (IS_XSLT_ELEM(cur)) {
	    if (!IS_XSLT_NAME(cur, "attribute")) {
		xsltGenericError(xsltGenericErrorContext,
		    "xslt:attribute-set : unexpected child xsl:%s\n",
		                 cur->name);
		delete = cur;
	    } else {
#ifdef DEBUG_PARSING
		xsltGenericDebug(xsltGenericDebugContext,
		    "add attribute to list %s\n", ncname);
#endif
                values = xsltAddAttrElemList(values, cur);
	    }
	} else {
	    xsltGenericError(xsltGenericErrorContext,
		"xslt:attribute-set : unexpected child %s\n", cur->name);
	    delete = cur;
	}
	list = list->next;
    }

    /*
     * Check a possible use-attribute-sets definition
     */
    /* TODO check recursion */

    attributes = xmlGetNsProp(cur, (const xmlChar *)"use-attribute-sets",
	                      XSLT_NAMESPACE);
    if (attributes == NULL) {
	goto done;
    }

    attribute = attributes;
    while (*attribute != 0) {
	while (IS_BLANK(*attribute)) attribute++;
	if (*attribute == 0)
	    break;
        end = attribute;
	while ((*end != 0) && (!IS_BLANK(*end))) end++;
	attribute = xmlStrndup(attribute, end - attribute);
	if (attribute) {
#ifdef DEBUG_PARSING
	    xsltGenericDebug(xsltGenericDebugContext,
		"xslt:attribute-set : %s adds use %s\n", ncname);
#endif
	    TODO /* add use-attribute-sets support to atribute-set */
	}
	attribute = end;
    }
    xmlFree(attributes);

done:
    /*
     * Update the value
     */
    xmlHashUpdateEntry2(style->attributeSets, ncname, prefix, values, NULL);
#ifdef DEBUG_PARSING
    xsltGenericDebug(xsltGenericDebugContext,
	"updated attribute list %s\n", ncname);
#endif

error:
    if (prop != NULL)
        xmlFree(prop);
    if (ncname != NULL)
        xmlFree(ncname);
    if (prefix != NULL)
        xmlFree(prefix);
}

/**
 * xsltAttribute:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt attribute node
 *
 * Process the xslt attribute node on the source node
 */
void
xsltAttribute(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst) {
    xmlChar *prop = NULL;
    xmlChar *ncname = NULL;
    xmlChar *prefix = NULL;
    xmlChar *value = NULL;
    xmlNsPtr ns = NULL;
    xmlAttrPtr attr;


    if (ctxt->insert == NULL)
	return;
    if (ctxt->insert->children != NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:attribute : node has already children\n");
	return;
    }
    prop = xsltEvalAttrValueTemplate(ctxt, inst, (const xmlChar *)"name");
    if (prop == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:attribute : name is missing\n");
	goto error;
    }

    ncname = xmlSplitQName2(prop, &prefix);
    if (ncname == NULL) {
	ncname = prop;
	prop = NULL;
	prefix = NULL;
    }
    if (xmlStrEqual(ncname, (const xmlChar *) "xmlns")) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:attribute : xmlns forbidden\n");
	goto error;
    }
    prop = xsltEvalAttrValueTemplate(ctxt, inst, (const xmlChar *)"namespace");
    if (prop != NULL) {
	TODO /* xsl:attribute namespace */
	xmlFree(prop);
	return;
    } else {
	if (prefix != NULL) {
	    ns = xmlSearchNs(inst->doc, inst, prefix);
	    if (ns == NULL) {
		xsltGenericError(xsltGenericErrorContext,
		    "no namespace bound to prefix %s\n", prefix);
	    } else {
		ns = xsltGetNamespace(ctxt, inst, ns, ctxt->insert);
	    }
	}
    }
    

    value = xsltEvalTemplateString(ctxt, node, inst);
    if (value == NULL) {
	if (ns) {
	    attr = xmlSetNsProp(ctxt->insert, ns, ncname, 
		                (const xmlChar *)"");
	} else
	    attr = xmlSetProp(ctxt->insert, ncname, (const xmlChar *)"");
    } else {
	if (ns) {
	    attr = xmlSetNsProp(ctxt->insert, ns, ncname, value);
	} else
	    attr = xmlSetProp(ctxt->insert, ncname, value);
	
    }

error:
    if (prop != NULL)
        xmlFree(prop);
    if (ncname != NULL)
        xmlFree(ncname);
    if (prefix != NULL)
        xmlFree(prefix);
    if (value != NULL)
        xmlFree(value);
}

/**
 * xsltApplyAttributeSet:
 * @ctxt:  the XSLT stylesheet
 * @node:  the node in the source tree.
 * @inst:  the xslt attribute node
 * @attributes:  the set list.
 *
 * Apply the xsl:use-attribute-sets
 */

void
xsltApplyAttributeSet(xsltTransformContextPtr ctxt, xmlNodePtr node,
	              xmlNodePtr inst, xmlChar *attributes) {
    xmlChar *ncname = NULL;
    xmlChar *prefix = NULL;
    xmlChar *attribute, *end;
    xsltAttrElemPtr values;

    if (attributes == NULL) {
	return;
    }

    attribute = attributes;
    while (*attribute != 0) {
	while (IS_BLANK(*attribute)) attribute++;
	if (*attribute == 0)
	    break;
        end = attribute;
	while ((*end != 0) && (!IS_BLANK(*end))) end++;
	attribute = xmlStrndup(attribute, end - attribute);
	if (attribute) {
#ifdef DEBUG_PARSING
	    xsltGenericDebug(xsltGenericDebugContext,
		"apply attribute set %s\n", attribute);
#endif
	    ncname = xmlSplitQName2(attribute, &prefix);
	    if (ncname == NULL) {
		ncname = attribute;
		attribute = NULL;
		prefix = NULL;
	    }

	    /* TODO: apply cascade */
	    values = xmlHashLookup2(ctxt->style->attributeSets, ncname, prefix);
	    while (values != NULL) {
		xsltAttribute(ctxt, node, values->attr);
		values = values->next;
	    }
	    if (attribute != NULL)
		xmlFree(attribute);
	    if (ncname != NULL)
		xmlFree(ncname);
	    if (prefix != NULL)
		xmlFree(prefix);
	}
	attribute = end;
    }
}

/**
 * xsltFreeAttributeSetsHashes:
 * @style: an XSLT stylesheet
 *
 * Free up the memory used by attribute sets
 */
void
xsltFreeAttributeSetsHashes(xsltStylesheetPtr style) {
    if (style->attributeSets != NULL)
	xmlHashFree((xmlHashTablePtr) style->attributeSets,
		    (xmlHashDeallocator) xsltFreeAttrElemList);
    style->attributeSets = NULL;
}
