/*
 * transform.c: Implemetation of the XSL Transformation 1.0 engine
 *            transform part, i.e. applying a Stylesheet to a document
 *
 * References:
 *   http://www.w3.org/TR/1999/REC-xslt-19991116
 *
 *   Michael Kay "XSLT Programmer's Reference" pp 637-643
 *   Writing Multiple Output Files
 *
 *   XSLT-1.1 Working Draft
 *   http://www.w3.org/TR/xslt11#multiple-output
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
#include <libxml/uri.h>
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
#include "extensions.h"
#include "extra.h"
#include "preproc.h"

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
PUSH_AND_POP(static, xsltTemplatePtr, templ)
PUSH_AND_POP(static, xsltStackElemPtr, vars)

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
static xsltTransformContextPtr
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
    cur->xpathCtxt->proximityPosition = 0;
    cur->xpathCtxt->contextSize = 0;
    XSLT_REGISTER_VARIABLE_LOOKUP(cur);
    cur->xpathCtxt->nsHash = style->nsHash;
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
static void
xsltFreeTransformContext(xsltTransformContextPtr ctxt) {
    if (ctxt == NULL)
	return;
    if (ctxt->xpathCtxt != NULL) {
	ctxt->xpathCtxt->nsHash = NULL;
	xmlXPathFreeContext(ctxt->xpathCtxt);
    }
    if (ctxt->templTab != NULL)
	xmlFree(ctxt->templTab);
    if (ctxt->varsTab != NULL)
	xmlFree(ctxt->varsTab);
    xsltFreeDocuments(ctxt);
    xsltFreeCtxtExts(ctxt);
    xsltFreeStylePreComps(ctxt);
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
 * xsltCopyProp:
 * @ctxt:  a XSLT process context
 * @target:  the element where the attribute will be grafted
 * @attr:  the attribute
 *
 * Do a copy of an attribute
 *
 * Returns: a new xmlAttrPtr, or NULL in case of error.
 */
static xmlAttrPtr
xsltCopyProp(xsltTransformContextPtr ctxt, xmlNodePtr target,
	     xmlAttrPtr attr) {
    xmlAttrPtr ret = NULL;
    xmlNsPtr ns;
    xmlChar *val;

    if (attr == NULL)
	return(NULL);

    if (attr->ns != NULL) {
	ns = xsltGetNamespace(ctxt, attr->parent, attr->ns, target);
    } else {
	ns = NULL;
    }
    val = xmlNodeListGetString(attr->doc, attr->children, 1);
    ret = xmlSetNsProp(target, ns, attr->name, val);
    if (val != NULL)
	xmlFree(val);
    return(ret);
}

/**
 * xsltCopyPropList:
 * @ctxt:  a XSLT process context
 * @target:  the element where the attributes will be grafted
 * @cur:  the first attribute
 *
 * Do a copy of an attribute list.
 *
 * Returns: a new xmlAttrPtr, or NULL in case of error.
 */
static xmlAttrPtr
xsltCopyPropList(xsltTransformContextPtr ctxt, xmlNodePtr target,
	         xmlAttrPtr cur) {
    xmlAttrPtr ret = NULL;
    xmlAttrPtr p = NULL,q;
    xmlNsPtr ns;

    while (cur != NULL) {
	if (cur->ns != NULL) {
	    ns = xsltGetNamespace(ctxt, cur->parent, cur->ns, target);
	} else {
	    ns = NULL;
	}
        q = xmlCopyProp(target, cur);
	if (q != NULL) {
	    q->ns = ns;
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
    return(ret);
}

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
static xmlNodePtr
xsltCopyNode(xsltTransformContextPtr ctxt, xmlNodePtr node,
	     xmlNodePtr insert) {
    xmlNodePtr copy;

    copy = xmlCopyNode(node, 0);
    if (copy != NULL) {
	copy->doc = ctxt->output;
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
static xmlNodePtr
xsltCopyTreeList(xsltTransformContextPtr ctxt, xmlNodePtr list,
	     xmlNodePtr insert) {
    xmlNodePtr copy, ret = NULL, last = NULL;

    while (list != NULL) {
	copy = xsltCopyTree(ctxt, list, insert);
	if (copy != NULL) {
	    if (ret == NULL) {
		ret = copy;
		last = ret;
	    } else {
		last->next = copy;
		last = copy;
	    }
	}
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
	if (node->properties != NULL)
	    copy->properties = xsltCopyPropList(ctxt, copy,
					       node->properties);
	if (node->children != NULL)
	    xsltCopyTreeList(ctxt, node->children, copy);
    } else {
	xsltGenericError(xsltGenericErrorContext,
		"xsltCopyTree: copy %s failed\n", node->name);
    }
    return(copy);
}

/************************************************************************
 *									*
 *			Default processing				*
 *									*
 ************************************************************************/

void xsltProcessOneNode(xsltTransformContextPtr ctxt, xmlNodePtr node);
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
static void
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
		xsltApplyOneTemplate(ctxt, node, template->content, 1);
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
		xsltApplyOneTemplate(ctxt, node, template->content, 1);
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
		    xmlAttrPtr attr = (xmlAttrPtr) node, ret = NULL, current;
		template = xsltGetTemplate(ctxt, node, NULL);
		if (template) {
		    xmlNodePtr oldNode;

		    oldNode = ctxt->node;
		    ctxt->node = node;
		    templPush(ctxt, template);
		    xsltApplyOneTemplate(ctxt, node, template->content, 1);
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

		    current = ctxt->insert->properties;
		    if (current != NULL) {
			while (current->next != NULL)
			    current = current->next;
			current->next = ret;
			ret->prev = current;
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
	    xsltApplyOneTemplate(ctxt, node, template->content, 1);
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
		varsPush( ctxt, NULL );
		xsltProcessOneNode(ctxt, cur);
		xsltFreeStackElemList( varsPop(ctxt) );
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
		    xsltApplyOneTemplate(ctxt, node, template->content, 1);
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
		    xsltApplyOneTemplate(ctxt, cur, template->content, 1);
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
		    xsltApplyOneTemplate(ctxt, cur, template->content, 1);
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

    /*
     * Cleanup children empty nodes if asked for
     */
    if ((node->children != NULL) &&
	(xsltFindElemSpaceHandling(ctxt, node))) {
	xmlNodePtr delete = NULL, cur = node->children;

	while (cur != NULL) {
	    if (IS_BLANK_NODE(cur))
		delete = cur;
	    
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
    }

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
	xsltApplyOneTemplate(ctxt, node, template->content, 1);
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
	xsltApplyOneTemplate(ctxt, node, template->content, 1);
	templPop(ctxt);
	ctxt->node = oldNode;
    }
}

/**
 * xsltApplyOneTemplate:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @list:  the template replacement nodelist
 * @real: is this a real template processing
 *
 * Process the apply-templates node on the source node
 */
void
xsltApplyOneTemplate(xsltTransformContextPtr ctxt, xmlNodePtr node,
	             xmlNodePtr list, int real) {
    xmlNodePtr cur = NULL, insert, copy = NULL;
    xmlNodePtr oldInsert;
    xmlNodePtr oldCurrent = NULL;
    xmlAttrPtr attrs;

    if (list == NULL)
	return;
    CHECK_STOPPED;

    if (ctxt->templNr >= xsltMaxDepth) {
	xsltGenericError(xsltGenericErrorContext,
		"xsltApplyOneTemplate: loop found ???\n");
	xsltGenericError(xsltGenericErrorContext,
		"try increasing xsltMaxDepth (--maxdepth)\n");
	xsltDebug(ctxt, node, list, NULL);
	return;
    }

    /*
     * stack and saves
     */
    oldInsert = insert = ctxt->insert;
    if (real) {
	oldCurrent = ctxt->node;
	ctxt->node = node;
    }

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
	    if (real)
		ctxt->node = oldCurrent;
	    return;
	}

	if (IS_XSLT_ELEM(cur)) {
	    if (cur->_private == NULL)
		xsltStylePreCompute(ctxt, cur);
	    
	    if (cur->_private != NULL) {
		xsltStylePreCompPtr info = (xsltStylePreCompPtr) cur->_private;
		if (info->func != NULL) {
		    ctxt->insert = insert;
		    info->func(ctxt, node, cur, info);
		    ctxt->insert = oldInsert;
		} else {
		    xsltGenericError(xsltGenericDebugContext,
		     "xsltApplyOneTemplate: %s has _private without function\n",
				     cur->name);
		}
		goto skip_children;
	    }

	    if (IS_XSLT_NAME(cur, "variable")) {
		xsltParseStylesheetVariable(ctxt, cur);
	    } else if (IS_XSLT_NAME(cur, "param")) {
		xsltParseStylesheetParam(ctxt, cur);
	    } else if (IS_XSLT_NAME(cur, "message")) {
		xsltMessage(ctxt, node, cur);
	    } else {
		xsltGenericError(xsltGenericDebugContext,
		     "xsltApplyOneTemplate: problem with xsl:%s\n",
	                         cur->name);
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
		   (cur->ns != NULL) && (cur->_private != NULL)) {
	    xsltTransformFunction function;
	    /*
	     * Flagged as an extension element
	     */
	    function = (xsltTransformFunction)
		xmlHashLookup2(ctxt->extElements, cur->name, cur->ns->href);
	    if (function == NULL) {
		xsltGenericError(xsltGenericErrorContext,
			"xsltApplyOneTemplate: failed to find extension %s\n",
			         cur->name);
	    } else {
#ifdef DEBUG_PROCESS
		xsltGenericDebug(xsltGenericDebugContext,
		 "xsltApplyOneTemplate: extension construct %s\n", cur->name);
#endif

		if (cur->_private == (void *) xsltExtMarker) {
		    cur->_private = NULL;
		    xsltStylePreCompute(ctxt, cur);
		}
		ctxt->insert = insert;
		function(ctxt, node, cur, cur->_private);
		ctxt->insert = oldInsert;
	    }
	    goto skip_children;
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
		    xmlAttrPtr current = copy->properties;
		    while (current->next != NULL)
			current = current->next;
		    current->next = attrs;
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
    if (real)
	ctxt->node = oldCurrent;
}


/************************************************************************
 *									*
 *		    XSLT-1.1 extensions					*
 *									*
 ************************************************************************/

/**
 * xsltDocumentElem:
 * @ctxt:  an XSLT processing context
 * @node:  The current node
 * @inst:  the instruction in the stylesheet
 * @comp:  precomputed informations
 *
 * Process an XSLT-1.1 document element
 */
void 
xsltDocumentElem(xsltTransformContextPtr ctxt, xmlNodePtr node,
	         xmlNodePtr inst, xsltStylePreCompPtr comp) {
    xsltStylesheetPtr style = NULL;
    int ret;
    xmlChar *filename = NULL;
    xmlDocPtr result = NULL;
    xmlDocPtr oldOutput;
    xmlNodePtr oldInsert;

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL) || (comp == NULL))
	return;

    if (comp->filename == NULL) {
	xmlChar *base = NULL;
	xmlChar *URL = NULL;
	if (xmlStrEqual(inst->name, (const xmlChar *) "output")) {
#ifdef DEBUG_EXTRA
	    xsltGenericDebug(xsltGenericDebugContext,
		"Found saxon:output extension\n");
#endif
	    filename = xsltEvalAttrValueTemplate(ctxt, inst,
			     (const xmlChar *)"file",
			     XSLT_SAXON_NAMESPACE);
	} else if (xmlStrEqual(inst->name, (const xmlChar *) "write")) {
#ifdef DEBUG_EXTRA
	    xsltGenericDebug(xsltGenericDebugContext,
		"Found xalan:write extension\n");
#endif
	    filename = xsltEvalAttrValueTemplate(ctxt, inst,
			     (const xmlChar *)"select",
			     XSLT_XALAN_NAMESPACE);
	} else if (xmlStrEqual(inst->name, (const xmlChar *) "document")) {
	    filename = xsltEvalAttrValueTemplate(ctxt, inst,
			     (const xmlChar *)"href",
			     XSLT_XT_NAMESPACE);
	    if (filename == NULL) {
#ifdef DEBUG_EXTRA
		xsltGenericDebug(xsltGenericDebugContext,
		    "Found xslt11:document construct\n");
#endif
		filename = xsltEvalAttrValueTemplate(ctxt, inst,
				 (const xmlChar *)"href",
				 XSLT_NAMESPACE);
		comp->ver11 = 1;
	    } else {
#ifdef DEBUG_EXTRA
		xsltGenericDebug(xsltGenericDebugContext,
		    "Found xt:document extension\n");
#endif
		comp->ver11 = 0;
	    }
	}
	if (filename == NULL)
	    return;

	/*
	 * Compute output URL
	 */
	base = xmlNodeGetBase(inst->doc, inst);
	URL = xmlBuildURI(filename, base);
	if (URL == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		"xsltDocumentComp: URL computation failed %s\n", filename);
	} else {
	    xmlFree(filename);
	    filename = URL;
	}
	if (base != NULL)
	    xmlFree(base);
    } else {
	filename = xmlStrdup(comp->filename);
    }

    oldOutput = ctxt->output;
    oldInsert = ctxt->insert;

    style = xsltNewStylesheet();
    if (style == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsltDocumentElem: out of memory\n");
	goto error;
    }

    /*
     * Version described in 1.1 draft allows full parametrization
     * of the output.
     */
    xsltParseStylesheetOutput(style, inst);

    /*
     * Create a new document tree and process the element template
     */
    result = xmlNewDoc(style->version);
    if (result == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsltDocumentElem: out of memory\n");
	goto error;
    }
    ctxt->output = result;
    ctxt->insert = (xmlNodePtr) result;
    varsPush(ctxt, NULL);
    xsltApplyOneTemplate(ctxt, node, inst->children, 0);
    xsltFreeStackElemList(varsPop(ctxt));

    /*
     * Save the result
     */
    ret = xsltSaveResultToFilename((const char *) filename,
				   result, style, 0);
    if (ret < 0) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsltDocumentElem: unable to save to %s\n", filename);
    } else {
#ifdef DEBUG_EXTRA
	xsltGenericDebug(xsltGenericDebugContext,
	    "Wrote %d bytes to %s\n", ret, , filename);
#endif
    }

error:
    ctxt->output = oldOutput;
    ctxt->insert = oldInsert;
    if (filename != NULL)
        xmlFree(filename);
    if (style != NULL)
	xsltFreeStylesheet(style);
    if (result != NULL)
	xmlFreeDoc(result);
}

/************************************************************************
 *									*
 *		Most of the XSLT-1.0 transformations			*
 *									*
 ************************************************************************/

void xsltProcessOneNode(xsltTransformContextPtr ctxt, xmlNodePtr node);

/**
 * xsltSort:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt sort node
 * @comp:  precomputed informations
 *
 * Process the xslt sort node on the source node
 */
void
xsltSort(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst, xsltStylePreCompPtr comp) {
    xmlXPathObjectPtr *results = NULL;
    xmlNodeSetPtr list = NULL;
    xmlXPathObjectPtr res;
    int len = 0;
    int i;
    xmlNodePtr oldNode;

    if (comp == NULL) {
	xsltStylePreCompute(ctxt, inst);
	comp = inst->_private;
    }

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL) || (comp == NULL))
	return;
    if (comp->select == NULL)
	return;
    if (comp->comp == NULL) {
	comp->comp = xmlXPathCompile(comp->select);
	if (comp->comp == NULL)
	    return;
    }


    list = ctxt->nodeList;
    if ((list == NULL) || (list->nodeNr <= 1))
	goto error; /* nothing to do */

    len = list->nodeNr;

    /* TODO: xsl:sort lang attribute */
    /* TODO: xsl:sort case-order attribute */


    results = xmlMalloc(len * sizeof(xmlXPathObjectPtr));
    if (results == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsltSort: memory allocation failure\n");
	goto error;
    }

    oldNode = ctxt->node;
    for (i = 0;i < len;i++) {
	ctxt->xpathCtxt->contextSize = len;
	ctxt->xpathCtxt->proximityPosition = i + 1;
	ctxt->node = list->nodeTab[i];
	ctxt->xpathCtxt->node = ctxt->node;
	ctxt->xpathCtxt->namespaces = comp->nsList;
	ctxt->xpathCtxt->nsNr = comp->nsNr;
	res = xmlXPathCompiledEval(comp->comp, ctxt->xpathCtxt);
	if (res != NULL) {
	    if (res->type != XPATH_STRING)
		res = xmlXPathConvertString(res);
	    if (comp->number)
		res = xmlXPathConvertNumber(res);
	    res->index = i;	/* Save original pos for dupl resolv */
	    if (comp->number) {
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

    xsltSortFunction(list, &results[0], comp->descending, comp->number);

error:
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
 * @comp:  precomputed informations
 *
 * Process the xslt copy node on the source node
 */
void
xsltCopy(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst, xsltStylePreCompPtr comp) {
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
		if (comp->use != NULL) {
		    xsltApplyAttributeSet(ctxt, node, inst, comp->use);
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
	    xsltApplyOneTemplate(ctxt, ctxt->node, inst->children, 0);
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
 * @comp:  precomputed informations
 *
 * Process the xslt text node on the source node
 */
void
xsltText(xsltTransformContextPtr ctxt, xmlNodePtr node ATTRIBUTE_UNUSED,
	    xmlNodePtr inst, xsltStylePreCompPtr comp) {
    xmlNodePtr copy;

    if (inst->children != NULL) {
	if ((inst->children->type != XML_TEXT_NODE) ||
	    (inst->children->next != NULL)) {
	    xsltGenericError(xsltGenericErrorContext,
		 "xslt:text has content problem !\n");
	} else {
	    xmlNodePtr text = inst->children;
	    
	    copy = xmlNewDocText(ctxt->output, text->content);
	    if (comp->noescape) {
#ifdef DEBUG_PARSING
		xsltGenericDebug(xsltGenericDebugContext,
		     "Disable escaping: %s\n", text->content);
#endif
		copy->name = xmlStringTextNoenc;
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
 * @comp:  precomputed informations
 *
 * Process the xslt element node on the source node
 */
void
xsltElement(xsltTransformContextPtr ctxt, xmlNodePtr node,
	    xmlNodePtr inst, xsltStylePreCompPtr comp) {
    xmlChar *prop = NULL, *attributes = NULL;
    xmlChar *ncname = NULL, *name, *namespace;
    xmlChar *prefix = NULL;
    xmlChar *value = NULL;
    xmlNsPtr ns = NULL;
    xmlNodePtr copy;
    xmlNodePtr oldInsert;


    if (ctxt->insert == NULL)
	return;
    if (!comp->has_name) {
	return;
    }

    /*
     * stack and saves
     */
    oldInsert = ctxt->insert;

    if (comp->name == NULL) {
	prop = xsltEvalAttrValueTemplate(ctxt, inst,
		      (const xmlChar *)"name", XSLT_NAMESPACE);
	if (prop == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		 "xslt:element : name is missing\n");
	    goto error;
	}
	name = prop;
    } else {
	name = comp->name;
    }

    ncname = xmlSplitQName2(name, &prefix);
    if (ncname == NULL) {
	prefix = NULL;
    } else {
	name = ncname;
    }
    if ((comp->ns == NULL) && (comp->has_ns)) {
	namespace = xsltEvalAttrValueTemplate(ctxt, inst,
		(const xmlChar *)"namespace", XSLT_NAMESPACE);
	if (namespace != NULL) {
	    ns = xsltGetSpecialNamespace(ctxt, inst, namespace, prefix,
		                         ctxt->insert);
	    xmlFree(namespace);
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
    } else if (comp->ns != NULL) {
	ns = xsltGetSpecialNamespace(ctxt, inst, comp->ns, prefix,
				     ctxt->insert);
    }

    copy = xmlNewDocNode(ctxt->output, ns, name, NULL);
    if (copy == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsl:element : creation of %s failed\n", name);
	goto error;
    }
    xmlAddChild(ctxt->insert, copy);
    ctxt->insert = copy;
    if (comp->has_use) {
	if (comp->use != NULL) {
	    xsltApplyAttributeSet(ctxt, node, inst, comp->use);
	} else {
	    attributes = xsltEvalAttrValueTemplate(ctxt, inst,
		       (const xmlChar *)"use-attribute-sets", XSLT_NAMESPACE);
	    if (attributes != NULL) {
		xsltApplyAttributeSet(ctxt, node, inst, attributes);
		xmlFree(attributes);
	    }
	}
    }
    
    varsPush(ctxt, NULL);
    xsltApplyOneTemplate(ctxt, ctxt->node, inst->children, 0);
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
 * xsltAttribute:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt attribute node
 * @comp:  precomputed informations
 *
 * Process the xslt attribute node on the source node
 */
void
xsltAttribute(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst, xsltStylePreCompPtr comp) {
    xmlChar *prop = NULL;
    xmlChar *ncname = NULL, *name, *namespace;
    xmlChar *prefix = NULL;
    xmlChar *value = NULL;
    xmlNsPtr ns = NULL;
    xmlAttrPtr attr;


    if (ctxt->insert == NULL)
	return;
    if (comp == NULL) {
	xsltStylePreCompute(ctxt, inst);
	comp = inst->_private;
    }

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL) || (comp == NULL))
	return;
    if (!comp->has_name) {
	return;
    }
    if (ctxt->insert->children != NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:attribute : node has already children\n");
	return;
    }
    if (comp->name == NULL) {
	prop = xsltEvalAttrValueTemplate(ctxt, inst, (const xmlChar *)"name",
		                         XSLT_NAMESPACE);
	if (prop == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		 "xslt:element : name is missing\n");
	    goto error;
	}
	name = prop;
    } else {
	name = comp->name;
    }

    ncname = xmlSplitQName2(name, &prefix);
    if (ncname == NULL) {
	prefix = NULL;
    } else {
	name = ncname;
    }
    if (!xmlStrncasecmp(prefix, (xmlChar *)"xml", 3)) {
#ifdef DEBUG_PARSING
	xsltGenericDebug(xsltGenericDebugContext,
	     "xslt:attribute : xml prefix forbidden\n");
#endif
	goto error;
    }
    if ((comp->ns == NULL) && (comp->has_ns)) {
	namespace = xsltEvalAttrValueTemplate(ctxt, inst,
		(const xmlChar *)"namespace", XSLT_NAMESPACE);
	if (namespace != NULL) {
	    ns = xsltGetSpecialNamespace(ctxt, inst, namespace, prefix,
		                         ctxt->insert);
	    xmlFree(namespace);
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
    } else if (comp->ns != NULL) {
	ns = xsltGetSpecialNamespace(ctxt, inst, comp->ns, prefix,
				     ctxt->insert);
    }

    value = xsltEvalTemplateString(ctxt, node, inst);
    if (value == NULL) {
	if (ns) {
	    attr = xmlSetNsProp(ctxt->insert, ns, name, 
		                (const xmlChar *)"");
	} else
	    attr = xmlSetProp(ctxt->insert, name, (const xmlChar *)"");
    } else {
	if (ns) {
	    attr = xmlSetNsProp(ctxt->insert, ns, name, value);
	} else
	    attr = xmlSetProp(ctxt->insert, name, value);
	
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
 * xsltComment:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt comment node
 * @comp:  precomputed informations
 *
 * Process the xslt comment node on the source node
 */
void
xsltComment(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst, xsltStylePreCompPtr comp ATTRIBUTE_UNUSED) {
    xmlChar *value = NULL;
    xmlNodePtr comment;

    value = xsltEvalTemplateString(ctxt, node, inst);
    /* TODO: use or generate the compiled form */
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
 * @comp:  precomputed informations
 *
 * Process the xslt processing-instruction node on the source node
 */
void
xsltProcessingInstruction(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst, xsltStylePreCompPtr comp) {
    xmlChar *ncname = NULL, *name;
    xmlChar *value = NULL;
    xmlNodePtr pi;


    if (ctxt->insert == NULL)
	return;
    if (comp->has_name == 0)
	return;
    if (comp->name == NULL) {
	ncname = xsltEvalAttrValueTemplate(ctxt, inst,
			    (const xmlChar *)"name", XSLT_NAMESPACE);
	if (ncname == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		 "xslt:processing-instruction : name is missing\n");
	    goto error;
	}
	name = ncname;
    } else {
	name = comp->name;
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

    pi = xmlNewPI(name, value);
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
 * @comp:  precomputed informations
 *
 * Process the xslt copy-of node on the source node
 */
void
xsltCopyOf(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst, xsltStylePreCompPtr comp) {
    xmlXPathObjectPtr res = NULL;
    xmlNodePtr copy = NULL;
    xmlNodeSetPtr list = NULL;
    int i;
    int oldProximityPosition, oldContextSize;

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL))
	return;

    if (comp->select == NULL)
	return;
    if (comp->comp == NULL) {
	comp->comp = xmlXPathCompile(comp->select);
	if (comp->comp == NULL)
	    return;
    }
#ifdef DEBUG_PROCESS
    xsltGenericDebug(xsltGenericDebugContext,
	 "xsltCopyOf: select %s\n", comp->select);
#endif

    oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
    oldContextSize = ctxt->xpathCtxt->contextSize;
    ctxt->xpathCtxt->node = node;
    ctxt->xpathCtxt->namespaces = comp->nsList;
    ctxt->xpathCtxt->nsNr = comp->nsNr;
    res = xmlXPathCompiledEval(comp->comp, ctxt->xpathCtxt);
    ctxt->xpathCtxt->proximityPosition = oldProximityPosition;
    ctxt->xpathCtxt->contextSize = oldContextSize;
    if (res != NULL) {
	if (res->type == XPATH_NODESET) {
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
		    } else if (list->nodeTab[i]->type == XML_ATTRIBUTE_NODE) {
			xsltCopyProp(ctxt, ctxt->insert, 
				     (xmlAttrPtr) list->nodeTab[i]);
		    } else {
			xsltCopyTree(ctxt, list->nodeTab[i], ctxt->insert);
		    }
		}
	    }
	} else if (res->type == XPATH_XSLT_TREE) {
	    list = res->nodesetval;
	    if ((list != NULL) && (list->nodeTab != NULL) &&
		(list->nodeTab[0] != NULL)) {
		xsltCopyTreeList(ctxt, list->nodeTab[0]->children,
			         ctxt->insert);
	    }
	} else {
	    /* convert to a string */
	    res = xmlXPathConvertString(res);
	    if ((res != NULL) && (res->type == XPATH_STRING)) {
		/* append content as text node */
		copy = xmlNewText(res->stringval);
		if (copy != NULL) {
		    xmlAddChild(ctxt->insert, copy);
		}
	    }
	    if (copy == NULL) {
		xsltGenericError(xsltGenericErrorContext,
		    "xsltCopyOf: text copy failed\n");
	    }
#ifdef DEBUG_PROCESS
	    else
		xsltGenericDebug(xsltGenericDebugContext,
		     "xslcopyOf: result %s\n", res->stringval);
#endif
	}
    }

    if (res != NULL)
	xmlXPathFreeObject(res);
}

/**
 * xsltValueOf:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt value-of node
 * @comp:  precomputed informations
 *
 * Process the xslt value-of node on the source node
 */
void
xsltValueOf(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst, xsltStylePreCompPtr comp) {
    xmlXPathObjectPtr res = NULL;
    xmlNodePtr copy = NULL;
    int oldProximityPosition, oldContextSize;

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL) || (comp == NULL))
	return;

    if (comp->select == NULL)
	return;
    if (comp->comp == NULL) {
	comp->comp = xmlXPathCompile(comp->select);
	if (comp->comp == NULL)
	    return;
    }

#ifdef DEBUG_PROCESS
    xsltGenericDebug(xsltGenericDebugContext,
	 "xsltValueOf: select %s\n", comp->select);
#endif

    oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
    oldContextSize = ctxt->xpathCtxt->contextSize;
    ctxt->xpathCtxt->node = node;
    ctxt->xpathCtxt->namespaces = comp->nsList;
    ctxt->xpathCtxt->nsNr = comp->nsNr;
    res = xmlXPathCompiledEval(comp->comp, ctxt->xpathCtxt);
    ctxt->xpathCtxt->proximityPosition = oldProximityPosition;
    ctxt->xpathCtxt->contextSize = oldContextSize;
    if (res != NULL) {
	if (res->type != XPATH_STRING)
	    res = xmlXPathConvertString(res);
	if (res->type == XPATH_STRING) {
            copy = xmlNewText(res->stringval);
	    if (copy != NULL) {
		if (comp->noescape)
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
    if (res != NULL)
	xmlXPathFreeObject(res);
}

/**
 * xsltNumber:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt number node
 * @comp:  precomputed informations
 *
 * Process the xslt number node on the source node
 */
void
xsltNumber(xsltTransformContextPtr ctxt, xmlNodePtr node,
	   xmlNodePtr inst, xsltStylePreCompPtr comp)
{
    if (comp == NULL) {
	xsltStylePreCompute(ctxt, inst);
	comp = inst->_private;
    }

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL) || (comp == NULL))
	return;

    comp->numdata.doc = inst->doc;
    comp->numdata.node = inst;
    
    xsltNumberFormat(ctxt, &comp->numdata, node);
}

/**
 * xsltApplyImports:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt apply-imports node
 * @comp:  precomputed informations
 *
 * Process the xslt apply-imports node on the source node
 */
void
xsltApplyImports(xsltTransformContextPtr ctxt, xmlNodePtr node,
	         xmlNodePtr inst ATTRIBUTE_UNUSED, xsltStylePreCompPtr comp ATTRIBUTE_UNUSED) {
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
	xsltApplyOneTemplate(ctxt, node, template->content, 1);
	xsltFreeStackElemList(varsPop(ctxt));
	templPop(ctxt);
    }
}

/**
 * xsltCallTemplate:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt call-template node
 * @comp:  precomputed informations
 *
 * Process the xslt call-template node on the source node
 */
void
xsltCallTemplate(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst, xsltStylePreCompPtr comp) {
    xmlNodePtr cur = NULL;
    xsltStackElemPtr params = NULL, param;


    if (comp == NULL) {
	xsltStylePreCompute(ctxt, inst);
	comp = inst->_private;
    }
    if (ctxt->insert == NULL)
	return;

    /*
     * The template must have been precomputed
     */
    if (comp->templ == NULL)
	return;

    /*
     * Create a new frame but block access to variables
     */
    templPush(ctxt, comp->templ);
    cur = inst->children;
    while (cur != NULL) {
	if (ctxt->state == XSLT_STATE_STOPPED) break;
	if (IS_XSLT_ELEM(cur)) {
	    if (IS_XSLT_NAME(cur, "with-param")) {
		param = xsltParseStylesheetCallerParam(ctxt, cur);
		if (param != NULL) {
		    param->next = params;
		    params = param;
		}
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
    varsPush(ctxt, params);
    xsltApplyOneTemplate(ctxt, node, comp->templ->content, 1);
    xsltFreeStackElemList(varsPop(ctxt));
    templPop(ctxt);
}

/**
 * xsltApplyTemplates:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the apply-templates node
 * @comp:  precomputed informations
 *
 * Process the apply-templates node on the source node
 */
void
xsltApplyTemplates(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst, xsltStylePreCompPtr comp) {
    xmlNodePtr cur, delete = NULL, oldnode;
    xmlXPathObjectPtr res = NULL;
    xmlNodeSetPtr list = NULL, oldlist;
    int i, oldProximityPosition, oldContextSize;
    const xmlChar *oldmode, *oldmodeURI;
    int have_sort=0;
    xsltStackElemPtr params = NULL, param, tmp, p;


    if (comp == NULL) {
	xsltStylePreCompute(ctxt, inst);
	comp = inst->_private;
    }
    if ((ctxt == NULL) || (node == NULL) || (inst == NULL) || (comp == NULL))
	return;

#ifdef DEBUG_PROCESS
    xsltGenericDebug(xsltGenericDebugContext,
	 "xsltApplyTemplates: node: %s\n", node->name);
#endif

    /*
     * Get mode if any
     */
    oldnode = ctxt->node;
    oldmode = ctxt->mode;
    oldmodeURI = ctxt->modeURI;
    ctxt->mode = comp->mode;
    ctxt->modeURI = comp->modeURI;

    if (comp->select != NULL) {
	if (comp->comp == NULL) {
	    comp->comp = xmlXPathCompile(comp->select);
	    if (comp->comp == NULL)
		goto error;
	}
#ifdef DEBUG_PROCESS
	xsltGenericDebug(xsltGenericDebugContext,
	     "xsltApplyTemplates: select %s\n", comp->select);
#endif

	oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
	oldContextSize = ctxt->xpathCtxt->contextSize;
	ctxt->xpathCtxt->node = node;
	ctxt->xpathCtxt->namespaces = comp->nsList;
	ctxt->xpathCtxt->nsNr = comp->nsNr;
	res = xmlXPathCompiledEval(comp->comp, ctxt->xpathCtxt);
	ctxt->xpathCtxt->contextSize = oldContextSize;
	ctxt->xpathCtxt->proximityPosition = oldProximityPosition;
	if (res != NULL) {
	    if (res->type == XPATH_NODESET) {
		list = res->nodesetval;
		res->nodesetval = NULL;
	     } else {
		list = NULL;
	     }
	}
	if (list == NULL) {
#ifdef DEBUG_PROCESS
	    xsltGenericDebug(xsltGenericDebugContext,
		"xsltApplyTemplates: select didn't evaluate to a node list\n");
#endif
	    goto error;
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
    if (list != NULL)
    xsltGenericDebug(xsltGenericDebugContext,
	"xsltApplyTemplates: list of %d nodes\n", list->nodeNr);
#endif

    oldlist = ctxt->nodeList;
    ctxt->nodeList = list;
    oldContextSize = ctxt->xpathCtxt->contextSize;
    oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
    ctxt->xpathCtxt->contextSize = list->nodeNr;

    /* 
     * handle (or skip) the xsl:sort and xsl:with-param
     */
    cur = inst->children;
    while (cur!=NULL) {
        if (ctxt->state == XSLT_STATE_STOPPED) break;
        if (IS_XSLT_ELEM(cur)) {
            if (IS_XSLT_NAME(cur, "with-param")) {
                param = xsltParseStylesheetCallerParam(ctxt, cur);
		if (param != NULL) {
		    param->next = params;
		    params = param;
		}
	    } else if (IS_XSLT_NAME(cur, "sort")) {
		if (!have_sort) {
		    have_sort = 1;
		    xsltSort(ctxt, node, cur, cur->_private);
		} else {
		    TODO /* imbricated sorts */
		}
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

    for (i = 0;i < list->nodeNr;i++) {
	ctxt->node = list->nodeTab[i];
	ctxt->xpathCtxt->proximityPosition = i + 1;
	varsPush(ctxt, params);
	xsltProcessOneNode(ctxt, list->nodeTab[i]);
	tmp = varsPop(ctxt);
	/*
	 * Free other parameter and variables which may have been
	 * added to the set defined in the caller.
	 */
	if (params == NULL) {
	    xsltFreeStackElemList(tmp);
	} else if (tmp != params) {
            p = tmp;
	    while ((p != NULL) && (p->next != params))
		p = p->next;
	    if (p == NULL) {
		xsltFreeStackElemList(tmp);
	    } else {
		p->next = NULL;
		xsltFreeStackElemList(tmp);
	    }
	}
    }
    xsltFreeStackElemList(params);	/* free the parameter list */
    ctxt->nodeList = oldlist;
    ctxt->xpathCtxt->contextSize = oldContextSize;
    ctxt->xpathCtxt->proximityPosition = oldProximityPosition;

error:
    ctxt->node = oldnode;
    ctxt->mode = oldmode;
    ctxt->modeURI = oldmodeURI;
    if (res != NULL)
	xmlXPathFreeObject(res);
    if (list != NULL)
	xmlXPathFreeNodeSet(list);
}


/**
 * xsltChoose:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt choose node
 * @comp:  precomputed informations
 *
 * Process the xslt choose node on the source node
 */
void
xsltChoose(xsltTransformContextPtr ctxt, xmlNodePtr node,
	   xmlNodePtr inst, xsltStylePreCompPtr comp ATTRIBUTE_UNUSED) {
    xmlChar *prop = NULL;
    xmlXPathObjectPtr res = NULL;
    xmlNodePtr replacement, when;
    int doit = 1;
    int oldProximityPosition, oldContextSize;

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
	xmlXPathCompExprPtr xpathComp;
        /* TODO: build a prexpathCompiled block for when too ! */
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

	xpathComp = xmlXPathCompile(prop);
	if (xpathComp == NULL)
	    goto error;
	oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
	oldContextSize = ctxt->xpathCtxt->contextSize;
  	ctxt->xpathCtxt->node = node;
	ctxt->xpathCtxt->namespaces = comp->nsList;
	ctxt->xpathCtxt->nsNr = comp->nsNr;
  	res = xmlXPathCompiledEval(xpathComp, ctxt->xpathCtxt);
	xmlXPathFreeCompExpr(xpathComp);
	ctxt->xpathCtxt->proximityPosition = oldProximityPosition;
	ctxt->xpathCtxt->contextSize = oldContextSize;
	if (res != NULL) {
	    if (res->type != XPATH_BOOLEAN)
		res = xmlXPathConvertBoolean(res);
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
	    xsltApplyOneTemplate(ctxt, ctxt->node, when->children, 0);
	    xsltFreeStackElemList(varsPop(ctxt));
	    goto done;
	}
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
	xsltApplyOneTemplate(ctxt, ctxt->node, replacement->children, 0);
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
 * @comp:  precomputed informations
 *
 * Process the xslt if node on the source node
 */
void
xsltIf(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst, xsltStylePreCompPtr comp) {
    xmlXPathObjectPtr res = NULL;
    int doit = 1;
    int oldContextSize, oldProximityPosition;

    if (comp == NULL) {
	xsltStylePreCompute(ctxt, inst);
	comp = inst->_private;
    }
    if ((ctxt == NULL) || (node == NULL) || (inst == NULL) || (comp == NULL))
	return;

    if (comp->test == NULL)
	return;
    if (comp->comp == NULL) {
	comp->comp = xmlXPathCompile(comp->test);
	if (comp->comp == NULL)
	    return;
    }

#ifdef DEBUG_PROCESS
    xsltGenericDebug(xsltGenericDebugContext,
	 "xsltIf: test %s\n", comp->test);
#endif

    oldContextSize = ctxt->xpathCtxt->contextSize;
    oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
    ctxt->xpathCtxt->node = node;
    ctxt->xpathCtxt->namespaces = comp->nsList;
    ctxt->xpathCtxt->nsNr = comp->nsNr;
    res = xmlXPathCompiledEval(comp->comp, ctxt->xpathCtxt);
    ctxt->xpathCtxt->contextSize = oldContextSize;
    ctxt->xpathCtxt->proximityPosition = oldProximityPosition;
    if (res != NULL) {
	if (res->type != XPATH_BOOLEAN)
	    res = xmlXPathConvertBoolean(res);
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
	xsltApplyOneTemplate(ctxt, node, inst->children, 0);
	xsltFreeStackElemList(varsPop(ctxt));
    }

error:
    if (res != NULL)
	xmlXPathFreeObject(res);
}

/**
 * xsltForEach:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt for-each node
 * @comp:  precomputed informations
 *
 * Process the xslt for-each node on the source node
 */
void
xsltForEach(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst, xsltStylePreCompPtr comp) {
    xmlXPathObjectPtr res = NULL;
    xmlNodePtr replacement;
    xmlNodeSetPtr list = NULL, oldlist;
    int i, oldProximityPosition, oldContextSize;
    /* xmlNodePtr oldInsert = ctxt->insert; */
    xmlNodePtr oldNode = ctxt->node;

    if (comp == NULL) {
	xsltStylePreCompute(ctxt, inst);
	comp = inst->_private;
    }
    if ((ctxt == NULL) || (node == NULL) || (inst == NULL) || (comp == NULL))
	return;

    if (comp->select == NULL)
	return;
    if (comp->comp == NULL) {
	comp->comp = xmlXPathCompile(comp->select);
	if (comp->comp == NULL)
	    return;
    }

#ifdef DEBUG_PROCESS
    xsltGenericDebug(xsltGenericDebugContext,
	 "xsltForEach: select %s\n", comp->select);
#endif

    oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
    oldContextSize = ctxt->xpathCtxt->contextSize;
    ctxt->xpathCtxt->node = node;
    ctxt->xpathCtxt->namespaces = comp->nsList;
    ctxt->xpathCtxt->nsNr = comp->nsNr;
    res = xmlXPathCompiledEval(comp->comp, ctxt->xpathCtxt);
    ctxt->xpathCtxt->contextSize = oldContextSize;
    ctxt->xpathCtxt->proximityPosition = oldProximityPosition;
    if (res != NULL) {
	if (res->type == XPATH_NODESET)
	    list = res->nodesetval;
    }
    if (list == NULL) {
#ifdef DEBUG_PROCESS
	xsltGenericDebug(xsltGenericDebugContext,
	    "xsltForEach: select didn't evaluate to a node list\n");
#endif
	goto error;
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
	xsltSort(ctxt, node, replacement, replacement->_private);
	replacement = replacement->next;
    }

    for (i = 0;i < list->nodeNr;i++) {
	ctxt->node = list->nodeTab[i];
	ctxt->xpathCtxt->proximityPosition = i + 1;
	/* ctxt->insert = oldInsert; */
	varsPush(ctxt, NULL);
	xsltApplyOneTemplate(ctxt, list->nodeTab[i], replacement, 0);
	xsltFreeStackElemList(varsPop(ctxt));
    }
    ctxt->nodeList = oldlist;
    ctxt->node = oldNode;
    ctxt->xpathCtxt->contextSize = oldContextSize;
    ctxt->xpathCtxt->proximityPosition = oldProximityPosition;

error:
    if (res != NULL)
	xmlXPathFreeObject(res);
}

/************************************************************************
 *									*
 *			Generic interface				*
 *									*
 ************************************************************************/

/**
 * xsltApplyStylesheet:
 * @style:  a parsed XSLT stylesheet
 * @doc:  a parsed XML document
 * @params:  a NULL terminated arry of parameters names/values tuples
 *
 * Apply the stylesheet to the document
 * NOTE: This may lead to a non-wellformed output XML wise !
 *
 * Returns the result document or NULL in case of error
 */
xmlDocPtr
xsltApplyStylesheet(xsltStylesheetPtr style, xmlDocPtr doc,
	            const char **params) {
    xmlDocPtr res = NULL;
    xsltTransformContextPtr ctxt = NULL;
    xmlNodePtr root;

    if ((style == NULL) || (doc == NULL))
	return(NULL);
    ctxt = xsltNewTransformContext(style, doc);
    xsltRegisterExtras(ctxt);
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
    if (params != NULL)
	xsltEvalUserParams(ctxt, params);
    xsltEvalGlobalVariables(ctxt);
    ctxt->node = (xmlNodePtr) doc;
    varsPush(ctxt, NULL);
    xsltProcessOneNode(ctxt, ctxt->node);
    xsltFreeStackElemList(varsPop(ctxt));
    xsltCleanupTemplates(style);


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

