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
#include "functions.h"
#include "templates.h"
#include "transform.h"
#include "namespaces.h"
#include "attributes.h"

#define DEBUG_TEMPLATES

/************************************************************************
 *									*
 *			Module interfaces				*
 *									*
 ************************************************************************/
 
/**
 * xsltEvalXPathPredicate:
 * @ctxt:  the XSLT transformation context
 * @str:  the XPath expression
 *
 * Process the expression using XPath and evaluate the result as
 * an XPath predicate
 *
 * Returns 1 is the predicate was true, 0 otherwise
 */
int
xsltEvalXPathPredicate(xsltTransformContextPtr ctxt, const xmlChar *expr) {
    int ret;
    xmlXPathObjectPtr res, tmp;
    xmlXPathParserContextPtr xpathParserCtxt;

    xpathParserCtxt =
	xmlXPathNewParserContext(expr, ctxt->xpathCtxt);
    if (xpathParserCtxt == NULL)
	return(0);
    ctxt->xpathCtxt->node = ctxt->node;
    xmlXPathEvalExpr(xpathParserCtxt);
    res = valuePop(xpathParserCtxt);
    do {
        tmp = valuePop(xpathParserCtxt);
	if (tmp != NULL) {
	    xmlXPathFreeObject(tmp);
	}
    } while (tmp != NULL);
    if (res != NULL) {
	ret = xmlXPathEvaluatePredicateResult(xpathParserCtxt, res);
	xmlXPathFreeObject(res);
#ifdef DEBUG_TEMPLATES
	xsltGenericDebug(xsltGenericDebugContext,
	     "xsltEvalXPathPredicate: %s returns %d\n", expr, ret);
#endif
    } else {
#ifdef DEBUG_TEMPLATES
	xsltGenericDebug(xsltGenericDebugContext,
	     "xsltEvalXPathPredicate: %s failed\n", expr);
#endif
	ret = 0;
    }
    xmlXPathFreeParserContext(xpathParserCtxt);
    return(ret);
}

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
 * xsltEvalTemplateString:
 * @ctxt:  the XSLT transformation context
 * @node:  the stylesheet node
 * @parent:  the content parent
 *
 * Evaluate a template string value, i.e. the parent list is interpreter
 * as template content and the resulting tree string value is returned
 * This is needed for example by xsl:comment and xsl:processing-instruction
 *
 * Returns the computed string value or NULL, must be deallocated by the
 *    caller.
 */
xmlChar *
xsltEvalTemplateString(xsltTransformContextPtr ctxt, xmlNodePtr node,
	               xmlNodePtr parent) {
    xmlChar *ret;
    xmlNodePtr oldInsert, insert = NULL;

    if ((ctxt == NULL) || (node == NULL) || (parent == NULL))
	return(NULL);

    if (parent->children == NULL)
	return(NULL);

    insert = xmlNewDocNode(ctxt->output, NULL,
	                   (const xmlChar *)"fake", NULL);
    if (insert == NULL)
	return(NULL);
    oldInsert = ctxt->insert;
    ctxt->insert = insert;

    xsltApplyOneTemplate(ctxt, node, parent->children, 0);

    ctxt->insert = oldInsert;

    ret = xmlNodeGetContent(insert);
    if (insert != NULL)
	xmlFreeNode(insert);
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
    xmlChar *ret = NULL;
    const xmlChar *cur;
    xmlChar *expr, *val;

    if (str == NULL) return(NULL);
    cur = str;
    while (*cur != 0) {
	if (*cur == '{') {
	    ret = xmlStrncat(ret, str, cur - str);
	    str = cur;
	    cur++;
	    while ((*cur != 0) && (*cur != '}')) cur++;
	    if (*cur == 0) {
		ret = xmlStrncat(ret, str, cur - str);
		return(ret);
	    }
	    str++;
	    expr = xmlStrndup(str, cur - str);
	    if (expr == NULL)
		return(ret);
	    else {
                val = xsltEvalXPathString(ctxt, expr);
		xmlFree(expr);
		if (val != NULL) {
		    ret = xmlStrcat(ret, val);
		    xmlFree(val);
		}
	    }
	    cur++;
	    str = cur;
	} else
	    cur++;
    }
    if (cur != str) {
	ret = xmlStrncat(ret, str, cur - str);
    }

    return(ret);
}

/**
 * xsltEvalAttrValueTemplate:
 * @ctxt:  the XSLT transformation context
 * @node:  the stylesheet node
 * @name:  the attribute QName
 *
 * Evaluate a attribute value template, i.e. the attribute value can
 * contain expressions contained in curly braces ({}) and those are
 * substituted by they computed value.
 *
 * Returns the computed string value or NULL, must be deallocated by the
 *    caller.
 */
xmlChar *
xsltEvalAttrValueTemplate(xsltTransformContextPtr ctxt, xmlNodePtr node,
	                  const xmlChar *name) {
    xmlChar *ret;
    xmlChar *expr;

    if ((ctxt == NULL) || (node == NULL) || (name == NULL))
	return(NULL);

    expr = xmlGetNsProp(node, name, XSLT_NAMESPACE);
    if (expr == NULL)
	return(NULL);

    /*
     * TODO: accelerator if there is no AttrValueTemplate in the stylesheet
     *       return expr directly
     */

    ret = xsltAttrTemplateValueProcess(ctxt, expr);
#ifdef DEBUG_TEMPLATES
    xsltGenericDebug(xsltGenericDebugContext,
	 "xsltEvalXPathString: %s returns %s\n", expr, ret);
#endif
    if (expr != NULL)
	xmlFree(expr);
    return(ret);
}

/**
 * xsltEvalStaticAttrValueTemplate:
 * @ctxt:  the XSLT transformation context
 * @node:  the stylesheet node
 * @name:  the attribute QName
 * @found:  indicator whether the attribute is present
 *
 * Check if an attribute value template has a static value, i.e. the
 * attribute value does not contain expressions contained in curly braces ({})
 *
 * Returns the static string value or NULL, must be deallocated by the
 *    caller.
 */
xmlChar *
xsltEvalStaticAttrValueTemplate(xsltTransformContextPtr ctxt, xmlNodePtr node,
	                        const xmlChar *name, int *found) {
    const xmlChar *ret;
    xmlChar *expr;

    if ((ctxt == NULL) || (node == NULL) || (name == NULL))
	return(NULL);

    expr = xmlGetNsProp(node, name, XSLT_NAMESPACE);
    if (expr == NULL) {
	*found = 0;
	return(NULL);
    }
    *found = 1;

    ret = xmlStrchr(expr, '{');
    if (ret != NULL) {
	xmlFree(expr);
	return(NULL);
    }
    return(expr);
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
	if (xmlStrEqual(cur->name, (const xmlChar *)"use-attribute-sets")) {
	    xmlChar *in;

	    in = xmlNodeListGetString(ctxt->document->doc, cur->children, 1);
	    if (in != NULL) {
		xsltApplyAttributeSet(ctxt, ctxt->node, NULL, in);
		xmlFree(in);
	    }
	}
	return(NULL);
    }
    ret = xmlNewDocProp(ctxt->output, cur->name, NULL);
    if (ret == NULL) return(NULL);
    ret->parent = target;
    
    if (cur->ns != NULL)
	ret->ns = xsltGetNamespace(ctxt, cur->parent, cur->ns, target);
    else
	ret->ns = NULL;

    if (cur->children != NULL) {
	xmlChar *in = xmlNodeListGetString(ctxt->document->doc,
		                           cur->children, 1);
	xmlChar *out;

	/* TODO: optimize if no template value was detected */
	if (in != NULL) {
	    xmlNodePtr child;

            out = xsltAttrTemplateValueProcess(ctxt, in);
	    child = xmlNewDocText(ctxt->output, out);
	    xmlAddChild((xmlNodePtr) ret, child);
	    if (out != NULL)
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
    xmlNodePtr oldInsert;

    oldInsert = ctxt->insert;
    ctxt->insert = target;
    while (cur != NULL) {
        q = xsltAttrTemplateProcess(ctxt, target, cur);
	if (q != NULL) {
	    q->parent = target;
	    q->doc = ctxt->output;
	    if (p == NULL) {
		ret = p = q;
	    } else {
		p->next = q;
		q->prev = p;
		p = q;
	    }
	}
	cur = cur->next;
    }
    ctxt->insert = oldInsert;
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


