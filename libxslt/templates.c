/*
 * templates.c: Implementation of the template processing
 *
 * Reference:
 *   http://www.w3.org/TR/1999/REC-xslt-19991116
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#define IN_LIBXSLT
#include "libxslt.h"

#include <string.h>

#include <libxml/xmlmemory.h>
#include <libxml/globals.h>
#include <libxml/xmlerror.h>
#include <libxml/tree.h>
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
 * @nsList:  the namespaces in scope
 * @nsNr:  the number of namespaces in scope
 *
 * Process the expression using XPath and evaluate the result as
 * an XPath predicate
 *
 * Returns 1 is the predicate was true, 0 otherwise
 */
int
xsltEvalXPathPredicate(xsltTransformContextPtr ctxt, xmlXPathCompExprPtr comp,
		       xmlNsPtr *nsList, int nsNr) {
    int ret;
    xmlXPathObjectPtr res;
    int oldNsNr;
    xmlNsPtr *oldNamespaces;
    xmlNodePtr oldInst;
    int oldProximityPosition, oldContextSize;

    oldContextSize = ctxt->xpathCtxt->contextSize;
    oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
    oldNsNr = ctxt->xpathCtxt->nsNr;
    oldNamespaces = ctxt->xpathCtxt->namespaces;
    oldInst = ctxt->inst;

    ctxt->xpathCtxt->node = ctxt->node;
    ctxt->xpathCtxt->namespaces = nsList;
    ctxt->xpathCtxt->nsNr = nsNr;

    res = xmlXPathCompiledEval(comp, ctxt->xpathCtxt);

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
	ctxt->state = XSLT_STATE_STOPPED;
	ret = 0;
    }
    ctxt->xpathCtxt->nsNr = oldNsNr;

    ctxt->xpathCtxt->namespaces = oldNamespaces;
    ctxt->inst = oldInst;
    ctxt->xpathCtxt->contextSize = oldContextSize;
    ctxt->xpathCtxt->proximityPosition = oldProximityPosition;

    return(ret);
}

/**
 * xsltEvalXPathStringNs:
 * @ctxt:  the XSLT transformation context
 * @comp:  the compiled XPath expression
 * @nsNr:  the number of namespaces in the list
 * @nsList:  the list of in-scope namespaces to use
 *
 * Process the expression using XPath, allowing to pass a namespace mapping
 * context and get a string
 *
 * Returns the computed string value or NULL, must be deallocated by the
 *    caller.
 */
xmlChar *
xsltEvalXPathStringNs(xsltTransformContextPtr ctxt, xmlXPathCompExprPtr comp,
	              int nsNr, xmlNsPtr *nsList) {
    xmlChar *ret = NULL;
    xmlXPathObjectPtr res;
    xmlNodePtr oldInst;
    xmlNodePtr oldNode;
    int	oldPos, oldSize;
    int oldNsNr;
    xmlNsPtr *oldNamespaces;

    oldInst = ctxt->inst;
    oldNode = ctxt->node;
    oldPos = ctxt->xpathCtxt->proximityPosition;
    oldSize = ctxt->xpathCtxt->contextSize;
    oldNsNr = ctxt->xpathCtxt->nsNr;
    oldNamespaces = ctxt->xpathCtxt->namespaces;

    ctxt->xpathCtxt->node = ctxt->node;
    /* TODO: do we need to propagate the namespaces here ? */
    ctxt->xpathCtxt->namespaces = nsList;
    ctxt->xpathCtxt->nsNr = nsNr;
    res = xmlXPathCompiledEval(comp, ctxt->xpathCtxt);
    if (res != NULL) {
	if (res->type != XPATH_STRING)
	    res = xmlXPathConvertString(res);
	if (res->type == XPATH_STRING) {
            ret = res->stringval;
	    res->stringval = NULL;
	} else {
	    xsltTransformError(ctxt, NULL, NULL,
		 "xpath : string() function didn't return a String\n");
	}
	xmlXPathFreeObject(res);
    } else {
	ctxt->state = XSLT_STATE_STOPPED;
    }
#ifdef WITH_XSLT_DEBUG_TEMPLATES
    xsltGenericDebug(xsltGenericDebugContext,
	 "xsltEvalXPathString: returns %s\n", ret);
#endif
    ctxt->inst = oldInst;
    ctxt->node = oldNode;
    ctxt->xpathCtxt->contextSize = oldSize;
    ctxt->xpathCtxt->proximityPosition = oldPos;
    ctxt->xpathCtxt->nsNr = oldNsNr;
    ctxt->xpathCtxt->namespaces = oldNamespaces;
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
    return(xsltEvalXPathStringNs(ctxt, comp, 0, NULL));
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
    xmlNodePtr oldInsert, insert = NULL;
    xmlChar *ret;

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

    xsltApplyOneTemplate(ctxt, node, parent->children, NULL, NULL);

    ctxt->insert = oldInsert;

    ret = xmlNodeGetContent(insert);
    if (insert != NULL)
	xmlFreeNode(insert);
    return(ret);
}

/**
 * xsltAttrTemplateValueProcessNode:
 * @ctxt:  the XSLT transformation context
 * @str:  the attribute template node value
 * @node:  the node hosting the attribute
 *
 * Process the given string, allowing to pass a namespace mapping
 * context and return the new string value.
 *
 * Returns the computed string value or NULL, must be deallocated by the
 *    caller.
 */
xmlChar *
xsltAttrTemplateValueProcessNode(xsltTransformContextPtr ctxt,
	  const xmlChar *str, xmlNodePtr node) {
    xmlChar *ret = NULL;
    const xmlChar *cur;
    xmlChar *expr, *val;
    xmlNsPtr *nsList = NULL;
    int nsNr = 0;

    if (str == NULL) return(NULL);
    if (*str == 0)
	return(xmlStrndup((xmlChar *)"", 0));

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
	    ret = xmlStrncat(ret, str, cur - str);
	    str = cur;
	    cur++;
	    while ((*cur != 0) && (*cur != '}')) cur++;
	    if (*cur == 0) {
	        xsltTransformError(ctxt, NULL, NULL,
			"xsltAttrTemplateValueProcessNode: unmatched '{'\n");
		ret = xmlStrncat(ret, str, cur - str);
		return(ret);
	    }
	    str++;
	    expr = xmlStrndup(str, cur - str);
	    if (expr == NULL)
		return(ret);
	    else if (*expr == '{') {
		ret = xmlStrcat(ret, expr);
		xmlFree(expr);
	    } else {
		xmlXPathCompExprPtr comp;
		/*
		 * TODO: keep precompiled form around
		 */
		if ((nsList == NULL) && (node != NULL)) {
		    int i = 0;

		    nsList = xmlGetNsList(node->doc, node);
		    if (nsList != NULL) {
			while (nsList[i] != NULL)
			    i++;
			nsNr = i;
		    }
		}
		comp = xmlXPathCompile(expr);
                val = xsltEvalXPathStringNs(ctxt, comp, nsNr, nsList);
		xmlXPathFreeCompExpr(comp);
		xmlFree(expr);
		if (val != NULL) {
		    ret = xmlStrcat(ret, val);
		    xmlFree(val);
		}
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
	        xsltTransformError(ctxt, NULL, NULL,
		     "xsltAttrTemplateValueProcessNode: unmatched '}'\n");
	    }
	} else
	    cur++;
    }
    if (cur != str) {
	ret = xmlStrncat(ret, str, cur - str);
    }

    if (nsList != NULL)
	xmlFree(nsList);

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
    return(xsltAttrTemplateValueProcessNode(ctxt, str, NULL));
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

    expr = xsltGetNsProp(node, name, ns);
    if (expr == NULL)
	return(NULL);

    /*
     * TODO: though now {} is detected ahead, it would still be good to
     *       optimize both functions to keep the splitted value if the
     *       attribute content and the XPath precompiled expressions around
     */

    ret = xsltAttrTemplateValueProcessNode(ctxt, expr, node);
#ifdef WITH_XSLT_DEBUG_TEMPLATES
    xsltGenericDebug(xsltGenericDebugContext,
	 "xsltEvalAttrValueTemplate: %s returns %s\n", expr, ret);
#endif
    if (expr != NULL)
	xmlFree(expr);
    return(ret);
}

/**
 * xsltEvalStaticAttrValueTemplate:
 * @style:  the XSLT stylesheet
 * @node:  the stylesheet node
 * @name:  the attribute Name
 * @ns:  the attribute namespace URI
 * @found:  indicator whether the attribute is present
 *
 * Check if an attribute value template has a static value, i.e. the
 * attribute value does not contain expressions contained in curly braces ({})
 *
 * Returns the static string value or NULL, must be deallocated by the
 *    caller.
 */
xmlChar *
xsltEvalStaticAttrValueTemplate(xsltStylesheetPtr style, xmlNodePtr node,
			const xmlChar *name, const xmlChar *ns, int *found) {
    const xmlChar *ret;
    xmlChar *expr;

    if ((style == NULL) || (node == NULL) || (name == NULL))
	return(NULL);

    expr = xsltGetNsProp(node, name, ns);
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
    xmlNsPtr ns;
    xmlAttrPtr ret;
    if ((ctxt == NULL) || (cur == NULL))
	return(NULL);
    
    if (cur->type != XML_ATTRIBUTE_NODE)
	return(NULL);

    if ((cur->ns != NULL) &&
	(xmlStrEqual(cur->ns->href, XSLT_NAMESPACE))) {
	if (xmlStrEqual(cur->name, (const xmlChar *)"use-attribute-sets")) {
	    xmlChar *in;

	    if (ctxt->document != NULL)
	        in = xmlNodeListGetString(ctxt->document->doc,
			                  cur->children, 1);
	    else
	        in = xmlNodeListGetString(NULL, cur->children, 1);
	    if (in != NULL) {
		xsltApplyAttributeSet(ctxt, ctxt->node, NULL, in);
		xmlFree(in);
	    }
	}
	return(NULL);
    }
    if (cur->ns != NULL)
	ns = xsltGetNamespace(ctxt, cur->parent, cur->ns, target);
    else
	ns = NULL;

    if (cur->children != NULL) {
	xmlChar *in;
	xmlChar *out;
	
	if (ctxt->document != NULL)
	    in = xmlNodeListGetString(ctxt->document->doc, cur->children, 1);
	else
	    in = xmlNodeListGetString(NULL, cur->children, 1);

	/* TODO: optimize if no template value was detected */
	if (in != NULL) {
            out = xsltAttrTemplateValueProcessNode(ctxt, in, cur->parent);
	    ret = xmlSetNsProp(target, ns, cur->name, out);
	    if (out != NULL)
		xmlFree(out);
	    xmlFree(in);
	} else
	    ret = xmlSetNsProp(target, ns, cur->name, (const xmlChar *)"");
       
    } else 
	ret = xmlSetNsProp(target, ns, cur->name, (const xmlChar *)"");
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
    xmlAttrPtr q;
    xmlNodePtr oldInsert;

    oldInsert = ctxt->insert;
    ctxt->insert = target;
    while (cur != NULL) {
        q = xsltAttrTemplateProcess(ctxt, target, cur);
	if (q != NULL) {
	    q->parent = target;
	    q->doc = ctxt->output;
	    if (ret == NULL) {
		ret = q;
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


