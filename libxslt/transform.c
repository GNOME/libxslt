/*
 * transform.c: Implemetation of the XSL Transformation 1.0 engine
 *            transform part, i.e. applying a Stylesheet to a document
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
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/valid.h>
#include <libxml/hash.h>
#include <libxml/encoding.h>
#include <libxml/xmlerror.h>
#include <libxml/xpath.h>
#include <libxml/parserInternals.h>
#include <libxml/xpathInternals.h>
#include <libxml/HTMLtree.h>
#include "xslt.h"
#include "xsltInternals.h"
#include "xsltutils.h"
#include "pattern.h"
#include "transform.h"
#include "variables.h"
#include "numbersInternals.h"
#include "namespaces.h"
#include "attributes.h"
#include "templates.h"
#include "imports.h"
#include "keys.h"
#include "documents.h"

#define DEBUG_PROCESS

int xsltMaxDepth = 250;

/*
 * Useful macros
 */

#ifndef FALSE
# define FALSE (0 == 1)
# define TRUE (!FALSE)
#endif

#define IS_BLANK_NODE(n)						\
    (((n)->type == XML_TEXT_NODE) && (xsltIsBlank((n)->content)))

/*
 * Generic function for accessing stacks in the transform Context
 */

#define PUSH_AND_POP(scope, type, name)					\
scope int name##Push(xsltTransformContextPtr ctxt, type value) {	\
    if (ctxt->name##Nr >= ctxt->name##Max) {				\
	ctxt->name##Max *= 2;						\
        ctxt->name##Tab = (type *) xmlRealloc(ctxt->name##Tab,		\
	             ctxt->name##Max * sizeof(ctxt->name##Tab[0]));	\
        if (ctxt->name##Tab == NULL) {					\
	    xmlGenericError(xmlGenericErrorContext,			\
		    "realloc failed !\n");				\
	    return(0);							\
	}								\
    }									\
    ctxt->name##Tab[ctxt->name##Nr] = value;				\
    ctxt->name = value;							\
    return(ctxt->name##Nr++);						\
}									\
scope type name##Pop(xsltTransformContextPtr ctxt) {			\
    type ret;								\
    if (ctxt->name##Nr <= 0) return(0);					\
    ctxt->name##Nr--;							\
    if (ctxt->name##Nr > 0)						\
	ctxt->name = ctxt->name##Tab[ctxt->name##Nr - 1];		\
    else								\
        ctxt->name = NULL;						\
    ret = ctxt->name##Tab[ctxt->name##Nr];				\
    ctxt->name##Tab[ctxt->name##Nr] = 0;				\
    return(ret);							\
}									\

/*
 * Those macros actually generate the functions
 */
PUSH_AND_POP(extern, xsltTemplatePtr, templ)
PUSH_AND_POP(extern, xsltStackElemPtr, vars)

/************************************************************************
 *									*
 *			handling of transformation contexts		*
 *									*
 ************************************************************************/

/**
 * xsltNewTransformContext:
 * @style:  a parsed XSLT stylesheet
 * @doc:  the input document
 *
 * Create a new XSLT TransformContext
 *
 * Returns the newly allocated xsltTransformContextPtr or NULL in case of error
 */
xsltTransformContextPtr
xsltNewTransformContext(xsltStylesheetPtr style, xmlDocPtr doc) {
    xsltTransformContextPtr cur;
    xsltDocumentPtr docu;

    cur = (xsltTransformContextPtr) xmlMalloc(sizeof(xsltTransformContext));
    if (cur == NULL) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltNewTransformContext : malloc failed\n");
	return(NULL);
    }
    memset(cur, 0, sizeof(xsltTransformContext));

    /*
     * initialize the template stack
     */
    cur->templTab = (xsltTemplatePtr *)
	        xmlMalloc(10 * sizeof(xsltTemplatePtr));
    if (cur->templTab == NULL) {
        xmlGenericError(xmlGenericErrorContext,
		"xsltNewTransformContext: out of memory\n");
	xmlFree(cur);
	return(NULL);
    }
    cur->templNr = 0;
    cur->templMax = 5;
    cur->templ = NULL;

    /*
     * initialize the variables stack
     */
    cur->varsTab = (xsltStackElemPtr *)
	        xmlMalloc(10 * sizeof(xsltStackElemPtr));
    if (cur->varsTab == NULL) {
        xmlGenericError(xmlGenericErrorContext,
		"xsltNewTransformContext: out of memory\n");
	xmlFree(cur->templTab);
	xmlFree(cur);
	return(NULL);
    }
    cur->varsNr = 0;
    cur->varsMax = 5;
    cur->vars = NULL;

    cur->style = style;
    xmlXPathInit();
    cur->xpathCtxt = xmlXPathNewContext(doc);
    if (cur->xpathCtxt == NULL) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltNewTransformContext : xmlXPathNewContext failed\n");
	xmlFree(cur->templTab);
	xmlFree(cur->varsTab);
	xmlFree(cur);
	return(NULL);
    }
    XSLT_REGISTER_VARIABLE_LOOKUP(cur);
    docu = xsltNewDocument(cur, doc);
    if (docu == NULL) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltNewTransformContext : xsltNewDocument failed\n");
	xmlFree(cur->templTab);
	xmlFree(cur->varsTab);
	xmlFree(cur);
	return(NULL);
    }
    docu->main = 1;
    cur->document = docu;
    return(cur);
}

/**
 * xsltFreeTransformContext:
 * @ctxt:  an XSLT parser context
 *
 * Free up the memory allocated by @ctxt
 */
void
xsltFreeTransformContext(xsltTransformContextPtr ctxt) {
    if (ctxt == NULL)
	return;
    if (ctxt->xpathCtxt != NULL)
	xmlXPathFreeContext(ctxt->xpathCtxt);
    if (ctxt->templTab != NULL)
	xmlFree(ctxt->templTab);
    if (ctxt->varsTab != NULL)
	xmlFree(ctxt->varsTab);
    xsltFreeDocuments(ctxt);
    memset(ctxt, -1, sizeof(xsltTransformContext));
    xmlFree(ctxt);
}

/************************************************************************
 *									*
 *			Copy of Nodes in an XSLT fashion		*
 *									*
 ************************************************************************/

xmlNodePtr xsltCopyTree(xsltTransformContextPtr ctxt, xmlNodePtr node,
			xmlNodePtr insert);

/**
 * xsltCopyNode:
 * @ctxt:  a XSLT process context
 * @node:  the element node in the source tree.
 * @insert:  the parent in the result tree.
 *
 * Make a copy of the element node @node
 * and insert it as last child of @insert
 *
 * Returns a pointer to the new node, or NULL in case of error
 */
xmlNodePtr
xsltCopyNode(xsltTransformContextPtr ctxt, xmlNodePtr node,
	     xmlNodePtr insert) {
    xmlNodePtr copy;

    copy = xmlCopyNode(node, 0);
    copy->doc = ctxt->output;
    if (copy != NULL) {
	xmlAddChild(insert, copy);
	if (node->type == XML_ELEMENT_NODE) {
	    /*
	     * Add namespaces as they are needed
	     */
	    if (node->nsDef != NULL)
		xsltCopyNamespaceList(ctxt, copy, node->nsDef);
	}
	if (node->ns != NULL) {
	    copy->ns = xsltGetNamespace(ctxt, node, node->ns, copy);
	}
    } else {
	xsltGenericError(xsltGenericErrorContext,
		"xsltCopyNode: copy %s failed\n", node->name);
    }
    return(copy);
}

/**
 * xsltCopyTreeList:
 * @ctxt:  a XSLT process context
 * @list:  the list of element node in the source tree.
 * @insert:  the parent in the result tree.
 *
 * Make a copy of the full list of tree @list
 * and insert them as last children of @insert
 *
 * Returns a pointer to the new list, or NULL in case of error
 */
xmlNodePtr
xsltCopyTreeList(xsltTransformContextPtr ctxt, xmlNodePtr list,
	     xmlNodePtr insert) {
    xmlNodePtr copy, ret = NULL;

    while (list != NULL) {
	copy = xsltCopyTree(ctxt, list, insert);
	if (ret != NULL)
	    ret = copy;
	list = list->next;
    }
    return(ret);
}

/**
 * xsltCopyTree:
 * @ctxt:  a XSLT process context
 * @node:  the element node in the source tree.
 * @insert:  the parent in the result tree.
 *
 * Make a copy of the full tree under the element node @node
 * and insert it as last child of @insert
 *
 * Returns a pointer to the new tree, or NULL in case of error
 */
xmlNodePtr
xsltCopyTree(xsltTransformContextPtr ctxt, xmlNodePtr node,
	     xmlNodePtr insert) {
    xmlNodePtr copy;

    copy = xmlCopyNode(node, 0);
    copy->doc = ctxt->output;
    if (copy != NULL) {
	xmlAddChild(insert, copy);
	/*
	 * Add namespaces as they are needed
	 */
	if (node->nsDef != NULL)
	    xsltCopyNamespaceList(ctxt, copy, node->nsDef);
	if (node->ns != NULL) {
	    copy->ns = xsltGetNamespace(ctxt, node, node->ns, insert);
	}
	if (node->children != NULL)
	    copy->children = xsltCopyTreeList(ctxt, node->children, copy);
    } else {
	xsltGenericError(xsltGenericErrorContext,
		"xsltCopyTree: copy %s failed\n", node->name);
    }
    return(copy);
}
/************************************************************************
 *									*
 *			
 *									*
 ************************************************************************/

void xsltProcessOneNode(xsltTransformContextPtr ctxt, xmlNodePtr node);
void xsltForEach(xsltTransformContextPtr ctxt, xmlNodePtr node,
	         xmlNodePtr inst);
void xsltIf(xsltTransformContextPtr ctxt, xmlNodePtr node, xmlNodePtr inst);
void xsltChoose(xsltTransformContextPtr ctxt, xmlNodePtr node, xmlNodePtr inst);

/**
 * xsltSort:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt sort node
 *
 * Process the xslt sort node on the source node
 */
void
xsltSort(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst) {
    xmlXPathObjectPtr *results = NULL;
    xmlNodeSetPtr list = NULL;
    xmlXPathParserContextPtr xpathParserCtxt = NULL;
    xmlChar *prop = NULL;
    xmlXPathObjectPtr res, tmp;
    const xmlChar *start;
    int descending = 0;
    int number = 0;
    int len = 0;
    int i;
    xmlNodePtr oldNode;

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL))
	return;

    list = ctxt->nodeList;
    if ((list == NULL) || (list->nodeNr <= 1))
	goto error; /* nothing to do */

    len = list->nodeNr;

    prop = xsltEvalAttrValueTemplate(ctxt, inst, (const xmlChar *)"data-type");
    if (prop != NULL) {
	if (xmlStrEqual(prop, (const xmlChar *) "text"))
	    number = 0;
	else if (xmlStrEqual(prop, (const xmlChar *) "number"))
	    number = 1;
	else {
	    xsltGenericError(xsltGenericErrorContext,
		 "xsltSort: no support for data-type = %s\n", prop);
	    goto error;
	}
	xmlFree(prop);
    }
    prop = xsltEvalAttrValueTemplate(ctxt, inst, (const xmlChar *)"order");
    if (prop != NULL) {
	if (xmlStrEqual(prop, (const xmlChar *) "ascending"))
	    descending = 0;
	else if (xmlStrEqual(prop, (const xmlChar *) "descending"))
	    descending = 1;
	else {
	    xsltGenericError(xsltGenericErrorContext,
		 "xsltSort: invalid value %s for order\n", prop);
	    goto error;
	}
	xmlFree(prop);
    }
    /* TODO: xsl:sort lang attribute */
    /* TODO: xsl:sort case-order attribute */

    prop = xmlGetNsProp(inst, (const xmlChar *)"select", XSLT_NAMESPACE);
    if (prop == NULL) {
	prop = xmlNodeGetContent(inst);
	if (prop == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		 "xsltSort: select is not defined\n");
	    return;
	}
    }

    xpathParserCtxt = xmlXPathNewParserContext(prop, ctxt->xpathCtxt);
    if (xpathParserCtxt == NULL)
	goto error;
    results = xmlMalloc(len * sizeof(xmlXPathObjectPtr));
    if (results == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsltSort: memory allocation failure\n");
	goto error;
    }

    start = xpathParserCtxt->cur;
    oldNode = ctxt->node;
    for (i = 0;i < len;i++) {
	xpathParserCtxt->cur = start;
	ctxt->xpathCtxt->contextSize = len;
	ctxt->xpathCtxt->proximityPosition = i + 1;
	ctxt->node = list->nodeTab[i];
	ctxt->xpathCtxt->node = ctxt->node;
	xmlXPathEvalExpr(xpathParserCtxt);
	xmlXPathStringFunction(xpathParserCtxt, 1);
	if (number)
	    xmlXPathNumberFunction(xpathParserCtxt, 1);
	res = valuePop(xpathParserCtxt);
	do {
	    tmp = valuePop(xpathParserCtxt);
	    if (tmp != NULL) {
		xmlXPathFreeObject(tmp);
	    }
	} while (tmp != NULL);

	if (res != NULL) {
	    if (number) {
		if (res->type == XPATH_NUMBER) {
		    results[i] = res;
		} else {
#ifdef DEBUG_PROCESS
		    xsltGenericDebug(xsltGenericDebugContext,
			"xsltSort: select didn't evaluate to a number\n");
#endif
		    results[i] = NULL;
		}
	    } else {
		if (res->type == XPATH_STRING) {
		    results[i] = res;
		} else {
#ifdef DEBUG_PROCESS
		    xsltGenericDebug(xsltGenericDebugContext,
			"xsltSort: select didn't evaluate to a string\n");
#endif
		    results[i] = NULL;
		}
	    }
	}
    }
    ctxt->node = oldNode;

    xsltSortFunction(list, &results[0], descending, number);

error:
    if (xpathParserCtxt != NULL)
	xmlXPathFreeParserContext(xpathParserCtxt);
    if (prop != NULL)
	xmlFree(prop);
    if (results != NULL) {
	for (i = 0;i < len;i++)
	    xmlXPathFreeObject(results[i]);
	xmlFree(results);
    }
}

/**
 * xsltCopy:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt copy node
 *
 * Process the xslt copy node on the source node
 */
void
xsltCopy(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst) {
    xmlChar *prop;
    xmlNodePtr copy, oldInsert;

    oldInsert = ctxt->insert;
    if (ctxt->insert != NULL) {
	switch (node->type) {
	    case XML_DOCUMENT_NODE:
	    case XML_HTML_DOCUMENT_NODE:
		break;
	    case XML_ELEMENT_NODE:
#ifdef DEBUG_PROCESS
		xsltGenericDebug(xsltGenericDebugContext,
				 "xsl:copy: node %s\n", node->name);
#endif
		copy = xsltCopyNode(ctxt, node, ctxt->insert);
		ctxt->insert = copy;
		prop = xmlGetNsProp(inst, (const xmlChar *)"use-attribute-sets",
				    XSLT_NAMESPACE);
		if (prop != NULL) {
		    xsltApplyAttributeSet(ctxt, node, inst, prop);
		    xmlFree(prop);
		}
		break;
	    case XML_ATTRIBUTE_NODE: {
#ifdef DEBUG_PROCESS
		xsltGenericDebug(xsltGenericDebugContext,
				 "xsl:copy: attribute %s\n", node->name);
#endif
		if (ctxt->insert->type == XML_ELEMENT_NODE) {
		    xmlAttrPtr attr = (xmlAttrPtr) node, ret = NULL, cur;
		    if (attr->ns != NULL) {
			if ((!xmlStrEqual(attr->ns->href, XSLT_NAMESPACE)) &&
			    (xmlStrncasecmp(attr->ns->prefix,
					    (xmlChar *)"xml", 3))) {
			    ret = xmlCopyProp(ctxt->insert, attr);
			    ret->ns = xsltGetNamespace(ctxt, node, attr->ns,
						       ctxt->insert);
			} 
		    } else
			ret = xmlCopyProp(ctxt->insert, attr);

		    cur = ctxt->insert->properties;
		    if (cur != NULL) {
			while (cur->next != NULL)
			    cur = cur->next;
			cur->next = ret;
			ret->prev = cur;
		    }else
			ctxt->insert->properties = ret;
		}
		break;
	    }
	    default:
		break;

	}
    }

    switch (node->type) {
	case XML_DOCUMENT_NODE:
	case XML_HTML_DOCUMENT_NODE:
	case XML_ELEMENT_NODE:
	    varsPush(ctxt, NULL);
	    xsltApplyOneTemplate(ctxt, ctxt->node, inst->children);
	    xsltFreeStackElemList(varsPop(ctxt));
	    break;
	default:
	    break;
    }
    ctxt->insert = oldInsert;
}

/**
 * xsltText:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt text node
 *
 * Process the xslt text node on the source node
 */
void
xsltText(xsltTransformContextPtr ctxt, xmlNodePtr node,
	    xmlNodePtr inst) {
    xmlNodePtr copy;

    if (inst->children != NULL) {
	if ((inst->children->type != XML_TEXT_NODE) ||
	    (inst->children->next != NULL)) {
	    xsltGenericError(xsltGenericErrorContext,
		 "xslt:text has content problem !\n");
	} else {
	    xmlChar *prop;
	    xmlNodePtr text = inst->children;
	    
	    copy = xmlNewDocText(ctxt->output, text->content);
	    prop = xmlGetNsProp(inst,
		    (const xmlChar *)"disable-output-escaping",
				XSLT_NAMESPACE);
	    if (prop != NULL) {
#ifdef DEBUG_PARSING
		xsltGenericDebug(xsltGenericDebugContext,
		     "Disable escaping: %s\n", text->content);
#endif
		if (xmlStrEqual(prop, (const xmlChar *)"yes")) {
		    copy->name = xmlStringTextNoenc;
		} else if (!xmlStrEqual(prop,
					(const xmlChar *)"no")){
		    xsltGenericError(xsltGenericErrorContext,
     "xslt:text: disable-output-escaping allow only yes or no\n");

		}
		xmlFree(prop);
	    }
	    xmlAddChild(ctxt->insert, copy);
	}
    }
}

/**
 * xsltElement:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt element node
 *
 * Process the xslt element node on the source node
 */
void
xsltElement(xsltTransformContextPtr ctxt, xmlNodePtr node,
	    xmlNodePtr inst) {
    xmlChar *prop = NULL, *attributes = NULL;
    xmlChar *ncname = NULL;
    xmlChar *prefix = NULL;
    xmlChar *value = NULL;
    xmlNsPtr ns = NULL;
    xmlNodePtr copy;
    xmlNodePtr oldInsert;


    if (ctxt->insert == NULL)
	return;
    /*
     * stack and saves
     */
    oldInsert = ctxt->insert;

    prop = xsltEvalAttrValueTemplate(ctxt, inst, (const xmlChar *)"name");
    if (prop == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:element : name is missing\n");
	goto error;
    }

    ncname = xmlSplitQName2(prop, &prefix);
    if (ncname == NULL) {
	ncname = prop;
	prop = NULL;
	prefix = NULL;
    }
    prop = xsltEvalAttrValueTemplate(ctxt, inst, (const xmlChar *)"namespace");
    if (prop != NULL) {
	ns = xsltGetSpecialNamespace(ctxt, inst, prop, prefix, ctxt->insert);
    } else {
	if (prefix != NULL) {
	    if (!xmlStrncasecmp(prefix, (xmlChar *)"xml", 3)) {
#ifdef DEBUG_PARSING
		xsltGenericDebug(xsltGenericDebugContext,
		     "xslt:element : xml prefix forbidden\n");
#endif
		goto error;
	    }
	    ns = xmlSearchNs(inst->doc, inst, prefix);
	    if (ns == NULL) {
		xsltGenericError(xsltGenericErrorContext,
		    "no namespace bound to prefix %s\n", prefix);
	    } else {
		ns = xsltGetNamespace(ctxt, inst, ns, ctxt->insert);
	    }
	}
    }

    copy = xmlNewDocNode(ctxt->output, ns, ncname, NULL);
    if (copy == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsl:element : creation of %s failed\n", ncname);
	goto error;
    }
    xmlAddChild(ctxt->insert, copy);
    ctxt->insert = copy;
    attributes = xsltEvalAttrValueTemplate(ctxt, inst,
	                               (const xmlChar *)"use-attribute-sets");
    if (attributes != NULL) {
	xsltApplyAttributeSet(ctxt, node, inst, attributes);
        xmlFree(attributes);
    }
    
    varsPush(ctxt, NULL);
    xsltApplyOneTemplate(ctxt, ctxt->node, inst->children);
    xsltFreeStackElemList(varsPop(ctxt));

    ctxt->insert = oldInsert;

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
 * xsltComment:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt comment node
 *
 * Process the xslt comment node on the source node
 */
void
xsltComment(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst) {
    xmlChar *value = NULL;
    xmlNodePtr comment;

    value = xsltEvalTemplateString(ctxt, node, inst);
    /* TODO: check that there is no -- sequence and doesn't end up with - */
#ifdef DEBUG_PROCESS
    if (value == NULL)
	xsltGenericDebug(xsltGenericDebugContext,
	     "xsl:comment: empty\n");
    else
	xsltGenericDebug(xsltGenericDebugContext,
	     "xsl:comment: content %s\n", value);
#endif

    comment = xmlNewComment(value);
    xmlAddChild(ctxt->insert, comment);

    if (value != NULL)
	xmlFree(value);
}

/**
 * xsltProcessingInstruction:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt processing-instruction node
 *
 * Process the xslt processing-instruction node on the source node
 */
void
xsltProcessingInstruction(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst) {
    xmlChar *ncname = NULL;
    xmlChar *value = NULL;
    xmlNodePtr pi;


    if (ctxt->insert == NULL)
	return;
    ncname = xsltEvalAttrValueTemplate(ctxt, inst, (const xmlChar *)"name");
    if (ncname == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:processing-instruction : name is missing\n");
	goto error;
    }
    /* TODO: check that it's both an an NCName and a PITarget. */


    value = xsltEvalTemplateString(ctxt, node, inst);
    /* TODO: check that there is no ?> sequence */
#ifdef DEBUG_PROCESS
    if (value == NULL)
	xsltGenericDebug(xsltGenericDebugContext,
	     "xsl:processing-instruction: %s empty\n", ncname);
    else
	xsltGenericDebug(xsltGenericDebugContext,
	     "xsl:processing-instruction: %s content %s\n", ncname, value);
#endif

    pi = xmlNewPI(ncname, value);
    xmlAddChild(ctxt->insert, pi);

error:
    if (ncname != NULL)
        xmlFree(ncname);
    if (value != NULL)
	xmlFree(value);
}

/**
 * xsltCopyOf:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt copy-of node
 *
 * Process the xslt copy-of node on the source node
 */
void
xsltCopyOf(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst) {
    xmlChar *prop = NULL;
    xmlXPathObjectPtr res = NULL, tmp;
    xmlXPathParserContextPtr xpathParserCtxt = NULL;
    xmlNodePtr copy = NULL;
    xmlNodeSetPtr list = NULL;
    int i;

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL))
	return;

    prop = xmlGetNsProp(inst, (const xmlChar *)"select", XSLT_NAMESPACE);
    if (prop == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:copy-of : select is missing\n");
	goto error;
    }
#ifdef DEBUG_PROCESS
    xsltGenericDebug(xsltGenericDebugContext,
	 "xsltCopyOf: select %s\n", prop);
#endif

    xpathParserCtxt =
	xmlXPathNewParserContext(prop, ctxt->xpathCtxt);
    if (xpathParserCtxt == NULL)
	goto error;
    ctxt->xpathCtxt->node = node;
    xmlXPathEvalExpr(xpathParserCtxt);
    res = valuePop(xpathParserCtxt);
    do {
        tmp = valuePop(xpathParserCtxt);
	if (tmp != NULL) {
	    xmlXPathFreeObject(tmp);
	}
    } while (tmp != NULL);
    if (res != NULL) {
	if ((res->type == XPATH_NODESET) || (res->type == XPATH_XSLT_TREE)) {
	    list = res->nodesetval;
	    if (list != NULL) {
		/* sort the list in document order */
		xsltDocumentSortFunction(list);
		/* append everything in this order under ctxt->insert */
		for (i = 0;i < list->nodeNr;i++) {
		    if (list->nodeTab[i] == NULL)
			continue;
		    if ((list->nodeTab[i]->type == XML_DOCUMENT_NODE) ||
			(list->nodeTab[i]->type == XML_HTML_DOCUMENT_NODE)) {
			xsltCopyTreeList(ctxt, list->nodeTab[i]->children,
				         ctxt->insert);
		    } else {
			xsltCopyTree(ctxt, list->nodeTab[i], ctxt->insert);
		    }
		}
	    }
	} else {
	    /* convert to a string */
	    valuePush(xpathParserCtxt, res);
	    xmlXPathStringFunction(xpathParserCtxt, 1);
	    res = valuePop(xpathParserCtxt);
	    if ((res != NULL) && (res->type == XPATH_STRING)) {
		/* append content as text node */
		copy = xmlNewText(res->stringval);
		if (copy != NULL) {
		    xmlAddChild(ctxt->insert, copy);
		}
	    }
	    do {
		tmp = valuePop(xpathParserCtxt);
		if (tmp != NULL) {
		    xmlXPathFreeObject(tmp);
		}
	    } while (tmp != NULL);
	    if (copy == NULL) {
		xsltGenericError(xsltGenericErrorContext,
		    "xsltDefaultProcessOneNode: text copy failed\n");
	    }
#ifdef DEBUG_PROCESS
	    else
		xsltGenericDebug(xsltGenericDebugContext,
		     "xslcopyOf: result %s\n", res->stringval);
#endif
	}
    }

error:
    if (xpathParserCtxt != NULL) {
	xmlXPathFreeParserContext(xpathParserCtxt);
        xpathParserCtxt = NULL;
    }
    if (prop != NULL)
	xmlFree(prop);
    if (res != NULL)
	xmlXPathFreeObject(res);
}

/**
 * xsltValueOf:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt value-of node
 *
 * Process the xslt value-of node on the source node
 */
void
xsltValueOf(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst) {
    xmlChar *prop;
    int disableEscaping = 0;
    xmlXPathObjectPtr res = NULL, tmp;
    xmlXPathParserContextPtr xpathParserCtxt = NULL;
    xmlNodePtr copy = NULL;

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL))
	return;

    prop = xmlGetNsProp(inst, (const xmlChar *)"disable-output-escaping",
	                XSLT_NAMESPACE);
    if (prop != NULL) {
	if (xmlStrEqual(prop, (const xmlChar *)"yes"))
	    disableEscaping = 1;
	else if (xmlStrEqual(prop, (const xmlChar *)"no"))
	    disableEscaping = 0;
	else 
	    xsltGenericError(xsltGenericErrorContext,
		 "invalud value %s for disable-output-escaping\n", prop);

	xmlFree(prop);
    }
    prop = xmlGetNsProp(inst, (const xmlChar *)"select", XSLT_NAMESPACE);
    if (prop == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsltValueOf: select is not defined\n");
	return;
    }
#ifdef DEBUG_PROCESS
    xsltGenericDebug(xsltGenericDebugContext,
	 "xsltValueOf: select %s\n", prop);
#endif

    xpathParserCtxt =
	xmlXPathNewParserContext(prop, ctxt->xpathCtxt);
    if (xpathParserCtxt == NULL)
	goto error;
    ctxt->xpathCtxt->node = node;
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
            copy = xmlNewText(res->stringval);
	    if (copy != NULL) {
		if (disableEscaping)
		    copy->name = xmlStringTextNoenc;
		xmlAddChild(ctxt->insert, copy);
	    }
	}
    }
    if (copy == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsltDefaultProcessOneNode: text copy failed\n");
    }
#ifdef DEBUG_PROCESS
    else
	xsltGenericDebug(xsltGenericDebugContext,
	     "xsltValueOf: result %s\n", res->stringval);
#endif
error:
    if (xpathParserCtxt != NULL) {
	xmlXPathFreeParserContext(xpathParserCtxt);
        xpathParserCtxt = NULL;
    }
    if (prop != NULL)
	xmlFree(prop);
    if (res != NULL)
	xmlXPathFreeObject(res);
}

/**
 * xsltNumber:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @cur:   the xslt number node
 *
 * Process the xslt number node on the source node
 */
void
xsltNumber(xsltTransformContextPtr ctxt,
	   xmlNodePtr node,
	   xmlNodePtr cur)
{
    xmlChar *prop;
    xsltNumberData numdata;

    if ((ctxt == NULL) || (cur == NULL))
	return;

    memset(&numdata, 0, sizeof(numdata));
    
    numdata.value = xmlGetNsProp(cur, (const xmlChar *)"value", XSLT_NAMESPACE);
    
    prop = xmlGetNsProp(cur, (const xmlChar *)"format", XSLT_NAMESPACE);
    if (prop != NULL) {
	numdata.format = prop;
    } else {
	numdata.format = xmlStrdup(BAD_CAST("1"));
    }
    
    numdata.count = xmlGetNsProp(cur, (const xmlChar *)"count", XSLT_NAMESPACE);
    numdata.from = xmlGetNsProp(cur, (const xmlChar *)"from", XSLT_NAMESPACE);
    
    prop = xmlGetNsProp(cur, (const xmlChar *)"level", XSLT_NAMESPACE);
    if (prop != NULL) {
	if (xmlStrEqual(prop, BAD_CAST("single")) ||
	    xmlStrEqual(prop, BAD_CAST("multiple")) ||
	    xmlStrEqual(prop, BAD_CAST("any"))) {
	    numdata.level = prop;
	} else {
	    xsltGenericError(xsltGenericErrorContext,
			     "invalid value %s for level\n", prop);
	    xmlFree(prop);
	}
    }
    
    prop = xmlGetNsProp(cur, (const xmlChar *)"lang", XSLT_NAMESPACE);
    if (prop != NULL) {
	TODO;
	xmlFree(prop);
    }
    
    prop = xmlGetNsProp(cur, (const xmlChar *)"letter-value", XSLT_NAMESPACE);
    if (prop != NULL) {
	if (xmlStrEqual(prop, BAD_CAST("alphabetic"))) {
	    TODO;
	} else if (xmlStrEqual(prop, BAD_CAST("traditional"))) {
	    TODO;
	} else {
	    xsltGenericError(xsltGenericErrorContext,
			     "invalid value %s for letter-value\n", prop);
	}
	xmlFree(prop);
    }
    
    prop = xmlGetNsProp(cur, (const xmlChar *)"grouping-separator", XSLT_NAMESPACE);
    if (prop != NULL) {
	numdata.groupingCharacter = prop[0];
	xmlFree(prop);
    }
    
    prop = xmlGetNsProp(cur, (const xmlChar *)"grouping-size", XSLT_NAMESPACE);
    if (prop != NULL) {
	sscanf((char *)prop, "%d", &numdata.digitsPerGroup);
	xmlFree(prop);
    } else {
	numdata.groupingCharacter = 0;
    }

    /* Set default values */
    if (numdata.value == NULL) {
	if (numdata.level == NULL) {
	    numdata.level = xmlStrdup(BAD_CAST("single"));
	}
    }
    
    xsltNumberFormat(ctxt, &numdata, node);

    if (numdata.level != NULL)
	xmlFree(numdata.level);
    if (numdata.count != NULL)
	xmlFree(numdata.count);
    if (numdata.from != NULL)
	xmlFree(numdata.from);
    if (numdata.value != NULL)
	xmlFree(numdata.value);
    if (numdata.format != NULL)
	xmlFree(numdata.format);
}

/**
 * xsltDefaultProcessOneNode:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 *
 * Process the source node with the default built-in template rule:
 * <xsl:template match="*|/">
 *   <xsl:apply-templates/>
 * </xsl:template>
 *
 * and
 *
 * <xsl:template match="text()|@*">
 *   <xsl:value-of select="."/>
 * </xsl:template>
 *
 * Note also that namespaces declarations are copied directly:
 *
 * the built-in template rule is the only template rule that is applied
 * for namespace nodes.
 */
void
xsltDefaultProcessOneNode(xsltTransformContextPtr ctxt, xmlNodePtr node) {
    xmlNodePtr copy;
    xmlAttrPtr attrs;
    xmlNodePtr delete = NULL, cur;
    int strip_spaces = -1;
    int nbchild = 0, oldSize;
    int childno = 0, oldPos;
    xsltTemplatePtr template;

    CHECK_STOPPED;
    /*
     * Handling of leaves
     */
    switch (node->type) {
	case XML_DOCUMENT_NODE:
	case XML_HTML_DOCUMENT_NODE:
	case XML_ELEMENT_NODE:
	    break;
	case XML_CDATA_SECTION_NODE:
	    template = xsltGetTemplate(ctxt, node, NULL);
	    if (template) {
		xmlNodePtr oldNode;

#ifdef DEBUG_PROCESS
		xsltGenericDebug(xsltGenericDebugContext,
		 "xsltDefaultProcessOneNode: applying template for CDATA %s\n",
		                 node->content);
#endif
		oldNode = ctxt->node;
		ctxt->node = node;
		templPush(ctxt, template);
		varsPush(ctxt, NULL);
		xsltApplyOneTemplate(ctxt, node, template->content);
		xsltFreeStackElemList(varsPop(ctxt));
		templPop(ctxt);
		ctxt->node = oldNode;
	    } else /* if (ctxt->mode == NULL) */ {
#ifdef DEBUG_PROCESS
		xsltGenericDebug(xsltGenericDebugContext,
		 "xsltDefaultProcessOneNode: copy CDATA %s\n",
		                 node->content);
#endif
		copy = xmlNewDocText(ctxt->output, node->content);
		if (copy != NULL) {
		    xmlAddChild(ctxt->insert, copy);
		} else {
		    xsltGenericError(xsltGenericErrorContext,
			"xsltDefaultProcessOneNode: cdata copy failed\n");
		}
	    }
	    return;
	case XML_TEXT_NODE:
	    template = xsltGetTemplate(ctxt, node, NULL);
	    if (template) {
		xmlNodePtr oldNode;

#ifdef DEBUG_PROCESS
		xsltGenericDebug(xsltGenericDebugContext,
		 "xsltDefaultProcessOneNode: applying template for text %s\n",
		                 node->content);
#endif
		oldNode = ctxt->node;
		ctxt->node = node;
		templPush(ctxt, template);
		varsPush(ctxt, NULL);
		xsltApplyOneTemplate(ctxt, node, template->content);
		xsltFreeStackElemList(varsPop(ctxt));
		templPop(ctxt);
		ctxt->node = oldNode;
	    } else /* if (ctxt->mode == NULL) */ {
#ifdef DEBUG_PROCESS
		if (node->content == NULL)
		    xsltGenericDebug(xsltGenericDebugContext,
		     "xsltDefaultProcessOneNode: copy empty text\n");
		else
		    xsltGenericDebug(xsltGenericDebugContext,
		     "xsltDefaultProcessOneNode: copy text %s\n",
				     node->content);
#endif
		copy = xmlCopyNode(node, 0);
		if (copy != NULL) {
		    xmlAddChild(ctxt->insert, copy);
		} else {
		    xsltGenericError(xsltGenericErrorContext,
			"xsltDefaultProcessOneNode: text copy failed\n");
		}
	    }
	    return;
	case XML_ATTRIBUTE_NODE:
	    if (ctxt->insert->type == XML_ELEMENT_NODE) {
		    xmlAttrPtr attr = (xmlAttrPtr) node, ret = NULL, cur;
		template = xsltGetTemplate(ctxt, node, NULL);
		if (template) {
		    xmlNodePtr oldNode;

		    oldNode = ctxt->node;
		    ctxt->node = node;
		    templPush(ctxt, template);
		    varsPush(ctxt, NULL);
		    xsltApplyOneTemplate(ctxt, node, template->content);
		    xsltFreeStackElemList(varsPop(ctxt));
		    templPop(ctxt);
		    ctxt->node = oldNode;
		} else if (ctxt->mode == NULL) {
		    if (attr->ns != NULL) {
			if ((!xmlStrEqual(attr->ns->href, XSLT_NAMESPACE)) &&
			    (xmlStrncasecmp(attr->ns->prefix,
					    (xmlChar *)"xml", 3))) {
			    ret = xmlCopyProp(ctxt->insert, attr);
			    ret->ns = xsltGetNamespace(ctxt, node, attr->ns,
						       ctxt->insert);
			} 
		    } else
			ret = xmlCopyProp(ctxt->insert, attr);

		    cur = ctxt->insert->properties;
		    if (cur != NULL) {
			while (cur->next != NULL)
			    cur = cur->next;
			cur->next = ret;
			ret->prev = cur;
		    }else
			ctxt->insert->properties = ret;
		}
	    }
	    return;
	default:
	    return;
    }
    /*
     * Handling of Elements: first pass, cleanup and counting
     */
    cur = node->children;
    while (cur != NULL) {
	switch (cur->type) {
	    case XML_TEXT_NODE:
		if ((IS_BLANK_NODE(cur)) &&
		    (cur->parent != NULL) &&
		    (ctxt->style->stripSpaces != NULL)) {
		    if (strip_spaces == -1)
			strip_spaces =
			    xsltFindElemSpaceHandling(ctxt, cur->parent);
		    if (strip_spaces == 1) {
			delete = cur;
			break;
		    }
		}
		/* no break on purpose */
	    case XML_CDATA_SECTION_NODE:
	    case XML_DOCUMENT_NODE:
	    case XML_HTML_DOCUMENT_NODE:
	    case XML_ELEMENT_NODE:
		nbchild++;
		break;
	    case XML_PI_NODE:
	    case XML_COMMENT_NODE:
		nbchild++;
		break;
	    default:
#ifdef DEBUG_PROCESS
		xsltGenericDebug(xsltGenericDebugContext,
		 "xsltDefaultProcessOneNode: skipping node type %d\n",
		                 cur->type);
#endif
		delete = cur;
	}
	cur = cur->next;
	if (delete != NULL) {
#ifdef DEBUG_PROCESS
	    xsltGenericDebug(xsltGenericDebugContext,
		 "xsltDefaultProcessOneNode: removing ignorable blank node\n");
#endif
	    xmlUnlinkNode(delete);
	    xmlFreeNode(delete);
	    delete = NULL;
	}
    }
    /*
     * Handling of Elements: second pass, actual processing
     */
    attrs = node->properties;
    while (attrs != NULL) {
	template = xsltGetTemplate(ctxt, (xmlNodePtr) attrs, NULL);
	if (template) {
	    xmlNodePtr oldNode;

	    oldNode = ctxt->node;
	    ctxt->node = node;
	    templPush(ctxt, template);
	    varsPush(ctxt, NULL);
	    xsltApplyOneTemplate(ctxt, node, template->content);
	    xsltFreeStackElemList(varsPop(ctxt));
	    templPop(ctxt);
	    ctxt->node = oldNode;
	}
	attrs = attrs->next;
    }
    oldSize = ctxt->xpathCtxt->contextSize;
    oldPos = ctxt->xpathCtxt->proximityPosition;
    cur = node->children;
    while (cur != NULL) {
	childno++;
	switch (cur->type) {
	    case XML_DOCUMENT_NODE:
	    case XML_HTML_DOCUMENT_NODE:
	    case XML_ELEMENT_NODE:
		ctxt->xpathCtxt->contextSize = nbchild;
		ctxt->xpathCtxt->proximityPosition = childno;
		xsltProcessOneNode(ctxt, cur);
		break;
	    case XML_CDATA_SECTION_NODE:
		template = xsltGetTemplate(ctxt, node, NULL);
		if (template) {
		    xmlNodePtr oldNode;

#ifdef DEBUG_PROCESS
		    xsltGenericDebug(xsltGenericDebugContext,
		 "xsltDefaultProcessOneNode: applying template for CDATA %s\n",
				     node->content);
#endif
		    oldNode = ctxt->node;
		    ctxt->node = node;
		    templPush(ctxt, template);
		    varsPush(ctxt, NULL);
		    xsltApplyOneTemplate(ctxt, node, template->content);
		    xsltFreeStackElemList(varsPop(ctxt));
		    templPop(ctxt);
		    ctxt->node = oldNode;
		} else /* if (ctxt->mode == NULL) */ {
#ifdef DEBUG_PROCESS
		    xsltGenericDebug(xsltGenericDebugContext,
		     "xsltDefaultProcessOneNode: copy CDATA %s\n",
				     node->content);
#endif
		    copy = xmlNewDocText(ctxt->output, node->content);
		    if (copy != NULL) {
			xmlAddChild(ctxt->insert, copy);
		    } else {
			xsltGenericError(xsltGenericErrorContext,
			    "xsltDefaultProcessOneNode: cdata copy failed\n");
		    }
		}
		break;
	    case XML_TEXT_NODE:
		template = xsltGetTemplate(ctxt, cur, NULL);
		if (template) {
		    xmlNodePtr oldNode;

#ifdef DEBUG_PROCESS
		    xsltGenericDebug(xsltGenericDebugContext,
	     "xsltDefaultProcessOneNode: applying template for text %s\n",
				     node->content);
#endif
		    oldNode = ctxt->node;
		    ctxt->node = cur;
		    ctxt->xpathCtxt->contextSize = nbchild;
		    ctxt->xpathCtxt->proximityPosition = childno;
		    templPush(ctxt, template);
		    varsPush(ctxt, NULL);
		    xsltApplyOneTemplate(ctxt, cur, template->content);
		    xsltFreeStackElemList(varsPop(ctxt));
		    templPop(ctxt);
		    ctxt->node = oldNode;
		} else /* if (ctxt->mode == NULL) */ {
#ifdef DEBUG_PROCESS
		    if (cur->content == NULL)
			xsltGenericDebug(xsltGenericDebugContext,
			 "xsltDefaultProcessOneNode: copy empty text\n");
		    else
			xsltGenericDebug(xsltGenericDebugContext,
		     "xsltDefaultProcessOneNode: copy text %s\n",
					 cur->content);
#endif
		    copy = xmlCopyNode(cur, 0);
		    if (copy != NULL) {
			xmlAddChild(ctxt->insert, copy);
		    } else {
			xsltGenericError(xsltGenericErrorContext,
			    "xsltDefaultProcessOneNode: text copy failed\n");
		    }
		}
		break;
	    case XML_PI_NODE:
	    case XML_COMMENT_NODE:
		template = xsltGetTemplate(ctxt, cur, NULL);
		if (template) {
		    xmlNodePtr oldNode;

		    oldNode = ctxt->node;
		    ctxt->node = cur;
		    ctxt->xpathCtxt->contextSize = nbchild;
		    ctxt->xpathCtxt->proximityPosition = childno;
		    templPush(ctxt, template);
		    varsPush(ctxt, NULL);
		    xsltApplyOneTemplate(ctxt, cur, template->content);
		    xsltFreeStackElemList(varsPop(ctxt));
		    templPop(ctxt);
		    ctxt->node = oldNode;
		}
		break;
	    default:
		break;
	}
	cur = cur->next;
    }
    ctxt->xpathCtxt->contextSize = oldSize;
    ctxt->xpathCtxt->proximityPosition = oldPos;
}

/**
 * xsltApplyImports:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt apply-imports node
 *
 * Process the xslt apply-imports node on the source node
 */
void
xsltApplyImports(xsltTransformContextPtr ctxt, xmlNodePtr node,
	         xmlNodePtr inst) {
    xsltTemplatePtr template;

    if ((ctxt->templ == NULL) || (ctxt->templ->style == NULL)) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:apply-imports : internal error no current template\n");
	return;
    }
    template = xsltGetTemplate(ctxt, node, ctxt->templ->style);
    if (template != NULL) {
	templPush(ctxt, template);
	varsPush(ctxt, NULL);
	xsltApplyOneTemplate(ctxt, node, template->content);
	xsltFreeStackElemList(varsPop(ctxt));
	templPop(ctxt);
    }
}

/**
 * xsltCallTemplate:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt call-template node
 *
 * Process the xslt call-template node on the source node
 */
void
xsltCallTemplate(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst) {
    xmlChar *prop = NULL;
    xmlChar *ncname = NULL;
    xmlChar *prefix = NULL;
    xmlNsPtr ns = NULL;
    xsltTemplatePtr template;
    xmlNodePtr cur = NULL;


    if (ctxt->insert == NULL)
	return;
    prop = xmlGetNsProp(inst, (const xmlChar *)"name", XSLT_NAMESPACE);
    if (prop == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:call-template : name is missing\n");
	goto error;
    }

    ncname = xmlSplitQName2(prop, &prefix);
    if (ncname == NULL) {
	ncname = prop;
	prop = NULL;
	prefix = NULL;
    }
    if ((prefix != NULL) && (ns == NULL)) {
	ns = xmlSearchNs(ctxt->insert->doc, ctxt->insert, prefix);
	if (ns == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		"no namespace bound to prefix %s\n", prefix);
	}
    }
    if (ns != NULL)
	template = xsltFindTemplate(ctxt, ncname, ns->href);
    else
	template = xsltFindTemplate(ctxt, ncname, NULL);
    if (template == NULL) {
	xsltGenericError(xsltGenericDebugContext,
	     "xslt:call-template: template %s not found\n", ncname);
	goto error;
    }
    templPush(ctxt, template);
    varsPush(ctxt, NULL);
    cur = inst->children;
    while (cur != NULL) {
	if (ctxt->state == XSLT_STATE_STOPPED) break;
	if (IS_XSLT_ELEM(cur)) {
	    if (IS_XSLT_NAME(cur, "with-param")) {
		xsltParseStylesheetParam(ctxt, cur);
	    } else {
		xsltGenericError(xsltGenericDebugContext,
		     "xslt:call-template: misplaced xslt:%s\n", cur->name);
	    }
	} else {
	    xsltGenericError(xsltGenericDebugContext,
		 "xslt:call-template: misplaced %s element\n", cur->name);
	}
	cur = cur->next;
    }
    xsltApplyOneTemplate(ctxt, node, template->content);

    xsltFreeStackElemList(varsPop(ctxt));
    templPop(ctxt);

error:
    if (prop != NULL)
        xmlFree(prop);
    if (ncname != NULL)
        xmlFree(ncname);
    if (prefix != NULL)
        xmlFree(prefix);
}

/**
 * xsltApplyTemplates:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the apply-templates node
 *
 * Process the apply-templates node on the source node
 */
void
xsltApplyTemplates(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst) {
    xmlChar *prop = NULL;
    xmlNodePtr cur, delete = NULL;
    xmlXPathObjectPtr res = NULL, tmp;
    xmlNodePtr replacement;
    xmlNodeSetPtr list = NULL, oldlist;
    xmlXPathParserContextPtr xpathParserCtxt = NULL;
    int i, oldProximityPosition, oldContextSize;
    xmlChar *mode, *modeURI;
    const xmlChar *oldmode, *oldmodeURI;

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL))
	return;

#ifdef DEBUG_PROCESS
    xsltGenericDebug(xsltGenericDebugContext,
	 "xsltApplyTemplates: node: %s\n", node->name);
#endif

    /*
     * Get mode if any
     */
    oldmode = ctxt->mode;
    oldmodeURI = ctxt->modeURI;
    prop = xmlGetNsProp(inst, (const xmlChar *)"mode", XSLT_NAMESPACE);
    if (prop != NULL) {
	xmlChar *prefix = NULL;

	mode = xmlSplitQName2(prop, &prefix);
	if (mode != NULL) {
	    if (prefix != NULL) {
		xmlNsPtr ns;

		ns = xmlSearchNs(inst->doc, inst, prefix);
		if (ns == NULL) {
		    xsltGenericError(xsltGenericErrorContext,
			"no namespace bound to prefix %s\n", prefix);
		    xmlFree(prefix);
		    xmlFree(mode);
		    mode = prop;
		    modeURI = NULL;
		} else {
		    modeURI = xmlStrdup(ns->href);
		    xmlFree(prefix);
		    xmlFree(prop);
		}
	    } else {
		xmlFree(prop);
		modeURI = NULL;
	    }
	} else {
	    mode = prop;
	    modeURI = NULL;
	}
#ifdef DEBUG_PROCESS
	xsltGenericDebug(xsltGenericDebugContext,
	     "xsltApplyTemplates: mode %s\n", mode);
#endif
    } else {
	mode = NULL;
	modeURI = NULL;
    }
    ctxt->mode = mode;
    ctxt->modeURI = modeURI;

    prop = xmlGetNsProp(inst, (const xmlChar *)"select", XSLT_NAMESPACE);
    if (prop != NULL) {
#ifdef DEBUG_PROCESS
	xsltGenericDebug(xsltGenericDebugContext,
	     "xsltApplyTemplates: select %s\n", prop);
#endif

	if (ctxt->xpathCtxt == NULL) {
	}
	xpathParserCtxt =
	    xmlXPathNewParserContext(prop, ctxt->xpathCtxt);
	if (xpathParserCtxt == NULL)
	    goto error;
	ctxt->xpathCtxt->node = node;
	xmlXPathEvalExpr(xpathParserCtxt);
	res = valuePop(xpathParserCtxt);
	do {
	    tmp = valuePop(xpathParserCtxt);
	    if (tmp != NULL) {
		xmlXPathFreeObject(tmp);
	    }
	} while (tmp != NULL);
	if (res != NULL) {
	    if (res->type == XPATH_NODESET) {
		list = res->nodesetval;
		res->nodesetval = NULL;
	     } else {
#ifdef DEBUG_PROCESS
		xsltGenericDebug(xsltGenericDebugContext,
		    "xsltApplyTemplates: select didn't evaluate to a node list\n");
#endif
		goto error;
	    }
	}
    } else {
	/*
	 * Build an XPath nodelist with the children
	 */
	list = xmlXPathNodeSetCreate(NULL);
	cur = node->children;
	while (cur != NULL) {
	    switch (cur->type) {
		case XML_TEXT_NODE:
		    if ((IS_BLANK_NODE(cur)) &&
			(cur->parent != NULL) &&
			(ctxt->style->stripSpaces != NULL)) {
			const xmlChar *val;

			val = (const xmlChar *)
			      xmlHashLookup(ctxt->style->stripSpaces,
					    cur->parent->name);
			if ((val != NULL) &&
			    (xmlStrEqual(val, (xmlChar *) "strip"))) {
			    delete = cur;
			    break;
			}
		    }
		    /* no break on purpose */
		case XML_DOCUMENT_NODE:
		case XML_HTML_DOCUMENT_NODE:
		case XML_ELEMENT_NODE:
		case XML_CDATA_SECTION_NODE:
		    xmlXPathNodeSetAdd(list, cur);
		    break;
		default:
#ifdef DEBUG_PROCESS
		    xsltGenericDebug(xsltGenericDebugContext,
		     "xsltApplyTemplates: skipping cur type %d\n",
				     cur->type);
#endif
		    delete = cur;
	    }
	    cur = cur->next;
	    if (delete != NULL) {
#ifdef DEBUG_PROCESS
		xsltGenericDebug(xsltGenericDebugContext,
		     "xsltApplyTemplates: removing ignorable blank cur\n");
#endif
		xmlUnlinkNode(delete);
		xmlFreeNode(delete);
		delete = NULL;
	    }
	}
    }

#ifdef DEBUG_PROCESS
    xsltGenericDebug(xsltGenericDebugContext,
	"xsltApplyTemplates: list of %d nodes\n", list->nodeNr);
#endif

    oldlist = ctxt->nodeList;
    ctxt->nodeList = list;
    oldContextSize = ctxt->xpathCtxt->contextSize;
    oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
    ctxt->xpathCtxt->contextSize = list->nodeNr;

    /* 
     * handle and skip the xsl:sort
     */
    replacement = inst->children;
    if (IS_XSLT_ELEM(replacement) && (IS_XSLT_NAME(replacement, "sort"))) {
	xsltSort(ctxt, node, replacement);
	replacement = replacement->next;
	while (IS_XSLT_ELEM(replacement) &&
	       (IS_XSLT_NAME(replacement, "sort"))) {
	    TODO /* imbricated sorts */
	    replacement = replacement->next;
	}
    }

    for (i = 0;i < list->nodeNr;i++) {
	ctxt->node = list->nodeTab[i];
	ctxt->xpathCtxt->proximityPosition = i + 1;
	xsltProcessOneNode(ctxt, list->nodeTab[i]);
    }
    ctxt->nodeList = oldlist;
    ctxt->xpathCtxt->contextSize = oldContextSize;
    ctxt->xpathCtxt->proximityPosition = oldProximityPosition;

error:
    ctxt->mode = oldmode;
    ctxt->modeURI = oldmodeURI;
    if (xpathParserCtxt != NULL)
	xmlXPathFreeParserContext(xpathParserCtxt);
    if (prop != NULL)
	xmlFree(prop);
    if (res != NULL)
	xmlXPathFreeObject(res);
    if (list != NULL)
	xmlXPathFreeNodeSet(list);
    if (mode != NULL)
	xmlFree(mode);
    if (modeURI != NULL)
	xmlFree(modeURI);
}


/**
 * xsltApplyOneTemplate:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @list:  the template replacement nodelist
 *
 * Process the apply-templates node on the source node
 */
void
xsltApplyOneTemplate(xsltTransformContextPtr ctxt, xmlNodePtr node,
	             xmlNodePtr list) {
    xmlNodePtr cur = NULL, insert, copy = NULL;
    xmlNodePtr oldInsert;
    xmlAttrPtr attrs;

    if (list == NULL)
	return;
    CHECK_STOPPED;

    if (ctxt->templNr >= xsltMaxDepth) {
	xsltGenericError(xsltGenericErrorContext,
		"xsltApplyOneTemplate: loop found ???\n");
	xsltGenericError(xsltGenericErrorContext,
		"try increasing xsltMaxDepth (--maxdepth)\n");
	xsltDebug(ctxt, node);
	return;
    }

    /*
     * stack and saves
     */
    oldInsert = insert = ctxt->insert;

    /*
     * Insert all non-XSLT nodes found in the template
     */
    cur = list;
    while (cur != NULL) {
	/*
	 * test, we must have a valid insertion point
	 */
	if (insert == NULL) {
#ifdef DEBUG_PROCESS
	    xsltGenericDebug(xsltGenericDebugContext,
		 "xsltApplyOneTemplate: insert == NULL !\n");
#endif
	    return;
	}

	if (IS_XSLT_ELEM(cur)) {
	    if (IS_XSLT_NAME(cur, "apply-templates")) {
		ctxt->insert = insert;
		xsltApplyTemplates(ctxt, node, cur);
		ctxt->insert = oldInsert;
	    } else if (IS_XSLT_NAME(cur, "value-of")) {
		ctxt->insert = insert;
		xsltValueOf(ctxt, node, cur);
		ctxt->insert = oldInsert;
	    } else if (IS_XSLT_NAME(cur, "copy")) {
		ctxt->insert = insert;
		xsltCopy(ctxt, node, cur);
		ctxt->insert = oldInsert;
	    } else if (IS_XSLT_NAME(cur, "copy-of")) {
		ctxt->insert = insert;
		xsltCopyOf(ctxt, node, cur);
		ctxt->insert = oldInsert;
	    } else if (IS_XSLT_NAME(cur, "if")) {
		ctxt->insert = insert;
		xsltIf(ctxt, node, cur);
		ctxt->insert = oldInsert;
	    } else if (IS_XSLT_NAME(cur, "choose")) {
		ctxt->insert = insert;
		xsltChoose(ctxt, node, cur);
		ctxt->insert = oldInsert;
	    } else if (IS_XSLT_NAME(cur, "for-each")) {
		ctxt->insert = insert;
		xsltForEach(ctxt, node, cur);
		ctxt->insert = oldInsert;
	    } else if (IS_XSLT_NAME(cur, "apply-imports")) {
		ctxt->insert = insert;
		xsltApplyImports(ctxt, node, cur);
		ctxt->insert = oldInsert;
	    } else if (IS_XSLT_NAME(cur, "attribute")) {
		ctxt->insert = insert;
		xsltAttribute(ctxt, node, cur);
		ctxt->insert = oldInsert;
	    } else if (IS_XSLT_NAME(cur, "element")) {
		ctxt->insert = insert;
		xsltElement(ctxt, node, cur);
		ctxt->insert = oldInsert;
	    } else if (IS_XSLT_NAME(cur, "text")) {
#ifdef DEBUG_PROCESS
		xsltGenericDebug(xsltGenericDebugContext,
		     "xsltApplyOneTemplate: found xslt:text, not expected\n");
#endif
		xsltText(ctxt, node, cur);
	    } else if (IS_XSLT_NAME(cur, "comment")) {
		ctxt->insert = insert;
		xsltComment(ctxt, node, cur);
		ctxt->insert = oldInsert;
	    } else if (IS_XSLT_NAME(cur, "number")) {
		ctxt->insert = insert;
		xsltNumber(ctxt, node, cur);
		ctxt->insert = oldInsert;
	    } else if (IS_XSLT_NAME(cur, "processing-instruction")) {
		ctxt->insert = insert;
		xsltProcessingInstruction(ctxt, node, cur);
		ctxt->insert = oldInsert;
	    } else if (IS_XSLT_NAME(cur, "variable")) {
		xsltParseStylesheetVariable(ctxt, cur);
	    } else if (IS_XSLT_NAME(cur, "param")) {
		xsltParseStylesheetParam(ctxt, cur);
	    } else if (IS_XSLT_NAME(cur, "call-template")) {
		ctxt->insert = insert;
		xsltCallTemplate(ctxt, node, cur);
		ctxt->insert = oldInsert;
	    } else if (IS_XSLT_NAME(cur, "message")) {
		xsltMessage(ctxt, node, cur);
	    } else {
		xsltGenericError(xsltGenericDebugContext,
		     "xsltApplyOneTemplate: found xslt:%s\n", cur->name);
		TODO
	    }
	    CHECK_STOPPED;
	    goto skip_children;
	} else if (cur->type == XML_TEXT_NODE) {
	    /*
	     * This text comes from the stylesheet
	     * For stylesheets, the set of whitespace-preserving
	     * element names consists of just xsl:text.
	     */
#ifdef DEBUG_PROCESS
	    if (cur->name == xmlStringTextNoenc)
		xsltGenericDebug(xsltGenericDebugContext,
		     "xsltApplyOneTemplate: copy unescaped text %s\n",
		                 cur->content);
	    else
		xsltGenericDebug(xsltGenericDebugContext,
		     "xsltApplyOneTemplate: copy text %s\n", cur->content);
#endif
	    copy = xmlCopyNode(cur, 0);
	    if (copy != NULL) {
		xmlAddChild(insert, copy);
	    } else {
		xsltGenericError(xsltGenericErrorContext,
			"xsltApplyOneTemplate: text copy failed\n");
	    }
	} else if ((cur->type == XML_ELEMENT_NODE) &&
		   (xmlStrEqual(cur->name, (const xmlChar *)"xsltdebug"))) {
	    xsltDebug(ctxt, cur);
	} else if (cur->type == XML_ELEMENT_NODE) {
#ifdef DEBUG_PROCESS
	    xsltGenericDebug(xsltGenericDebugContext,
		 "xsltApplyOneTemplate: copy node %s\n", cur->name);
#endif
	    copy = xsltCopyNode(ctxt, cur, insert);
	    /*
	     * all the attributes are directly inherited
	     */
	    if (cur->properties != NULL) {
		attrs = xsltAttrListTemplateProcess(ctxt, copy,
			                            cur->properties);
		if (copy->properties != NULL) {
		    xmlAttrPtr cur = copy->properties;
		    while (cur->next != NULL)
			cur = cur->next;
		    cur->next = attrs;
		} else
		    copy->properties = attrs;
	    }
	}

	/*
	 * Skip to next node, in document order.
	 */
	if (cur->children != NULL) {
	    if (cur->children->type != XML_ENTITY_DECL) {
		cur = cur->children;
		if (copy != NULL)
		    insert = copy;
		continue;
	    }
	}
skip_children:
	if (cur->next != NULL) {
	    cur = cur->next;
	    continue;
	}
	
	do {
	    cur = cur->parent;
	    insert = insert->parent;
	    if (cur == NULL)
		break;
	    if (cur == list->parent) {
		cur = NULL;
		break;
	    }
	    if (cur->next != NULL) {
		cur = cur->next;
		break;
	    }
	} while (cur != NULL);
    }
}

/**
 * xsltChoose:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt choose node
 *
 * Process the xslt choose node on the source node
 */
void
xsltChoose(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst) {
    xmlChar *prop = NULL;
    xmlXPathObjectPtr res = NULL, tmp;
    xmlXPathParserContextPtr xpathParserCtxt = NULL;
    xmlNodePtr replacement, when;
    int doit = 1;

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL))
	return;

    /* 
     * Check the when's
     */
    replacement = inst->children;
    if (replacement == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:choose: empty content not allowed\n");
	goto error;
    }
    if ((!IS_XSLT_ELEM(replacement)) ||
	(!IS_XSLT_NAME(replacement, "when"))) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:choose: xsl:when expected first\n");
	goto error;
    }
    while (IS_XSLT_ELEM(replacement) && (IS_XSLT_NAME(replacement, "when"))) {

	when = replacement;
	prop = xmlGetNsProp(when, (const xmlChar *)"test", XSLT_NAMESPACE);
	if (prop == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		 "xsl:when: test is not defined\n");
	    return;
	}
#ifdef DEBUG_PROCESS
	xsltGenericDebug(xsltGenericDebugContext,
	     "xsl:when: test %s\n", prop);
#endif

	xpathParserCtxt = xmlXPathNewParserContext(prop, ctxt->xpathCtxt);
	if (xpathParserCtxt == NULL)
	    goto error;
	ctxt->xpathCtxt->node = node;
	xmlXPathEvalExpr(xpathParserCtxt);
	xmlXPathBooleanFunction(xpathParserCtxt, 1);
	res = valuePop(xpathParserCtxt);
	do {
	    tmp = valuePop(xpathParserCtxt);
	    if (tmp != NULL) {
		xmlXPathFreeObject(tmp);
	    }
	} while (tmp != NULL);

	if (res != NULL) {
	    if (res->type == XPATH_BOOLEAN)
		doit = res->boolval;
	    else {
#ifdef DEBUG_PROCESS
		xsltGenericDebug(xsltGenericDebugContext,
		    "xsl:when: test didn't evaluate to a boolean\n");
#endif
		goto error;
	    }
	}

#ifdef DEBUG_PROCESS
	xsltGenericDebug(xsltGenericDebugContext,
	    "xsl:when: test evaluate to %d\n", doit);
#endif
	if (doit) {
	    varsPush(ctxt, NULL);
	    xsltApplyOneTemplate(ctxt, ctxt->node, when->children);
	    xsltFreeStackElemList(varsPop(ctxt));
	    goto done;
	}
	if (xpathParserCtxt != NULL)
	    xmlXPathFreeParserContext(xpathParserCtxt);
	xpathParserCtxt = NULL;
	if (prop != NULL)
	    xmlFree(prop);
	prop = NULL;
	if (res != NULL)
	    xmlXPathFreeObject(res);
	res = NULL;
	replacement = replacement->next;
    }
    if (IS_XSLT_ELEM(replacement) && (IS_XSLT_NAME(replacement, "otherwise"))) {
	varsPush(ctxt, NULL);
	xsltApplyOneTemplate(ctxt, ctxt->node, replacement->children);
	xsltFreeStackElemList(varsPop(ctxt));
	replacement = replacement->next;
    }
    if (replacement != NULL) {
#ifdef DEBUG_PROCESS
	xsltGenericDebug(xsltGenericDebugContext,
	    "xsl:otherwise: applying default fallback\n");
#endif
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:choose: unexpected content %s\n", replacement->name);
	goto error;
    }

done:
error:
    if (xpathParserCtxt != NULL)
	xmlXPathFreeParserContext(xpathParserCtxt);
    if (prop != NULL)
	xmlFree(prop);
    if (res != NULL)
	xmlXPathFreeObject(res);
}

/**
 * xsltIf:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt if node
 *
 * Process the xslt if node on the source node
 */
void
xsltIf(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst) {
    xmlChar *prop;
    xmlXPathObjectPtr res = NULL, tmp;
    xmlXPathParserContextPtr xpathParserCtxt = NULL;
    int doit = 1;

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL))
	return;

    prop = xmlGetNsProp(inst, (const xmlChar *)"test", XSLT_NAMESPACE);
    if (prop == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsltIf: test is not defined\n");
	return;
    }
#ifdef DEBUG_PROCESS
    xsltGenericDebug(xsltGenericDebugContext,
	 "xsltIf: test %s\n", prop);
#endif

    xpathParserCtxt = xmlXPathNewParserContext(prop, ctxt->xpathCtxt);
    if (xpathParserCtxt == NULL)
	goto error;
    ctxt->xpathCtxt->node = node;
    xmlXPathEvalExpr(xpathParserCtxt);
    xmlXPathBooleanFunction(xpathParserCtxt, 1);
    res = valuePop(xpathParserCtxt);
    do {
        tmp = valuePop(xpathParserCtxt);
	if (tmp != NULL) {
	    xmlXPathFreeObject(tmp);
	}
    } while (tmp != NULL);

    if (res != NULL) {
	if (res->type == XPATH_BOOLEAN)
	    doit = res->boolval;
	else {
#ifdef DEBUG_PROCESS
	    xsltGenericDebug(xsltGenericDebugContext,
		"xsltIf: test didn't evaluate to a boolean\n");
#endif
	    goto error;
	}
    }

#ifdef DEBUG_PROCESS
    xsltGenericDebug(xsltGenericDebugContext,
	"xsltIf: test evaluate to %d\n", doit);
#endif
    if (doit) {
	varsPush(ctxt, NULL);
	xsltApplyOneTemplate(ctxt, node, inst->children);
	xsltFreeStackElemList(varsPop(ctxt));
    }

error:
    if (xpathParserCtxt != NULL)
	xmlXPathFreeParserContext(xpathParserCtxt);
    if (prop != NULL)
	xmlFree(prop);
    if (res != NULL)
	xmlXPathFreeObject(res);
}

/**
 * xsltForEach:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt for-each node
 *
 * Process the xslt for-each node on the source node
 */
void
xsltForEach(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst) {
    xmlChar *prop;
    xmlXPathObjectPtr res = NULL, tmp;
    xmlNodePtr replacement;
    xmlNodeSetPtr list = NULL, oldlist;
    xmlXPathParserContextPtr xpathParserCtxt = NULL;
    int i, oldProximityPosition, oldContextSize;

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL))
	return;

    prop = xmlGetNsProp(inst, (const xmlChar *)"select", XSLT_NAMESPACE);
    if (prop == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsltForEach: select is not defined\n");
	return;
    }
#ifdef DEBUG_PROCESS
    xsltGenericDebug(xsltGenericDebugContext,
	 "xsltForEach: select %s\n", prop);
#endif

    xpathParserCtxt = xmlXPathNewParserContext(prop, ctxt->xpathCtxt);
    if (xpathParserCtxt == NULL)
	goto error;
    ctxt->xpathCtxt->node = node;
    xmlXPathEvalExpr(xpathParserCtxt);
    res = valuePop(xpathParserCtxt);
    do {
        tmp = valuePop(xpathParserCtxt);
	if (tmp != NULL) {
	    xmlXPathFreeObject(tmp);
	}
    } while (tmp != NULL);

    if (res != NULL) {
	if (res->type == XPATH_NODESET)
	    list = res->nodesetval;
	else {
#ifdef DEBUG_PROCESS
	    xsltGenericDebug(xsltGenericDebugContext,
		"xsltForEach: select didn't evaluate to a node list\n");
#endif
	    goto error;
	}
    }

#ifdef DEBUG_PROCESS
    xsltGenericDebug(xsltGenericDebugContext,
	"xsltForEach: select evaluate to %d nodes\n", list->nodeNr);
#endif

    oldlist = ctxt->nodeList;
    ctxt->nodeList = list;
    oldContextSize = ctxt->xpathCtxt->contextSize;
    oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
    ctxt->xpathCtxt->contextSize = list->nodeNr;

    /* 
     * handle and skip the xsl:sort
     */
    replacement = inst->children;
    while (IS_XSLT_ELEM(replacement) && (IS_XSLT_NAME(replacement, "sort"))) {
	xsltSort(ctxt, node, replacement);
	replacement = replacement->next;
    }

    for (i = 0;i < list->nodeNr;i++) {
	ctxt->node = list->nodeTab[i];
	ctxt->xpathCtxt->proximityPosition = i + 1;
	varsPush(ctxt, NULL);
	xsltApplyOneTemplate(ctxt, list->nodeTab[i], replacement);
	xsltFreeStackElemList(varsPop(ctxt));
    }
    ctxt->nodeList = oldlist;
    ctxt->xpathCtxt->contextSize = oldContextSize;
    ctxt->xpathCtxt->proximityPosition = oldProximityPosition;

error:
    if (xpathParserCtxt != NULL)
	xmlXPathFreeParserContext(xpathParserCtxt);
    if (prop != NULL)
	xmlFree(prop);
    if (res != NULL)
	xmlXPathFreeObject(res);
}

/**
 * xsltProcessOneNode:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 *
 * Process the source node.
 */
void
xsltProcessOneNode(xsltTransformContextPtr ctxt, xmlNodePtr node) {
    xsltTemplatePtr template;
    xmlNodePtr oldNode;

    template = xsltGetTemplate(ctxt, node, NULL);
    /*
     * If no template is found, apply the default rule.
     */
    if (template == NULL) {
#ifdef DEBUG_PROCESS
	if (node->type == XML_DOCUMENT_NODE)
	    xsltGenericDebug(xsltGenericDebugContext,
	     "xsltProcessOneNode: no template found for /\n");
	else if (node->type == XML_CDATA_SECTION_NODE)
	    xsltGenericDebug(xsltGenericDebugContext,
	     "xsltProcessOneNode: no template found for CDATA\n");
	else 
	    xsltGenericDebug(xsltGenericDebugContext,
	     "xsltProcessOneNode: no template found for %s\n", node->name);
#endif
	oldNode = ctxt->node;
	ctxt->node = node;
	xsltDefaultProcessOneNode(ctxt, node);
	ctxt->node = oldNode;
	return;
    }

    if (node->type == XML_ATTRIBUTE_NODE) {
#ifdef DEBUG_PROCESS
	xsltGenericDebug(xsltGenericDebugContext,
	     "xsltProcessOneNode: applying template for attribute %s\n",
	                 node->name);
#endif
	templPush(ctxt, template);
	varsPush(ctxt, NULL);
	xsltApplyOneTemplate(ctxt, node, template->content);
	xsltFreeStackElemList(varsPop(ctxt));
	templPop(ctxt);
    } else {
#ifdef DEBUG_PROCESS
	if (node->type == XML_DOCUMENT_NODE)
	    xsltGenericDebug(xsltGenericDebugContext,
	     "xsltProcessOneNode: applying template for /\n");
	else 
	    xsltGenericDebug(xsltGenericDebugContext,
	     "xsltProcessOneNode: applying template for %s\n", node->name);
#endif
	oldNode = ctxt->node;
	ctxt->node = node;
	templPush(ctxt, template);
	varsPush(ctxt, NULL);
	xsltApplyOneTemplate(ctxt, node, template->content);
	xsltFreeStackElemList(varsPop(ctxt));
	templPop(ctxt);
	ctxt->node = oldNode;
    }
}

/**
 * xsltApplyStylesheet:
 * @style:  a parsed XSLT stylesheet
 * @doc:  a parsed XML document
 *
 * Apply the stylesheet to the document
 * NOTE: This may lead to a non-wellformed output XML wise !
 *
 * Returns the result document or NULL in case of error
 */
xmlDocPtr
xsltApplyStylesheet(xsltStylesheetPtr style, xmlDocPtr doc) {
    xmlDocPtr res = NULL;
    xsltTransformContextPtr ctxt = NULL;
    xmlNodePtr root;

    if ((style == NULL) || (doc == NULL))
	return(NULL);
    ctxt = xsltNewTransformContext(style, doc);
    if (ctxt == NULL)
	return(NULL);
    if ((style->method != NULL) &&
	(!xmlStrEqual(style->method, (const xmlChar *) "xml"))) {
	if (xmlStrEqual(style->method, (const xmlChar *) "html")) {
	    ctxt->type = XSLT_OUTPUT_HTML;
	    res = htmlNewDoc(style->doctypePublic, style->doctypeSystem);
	    if (res == NULL)
		goto error;
	} else if (xmlStrEqual(style->method, (const xmlChar *) "xhtml")) {
	    xsltGenericError(xsltGenericErrorContext,
	     "xsltApplyStylesheet: insupported method xhtml, using html\n",
		             style->method);
	    ctxt->type = XSLT_OUTPUT_HTML;
	    res = htmlNewDoc(style->doctypePublic, style->doctypeSystem);
	    if (res == NULL)
		goto error;
	} else if (xmlStrEqual(style->method, (const xmlChar *) "text")) {
	    ctxt->type = XSLT_OUTPUT_TEXT;
	    res = xmlNewDoc(style->version);
	    if (res == NULL)
		goto error;
	} else {
	    xsltGenericError(xsltGenericErrorContext,
			     "xsltApplyStylesheet: insupported method %s\n",
		             style->method);
	    goto error;
	}
    } else {
	ctxt->type = XSLT_OUTPUT_XML;
	res = xmlNewDoc(style->version);
	if (res == NULL)
	    goto error;
    }
    res->charset = XML_CHAR_ENCODING_UTF8;
    if (style->encoding != NULL)
	res->encoding = xmlStrdup(style->encoding);

    /*
     * Start.
     */
    ctxt->output = res;
    ctxt->insert = (xmlNodePtr) res;
    ctxt->node = (xmlNodePtr) doc;
    varsPush(ctxt, NULL);
    xsltEvalGlobalVariables(ctxt);
    xsltProcessOneNode(ctxt, ctxt->node);
    xsltFreeStackElemList(varsPop(ctxt));


    if ((ctxt->type = XSLT_OUTPUT_XML) &&
	((style->doctypePublic != NULL) ||
	 (style->doctypeSystem != NULL))) {
	root = xmlDocGetRootElement(res);
	if (root != NULL)
	    res->intSubset = xmlCreateIntSubset(res, root->name,
		         style->doctypePublic, style->doctypeSystem);
    }
    xmlXPathFreeNodeSet(ctxt->nodeList);
    xsltFreeTransformContext(ctxt);
    return(res);

error:
    if (res != NULL)
        xmlFreeDoc(res);
    if (ctxt != NULL)
        xsltFreeTransformContext(ctxt);
    return(NULL);
}

