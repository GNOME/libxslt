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
#include "imports.h"
#include "transform.h"

#define DEBUG_ATTRIBUTES

/*
 * TODO: merge attribute sets from different import precedence.
 *       all this should be precomputed just before the transformation
 *       starts or at first hit with a cache in the context.
 *       The simple way for now would be to not allow redefinition of
 *       attributes once generated in the output tree, possibly costlier.
 */

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
static xsltAttrElemPtr
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
static void
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
static void
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
static xsltAttrElemPtr
xsltAddAttrElemList(xsltAttrElemPtr list, xmlNodePtr attr) {
    xsltAttrElemPtr next, cur;

    if (attr == NULL)
	return(list);
    if (list == NULL)
	return(xsltNewAttrElem(attr));
    cur = list;
    while (cur != NULL) {
	next = cur->next;
	if (cur->attr == attr)
	    return(cur);
	if (cur->next == NULL) {
	    cur->next = xsltNewAttrElem(attr);
	}
	cur = next;
    }
    return(list);
}

/**
 * xsltMergeAttrElemList:
 * @list:  an XSLT AttrElem list
 * @old:  another XSLT AttrElem list
 *
 * Add all the attributes from list @old to list @list,
 * but drop redefinition of existing values.
 *
 * Returns the new list pointer
 */
static xsltAttrElemPtr
xsltMergeAttrElemList(xsltAttrElemPtr list, xsltAttrElemPtr old) {
    xsltAttrElemPtr cur;
    int add;

    while (old != NULL) {

	/*
	 * Check taht the attribute is not yet in the list
	 */
	cur = list;
	add = 1;
	while (cur != NULL) {
	    if (cur->attr == old->attr) {
		xsltGenericError(xsltGenericErrorContext,
	     "xslt:attribute-set : use-attribute-sets recursion detected\n");
		return(list);
	    }
	    if (xmlStrEqual(cur->attr->name, old->attr->name)) {
		if (cur->attr->ns == old->attr->ns) {
		    add = 0;
		    break;
		}
		if ((cur->attr->ns != NULL) && (old->attr->ns != NULL) &&
		    (xmlStrEqual(cur->attr->ns->href, old->attr->ns->href))) {
		    add = 0;
		    break;
		}
	    }
	    if (cur->next == NULL)
		break;
            cur = cur->next;
	}

	if (cur == NULL) {
	    list = xsltNewAttrElem(old->attr);
	} else if (add) {
	    cur->next = xsltNewAttrElem(old->attr);
	}

	old = old->next;
    }
    return(list);
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

    if (style->attributeSets == NULL) {
#ifdef DEBUG_ATTRIBUTES
	xsltGenericDebug(xsltGenericDebugContext,
	    "creating attribute set table\n");
#endif
	style->attributeSets = xmlHashCreate(10);
    }
    if (style->attributeSets == NULL)
	goto error;

    values = xmlHashLookup2(style->attributeSets, ncname, prefix);

    /*
     * check the children list
     */
    list = cur->children;
    delete = NULL;
    while (list != NULL) {
	if (IS_XSLT_ELEM(list)) {
	    if (!IS_XSLT_NAME(list, "attribute")) {
		xsltGenericError(xsltGenericErrorContext,
		    "xslt:attribute-set : unexpected child xsl:%s\n",
		                 list->name);
		delete = list;
	    } else {
#ifdef DEBUG_ATTRIBUTES
		xsltGenericDebug(xsltGenericDebugContext,
		    "add attribute to list %s\n", ncname);
#endif
                values = xsltAddAttrElemList(values, list);
	    }
	} else {
	    xsltGenericError(xsltGenericErrorContext,
		"xslt:attribute-set : unexpected child %s\n", list->name);
	    delete = list;
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
	    xmlChar *ncname2 = NULL;
	    xmlChar *prefix2 = NULL;
	    xsltAttrElemPtr values2;
#ifdef DEBUG_ATTRIBUTES
	    xsltGenericDebug(xsltGenericDebugContext,
		"xslt:attribute-set : %s adds use %s\n", ncname, attribute);
#endif
	    ncname2 = xmlSplitQName2(attribute, &prefix2);
	    if (ncname2 == NULL) {
		ncname2 = attribute;
		attribute = NULL;
		prefix = NULL;
	    }
	    values2 = xmlHashLookup2(style->attributeSets, ncname2, prefix2);
	    values = xsltMergeAttrElemList(values, values2);

	    if (attribute != NULL)
		xmlFree(attribute);
	    if (ncname2 != NULL)
		xmlFree(ncname2);
	    if (prefix2 != NULL)
		xmlFree(prefix2);
	}
	attribute = end;
    }
    xmlFree(attributes);

done:
    /*
     * Update the value
     */
    xmlHashUpdateEntry2(style->attributeSets, ncname, prefix, values, NULL);
#ifdef DEBUG_ATTRIBUTES
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
	              xmlNodePtr inst ATTRIBUTE_UNUSED, xmlChar *attributes) {
    xmlChar *ncname = NULL;
    xmlChar *prefix = NULL;
    xmlChar *attribute, *end;
    xsltAttrElemPtr values;
    xsltStylesheetPtr style;

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
#ifdef DEBUG_ATTRIBUTES
	    xsltGenericDebug(xsltGenericDebugContext,
		"apply attribute set %s\n", attribute);
#endif
	    ncname = xmlSplitQName2(attribute, &prefix);
	    if (ncname == NULL) {
		ncname = attribute;
		attribute = NULL;
		prefix = NULL;
	    }

	    style = ctxt->style;
	    while (style != NULL) {
		values = xmlHashLookup2(style->attributeSets, ncname, prefix);
		while (values != NULL) {
		    xsltAttribute(ctxt, node, values->attr, NULL);
		    values = values->next;
		}
		style = xsltNextImport(style);
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
