/*
 * templates.c: Implementation of the template processing
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
#include <libxml/xmlerror.h>
#include <libxml/xpathInternals.h>
#include <libxml/parserInternals.h>
#include "xslt.h"
#include "xsltInternals.h"
#include "xsltutils.h"
#include "variables.h"
#include "templates.h"

#define DEBUG_TEMPLATES

/************************************************************************
 *									*
 *			Module interfaces				*
 *									*
 ************************************************************************/
 
/**
 * xsltEvalXPathString:
 * @ctxt:  the XSLT transformation context
 * @str:  the XPath expression
 *
 * Process the expression using XPath and get a string
 *
 * Returns the computed string value or NULL, must be deallocated by the
 *    caller.
 */
xmlChar *
xsltEvalXPathString(xsltTransformContextPtr ctxt, const xmlChar *expr) {
    xmlChar *ret = NULL;
    xmlXPathObjectPtr res, tmp;
    xmlXPathParserContextPtr xpathParserCtxt;

    if (ctxt->xpathCtxt == NULL) {
	xmlXPathInit();
	ctxt->xpathCtxt = xmlXPathNewContext(ctxt->doc);
	if (ctxt->xpathCtxt == NULL)
	    return(NULL);
	XSLT_REGISTER_VARIABLE_LOOKUP(ctxt);
    }
    xpathParserCtxt =
	xmlXPathNewParserContext(expr, ctxt->xpathCtxt);
    if (xpathParserCtxt == NULL)
	return(NULL);
    ctxt->xpathCtxt->node = ctxt->node;
    xmlXPathEvalExpr(xpathParserCtxt);
    xmlXPathStringFunction(xpathParserCtxt, 1);
    res = valuePop(xpathParserCtxt);
    do {
        tmp = valuePop(xpathParserCtxt);
	if (tmp != NULL) {
	    xmlXPathFreeObject(tmp);
	}
    } while (tmp != NULL);
    if (res != NULL) {
	if (res->type == XPATH_STRING) {
            ret = res->stringval;
	    res->stringval = NULL;
	} else {
	    xsltGenericError(xsltGenericErrorContext,
		 "xpath : string() function didn't returned a String\n");
	}
	xmlXPathFreeObject(res);
    }
    xmlXPathFreeParserContext(xpathParserCtxt);
#ifdef DEBUG_TEMPLATES
    xsltGenericDebug(xsltGenericDebugContext,
	 "xsltEvalXPathString: %s returns %s\n", expr, ret);
#endif
    return(ret);
}

/**
 * xsltAttrTemplateValueProcess:
 * @ctxt:  the XSLT transformation context
 * @str:  the attribute template node value
 *
 * Process the given node and return the new string value.
 *
 * Returns the computed string value or NULL, must be deallocated by the
 *    caller.
 */
xmlChar *
xsltAttrTemplateValueProcess(xsltTransformContextPtr ctxt, const xmlChar *str) {
    xmlChar *ret = NULL, *ret2;
    const xmlChar *cur;
    xmlChar *expr, *val;

    if (str == NULL) return(NULL);
    cur = str;
    while (*cur != 0) {
	if (*cur == '{') {
	    ret2 = xmlStrncat(ret, str, cur - str);
	    if (ret != NULL)
		xmlFree(ret);
	    ret = ret2;
	    str = cur;
	    cur++;
	    while ((*cur != 0) && (*cur != '}')) cur++;
	    if (*cur == 0) {
		ret2 = xmlStrncat(ret, str, cur - str);
		xmlFree(ret);
		return(ret2);
	    }
	    str++;
	    expr = xmlStrndup(str, cur - str);
	    if (expr == NULL)
		return(ret);
	    else {
                val = xsltEvalXPathString(ctxt, expr);
		xmlFree(expr);
		if (val != NULL) {
		    ret2 = xmlStrcat(ret, val);
		    if (ret != NULL)
			xmlFree(ret);
		    xmlFree(val);
		    ret = ret2;
		}
	    }
	    cur++;
	    str = cur;
	} else
	    cur++;
    }
    if (cur != str) {
	ret2 = xmlStrncat(ret, str, cur - str);
	if (ret != NULL)
	    xmlFree(ret);
	ret = ret2;
    }

    return(ret);
}

/**
 * xsltAttrTemplateProcess:
 * @ctxt:  the XSLT transformation context
 * @target:  the result node
 * @cur:  the attribute template node
 *
 * Process the given attribute and return the new processed copy.
 *
 * Returns the attribute replacement.
 */
xmlAttrPtr
xsltAttrTemplateProcess(xsltTransformContextPtr ctxt, xmlNodePtr target,
	                xmlAttrPtr cur) {
    xmlAttrPtr ret;
    if ((ctxt == NULL) || (cur == NULL))
	return(NULL);
    
    if (cur->type != XML_ATTRIBUTE_NODE)
	return(NULL);

    if ((cur->ns != NULL) &&
	(xmlStrEqual(cur->ns->href, XSLT_NAMESPACE))) {
	/* TODO: check for replacement namespaces */
	return(NULL);
    }

    ret = xmlNewDocProp(ctxt->output, cur->name, NULL);
    if (ret == NULL) return(NULL);
    ret->parent = target;
    
    if ((cur->ns != NULL) && (target != NULL)) {
	if ((target != NULL) && (target->ns != NULL) &&
	    (xmlStrEqual(target->ns->href, cur->ns->href))) {
	    ret->ns = target->ns;
	} else {
	    xmlNsPtr ns;

	    ns = xmlSearchNsByHref(ctxt->output, target, cur->ns->href);
	    if (ns != NULL) {
		ret->ns = ns;
	    } else {
		ns = xmlNewNs(target, cur->ns->href, cur->ns->prefix);
		ret->ns = ns;
	    }
	}
    } else
        ret->ns = NULL;

    if (cur->children != NULL) {
	xmlChar *in = xmlNodeListGetString(ctxt->doc, cur->children, 1);
	xmlChar *out;

	if (in != NULL) {
            out = xsltAttrTemplateValueProcess(ctxt, in);
	    ret->children = xmlNewDocText(ctxt->output, out);
	    xmlFree(out);
	    xmlFree(in);
	} else
	    ret->children = NULL;
       
    } else 
	ret->children = NULL;
    return(ret);
}


/**
 * xsltAttrListTemplateProcess:
 * @ctxt:  the XSLT transformation context
 * @target:  the element where the attributes will be grafted
 * @cur:  the first attribute
 *
 * Do a copy of an attribute list with attribute template processing
 *
 * Returns: a new xmlAttrPtr, or NULL in case of error.
 */
xmlAttrPtr
xsltAttrListTemplateProcess(xsltTransformContextPtr ctxt, 
	                    xmlNodePtr target, xmlAttrPtr cur) {
    xmlAttrPtr ret = NULL;
    xmlAttrPtr p = NULL,q;

    while (cur != NULL) {
        q = xsltAttrTemplateProcess(ctxt, target, cur);
	if (p == NULL) {
	    ret = p = q;
	} else {
	    p->next = q;
	    q->prev = p;
	    p = q;
	}
	cur = cur->next;
    }
    return(ret);
}


/**
 * xsltTemplateProcess:
 * @ctxt:  the XSLT transformation context
 * @node:  the attribute template node
 *
 * Process the given node and return the new string value.
 *
 * Returns the computed tree replacement
 */
xmlNodePtr *
xsltTemplateProcess(xsltTransformContextPtr ctxt, xmlNodePtr node) {
    if (node == NULL)
	return(NULL);
    
    return(0);
}


