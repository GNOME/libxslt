/*
 * attrvt.c: Implementation of the XSL Transformation 1.0 engine
 *           attribute value template handling part.
 *
 * References:
 *   http://www.w3.org/TR/1999/REC-xslt-19991116
 *
 *   Michael Kay "XSLT Programmer's Reference" pp 637-643
 *   Writing Multiple Output Files
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#define IN_LIBXSLT
#include "libxslt.h"

#include <string.h>

#include <libxml/xmlmemory.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include "xslt.h"
#include "xsltutils.h"
#include "xsltInternals.h"
#include "templates.h"

#ifdef WITH_XSLT_DEBUG
#define WITH_XSLT_DEBUG_AVT
#define WITH_XSLT_DEBUG_AVT
#endif

#define MAX_AVT_SEG 10

typedef struct _xsltAttrVT xsltAttrVT;
typedef xsltAttrVT *xsltAttrVTPtr;
struct _xsltAttrVT {
    struct _xsltAttrVT *next; /* next xsltAttrVT */
    int nb_seg;		/* Number of segments */
    int strstart;	/* is the start a string */
    /*
     * the namespaces in scope
     */
    xmlNsPtr *nsList;
    int nsNr;
    /*
     * the content is an alternate of string and xmlXPathCompExprPtr
     */
    void *segments[MAX_AVT_SEG];
};

/**
 * xsltNewAttrVT:
 * @style:  a XSLT process context
 *
 * Build a new xsltAttrVT structure
 *
 * Returns the structure or NULL in case of error
 */
static xsltAttrVTPtr
xsltNewAttrVT(xsltStylesheetPtr style) {
    xsltAttrVTPtr cur;

    cur = (xsltAttrVTPtr) xmlMalloc(sizeof(xsltAttrVT));
    if (cur == NULL) {
	xsltTransformError(NULL, style, NULL,
		"xsltNewAttrVTPtr : malloc failed\n");
	if (style != NULL) style->errors++;
	return(NULL);
    }
    memset(cur, 0, sizeof(xsltAttrVT));

    cur->nb_seg = 0;
    cur->strstart = 0;
    cur->next = style->attVTs;
    style->attVTs = (xsltAttrVTPtr) cur;

    return(cur);
}

/**
 * xsltFreeAttrVT:
 * @avt: pointer to an xsltAttrVT structure
 *
 * Free up the memory associated to the attribute value template
 */
static void
xsltFreeAttrVT(xsltAttrVTPtr avt) {
    int i;

    if (avt == NULL) return;

    if (avt->strstart == 1) {
	for (i = 0;i < avt->nb_seg; i += 2)
	    if (avt->segments[i] != NULL)
		xmlFree((xmlChar *) avt->segments[i]);
	for (i = 1;i < avt->nb_seg; i += 2)
	    xmlXPathFreeCompExpr((xmlXPathCompExprPtr) avt->segments[i]);
    } else {
	for (i = 0;i < avt->nb_seg; i += 2)
	    xmlXPathFreeCompExpr((xmlXPathCompExprPtr) avt->segments[i]);
	for (i = 1;i < avt->nb_seg; i += 2)
	    if (avt->segments[i] != NULL)
		xmlFree((xmlChar *) avt->segments[i]);
    }
    if (avt->nsList != NULL)
        xmlFree(avt->nsList);
    xmlFree(avt);
}

/**
 * xsltFreeAVTList:
 * @avt: pointer to an list of AVT structures
 *
 * Free up the memory associated to the attribute value templates
 */
void
xsltFreeAVTList(void *avt) {
    xsltAttrVTPtr cur = (xsltAttrVTPtr) avt, next;

    while (cur != NULL) {
        next = cur->next;
	xsltFreeAttrVT(cur);
	cur = next;
    }
}

/**
 * xsltCompileAttr:
 * @style:  a XSLT process context
 * @attr: the attribute coming from the stylesheet.
 *
 * Precompile an attribute in a stylesheet, basically it checks if it is
 * an attrubute value template, and if yes establish some structures needed
 * to process it at transformation time.
 */
void
xsltCompileAttr(xsltStylesheetPtr style, xmlAttrPtr attr) {
    const xmlChar *str;
    const xmlChar *cur;
    xmlChar *ret = NULL;
    xmlChar *expr = NULL;
    xsltAttrVTPtr avt;
    int i = 0, lastavt = 0;

    if ((style == NULL) || (attr == NULL) || (attr->children == NULL))
        return;
    if ((attr->children->type != XML_TEXT_NODE) || 
        (attr->children->next != NULL)) {
        xsltTransformError(NULL, style, attr->parent,
	        "Attribute %s content is not a text node\n", attr->name);
	style->errors++;
	return;
    }
    str = attr->children->content;
    if ((xmlStrchr(str, '{') == NULL) &&
        (xmlStrchr(str, '}') == NULL)) return;

    avt = xsltNewAttrVT(style);
    if (avt == NULL) return;
    attr->_private = avt;

    avt->nsList = xmlGetNsList(attr->doc, attr->parent);
    if (avt->nsList != NULL) {
	while (avt->nsList[i] != NULL)
	    i++;
    }
    avt->nsNr = i;

    cur = str;
    while (*cur != 0) {
	if (*cur == '{') {
	    if (*(cur+1) == '{') {	/* escaped '{' */
	        cur++;
		ret = xmlStrncat(ret, str, cur - str);
		cur++;
		str = cur;
		continue;
	    }
	    if (*(cur+1) == '}') {	/* skip empty AVT */
		ret = xmlStrncat(ret, str, cur - str);
	        cur+=2;
		str = cur;
		continue;
	    }
	    if ((ret != NULL) || (cur - str > 0)) {
		ret = xmlStrncat(ret, str, cur - str);
		str = cur;
		if (avt->nb_seg == 0)
		    avt->strstart = 1;
		avt->segments[avt->nb_seg++] = (void *) ret;
		ret = NULL;
		lastavt = 0;
	    }

	    cur++;
	    while ((*cur != 0) && (*cur != '}')) cur++;
	    if (*cur == 0) {
	        xsltTransformError(NULL, style, attr->parent,
		     "Attribute template %s: unmatched '{'\n", attr->name);
		style->errors++;
		goto error;
	    }
	    str++;
	    expr = xmlStrndup(str, cur - str);
	    if (expr == NULL) {
	        XSLT_TODO
		goto error;
	    } else {
		xmlXPathCompExprPtr comp;

		comp = xsltXPathCompile(style, expr);
		if (comp == NULL) {
		    xsltTransformError(NULL, style, attr->parent,
			 "Attribute template %s: failed to compile %s\n",
			               attr->name, expr);
		    style->errors++;
		    goto error;
		}
		if (avt->nb_seg == 0)
		    avt->strstart = 0;
		if (lastavt == 1)
		    avt->segments[avt->nb_seg++] = NULL;
		avt->segments[avt->nb_seg++] = (void *) comp;
		lastavt = 1;
		xmlFree(expr);
		expr = NULL;
	    }
	    cur++;
	    str = cur;
	} else if (*cur == '}') {
	    cur++;
	    if (*cur == '}') {	/* escaped '}' */
		ret = xmlStrncat(ret, str, cur - str);
		cur++;
		str = cur;
		continue;
	    } else {
	        xsltTransformError(NULL, style, attr->parent,
		     "Attribute template %s: unmatched '}'\n", attr->name);
		goto error;
	    }
	} else
	    cur++;
    }
    if ((ret != NULL) || (cur - str > 0)) {
	ret = xmlStrncat(ret, str, cur - str);
	str = cur;
	if (avt->nb_seg == 0)
	    avt->strstart = 1;
	avt->segments[avt->nb_seg++] = (void *) ret;
	ret = NULL;
    }

error:
    if (ret != NULL)
	xmlFree(ret);
    if (expr != NULL)
	xmlFree(expr);
}


/**
 * xsltEvalAVT:
 * @ctxt: the XSLT transformation context
 * @avt: the prevompiled attribute value template info
 * @node: the node hosting the attribute
 *
 * Process the given AVT, and return the new string value.
 *
 * Returns the computed string value or NULL, must be deallocated by the
 *         caller.
 */
xmlChar *
xsltEvalAVT(xsltTransformContextPtr ctxt, void *avt, xmlNodePtr node) {
    xmlChar *ret = NULL, *tmp;
    xmlXPathCompExprPtr comp;
    xsltAttrVTPtr cur = (xsltAttrVTPtr) avt;
    int i;
    int str;

    if ((ctxt == NULL) || (avt == NULL) || (node == NULL))
        return(NULL);
    str = cur->strstart;
    for (i = 0;i < cur->nb_seg;i++) {
        if (str) {
	    ret = xmlStrcat(ret, (const xmlChar *) cur->segments[i]);
	} else {
	    comp = (xmlXPathCompExprPtr) cur->segments[i];
	    tmp = xsltEvalXPathStringNs(ctxt, comp, cur->nsNr, cur->nsList);
	    if (tmp != NULL) {
	        if (ret != NULL) {
		    ret = xmlStrcat(ret, tmp);
		    xmlFree(tmp);
		} else {
		    ret = tmp;
		}
	    }
	}
	str = !str;
    }
    return(ret);
}
