/*
 * transform.c: Implemetation of the XSL Transformation 1.0 engine
 *              transform part, i.e. applying a Stylesheet to a document
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
 * daniel@veillard.com
 */

#include "libxslt.h"

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

#ifdef WITH_XSLT_DEBUG
#define WITH_XSLT_DEBUG_PROCESS
#endif

int xsltMaxDepth = 500;

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
    if (ctxt->name##Max == 0) {						\
	ctxt->name##Max = 4;						\
        ctxt->name##Tab = (type *) xmlMalloc(ctxt->name##Max *		\
	              sizeof(ctxt->name##Tab[0]));			\
        if (ctxt->name##Tab == NULL) {					\
	    xmlGenericError(xmlGenericErrorContext,			\
		    "malloc failed !\n");				\
	    return(0);							\
	}								\
    }									\
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
        ctxt->name = (type) 0;						\
    ret = ctxt->name##Tab[ctxt->name##Nr];				\
    ctxt->name##Tab[ctxt->name##Nr] = 0;				\
    return(ret);							\
}									\

/*
 * Those macros actually generate the functions
 */
PUSH_AND_POP(static, xsltTemplatePtr, templ)
PUSH_AND_POP(static, xsltStackElemPtr, vars)
PUSH_AND_POP(static, long, prof)

/************************************************************************
 *									*
 *			XInclude default settings			*
 *									*
 ************************************************************************/

static int xsltDoXIncludeDefault = 0;

/**
 * xsltSetXIncludeDefault:
 * @xinclude: whether to do XInclude processing
 *
 * Set whether XInclude should be processed on document being loaded by default
 */
void
xsltSetXIncludeDefault(int xinclude) {
    xsltDoXIncludeDefault = (xinclude != 0);
}

/**
 * xsltGetXIncludeDefault:
 *
 * return the default state for XInclude processing
 *
 * Returns 0 if there is no processing 1 otherwise
 */
int
xsltGetXIncludeDefault(void) {
    return(xsltDoXIncludeDefault);
}

/************************************************************************
 *									*
 *			Handling of Transformation Contexts		*
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
    cur->varsBase = 0;
    /*
     * the profiling stcka is not initialized by default
     */
    cur->profTab = NULL;
    cur->profNr = 0;
    cur->profMax = 0;
    cur->prof = 0;

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
    cur->inst = NULL;
    cur->xinclude = xsltDoXIncludeDefault;
    cur->outputFile = NULL;
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
    xsltFreeGlobalVariables(ctxt);
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
 * xsltCopyTextString:
 * @ctxt:  a XSLT process context
 * @target:  the element where the text will be attached
 * @string:  the text string
 *
 * Create a text node
 *
 * Returns: a new xmlNodePtr, or NULL in case of error.
 */
static xmlNodePtr
xsltCopyTextString(xsltTransformContextPtr ctxt, xmlNodePtr target,
	     const xmlChar *string) {
    xmlNodePtr copy;

    if (string == NULL)
	return(NULL);

#ifdef WITH_XSLT_DEBUG_PROCESS
    xsltGenericDebug(xsltGenericDebugContext,
		     "xsltCopyTextString: copy text %s\n",
		     string);
#endif

    /* TODO: handle coalescing of text nodes here */
    if ((ctxt->type == XSLT_OUTPUT_XML) &&
	(ctxt->style->cdataSection != NULL) &&
	(target != NULL) &&
	(xmlHashLookup(ctxt->style->cdataSection,
		       target->name) != NULL)) {
	copy = xmlNewCDataBlock(ctxt->output, string,
				xmlStrlen(string));
    } else {
	if ((target != NULL) && (target->last != NULL) &&
	    (target->last->type == XML_TEXT_NODE) &&
	    (target->last->name == xmlStringText)) {
	    xmlNodeAddContent(target->last, string);
	    return(target->last);
	}
	copy = xmlNewText(string);
    }
    if (copy != NULL) {
	if (target != NULL)
	    xmlAddChild(target, copy);
    } else {
	xsltGenericError(xsltGenericErrorContext,
			 "xsltCopyTextString: text copy failed\n");
    }
    return(copy);
}
/**
 * xsltCopyText:
 * @ctxt:  a XSLT process context
 * @target:  the element where the text will be attached
 * @cur:  the text or CDATA node
 *
 * Do a copy of a text node
 *
 * Returns: a new xmlNodePtr, or NULL in case of error.
 */
static xmlNodePtr
xsltCopyText(xsltTransformContextPtr ctxt, xmlNodePtr target,
	     xmlNodePtr cur) {
    xmlNodePtr copy;

    if ((cur->type != XML_TEXT_NODE) &&
	(cur->type != XML_CDATA_SECTION_NODE))
	return(NULL);
    if (cur->content == NULL)
	return(NULL);

#ifdef WITH_XSLT_DEBUG_PROCESS
    if (cur->type == XML_CDATA_SECTION_NODE)
	xsltGenericDebug(xsltGenericDebugContext,
			 "xsltCopyText: copy CDATA text %s\n",
			 cur->content);
    else if (cur->name == xmlStringTextNoenc)
	xsltGenericDebug(xsltGenericDebugContext,
		     "xsltCopyText: copy unescaped text %s\n",
			 cur->content);
    else
	xsltGenericDebug(xsltGenericDebugContext,
			 "xsltCopyText: copy text %s\n",
			 cur->content);
#endif

    /* TODO: handle coalescing of text nodes here */
    if ((ctxt->type == XSLT_OUTPUT_XML) &&
	(ctxt->style->cdataSection != NULL) &&
	(target != NULL) &&
	(xmlHashLookup(ctxt->style->cdataSection,
		       target->name) != NULL)) {
	copy = xmlNewCDataBlock(ctxt->output, cur->content,
				xmlStrlen(cur->content));
    } else {
	if ((target != NULL) && (target->last != NULL) &&
	    (target->last->type == XML_TEXT_NODE) &&
	    (target->last->name == xmlStringText) &&
	    (cur->name != xmlStringTextNoenc) &&
	    (cur->type != XML_CDATA_SECTION_NODE)) {
	    xmlNodeAddContent(target->last, cur->content);
	    return(target->last);
	}
	copy = xmlNewText(cur->content);
	if ((cur->name == xmlStringTextNoenc) ||
	    (cur->type == XML_CDATA_SECTION_NODE))
	    copy->name = xmlStringTextNoenc;
    }
    if (copy != NULL) {
	if (target != NULL)
	    xmlAddChild(target, copy);
    } else {
	xsltGenericError(xsltGenericErrorContext,
			 "xsltCopyText: text copy failed\n");
    }
    return(copy);
}

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
 * @target:  the element where the properties will be grafted
 * @cur:  the first property
 *
 * Do a copy of a properties list.
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

    if ((node->type == XML_TEXT_NODE) ||
	(node->type == XML_CDATA_SECTION_NODE))
	return(xsltCopyText(ctxt, insert, node));
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
 * @list:  the list of element nodes in the source tree.
 * @insert:  the parent in the result tree.
 *
 * Make a copy of the full list of tree @list
 * and insert it as last children of @insert
 *
 * Returns a pointer to the new list, or NULL in case of error
 */
static xmlNodePtr
xsltCopyTreeList(xsltTransformContextPtr ctxt, xmlNodePtr list,
	     xmlNodePtr insert) {
    xmlNodePtr copy, ret = NULL;

    while (list != NULL) {
	copy = xsltCopyTree(ctxt, list, insert);
	if (copy != NULL) {
	    if (ret == NULL) {
		ret = copy;
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

    if (node == NULL)
	return(NULL);
    switch (node->type) {
        case XML_ELEMENT_NODE:
        case XML_TEXT_NODE:
        case XML_CDATA_SECTION_NODE:
        case XML_ENTITY_REF_NODE:
        case XML_ENTITY_NODE:
        case XML_PI_NODE:
        case XML_COMMENT_NODE:
        case XML_DOCUMENT_NODE:
        case XML_HTML_DOCUMENT_NODE:
#ifdef LIBXML_DOCB_ENABLED
        case XML_DOCB_DOCUMENT_NODE:
#endif
	    break;
        case XML_ATTRIBUTE_NODE:
	    return((xmlNodePtr)
		   xsltCopyProp(ctxt, insert, (xmlAttrPtr) node));
        case XML_NAMESPACE_DECL:
	    return((xmlNodePtr)
		   xsltCopyNamespaceList(ctxt, insert, (xmlNsPtr) node));
	    
        case XML_DOCUMENT_TYPE_NODE:
        case XML_DOCUMENT_FRAG_NODE:
        case XML_NOTATION_NODE:
        case XML_DTD_NODE:
        case XML_ELEMENT_DECL:
        case XML_ATTRIBUTE_DECL:
        case XML_ENTITY_DECL:
        case XML_XINCLUDE_START:
        case XML_XINCLUDE_END:
            return(NULL);
    }
    copy = xmlCopyNode(node, 0);
    copy->doc = ctxt->output;
    if (copy != NULL) {
	xmlAddChild(insert, copy);
	copy->next = NULL;
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

void xsltProcessOneNode(xsltTransformContextPtr ctxt, xmlNodePtr node,
	                xsltStackElemPtr params);
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
 * Note also that namespace declarations are copied directly:
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
#ifdef WITH_XSLT_DEBUG_PROCESS
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
	    return;
	case XML_TEXT_NODE:
#ifdef WITH_XSLT_DEBUG_PROCESS
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
	    return;
	case XML_ATTRIBUTE_NODE:
	    cur = node->children;
	    while ((cur != NULL) && (cur->type != XML_TEXT_NODE))
		cur = cur->next;
	    if (cur == NULL) {
		xsltGenericError(xsltGenericErrorContext,
		 "xsltDefaultProcessOneNode: no text for attribute\n");
	    } else {
#ifdef WITH_XSLT_DEBUG_PROCESS
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
#ifdef WITH_XSLT_DEBUG_PROCESS
		xsltGenericDebug(xsltGenericDebugContext,
		 "xsltDefaultProcessOneNode: skipping node type %d\n",
		                 cur->type);
#endif
		delete = cur;
	}
	cur = cur->next;
	if (delete != NULL) {
#ifdef WITH_XSLT_DEBUG_PROCESS
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
	    xsltApplyOneTemplate(ctxt, node, template->content, template,
		                 NULL);
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
		xsltProcessOneNode(ctxt, cur, NULL);
		break;
	    case XML_CDATA_SECTION_NODE:
		template = xsltGetTemplate(ctxt, node, NULL);
		if (template) {
#ifdef WITH_XSLT_DEBUG_PROCESS
		    xsltGenericDebug(xsltGenericDebugContext,
		 "xsltDefaultProcessOneNode: applying template for CDATA %s\n",
				     node->content);
#endif
		    xsltApplyOneTemplate(ctxt, node, template->content,
			                 template, NULL);
		} else /* if (ctxt->mode == NULL) */ {
#ifdef WITH_XSLT_DEBUG_PROCESS
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
#ifdef WITH_XSLT_DEBUG_PROCESS
		    xsltGenericDebug(xsltGenericDebugContext,
	     "xsltDefaultProcessOneNode: applying template for text %s\n",
				     node->content);
#endif
		    ctxt->xpathCtxt->contextSize = nbchild;
		    ctxt->xpathCtxt->proximityPosition = childno;
		    xsltApplyOneTemplate(ctxt, cur, template->content,
			                 template, NULL);
		} else /* if (ctxt->mode == NULL) */ {
#ifdef WITH_XSLT_DEBUG_PROCESS
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
#ifdef WITH_XSLT_DEBUG_PROCESS
		    if (cur->type == XML_PI_NODE)
			xsltGenericDebug(xsltGenericDebugContext,
		     "xsltDefaultProcessOneNode: template found for PI %s\n",
			                 cur->name);
		    else if (cur->type == XML_COMMENT_NODE)
			xsltGenericDebug(xsltGenericDebugContext,
		     "xsltDefaultProcessOneNode: template found for comment\n");
#endif
		    ctxt->xpathCtxt->contextSize = nbchild;
		    ctxt->xpathCtxt->proximityPosition = childno;
		    xsltApplyOneTemplate(ctxt, cur, template->content,
			                 template, NULL);
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
xsltProcessOneNode(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xsltStackElemPtr params) {
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
#ifdef WITH_XSLT_DEBUG_PROCESS
		xsltGenericDebug(xsltGenericDebugContext,
	     "xsltProcessOneNode: removing ignorable blank node\n");
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
#ifdef WITH_XSLT_DEBUG_PROCESS
	if (node->type == XML_DOCUMENT_NODE)
	    xsltGenericDebug(xsltGenericDebugContext,
	     "xsltProcessOneNode: no template found for /\n");
	else if (node->type == XML_CDATA_SECTION_NODE)
	    xsltGenericDebug(xsltGenericDebugContext,
	     "xsltProcessOneNode: no template found for CDATA\n");
	else if (node->type == XML_ATTRIBUTE_NODE)
	    xsltGenericDebug(xsltGenericDebugContext,
	     "xsltProcessOneNode: no template found for attribute %s\n",
	                     ((xmlAttrPtr) node)->name);
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
#ifdef WITH_XSLT_DEBUG_PROCESS
	xsltGenericDebug(xsltGenericDebugContext,
	     "xsltProcessOneNode: applying template '%s' for attribute %s\n",
	                 template->match, node->name);
#endif
	xsltApplyOneTemplate(ctxt, node, template->content, template, params);
    } else {
#ifdef WITH_XSLT_DEBUG_PROCESS
	if (node->type == XML_DOCUMENT_NODE)
	    xsltGenericDebug(xsltGenericDebugContext,
	     "xsltProcessOneNode: applying template '%s' for /\n",
	                     template->match);
	else 
	    xsltGenericDebug(xsltGenericDebugContext,
	     "xsltProcessOneNode: applying template '%s' for %s\n",
	                     template->match, node->name);
#endif
	xsltApplyOneTemplate(ctxt, node, template->content, template, params);
    }
}

/**
 * xsltApplyOneTemplate:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @list:  the template replacement nodelist
 * @templ: if is this a real template processing, the template processed
 * @params:  a set of parameters for the template or NULL
 *
 * Process the apply-templates node on the source node, if params are passed
 * they are pushed on the variable stack but not popped, it's left to the
 * caller to handle them back (they may be reused).
 */
void
xsltApplyOneTemplate(xsltTransformContextPtr ctxt, xmlNodePtr node,
                     xmlNodePtr list, xsltTemplatePtr templ,
		     xsltStackElemPtr params)
{
    xmlNodePtr cur = NULL, insert, copy = NULL;
    xmlNodePtr oldInsert;
    xmlNodePtr oldCurrent = NULL;
    xmlNodePtr oldInst = NULL;
    xmlAttrPtr attrs;
    int oldBase;
    long start = 0;

    if ((ctxt == NULL) || (list == NULL))
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
     * stack saves, beware ordering of operations counts
     */
    oldInsert = insert = ctxt->insert;
    oldInst = ctxt->inst;
    oldCurrent = ctxt->node;
    varsPush(ctxt, params);
    oldBase = ctxt->varsBase; /* only needed if templ != NULL */
    if (templ != NULL) {
	ctxt->varsBase = ctxt->varsNr - 1;
        ctxt->node = node;
	if (ctxt->profile) {
	    templ->nbCalls++;
	    start = xsltTimestamp();
	    profPush(ctxt, 0);
	}
	templPush(ctxt, templ);
#ifdef WITH_XSLT_DEBUG_PROCESS
	if (templ->name != NULL)
	    xsltGenericDebug(xsltGenericDebugContext,
			     "applying template '%s'\n",
	                     templ->name);
#endif
    }

    /*
     * Insert all non-XSLT nodes found in the template
     */
    cur = list;
    while (cur != NULL) {
        ctxt->inst = cur;
        /*
         * test, we must have a valid insertion point
         */
        if (insert == NULL) {
#ifdef WITH_XSLT_DEBUG_PROCESS
            xsltGenericDebug(xsltGenericDebugContext,
                             "xsltApplyOneTemplate: insert == NULL !\n");
#endif
	    goto error;
        }

        if (IS_XSLT_ELEM(cur)) {
            /*
             * This is an XSLT node
             */
            xsltStylePreCompPtr info = (xsltStylePreCompPtr) cur->_private;

            if (info == NULL) {
                if (IS_XSLT_NAME(cur, "message")) {
                    xsltMessage(ctxt, node, cur);
                } else {
                    xsltGenericError(xsltGenericDebugContext,
                                "xsltApplyOneTemplate: %s was not compiled\n",
                                     cur->name);
                }
                goto skip_children;
            }

            if (info->func != NULL) {
                ctxt->insert = insert;
                info->func(ctxt, node, cur, info);
                ctxt->insert = oldInsert;
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
        } else if ((cur->type == XML_TEXT_NODE) ||
                   (cur->type == XML_CDATA_SECTION_NODE)) {
            /*
             * This text comes from the stylesheet
             * For stylesheets, the set of whitespace-preserving
             * element names consists of just xsl:text.
             */
#ifdef WITH_XSLT_DEBUG_PROCESS
            if (cur->type == XML_CDATA_SECTION_NODE)
                xsltGenericDebug(xsltGenericDebugContext,
                                 "xsltApplyOneTemplate: copy CDATA text %s\n",
                                 cur->content);
            else if (cur->name == xmlStringTextNoenc)
                xsltGenericDebug(xsltGenericDebugContext,
                             "xsltApplyOneTemplate: copy unescaped text %s\n",
                                 cur->content);
            else
                xsltGenericDebug(xsltGenericDebugContext,
                                 "xsltApplyOneTemplate: copy text %s\n",
                                 cur->content);
#endif
	    xsltCopyText(ctxt, insert, cur);
        } else if ((cur->type == XML_ELEMENT_NODE) &&
                   (cur->ns != NULL) && (cur->_private != NULL)) {
            xsltTransformFunction function;

            /*
             * Flagged as an extension element
             */
            function = (xsltTransformFunction)
                xmlHashLookup2(ctxt->extElements, cur->name,
                               cur->ns->href);
            if (function == NULL) {
                xmlNodePtr child;
                int found = 0;

#ifdef WITH_XSLT_DEBUG_PROCESS
                xsltGenericDebug(xsltGenericDebugContext,
                                "xsltApplyOneTemplate: unknown extension %s\n",
                                 cur->name);
#endif
                /*
                 * Search if there are fallbacks
                 */
                child = cur->children;
                while (child != NULL) {
                    if ((IS_XSLT_ELEM(child)) &&
                        (IS_XSLT_NAME(child, "fallback"))) {
                        found = 1;
                        xsltApplyOneTemplate(ctxt, node, child->children,
				             NULL, NULL);
                    }
                    child = child->next;
                }

                if (!found) {
                    xsltGenericError(xsltGenericErrorContext,
                         "xsltApplyOneTemplate: failed to find extension %s\n",
                                     cur->name);
                }
            } else {
#ifdef WITH_XSLT_DEBUG_PROCESS
                xsltGenericDebug(xsltGenericDebugContext,
                              "xsltApplyOneTemplate: extension construct %s\n",
                                 cur->name);
#endif

                ctxt->insert = insert;
                function(ctxt, node, cur, cur->_private);
                ctxt->insert = oldInsert;
            }
            goto skip_children;
        } else if (cur->type == XML_ELEMENT_NODE) {
#ifdef WITH_XSLT_DEBUG_PROCESS
            xsltGenericDebug(xsltGenericDebugContext,
                             "xsltApplyOneTemplate: copy node %s\n",
                             cur->name);
#endif
            copy = xsltCopyNode(ctxt, cur, insert);
            /*
             * all the attributes are directly inherited
             */
            if (cur->properties != NULL) {
                attrs = xsltAttrListTemplateProcess(ctxt, copy,
                                                    cur->properties);
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
error:
    ctxt->node = oldCurrent;
    ctxt->inst = oldInst;
    ctxt->insert = oldInsert;
    if (params == NULL)
	xsltFreeStackElemList(varsPop(ctxt));
    else {
	xsltStackElemPtr p, tmp = varsPop(ctxt);
	if (tmp != params) {
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
    if (templ != NULL) {
	ctxt->varsBase = oldBase;
	templPop(ctxt);
	if (ctxt->profile) {
	    long spent, child, total, end;
	    
	    end = xsltTimestamp(); 
	    child = profPop(ctxt);
	    total = end - start;
	    spent = total - child;

	    templ->time += spent;
	    if (ctxt->profNr > 0)
		ctxt->profTab[ctxt->profNr - 1] += total;
	}
    }
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
 * @comp:  precomputed information
 *
 * Process an XSLT-1.1 document element
 */
void
xsltDocumentElem(xsltTransformContextPtr ctxt, xmlNodePtr node,
                 xmlNodePtr inst, xsltStylePreCompPtr comp)
{
    xsltStylesheetPtr style = NULL;
    int ret;
    xmlChar *filename = NULL, *prop, *elements;
    xmlChar *element, *end;
    xmlDocPtr res = NULL;
    xmlDocPtr oldOutput;
    xmlNodePtr oldInsert;
    const char *oldOutputFile;
    xsltOutputType oldType;
    xmlChar *URL = NULL;
    const xmlChar *method;
    const xmlChar *doctypePublic;
    const xmlChar *doctypeSystem;
    const xmlChar *version;

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL)
        || (comp == NULL))
        return;

    if (comp->filename == NULL) {

        if (xmlStrEqual(inst->name, (const xmlChar *) "output")) {
#ifdef WITH_XSLT_DEBUG_EXTRA
            xsltGenericDebug(xsltGenericDebugContext,
                             "Found saxon:output extension\n");
#endif
            URL = xsltEvalAttrValueTemplate(ctxt, inst,
                                                 (const xmlChar *) "file",
                                                 XSLT_SAXON_NAMESPACE);
        } else if (xmlStrEqual(inst->name, (const xmlChar *) "write")) {
#ifdef WITH_XSLT_DEBUG_EXTRA
            xsltGenericDebug(xsltGenericDebugContext,
                             "Found xalan:write extension\n");
#endif
            URL = xsltEvalAttrValueTemplate(ctxt, inst,
                                                 (const xmlChar *)
                                                 "select",
                                                 XSLT_XALAN_NAMESPACE);
        } else if (xmlStrEqual(inst->name, (const xmlChar *) "document")) {
            URL = xsltEvalAttrValueTemplate(ctxt, inst,
                                                 (const xmlChar *) "href",
                                                 XSLT_XT_NAMESPACE);
            if (URL == NULL) {
#ifdef WITH_XSLT_DEBUG_EXTRA
                xsltGenericDebug(xsltGenericDebugContext,
                                 "Found xslt11:document construct\n");
#endif
                URL = xsltEvalAttrValueTemplate(ctxt, inst,
                                                     (const xmlChar *)
                                                     "href",
                                                     XSLT_NAMESPACE);
                comp->ver11 = 1;
            } else {
#ifdef WITH_XSLT_DEBUG_EXTRA
                xsltGenericDebug(xsltGenericDebugContext,
                                 "Found xt:document extension\n");
#endif
                comp->ver11 = 0;
            }
        }

    } else {
        URL = xmlStrdup(comp->filename);
    }

    if (URL == NULL) {
	xsltGenericError(xsltGenericErrorContext,
		         "xsltDocumentElem: href/URI-Reference not found\n");
	return;
    }
    filename = xmlBuildURI(URL, (const xmlChar *) ctxt->outputFile);
    if (filename == NULL) {
	xsltGenericError(xsltGenericErrorContext,
		         "xsltDocumentElem: URL computation failed for %s\n",
			 URL);
	xmlFree(URL);
	return;
    }

    oldOutputFile = ctxt->outputFile;
    oldOutput = ctxt->output;
    oldInsert = ctxt->insert;
    oldType = ctxt->type;
    ctxt->outputFile = (const char *) filename;

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
    if (comp->ver11) {
        prop = xsltEvalAttrValueTemplate(ctxt, inst,
                                         (const xmlChar *) "version",
                                         XSLT_NAMESPACE);
        if (prop != NULL) {
            if (style->version != NULL)
                xmlFree(style->version);
            style->version = prop;
        }
        prop = xsltEvalAttrValueTemplate(ctxt, inst,
                                         (const xmlChar *) "encoding",
                                         XSLT_NAMESPACE);
        if (prop != NULL) {
            if (style->encoding != NULL)
                xmlFree(style->encoding);
            style->encoding = prop;
        }
        prop = xsltEvalAttrValueTemplate(ctxt, inst,
                                         (const xmlChar *) "method",
                                         XSLT_NAMESPACE);
        if (prop != NULL) {
	    const xmlChar *URI;

	    if (style->method != NULL)
		xmlFree(style->method);
	    style->method = NULL;
	    if (style->methodURI != NULL)
		xmlFree(style->methodURI);
	    style->methodURI = NULL;

	    URI = xsltGetQNameURI(inst, &prop);
	    if (prop == NULL) {
		style->errors++;
	    } else if (URI == NULL) {
		if ((xmlStrEqual(prop, (const xmlChar *) "xml")) ||
		    (xmlStrEqual(prop, (const xmlChar *) "html")) ||
		    (xmlStrEqual(prop, (const xmlChar *) "text"))) {
		    style->method = prop;
		} else {
		    xsltGenericError(xsltGenericErrorContext,
				     "invalid value for method: %s\n", prop);
		    style->warnings++;
		}
	    } else {
		style->method = prop;
		style->methodURI = xmlStrdup(URI);
	    }
        }
        prop = xsltEvalAttrValueTemplate(ctxt, inst,
                                         (const xmlChar *)
                                         "doctype-system", XSLT_NAMESPACE);
        if (prop != NULL) {
            if (style->doctypeSystem != NULL)
                xmlFree(style->doctypeSystem);
            style->doctypeSystem = prop;
        }
        prop = xsltEvalAttrValueTemplate(ctxt, inst,
                                         (const xmlChar *)
                                         "doctype-public", XSLT_NAMESPACE);
        if (prop != NULL) {
            if (style->doctypePublic != NULL)
                xmlFree(style->doctypePublic);
            style->doctypePublic = prop;
        }
        prop = xsltEvalAttrValueTemplate(ctxt, inst,
                                         (const xmlChar *) "standalone",
                                         XSLT_NAMESPACE);
        if (prop != NULL) {
            if (xmlStrEqual(prop, (const xmlChar *) "yes")) {
                style->standalone = 1;
            } else if (xmlStrEqual(prop, (const xmlChar *) "no")) {
                style->standalone = 0;
            } else {
                xsltGenericError(xsltGenericErrorContext,
                                 "invalid value for standalone: %s\n",
                                 prop);
                style->warnings++;
            }
            xmlFree(prop);
        }

        prop = xsltEvalAttrValueTemplate(ctxt, inst,
                                         (const xmlChar *) "indent",
                                         XSLT_NAMESPACE);
        if (prop != NULL) {
            if (xmlStrEqual(prop, (const xmlChar *) "yes")) {
                style->indent = 1;
            } else if (xmlStrEqual(prop, (const xmlChar *) "no")) {
                style->indent = 0;
            } else {
                xsltGenericError(xsltGenericErrorContext,
                                 "invalid value for indent: %s\n", prop);
                style->warnings++;
            }
            xmlFree(prop);
        }

        prop = xsltEvalAttrValueTemplate(ctxt, inst,
                                         (const xmlChar *)
                                         "omit-xml-declaration",
                                         XSLT_NAMESPACE);
        if (prop != NULL) {
            if (xmlStrEqual(prop, (const xmlChar *) "yes")) {
                style->omitXmlDeclaration = 1;
            } else if (xmlStrEqual(prop, (const xmlChar *) "no")) {
                style->omitXmlDeclaration = 0;
            } else {
                xsltGenericError(xsltGenericErrorContext,
                                 "invalid value for omit-xml-declaration: %s\n",
                                 prop);
                style->warnings++;
            }
            xmlFree(prop);
        }

        elements = xsltEvalAttrValueTemplate(ctxt, inst,
                                             (const xmlChar *)
                                             "cdata-section-elements",
                                             XSLT_NAMESPACE);
        if (elements != NULL) {
            if (style->stripSpaces == NULL)
                style->stripSpaces = xmlHashCreate(10);
            if (style->stripSpaces == NULL)
                return;

            element = elements;
            while (*element != 0) {
                while (IS_BLANK(*element))
                    element++;
                if (*element == 0)
                    break;
                end = element;
                while ((*end != 0) && (!IS_BLANK(*end)))
                    end++;
                element = xmlStrndup(element, end - element);
                if (element) {
#ifdef WITH_XSLT_DEBUG_PARSING
                    xsltGenericDebug(xsltGenericDebugContext,
                                     "add cdata section output element %s\n",
                                     element);
#endif
                    xmlHashAddEntry(style->stripSpaces, element,
                                    (xmlChar *) "cdata");
                    xmlFree(element);
                }
                element = end;
            }
            xmlFree(elements);
        }
    } else {
        xsltParseStylesheetOutput(style, inst);
    }

    /*
     * Create a new document tree and process the element template
     */
    XSLT_GET_IMPORT_PTR(method, style, method)
    XSLT_GET_IMPORT_PTR(doctypePublic, style, doctypePublic)
    XSLT_GET_IMPORT_PTR(doctypeSystem, style, doctypeSystem)
    XSLT_GET_IMPORT_PTR(version, style, version)

    if ((method != NULL) &&
	(!xmlStrEqual(method, (const xmlChar *) "xml"))) {
	if (xmlStrEqual(method, (const xmlChar *) "html")) {
	    ctxt->type = XSLT_OUTPUT_HTML;
	    if (((doctypePublic != NULL) || (doctypeSystem != NULL)))
		res = htmlNewDoc(doctypeSystem, doctypePublic);
	    else {
		if (version == NULL)
		    version = (const xmlChar *) "4.0";
#ifdef XSLT_GENERATE_HTML_DOCTYPE
		xsltGetHTMLIDs(version, &doctypePublic, &doctypeSystem);
#endif
		res = htmlNewDoc(doctypeSystem, doctypePublic);
	    }
	    if (res == NULL)
		goto error;
	} else if (xmlStrEqual(method, (const xmlChar *) "xhtml")) {
	    xsltGenericError(xsltGenericErrorContext,
	     "xsltDocumentElem: unsupported method xhtml, using html\n",
		             style->method);
	    ctxt->type = XSLT_OUTPUT_HTML;
	    res = htmlNewDoc(doctypeSystem, doctypePublic);
	    if (res == NULL)
		goto error;
	} else if (xmlStrEqual(style->method, (const xmlChar *) "text")) {
	    ctxt->type = XSLT_OUTPUT_TEXT;
	    res = xmlNewDoc(style->version);
	    if (res == NULL)
		goto error;
	} else {
	    xsltGenericError(xsltGenericErrorContext,
			     "xsltDocumentElem: unsupported method %s\n",
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
    ctxt->output = res;
    ctxt->insert = (xmlNodePtr) res;
    xsltApplyOneTemplate(ctxt, node, inst->children, NULL, NULL);

    /*
     * Save the result
     */
    ret = xsltSaveResultToFilename((const char *) filename,
                                   res, style, 0);
    if (ret < 0) {
        xsltGenericError(xsltGenericErrorContext,
                         "xsltDocumentElem: unable to save to %s\n",
                         filename);
#ifdef WITH_XSLT_DEBUG_EXTRA
    } else {
        xsltGenericDebug(xsltGenericDebugContext,
                         "Wrote %d bytes to %s\n", ret,, filename);
#endif
    }

  error:
    ctxt->output = oldOutput;
    ctxt->insert = oldInsert;
    ctxt->type = oldType;
    ctxt->outputFile = oldOutputFile;
    if (URL != NULL)
        xmlFree(URL);
    if (filename != NULL)
        xmlFree(filename);
    if (style != NULL)
        xsltFreeStylesheet(style);
    if (res != NULL)
        xmlFreeDoc(res);
}

/************************************************************************
 *									*
 *		Most of the XSLT-1.0 transformations			*
 *									*
 ************************************************************************/

void xsltProcessOneNode(xsltTransformContextPtr ctxt, xmlNodePtr node,
	                xsltStackElemPtr params);

/**
 * xsltSort:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt sort node
 * @comp:  precomputed information
 *
 * function attached to xsl:sort nodes, but this should not be
 * called directly
 */
void
xsltSort(xsltTransformContextPtr ctxt ATTRIBUTE_UNUSED,
	xmlNodePtr node ATTRIBUTE_UNUSED, xmlNodePtr inst ATTRIBUTE_UNUSED,
	xsltStylePreCompPtr comp) {
    if (comp == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsl:sort : compilation failed\n");
	return;
    }
    xsltGenericError(xsltGenericErrorContext,
	 "xsl:sort : improper use this should not be reached\n");
}

/**
 * xsltCopy:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt copy node
 * @comp:  precomputed information
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
	    case XML_TEXT_NODE:
	    case XML_CDATA_SECTION_NODE:
		/*
		 * This text comes from the stylesheet
		 * For stylesheets, the set of whitespace-preserving
		 * element names consists of just xsl:text.
		 */
#ifdef WITH_XSLT_DEBUG_PROCESS
		if (node->type == XML_CDATA_SECTION_NODE)
		    xsltGenericDebug(xsltGenericDebugContext,
			 "xsltCopy: CDATA text %s\n", node->content);
		else
		    xsltGenericDebug(xsltGenericDebugContext,
			 "xsltCopy: text %s\n", node->content);
#endif
		xsltCopyText(ctxt, ctxt->insert, node);
		break;
	    case XML_DOCUMENT_NODE:
	    case XML_HTML_DOCUMENT_NODE:
		break;
	    case XML_ELEMENT_NODE:
#ifdef WITH_XSLT_DEBUG_PROCESS
		xsltGenericDebug(xsltGenericDebugContext,
				 "xsltCopy: node %s\n", node->name);
#endif
		copy = xsltCopyNode(ctxt, node, ctxt->insert);
		ctxt->insert = copy;
		if (comp->use != NULL) {
		    xsltApplyAttributeSet(ctxt, node, inst, comp->use);
		}
		break;
	    case XML_ATTRIBUTE_NODE: {
#ifdef WITH_XSLT_DEBUG_PROCESS
		xsltGenericDebug(xsltGenericDebugContext,
				 "xsltCopy: attribute %s\n", node->name);
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
	    case XML_PI_NODE:
#ifdef WITH_XSLT_DEBUG_PROCESS
		xsltGenericDebug(xsltGenericDebugContext,
				 "xsltCopy: PI %s\n", node->name);
#endif
		copy = xmlNewPI(node->name, node->content);
		xmlAddChild(ctxt->insert, copy);
		break;
	    case XML_COMMENT_NODE:
#ifdef WITH_XSLT_DEBUG_PROCESS
		xsltGenericDebug(xsltGenericDebugContext,
				 "xsltCopy: comment\n");
#endif
		copy = xmlNewComment(node->content);
		xmlAddChild(ctxt->insert, copy);
		break;
	    default:
		break;

	}
    }

    switch (node->type) {
	case XML_DOCUMENT_NODE:
	case XML_HTML_DOCUMENT_NODE:
	case XML_ELEMENT_NODE:
	    xsltApplyOneTemplate(ctxt, ctxt->node, inst->children,
		                 NULL, NULL);
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
 * @comp:  precomputed information
 *
 * Process the xslt text node on the source node
 */
void
xsltText(xsltTransformContextPtr ctxt, xmlNodePtr node ATTRIBUTE_UNUSED,
	    xmlNodePtr inst, xsltStylePreCompPtr comp) {
    if ((inst->children != NULL) && (comp != NULL)) {
	xmlNodePtr text = inst->children;
	xmlNodePtr copy;

	while (text != NULL) {
	    if (((text->type != XML_TEXT_NODE) &&
		 (text->type != XML_CDATA_SECTION_NODE)) ||
		(text->next != NULL)) {
		xsltGenericError(xsltGenericErrorContext,
				 "xsl:text content problem\n");
		break;
	    }
	    copy = xmlNewDocText(ctxt->output, text->content);
	    if ((comp->noescape) || (text->type != XML_CDATA_SECTION_NODE)) {
#ifdef WITH_XSLT_DEBUG_PARSING
		xsltGenericDebug(xsltGenericDebugContext,
		     "Disable escaping: %s\n", text->content);
#endif
		copy->name = xmlStringTextNoenc;
	    }
	    xmlAddChild(ctxt->insert, copy);
	    text = text->next;
	}
    }
}

/**
 * xsltElement:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt element node
 * @comp:  precomputed information
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
    xmlNsPtr ns = NULL, oldns = NULL;
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
		 "xsl:element : name is missing\n");
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
	}
    } else if (comp->ns != NULL) {
	ns = xsltGetSpecialNamespace(ctxt, inst, comp->ns, prefix,
				     ctxt->insert);
    }
    if ((ns == NULL) && (prefix != NULL)) {
	if (!xmlStrncasecmp(prefix, (xmlChar *)"xml", 3)) {
#ifdef WITH_XSLT_DEBUG_PARSING
	    xsltGenericDebug(xsltGenericDebugContext,
		 "xsltElement: xml prefix forbidden\n");
#endif
	    goto error;
	}
	oldns = xmlSearchNs(inst->doc, inst, prefix);
	if (oldns == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		"xsl:element : no namespace bound to prefix %s\n", prefix);
	} else {
	    ns = xsltGetNamespace(ctxt, inst, ns, ctxt->insert);
	}
    }

    copy = xmlNewDocNode(ctxt->output, ns, name, NULL);
    if (copy == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsl:element : creation of %s failed\n", name);
	goto error;
    }
    if ((ns == NULL) && (oldns != NULL)) {
	/* very specific case xsltGetNamespace failed */
        ns = xmlNewNs(copy, oldns->href, oldns->prefix);
	copy->ns = ns;
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
    
    xsltApplyOneTemplate(ctxt, ctxt->node, inst->children, NULL, NULL);

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
 * @comp:  precomputed information
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
	xsltGenericError(xsltGenericErrorContext,
	     "xsl:attribute : compilation failed\n");
	return;
    }

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL) || (comp == NULL))
	return;
    if (!comp->has_name) {
	return;
    }
    if (ctxt->insert->children != NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsl:attribute : node already has children\n");
	return;
    }
    if (comp->name == NULL) {
	prop = xsltEvalAttrValueTemplate(ctxt, inst, (const xmlChar *)"name",
		                         XSLT_NAMESPACE);
	if (prop == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		 "xsl:attribute : name is missing\n");
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
#ifdef WITH_XSLT_DEBUG_PARSING
	xsltGenericDebug(xsltGenericDebugContext,
	     "xsltAttribute: xml prefix forbidden\n");
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
			"xsl:attribute : no namespace bound to prefix %s\n", prefix);
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
 * @comp:  precomputed information
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
#ifdef WITH_XSLT_DEBUG_PROCESS
    if (value == NULL)
	xsltGenericDebug(xsltGenericDebugContext,
	     "xsltComment: empty\n");
    else
	xsltGenericDebug(xsltGenericDebugContext,
	     "xsltComment: content %s\n", value);
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
 * @comp:  precomputed information
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
		 "xsl:processing-instruction : name is missing\n");
	    goto error;
	}
	name = ncname;
    } else {
	name = comp->name;
    }
    /* TODO: check that it's both an an NCName and a PITarget. */


    value = xsltEvalTemplateString(ctxt, node, inst);
    /* TODO: check that there is no ?> sequence */
#ifdef WITH_XSLT_DEBUG_PROCESS
    if (value == NULL)
	xsltGenericDebug(xsltGenericDebugContext,
	     "xsltProcessingInstruction: %s empty\n", ncname);
    else
	xsltGenericDebug(xsltGenericDebugContext,
	     "xsltProcessingInstruction: %s content %s\n", ncname, value);
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
 * @comp:  precomputed information
 *
 * Process the xslt copy-of node on the source node
 */
void
xsltCopyOf(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst, xsltStylePreCompPtr comp) {
    xmlXPathObjectPtr res = NULL;
    xmlNodeSetPtr list = NULL;
    int i;
    int oldProximityPosition, oldContextSize;
    int oldNsNr;
    xmlNsPtr *oldNamespaces;

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL))
	return;
    if ((comp == NULL) || (comp->select == NULL) || (comp->comp == NULL)) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsl:copy-of : compilation failed\n");
	return;
    }

#ifdef WITH_XSLT_DEBUG_PROCESS
    xsltGenericDebug(xsltGenericDebugContext,
	 "xsltCopyOf: select %s\n", comp->select);
#endif

    oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
    oldContextSize = ctxt->xpathCtxt->contextSize;
    oldNsNr = ctxt->xpathCtxt->nsNr;
    oldNamespaces = ctxt->xpathCtxt->namespaces;
    ctxt->xpathCtxt->node = node;
    ctxt->xpathCtxt->namespaces = comp->nsList;
    ctxt->xpathCtxt->nsNr = comp->nsNr;
    res = xmlXPathCompiledEval(comp->comp, ctxt->xpathCtxt);
    ctxt->xpathCtxt->proximityPosition = oldProximityPosition;
    ctxt->xpathCtxt->contextSize = oldContextSize;
    ctxt->xpathCtxt->nsNr = oldNsNr;
    ctxt->xpathCtxt->namespaces = oldNamespaces;
    if (res != NULL) {
	if (res->type == XPATH_NODESET) {
#ifdef WITH_XSLT_DEBUG_PROCESS
	    xsltGenericDebug(xsltGenericDebugContext,
		 "xsltCopyOf: result is a node set\n");
#endif
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
#ifdef WITH_XSLT_DEBUG_PROCESS
	    xsltGenericDebug(xsltGenericDebugContext,
		 "xsltCopyOf: result is a result tree fragment\n");
#endif
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
#ifdef WITH_XSLT_DEBUG_PROCESS
		xsltGenericDebug(xsltGenericDebugContext,
		     "xsltCopyOf: result %s\n", res->stringval);
#endif
		/* append content as text node */
		xsltCopyTextString(ctxt, ctxt->insert, res->stringval);
	    }
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
 * @comp:  precomputed information
 *
 * Process the xslt value-of node on the source node
 */
void
xsltValueOf(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst, xsltStylePreCompPtr comp) {
    xmlXPathObjectPtr res = NULL;
    xmlNodePtr copy = NULL;
    int oldProximityPosition, oldContextSize;
    int oldNsNr;
    xmlNsPtr *oldNamespaces;

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL))
	return;
    if ((comp == NULL) || (comp->select == NULL) || (comp->comp == NULL)) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsl:value-of : compilation failed\n");
	return;
    }

#ifdef WITH_XSLT_DEBUG_PROCESS
    xsltGenericDebug(xsltGenericDebugContext,
	 "xsltValueOf: select %s\n", comp->select);
#endif

    oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
    oldContextSize = ctxt->xpathCtxt->contextSize;
    oldNsNr = ctxt->xpathCtxt->nsNr;
    oldNamespaces = ctxt->xpathCtxt->namespaces;
    ctxt->xpathCtxt->node = node;
    ctxt->xpathCtxt->namespaces = comp->nsList;
    ctxt->xpathCtxt->nsNr = comp->nsNr;
    res = xmlXPathCompiledEval(comp->comp, ctxt->xpathCtxt);
    ctxt->xpathCtxt->proximityPosition = oldProximityPosition;
    ctxt->xpathCtxt->contextSize = oldContextSize;
    ctxt->xpathCtxt->nsNr = oldNsNr;
    ctxt->xpathCtxt->namespaces = oldNamespaces;
    if (res != NULL) {
	if (res->type != XPATH_STRING)
	    res = xmlXPathConvertString(res);
	if (res->type == XPATH_STRING) {
	    /* TODO: integrate with xsltCopyTextString */
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
	    "xsltValueOf: text copy failed\n");
    }
#ifdef WITH_XSLT_DEBUG_PROCESS
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
 * @comp:  precomputed information
 *
 * Process the xslt number node on the source node
 */
void
xsltNumber(xsltTransformContextPtr ctxt, xmlNodePtr node,
	   xmlNodePtr inst, xsltStylePreCompPtr comp)
{
    if (comp == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsl:number : compilation failed\n");
	return;
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
 * @comp:  precomputed information
 *
 * Process the xslt apply-imports node on the source node
 */
void
xsltApplyImports(xsltTransformContextPtr ctxt, xmlNodePtr node,
	         xmlNodePtr inst ATTRIBUTE_UNUSED,
		 xsltStylePreCompPtr comp ATTRIBUTE_UNUSED) {
    xsltTemplatePtr template;

    if ((ctxt->templ == NULL) || (ctxt->templ->style == NULL)) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsl:apply-imports : internal error no current template\n");
	return;
    }
    template = xsltGetTemplate(ctxt, node, ctxt->templ->style);
    if (template != NULL) {
	xsltApplyOneTemplate(ctxt, node, template->content, template, NULL);
    }
}

/**
 * xsltCallTemplate:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt call-template node
 * @comp:  precomputed information
 *
 * Process the xslt call-template node on the source node
 */
void
xsltCallTemplate(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst, xsltStylePreCompPtr comp) {
    xmlNodePtr cur = NULL;
    xsltStackElemPtr params = NULL, param;

    if (ctxt->insert == NULL)
	return;
    if (comp == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsl:call-template : compilation failed\n");
	return;
    }

    /*
     * The template must have been precomputed
     */
    if (comp->templ == NULL) {
	comp->templ = xsltFindTemplate(ctxt, comp->name, comp->ns);
	if (comp->templ == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		 "xsl:call-template : template %s not found\n", comp->name);
	    return;
	}
    }

#ifdef WITH_XSLT_DEBUG_PROCESS
    if ((comp != NULL) && (comp->name != NULL))
	xsltGenericDebug(xsltGenericDebugContext,
			 "call-template: name %s\n", comp->name);
#endif

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
		     "xsl:call-template: misplaced xsl:%s\n", cur->name);
	    }
	} else {
	    xsltGenericError(xsltGenericDebugContext,
		 "xsl:call-template: misplaced %s element\n", cur->name);
	}
	cur = cur->next;
    }
    /*
     * Create a new frame using the params first
     */
    xsltApplyOneTemplate(ctxt, node, comp->templ->content, comp->templ, params);
    if (params != NULL)
	xsltFreeStackElemList(params);

#ifdef WITH_XSLT_DEBUG_PROCESS
    if ((comp != NULL) && (comp->name != NULL))
	xsltGenericDebug(xsltGenericDebugContext,
			 "call-template returned: name %s\n", comp->name);
#endif
}

/**
 * xsltApplyTemplates:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the apply-templates node
 * @comp:  precomputed information
 *
 * Process the apply-templates node on the source node
 */
void
xsltApplyTemplates(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst, xsltStylePreCompPtr comp) {
    xmlNodePtr cur, delete = NULL, oldNode;
    xmlXPathObjectPtr res = NULL;
    xmlNodeSetPtr list = NULL, oldList;
    int i, oldProximityPosition, oldContextSize;
    const xmlChar *oldMode, *oldModeURI;
    xsltStackElemPtr params = NULL, param;
    int nbsorts = 0;
    xmlNodePtr sorts[XSLT_MAX_SORT];
    xmlDocPtr oldXDocPtr;
    xsltDocumentPtr oldCDocPtr;
    int oldNsNr;
    xmlNsPtr *oldNamespaces;

    if (comp == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsl:apply-templates : compilation failed\n");
	return;
    }
    if ((ctxt == NULL) || (node == NULL) || (inst == NULL) || (comp == NULL))
	return;

#ifdef WITH_XSLT_DEBUG_PROCESS
    if ((node != NULL) && (node->name != NULL))
	xsltGenericDebug(xsltGenericDebugContext,
	     "xsltApplyTemplates: node: %s\n", node->name);
#endif

    /*
     * Get mode if any
     */
    oldNode = ctxt->node;
    oldMode = ctxt->mode;
    oldModeURI = ctxt->modeURI;
    ctxt->mode = comp->mode;
    ctxt->modeURI = comp->modeURI;

    /*
     * The xpath context size and proximity position, as
     * well as the xpath and context documents, may be changed
     * so we save their initial state and will restore on exit
     */
    oldXDocPtr = ctxt->xpathCtxt->doc;
    oldCDocPtr = ctxt->document;
    oldContextSize = ctxt->xpathCtxt->contextSize;
    oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
    oldNsNr = ctxt->xpathCtxt->nsNr;
    oldNamespaces = ctxt->xpathCtxt->namespaces;
    oldList = ctxt->nodeList;

    if (comp->select != NULL) {
	if (comp->comp == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		 "xsl:apply-templates : compilation failed\n");
	    goto error;
	}
#ifdef WITH_XSLT_DEBUG_PROCESS
	xsltGenericDebug(xsltGenericDebugContext,
	     "xsltApplyTemplates: select %s\n", comp->select);
#endif

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
#ifdef WITH_XSLT_DEBUG_PROCESS
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
		case XML_PI_NODE:
		case XML_COMMENT_NODE:
		    xmlXPathNodeSetAdd(list, cur);
		    break;
		default:
#ifdef WITH_XSLT_DEBUG_PROCESS
		    xsltGenericDebug(xsltGenericDebugContext,
		     "xsltApplyTemplates: skipping cur type %d\n",
				     cur->type);
#endif
		    delete = cur;
	    }
	    cur = cur->next;
	    if (delete != NULL) {
#ifdef WITH_XSLT_DEBUG_PROCESS
		xsltGenericDebug(xsltGenericDebugContext,
		     "xsltApplyTemplates: removing ignorable blank cur\n");
#endif
		xmlUnlinkNode(delete);
		xmlFreeNode(delete);
		delete = NULL;
	    }
	}
    }

#ifdef WITH_XSLT_DEBUG_PROCESS
    if (list != NULL)
    xsltGenericDebug(xsltGenericDebugContext,
	"xsltApplyTemplates: list of %d nodes\n", list->nodeNr);
#endif

    ctxt->nodeList = list;
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
		if (nbsorts >= XSLT_MAX_SORT) {
		    xsltGenericError(xsltGenericDebugContext,
			"xsl:apply-template: %s too many sort\n", node->name);
		} else {
		    sorts[nbsorts++] = cur;
		}
	    } else {
		xsltGenericError(xsltGenericDebugContext,
		    "xsl:apply-template: misplaced xsl:%s\n", cur->name);
	    }
        } else {
            xsltGenericError(xsltGenericDebugContext,
                 "xsl:apply-template: misplaced %s element\n", cur->name);
        }
        cur = cur->next;
    }

    if (nbsorts > 0) {
	xsltDoSortFunction(ctxt, sorts, nbsorts);
    }

    for (i = 0;i < list->nodeNr;i++) {
	ctxt->node = list->nodeTab[i];
	ctxt->xpathCtxt->proximityPosition = i + 1;
	/* For a 'select' nodeset, need to check if document has changed */
	if ( (list->nodeTab[i]->doc!=NULL) &&
	     (list->nodeTab[i]->doc->doc!=NULL) &&
	     (list->nodeTab[i]->doc->doc)!=ctxt->xpathCtxt->doc) {	  
	    /* The nodeset is from another document, so must change */
	    ctxt->xpathCtxt->doc=list->nodeTab[i]->doc->doc;
	    if ((ctxt->document =
		  xsltFindDocument(ctxt,list->nodeTab[i]->doc->doc))==NULL) { 
    		xsltGenericError(xsltGenericErrorContext,
	 		"xsl:apply-templates : can't find doc\n");
    		goto error;
	    }
	    ctxt->xpathCtxt->node = list->nodeTab[i];
#ifdef WITH_XSLT_DEBUG_PROCESS
	xsltGenericDebug(xsltGenericDebugContext,
	     "xsltApplyTemplates: Changing document - context doc %s, xpathdoc %s\n",
	     ctxt->document->doc->URL, ctxt->xpathCtxt->doc->URL);
#endif
	}
	xsltProcessOneNode(ctxt, list->nodeTab[i], params);
    }
    if (params != NULL)
	xsltFreeStackElemList(params);	/* free the parameter list */
error:

    ctxt->nodeList = oldList;
    ctxt->xpathCtxt->contextSize = oldContextSize;
    ctxt->xpathCtxt->proximityPosition = oldProximityPosition;
    ctxt->xpathCtxt->doc = oldXDocPtr;
    ctxt->document = oldCDocPtr;
    ctxt->xpathCtxt->nsNr = oldNsNr;
    ctxt->xpathCtxt->namespaces = oldNamespaces;

    ctxt->node = oldNode;
    ctxt->mode = oldMode;
    ctxt->modeURI = oldModeURI;
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
 * @comp:  precomputed information
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
    int oldNsNr;
    xmlNsPtr *oldNamespaces;

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL))
	return;

    /* 
     * Check the when's
     */
    replacement = inst->children;
    if (replacement == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsl:choose: empty content not allowed\n");
	goto error;
    }
    if ((!IS_XSLT_ELEM(replacement)) ||
	(!IS_XSLT_NAME(replacement, "when"))) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsl:choose: xsl:when expected first\n");
	goto error;
    }
    while (IS_XSLT_ELEM(replacement) && (IS_XSLT_NAME(replacement, "when"))) {
	xsltStylePreCompPtr wcomp = replacement->_private;

	if ((wcomp == NULL) || (wcomp->test == NULL) || (wcomp->comp == NULL)) {
	    xsltGenericError(xsltGenericErrorContext,
		 "xsl:choose: compilation failed !\n");
	    goto error;
	}
	when = replacement;
#ifdef WITH_XSLT_DEBUG_PROCESS
	xsltGenericDebug(xsltGenericDebugContext,
	     "xsltChoose: test %s\n", wcomp->test);
#endif

	oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
	oldContextSize = ctxt->xpathCtxt->contextSize;
	oldNsNr = ctxt->xpathCtxt->nsNr;
	oldNamespaces = ctxt->xpathCtxt->namespaces;
  	ctxt->xpathCtxt->node = node;
	ctxt->xpathCtxt->namespaces = comp->nsList;
	ctxt->xpathCtxt->nsNr = comp->nsNr;
  	res = xmlXPathCompiledEval(wcomp->comp, ctxt->xpathCtxt);
	ctxt->xpathCtxt->proximityPosition = oldProximityPosition;
	ctxt->xpathCtxt->contextSize = oldContextSize;
	ctxt->xpathCtxt->nsNr = oldNsNr;
	ctxt->xpathCtxt->namespaces = oldNamespaces;
	if (res != NULL) {
	    if (res->type != XPATH_BOOLEAN)
		res = xmlXPathConvertBoolean(res);
	    if (res->type == XPATH_BOOLEAN)
		doit = res->boolval;
	    else {
#ifdef WITH_XSLT_DEBUG_PROCESS
		xsltGenericDebug(xsltGenericDebugContext,
		    "xsltChoose: test didn't evaluate to a boolean\n");
#endif
		goto error;
	    }
	}

#ifdef WITH_XSLT_DEBUG_PROCESS
	xsltGenericDebug(xsltGenericDebugContext,
	    "xsltChoose: test evaluate to %d\n", doit);
#endif
	if (doit) {
	    xsltApplyOneTemplate(ctxt, ctxt->node, when->children,
		                 NULL, NULL);
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
#ifdef WITH_XSLT_DEBUG_PROCESS
	xsltGenericDebug(xsltGenericDebugContext,
			 "evaluating xsl:otherwise\n");
#endif
	xsltApplyOneTemplate(ctxt, ctxt->node, replacement->children,
		             NULL, NULL);
	replacement = replacement->next;
    }
    if (replacement != NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsl:choose: unexpected content %s\n", replacement->name);
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
 * @comp:  precomputed information
 *
 * Process the xslt if node on the source node
 */
void
xsltIf(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst, xsltStylePreCompPtr comp) {
    xmlXPathObjectPtr res = NULL;
    int doit = 1;
    int oldContextSize, oldProximityPosition;
    int oldNsNr;
    xmlNsPtr *oldNamespaces;

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL))
	return;
    if ((comp == NULL) || (comp->test == NULL) || (comp->comp == NULL)) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsl:if : compilation failed\n");
	return;
    }

#ifdef WITH_XSLT_DEBUG_PROCESS
    xsltGenericDebug(xsltGenericDebugContext,
	 "xsltIf: test %s\n", comp->test);
#endif

    oldContextSize = ctxt->xpathCtxt->contextSize;
    oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
    oldNsNr = ctxt->xpathCtxt->nsNr;
    oldNamespaces = ctxt->xpathCtxt->namespaces;
    ctxt->xpathCtxt->node = node;
    ctxt->xpathCtxt->namespaces = comp->nsList;
    ctxt->xpathCtxt->nsNr = comp->nsNr;
    res = xmlXPathCompiledEval(comp->comp, ctxt->xpathCtxt);
    ctxt->xpathCtxt->contextSize = oldContextSize;
    ctxt->xpathCtxt->proximityPosition = oldProximityPosition;
    ctxt->xpathCtxt->nsNr = oldNsNr;
    ctxt->xpathCtxt->namespaces = oldNamespaces;
    if (res != NULL) {
	if (res->type != XPATH_BOOLEAN)
	    res = xmlXPathConvertBoolean(res);
	if (res->type == XPATH_BOOLEAN)
	    doit = res->boolval;
	else {
#ifdef WITH_XSLT_DEBUG_PROCESS
	    xsltGenericDebug(xsltGenericDebugContext,
		"xsltIf: test didn't evaluate to a boolean\n");
#endif
	    goto error;
	}
    }

#ifdef WITH_XSLT_DEBUG_PROCESS
    xsltGenericDebug(xsltGenericDebugContext,
	"xsltIf: test evaluate to %d\n", doit);
#endif
    if (doit) {
	xsltApplyOneTemplate(ctxt, node, inst->children, NULL, NULL);
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
 * @comp:  precomputed information
 *
 * Process the xslt for-each node on the source node
 */
void
xsltForEach(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst, xsltStylePreCompPtr comp) {
    xmlXPathObjectPtr res = NULL;
    xmlNodePtr replacement;
    xmlNodeSetPtr list = NULL, oldList;
    int i, oldProximityPosition, oldContextSize;
    xmlNodePtr oldNode = ctxt->node;
    int nbsorts = 0;
    xmlNodePtr sorts[XSLT_MAX_SORT];
    xmlDocPtr oldXDocPtr;
    xsltDocumentPtr oldCDocPtr;
    int oldNsNr;
    xmlNsPtr *oldNamespaces;

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL))
	return;
    if ((comp == NULL) || (comp->select == NULL) || (comp->comp == NULL)) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsl:for-each : compilation failed\n");
	return;
    }

#ifdef WITH_XSLT_DEBUG_PROCESS
    xsltGenericDebug(xsltGenericDebugContext,
	 "xsltForEach: select %s\n", comp->select);
#endif

    oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
    oldContextSize = ctxt->xpathCtxt->contextSize;
    oldNsNr = ctxt->xpathCtxt->nsNr;
    oldNamespaces = ctxt->xpathCtxt->namespaces;
    ctxt->xpathCtxt->node = node;
    ctxt->xpathCtxt->namespaces = comp->nsList;
    ctxt->xpathCtxt->nsNr = comp->nsNr;
    oldCDocPtr = ctxt->document;
    oldXDocPtr = ctxt->xpathCtxt->doc;
    res = xmlXPathCompiledEval(comp->comp, ctxt->xpathCtxt);
    ctxt->xpathCtxt->contextSize = oldContextSize;
    ctxt->xpathCtxt->proximityPosition = oldProximityPosition;
    ctxt->xpathCtxt->nsNr = oldNsNr;
    ctxt->xpathCtxt->namespaces = oldNamespaces;
    if (res != NULL) {
	if (res->type == XPATH_NODESET)
	    list = res->nodesetval;
    }
    if (list == NULL) {
#ifdef WITH_XSLT_DEBUG_PROCESS
	xsltGenericDebug(xsltGenericDebugContext,
	    "xsltForEach: select didn't evaluate to a node list\n");
#endif
	goto error;
    }

#ifdef WITH_XSLT_DEBUG_PROCESS
    xsltGenericDebug(xsltGenericDebugContext,
	"xsltForEach: select evaluates to %d nodes\n", list->nodeNr);
#endif

    oldList = ctxt->nodeList;
    ctxt->nodeList = list;
    oldContextSize = ctxt->xpathCtxt->contextSize;
    oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
    ctxt->xpathCtxt->contextSize = list->nodeNr;

    /* 
     * handle and skip the xsl:sort
     */
    replacement = inst->children;
    while (IS_XSLT_ELEM(replacement) && (IS_XSLT_NAME(replacement, "sort"))) {
	if (nbsorts >= XSLT_MAX_SORT) {
	    xsltGenericError(xsltGenericDebugContext,
		"xsl:for-each: too many sorts\n");
	} else {
	    sorts[nbsorts++] = replacement;
	}
	replacement = replacement->next;
    }

    if (nbsorts > 0) {
	xsltDoSortFunction(ctxt, sorts, nbsorts);
    }


    for (i = 0;i < list->nodeNr;i++) {
	ctxt->node = list->nodeTab[i];
	ctxt->xpathCtxt->proximityPosition = i + 1;
	/* For a 'select' nodeset, need to check if document has changed */
	if ( (list->nodeTab[i]->doc!=NULL) &&
	     (list->nodeTab[i]->doc->doc!=NULL) &&
	     (list->nodeTab[i]->doc->doc)!=ctxt->xpathCtxt->doc) {	  
	    /* The nodeset is from another document, so must change */
	    ctxt->xpathCtxt->doc=list->nodeTab[i]->doc->doc;
	    if ((ctxt->document =
		  xsltFindDocument(ctxt,list->nodeTab[i]->doc->doc))==NULL) { 
    		xsltGenericError(xsltGenericErrorContext,
	 		"xsl:for-each : can't find doc\n");
    		goto error;
	    }
	    ctxt->xpathCtxt->node = list->nodeTab[i];
#ifdef WITH_XSLT_DEBUG_PROCESS
	xsltGenericDebug(xsltGenericDebugContext,
	     "xsltForEach: Changing document - context doc %s, xpathdoc %s\n",
	     ctxt->document->doc->URL, ctxt->xpathCtxt->doc->URL);
#endif
	}
	xsltApplyOneTemplate(ctxt, list->nodeTab[i], replacement, NULL, NULL);
    }
    ctxt->document = oldCDocPtr;
    ctxt->nodeList = oldList;
    ctxt->node = oldNode;
    ctxt->xpathCtxt->doc = oldXDocPtr;
    ctxt->xpathCtxt->contextSize = oldContextSize;
    ctxt->xpathCtxt->proximityPosition = oldProximityPosition;
    ctxt->xpathCtxt->nsNr = oldNsNr;
    ctxt->xpathCtxt->namespaces = oldNamespaces;

error:
    if (res != NULL)
	xmlXPathFreeObject(res);
}

/************************************************************************
 *									*
 *			Generic interface				*
 *									*
 ************************************************************************/

#ifdef XSLT_GENERATE_HTML_DOCTYPE
typedef struct xsltHTMLVersion {
    const char *version;
    const char *public;
    const char *system;
} xsltHTMLVersion;

static xsltHTMLVersion xsltHTMLVersions[] = {
    { "4.01frame", "-//W3C//DTD HTML 4.01 Frameset//EN",
      "http://www.w3.org/TR/1999/REC-html401-19991224/frameset.dtd"},
    { "4.01strict", "-//W3C//DTD HTML 4.01//EN",
      "http://www.w3.org/TR/1999/REC-html401-19991224/strict.dtd"},
    { "4.01trans", "-//W3C//DTD HTML 4.01 Transitional//EN",
      "http://www.w3.org/TR/1999/REC-html401-19991224/loose.dtd"},
    { "4.01", "-//W3C//DTD HTML 4.01 Transitional//EN",
      "http://www.w3.org/TR/1999/REC-html401-19991224/loose.dtd"},
    { "4.0strict", "-//W3C//DTD HTML 4.01//EN",
      "http://www.w3.org/TR/html4/strict.dtd"},
    { "4.0trans", "-//W3C//DTD HTML 4.01 Transitional//EN",
      "http://www.w3.org/TR/html4/loose.dtd"},
    { "4.0frame", "-//W3C//DTD HTML 4.01 Frameset//EN",
      "http://www.w3.org/TR/html4/frameset.dtd"},
    { "4.0", "-//W3C//DTD HTML 4.01 Transitional//EN",
      "http://www.w3.org/TR/html4/loose.dtd"},
    { "3.2", "-//W3C//DTD HTML 3.2//EN", NULL }
};

/**
 * xsltGetHTMLIDs:
 * @version:  the version string
 * @public:  used to return the public ID
 * @system:  used to return the system ID
 *
 * Returns -1 if not found, 0 otherwise and the system and public
 *         Identifier for this given verion of HTML
 */
static int
xsltGetHTMLIDs(const xmlChar *version, const xmlChar **public,
	            const xmlChar **system) {
    unsigned int i;
    if (version == NULL)
	return(-1);
    for (i = 0;i < (sizeof(xsltHTMLVersions)/sizeof(xsltHTMLVersions[1]));
	 i++) {
	if (!xmlStrcasecmp(version,
		           (const xmlChar *) xsltHTMLVersions[i].version)) {
	    if (public != NULL)
		*public = (const xmlChar *) xsltHTMLVersions[i].public;
	    if (system != NULL)
		*system = (const xmlChar *) xsltHTMLVersions[i].system;
	    return(0);
	}
    }
    return(-1);
}
#endif

/**
 * xsltApplyStylesheetInternal:
 * @style:  a parsed XSLT stylesheet
 * @doc:  a parsed XML document
 * @params:  a NULL terminated arry of parameters names/values tuples
 * @output:  the targetted output
 * @profile:  profile FILE * output or NULL
 *
 * Apply the stylesheet to the document
 * NOTE: This may lead to a non-wellformed output XML wise !
 *
 * Returns the result document or NULL in case of error
 */
static xmlDocPtr
xsltApplyStylesheetInternal(xsltStylesheetPtr style, xmlDocPtr doc,
                            const char **params, const char *output,
                            FILE * profile)
{
    xmlDocPtr res = NULL;
    xsltTransformContextPtr ctxt = NULL;
    xmlNodePtr root;
    const xmlChar *method;
    const xmlChar *doctypePublic;
    const xmlChar *doctypeSystem;
    const xmlChar *version;
    xsltStackElemPtr variables;
    xsltStackElemPtr vptr;

    if ((style == NULL) || (doc == NULL))
        return (NULL);
    ctxt = xsltNewTransformContext(style, doc);
    if (ctxt == NULL)
        return (NULL);

    if (profile != NULL)
        ctxt->profile = 1;

    xsltRegisterExtras(ctxt);
    if (output != NULL)
        ctxt->outputFile = output;
    else
        ctxt->outputFile = NULL;

    XSLT_GET_IMPORT_PTR(method, style, method)
        XSLT_GET_IMPORT_PTR(doctypePublic, style, doctypePublic)
        XSLT_GET_IMPORT_PTR(doctypeSystem, style, doctypeSystem)
        XSLT_GET_IMPORT_PTR(version, style, version)

        if ((method != NULL) &&
            (!xmlStrEqual(method, (const xmlChar *) "xml"))) {
        if (xmlStrEqual(method, (const xmlChar *) "html")) {
            ctxt->type = XSLT_OUTPUT_HTML;
            if (((doctypePublic != NULL) || (doctypeSystem != NULL)))
                res = htmlNewDoc(doctypeSystem, doctypePublic);
            else {
                if (version == NULL)
                    version = (const xmlChar *) "4.0";
#ifdef XSLT_GENERATE_HTML_DOCTYPE
                xsltGetHTMLIDs(version, &doctypePublic, &doctypeSystem);
#endif
                res = htmlNewDoc(doctypeSystem, doctypePublic);
            }
            if (res == NULL)
                goto error;
        } else if (xmlStrEqual(method, (const xmlChar *) "xhtml")) {
            xsltGenericError(xsltGenericErrorContext,
     "xsltApplyStylesheetInternal: unsupported method xhtml, using html\n",
			 style->method);
            ctxt->type = XSLT_OUTPUT_HTML;
            res = htmlNewDoc(doctypeSystem, doctypePublic);
            if (res == NULL)
                goto error;
        } else if (xmlStrEqual(style->method, (const xmlChar *) "text")) {
            ctxt->type = XSLT_OUTPUT_TEXT;
            res = xmlNewDoc(style->version);
            if (res == NULL)
                goto error;
        } else {
            xsltGenericError(xsltGenericErrorContext,
		     "xsltApplyStylesheetInternal: unsupported method %s\n",
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
    variables = style->variables;

    /*
     * Start the evaluation, evaluate the params, the stylesheets globals
     * and start by processing the top node.
     */
    ctxt->output = res;
    ctxt->insert = (xmlNodePtr) res;
    ctxt->globalVars = xmlHashCreate(20);
    if (params != NULL)
        xsltEvalUserParams(ctxt, params);
    xsltInitCtxtExts(ctxt);
    xsltEvalGlobalVariables(ctxt);
    ctxt->node = (xmlNodePtr) doc;
    varsPush(ctxt, NULL);
    ctxt->varsBase = ctxt->varsNr - 1;
    xsltProcessOneNode(ctxt, ctxt->node, NULL);
    xsltFreeStackElemList(varsPop(ctxt));
    xsltShutdownCtxtExts(ctxt);

    xsltCleanupTemplates(style); /* TODO: <- style should be read only */

    /*
     * Now cleanup our variables so stylesheet can be re-used
     *
     * TODO: this is not needed anymore global variables are copied
     *       and not evaluated directly anymore, keep this as a check
     */
    if (style->variables != variables) {
        vptr = style->variables;
        while (vptr->next != variables)
            vptr = vptr->next;
        vptr->next = NULL;
        xsltFreeStackElemList(style->variables);
        style->variables = variables;
    }
    vptr = style->variables;
    while (vptr != NULL) {
        if (vptr->computed) {
            if (vptr->value != NULL) {
                xmlXPathFreeObject(vptr->value);
                vptr->value = NULL;
                vptr->computed = 0;
            }
        }
        vptr = vptr->next;
    }


    /*
     * Do some post processing work depending on the generated output
     */
    root = xmlDocGetRootElement(res);
    if (root != NULL) {
        /*
         * Apply the default selection of the method
         */
        if ((method == NULL) &&
            (root->ns == NULL) &&
            (!xmlStrcasecmp(root->name, (const xmlChar *) "html"))) {
            xmlNodePtr tmp;

            tmp = res->children;
            while ((tmp != NULL) && (tmp != root)) {
                if (tmp->type == XML_ELEMENT_NODE)
                    break;
                if ((tmp->type == XML_TEXT_NODE) && (!xmlIsBlankNode(tmp)))
                    break;
            }
            if (tmp == root) {
                ctxt->type = XSLT_OUTPUT_HTML;
                res->type = XML_HTML_DOCUMENT_NODE;
                if (((doctypePublic != NULL) || (doctypeSystem != NULL)))
                    res->intSubset = xmlCreateIntSubset(res, root->name,
                                                        doctypePublic,
                                                        doctypeSystem);
#ifdef XSLT_GENERATE_HTML_DOCTYPE
                else {
                    if (version == NULL)
                        version = (const xmlChar *) "4.0";
                    xsltGetHTMLIDs(version, &doctypePublic,
                                   &doctypeSystem);
                    if (((doctypePublic != NULL)
                         || (doctypeSystem != NULL)))
                        res->intSubset =
                            xmlCreateIntSubset(res, root->name,
                                               doctypePublic,
                                               doctypeSystem);
                }
#endif
            }

        }
        if (ctxt->type == XSLT_OUTPUT_XML) {
            XSLT_GET_IMPORT_PTR(doctypePublic, style, doctypePublic)
                XSLT_GET_IMPORT_PTR(doctypeSystem, style, doctypeSystem)
                if (((doctypePublic != NULL) || (doctypeSystem != NULL)))
                res->intSubset = xmlCreateIntSubset(res, root->name,
                                                    doctypePublic,
                                                    doctypeSystem);
        }
    }
    xmlXPathFreeNodeSet(ctxt->nodeList);
    if (profile != NULL) {
        xsltSaveProfiling(ctxt, profile);
    }
    xsltFreeTransformContext(ctxt);
    return (res);

error:
    if (res != NULL)
        xmlFreeDoc(res);
    if (ctxt != NULL)
        xsltFreeTransformContext(ctxt);
    return (NULL);
}

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
                    const char **params)
{
    return (xsltApplyStylesheetInternal(style, doc, params, NULL, NULL));
}

/**
 * xsltProfileStylesheet:
 * @style:  a parsed XSLT stylesheet
 * @doc:  a parsed XML document
 * @params:  a NULL terminated arry of parameters names/values tuples
 * @output:  a FILE * for the profiling output
 *
 * Apply the stylesheet to the document and dump the profiling to
 * the given output.
 *
 * Returns the result document or NULL in case of error
 */
xmlDocPtr
xsltProfileStylesheet(xsltStylesheetPtr style, xmlDocPtr doc,
                      const char **params, FILE * output)
{
    xmlDocPtr res;

    res = xsltApplyStylesheetInternal(style, doc, params, NULL, output);
    return (res);
}

/**
 * xsltRunStylesheet:
 * @style:  a parsed XSLT stylesheet
 * @doc:  a parsed XML document
 * @params:  a NULL terminated arry of parameters names/values tuples
 * @output:  the URL/filename ot the generated resource if available
 * @SAX:  a SAX handler for progressive callback output (not implemented yet)
 * @IObuf:  an output buffer for progressive output (not implemented yet)
 *
 * Apply the stylesheet to the document and generate the output according
 * to @output @SAX and @IObuf. It's an error to specify both @SAX and @IObuf.
 *
 * NOTE: This may lead to a non-wellformed output XML wise !
 * NOTE: This may also result in multiple files being generated
 * NOTE: using IObuf, the result encoding used will be the one used for
 *       creating the output buffer, use the following macro to read it
 *       from the stylesheet
 *       XSLT_GET_IMPORT_PTR(encoding, style, encoding)
 * NOTE: using SAX, any encoding specified in the stylesheet will be lost
 *       since the interface uses only UTF8
 *
 * Returns the number of by written to the main resource or -1 in case of
 *         error.
 */
int
xsltRunStylesheet(xsltStylesheetPtr style, xmlDocPtr doc,
                  const char **params, const char *output,
                  xmlSAXHandlerPtr SAX, xmlOutputBufferPtr IObuf)
{
    xmlDocPtr tmp;
    int ret;

    if ((output == NULL) && (SAX == NULL) && (IObuf == NULL))
        return (-1);
    if ((SAX != NULL) && (IObuf != NULL))
        return (-1);

    /* unsupported yet */
    if (SAX != NULL) {
        TODO                    /* xsltRunStylesheet xmlSAXHandlerPtr SAX */
            return (-1);
    }

    tmp = xsltApplyStylesheetInternal(style, doc, params, output, NULL);
    if (tmp == NULL) {
        xsltGenericError(xsltGenericErrorContext,
                         "xsltRunStylesheet : run failed\n");
        return (-1);
    }
    if (IObuf != NULL) {
        /* TODO: incomplete, IObuf output not progressive */
        ret = xsltSaveResultTo(IObuf, tmp, style);
    } else {
        ret = xsltSaveResultToFilename(output, tmp, style, 0);
    }
    xmlFreeDoc(tmp);
    return (ret);
}

/**
 * xsltRegisterAllElements:
 * @ctxt:  the XPath context
 *
 * Registers all default XSLT elements in this context
 */
void
xsltRegisterAllElement(xsltTransformContextPtr ctxt)
{
    xsltRegisterExtElement(ctxt, (const xmlChar *) "apply-templates",
                           XSLT_NAMESPACE, xsltApplyTemplates);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "apply-imports",
                           XSLT_NAMESPACE, xsltApplyImports);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "call-template",
                           XSLT_NAMESPACE, xsltCallTemplate);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "element",
                           XSLT_NAMESPACE, xsltElement);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "attribute",
                           XSLT_NAMESPACE, xsltAttribute);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "text",
                           XSLT_NAMESPACE, xsltText);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "processing-instruction",
                           XSLT_NAMESPACE, xsltProcessingInstruction);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "comment",
                           XSLT_NAMESPACE, xsltComment);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "copy",
                           XSLT_NAMESPACE, xsltCopy);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "value-of",
                           XSLT_NAMESPACE, xsltValueOf);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "number",
                           XSLT_NAMESPACE, xsltNumber);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "for-each",
                           XSLT_NAMESPACE, xsltForEach);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "if",
                           XSLT_NAMESPACE, xsltIf);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "choose",
                           XSLT_NAMESPACE, xsltChoose);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "sort",
                           XSLT_NAMESPACE, xsltSort);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "copy-of",
                           XSLT_NAMESPACE, xsltCopyOf);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "message",
                           XSLT_NAMESPACE, xsltMessage);

    /*
     * Those don't have callable entry points but are registered anyway
     */
    xsltRegisterExtElement(ctxt, (const xmlChar *) "variable",
                           XSLT_NAMESPACE, xsltDebug);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "param",
                           XSLT_NAMESPACE, xsltDebug);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "with-param",
                           XSLT_NAMESPACE, xsltDebug);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "decimal-format",
                           XSLT_NAMESPACE, xsltDebug);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "when",
                           XSLT_NAMESPACE, xsltDebug);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "otherwise",
                           XSLT_NAMESPACE, xsltDebug);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "fallback",
                           XSLT_NAMESPACE, xsltDebug);

}
