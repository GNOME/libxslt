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

#ifdef WITH_XSLT_DEBUG
#define WITH_XSLT_DEBUG_TEMPLATES
#endif

/************************************************************************
 *									*
 *			Module interfaces				*
 *									*
 ************************************************************************/
 
/**
 * xsltEvalXPathPredicate:
 * @ctxt:  the XSLT transformation context
 * @comp:  the XPath compiled expression
 *
 * Process the expression using XPath and evaluate the result as
 * an XPath predicate
 *
 * Returns 1 is the predicate was true, 0 otherwise
 */
int
xsltEvalXPathPredicate(xsltTransformContextPtr ctxt,
	               xmlXPathCompExprPtr comp) {
    int ret, position;
    xmlXPathObjectPtr res;

    position = ctxt->xpathCtxt->proximityPosition;
    ctxt->xpathCtxt->node = ctxt->node;
    /* TODO: do we need to propagate the namespaces here ? */
    ctxt->xpathCtxt->namespaces = NULL;
    ctxt->xpathCtxt->nsNr = 0;
    res = xmlXPathCompiledEval(comp, ctxt->xpathCtxt);
    ctxt->xpathCtxt->proximityPosition = position;
    if (res != NULL) {
	ret = xmlXPathEvalPredicate(ctxt->xpathCtxt, res);
	xmlXPathFreeObject(res);
#ifdef WITH_XSLT_DEBUG_TEMPLATES
	xsltGenericDebug(xsltGenericDebugContext,
	     "xsltEvalXPathPredicate: returns %d\n", ret);
#endif
    } else {
#ifdef WITH_XSLT_DEBUG_TEMPLATES
	xsltGenericDebug(xsltGenericDebugContext,
	     "xsltEvalXPathPredicate: failed\n");
#endif
	ret = 0;
    }
    return(ret);
}

/**
 * xsltEvalXPathString:
 * @ctxt:  the XSLT transformation context
 * @comp:  the compiled XPath expression
 *
 * Process the expression using XPath and get a string
 *
 * Returns the computed string value or NULL, must be deallocated by the
 *    caller.
 */
xmlChar *
xsltEvalXPathString(xsltTransformContextPtr ctxt, xmlXPathCompExprPtr comp) {
    xmlChar *ret = NULL;
    xmlXPathObjectPtr res;

    ctxt->xpathCtxt->node = ctxt->node;
    /* TODO: do we need to propagate the namespaces here ? */
    ctxt->xpathCtxt->namespaces = NULL;
    ctxt->xpathCtxt->nsNr = 0;
    res = xmlXPathCompiledEval(comp, ctxt->xpathCtxt);
    if (res != NULL) {
	if (res->type != XPATH_STRING)
	    res = xmlXPathConvertString(res);
	if (res->type == XPATH_STRING) {
            ret = res->stringval;
	    res->stringval = NULL;
	} else {
	    xsltGenericError(xsltGenericErrorContext,
		 "xpath : string() function didn't returned a String\n");
	}
	xmlXPathFreeObject(res);
    }
#ifdef WITH_XSLT_DEBUG_TEMPLATES
    xsltGenericDebug(xsltGenericDebugContext,
	 "xsltEvalXPathString: returns %s\n", ret);
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
		xmlXPathCompExprPtr comp;
		/*
		 * TODO: keep precompiled form around
		 */
		comp = xmlXPathCompile(expr);
                val = xsltEvalXPathString(ctxt, comp);
		xmlXPathFreeCompExpr(comp);
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
 * @ns:  the attribute namespace URI
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
	                  const xmlChar *name, const xmlChar *ns) {
    xmlChar *ret;
    xmlChar *expr;

    if ((ctxt == NULL) || (node == NULL) || (name == NULL))
	return(NULL);

    expr = xmlGetNsProp(node, name, ns);
    if (expr == NULL)
	return(NULL);

    /*
     * TODO: though now {} is detected ahead, it would still be good to
     *       optimize both functions to keep the splitted value if the
     *       attribute content and the XPath precompiled expressions around
     */

    ret = xsltAttrTemplateValueProcess(ctxt, expr);
#ifdef WITH_XSLT_DEBUG_TEMPLATES
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
 * @name:  the attribute Name
 * @name:  the attribute namespace URI
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
			const xmlChar *name, const xmlChar *ns, int *found) {
    const xmlChar *ret;
    xmlChar *expr;

    if ((ctxt == NULL) || (node == NULL) || (name == NULL))
	return(NULL);

    expr = xmlGetNsProp(node, name, ns);
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
xsltTemplateProcess(xsltTransformContextPtr ctxt ATTRIBUTE_UNUSED, xmlNodePtr node) {
    if (node == NULL)
	return(NULL);
    
    return(0);
}


