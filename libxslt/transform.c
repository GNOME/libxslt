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
#include <libxml/xpathInternals.h>
#include <libxml/HTMLtree.h>
#include "xslt.h"
#include "xsltInternals.h"
#include "pattern.h"
#include "transform.h"

/* #define DEBUG_PROCESS */

/*
 * To cleanup
 */
xmlChar *xmlSplitQName2(const xmlChar *name, xmlChar **prefix);

/*
 * There is no XSLT specific error reporting module yet
 */
#define xsltGenericError xmlGenericError
#define xsltGenericErrorContext xmlGenericErrorContext

/*
 * Useful macros
 */

#define TODO 								\
    xsltGenericError(xsltGenericErrorContext,				\
	    "Unimplemented block at %s:%d\n",				\
            __FILE__, __LINE__);

#define STRANGE 							\
    xsltGenericError(xsltGenericErrorContext,				\
	    "Internal error at %s:%d\n",				\
            __FILE__, __LINE__);

#define IS_XSLT_ELEM(n)							\
    ((n)->ns != NULL) && (xmlStrEqual((n)->ns->href, XSLT_NAMESPACE))

#define IS_XSLT_NAME(n, val)						\
    (xmlStrEqual((n)->name, (const xmlChar *) (val)))

#define IS_BLANK_NODE(n)						\
    (((n)->type == XML_TEXT_NODE) && (xsltIsBlank((n)->content)))

/*
 * Types are private:
 */

typedef enum xsltOutputType {
    XSLT_OUTPUT_XML = 0,
    XSLT_OUTPUT_HTML,
    XSLT_OUTPUT_TEXT
} xsltOutputType;

typedef struct _xsltTransformContext xsltTransformContext;
typedef xsltTransformContext *xsltTransformContextPtr;
struct _xsltTransformContext {
    xsltStylesheetPtr style;		/* the stylesheet used */
    xsltOutputType type;		/* the type of output */

    xmlDocPtr doc;			/* the current doc */
    xmlNodePtr node;			/* the current node */
    xmlNodeSetPtr nodeList;		/* the current node list */

    xmlDocPtr output;			/* the resulting document */
    xmlNodePtr insert;			/* the insertion node */

    xmlXPathContextPtr xpathCtxt;	/* the XPath context */
    xmlXPathParserContextPtr xpathParserCtxt;/* the XPath parser context */
};

/************************************************************************
 *									*
 *			
 *									*
 ************************************************************************/

/**
 * xsltNewTransformContext:
 *
 * Create a new XSLT TransformContext
 *
 * Returns the newly allocated xsltTransformContextPtr or NULL in case of error
 */
xsltTransformContextPtr
xsltNewTransformContext(void) {
    xsltTransformContextPtr cur;

    cur = (xsltTransformContextPtr) xmlMalloc(sizeof(xsltTransformContext));
    if (cur == NULL) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltNewTransformContext : malloc failed\n");
	return(NULL);
    }
    memset(cur, 0, sizeof(xsltTransformContext));
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
    memset(ctxt, -1, sizeof(xsltTransformContext));
    xmlFree(ctxt);
}

/************************************************************************
 *									*
 *			
 *									*
 ************************************************************************/

void xsltProcessOneNode(xsltTransformContextPtr ctxt, xmlNodePtr node);

/**
 * xsltValueOf:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xsltValueOf node
 *
 * Process the xsltValueOf node on the source node
 */
void
xsltValueOf(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst) {
    xmlChar *prop;
    int disableEscaping = 0;
    xmlXPathObjectPtr res, tmp;
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
	if (disableEscaping) {
	    TODO /* disable-output-escaping */
	}
    }
    prop = xmlGetNsProp(inst, (const xmlChar *)"select", XSLT_NAMESPACE);
    if (prop == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsltValueOf: select is not defined\n", prop);
	return;
    }
#ifdef DEBUG_PROCESS
    xsltGenericError(xsltGenericErrorContext,
	 "xsltValueOf: select %s\n", prop);
#endif

    if (ctxt->xpathCtxt == NULL) {
	xmlXPathInit();
	ctxt->xpathCtxt = xmlXPathNewContext(ctxt->doc);
	if (ctxt->xpathCtxt == NULL)
	    goto error;
    }
    ctxt->xpathParserCtxt =
	xmlXPathNewParserContext(prop, ctxt->xpathCtxt);
    if (ctxt->xpathParserCtxt == NULL)
	goto error;
    ctxt->xpathCtxt->node = node;
    valuePush(ctxt->xpathParserCtxt, xmlXPathNewNodeSet(node));
    xmlXPathEvalExpr(ctxt->xpathParserCtxt);
    xmlXPathStringFunction(ctxt->xpathParserCtxt, 1);
    res = valuePop(ctxt->xpathParserCtxt);
    do {
        tmp = valuePop(ctxt->xpathParserCtxt);
	if (tmp != NULL) {
	    xmlXPathFreeObject(tmp);
	}
    } while (tmp != NULL);
    if (res != NULL) {
	if (res->type == XPATH_STRING) {
            copy = xmlNewText(res->stringval);
	    if (copy != NULL) {
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
	xsltGenericError(xsltGenericErrorContext,
	     "xsltValueOf: result %s\n", res->stringval);
#endif
error:
    if (ctxt->xpathParserCtxt != NULL)
	xmlXPathFreeParserContext(ctxt->xpathParserCtxt);
    if (prop != NULL)
	xmlFree(prop);
    if (res != NULL)
	xmlXPathFreeObject(res);
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
xmlNodePtr
xsltCopyNode(xsltTransformContextPtr ctxt, xmlNodePtr node,
	     xmlNodePtr insert) {
    xmlNodePtr copy;

    copy = xmlCopyNode(node, 0);
    if (copy != NULL) {
	xmlAddChild(insert, copy);
	/*
	 * Add namespaces as they are needed
	 */
	if (node->nsDef != NULL)
	    copy->nsDef = xmlCopyNamespaceList(node->nsDef);
	if (node->ns != NULL) {
	    /*
	     * optimization, if the namespace is already the
	     * on on the parent node, reuse it directly
	     *
	     * TODO: check possible mess with xmlCopyNamespaceList
	     */
	    if ((insert->type == XML_ELEMENT_NODE) &&
		(insert->ns != NULL) && 
		(xmlStrEqual(insert->ns->href, node->ns->href))) {
		copy->ns = insert->ns;
	    } else {
		xmlNsPtr ns;

		/*
		 * Look in the output tree if the namespace is
		 * already in scope.
		 */
		ns = xmlSearchNsByHref(ctxt->output, copy,
				       node->ns->href);
		if (ns != NULL)
		    copy->ns = ns;
		else {
		    ns = xmlNewNs(copy, node->ns->href,
				  node->ns->prefix);
		}
	    }
	}
    } else {
	xsltGenericError(xsltGenericErrorContext,
		"xsltProcessOneNode: copy %s failed\n", node->name);
    }
    return(copy);
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

    switch (node->type) {
	case XML_DOCUMENT_NODE:
	case XML_HTML_DOCUMENT_NODE:
	case XML_ELEMENT_NODE:
	    break;
	default:
	    return;
    }
    node = node->children;
    while (node != NULL) {
	switch (node->type) {
	    case XML_DOCUMENT_NODE:
	    case XML_HTML_DOCUMENT_NODE:
	    case XML_ELEMENT_NODE:
		xsltProcessOneNode(ctxt, node);
		break;
	    case XML_TEXT_NODE:
		/* TODO: check the whitespace stripping rules ! */
		if ((IS_BLANK_NODE(node)) &&
		    (node->parent != NULL) &&
		    (ctxt->style->stripSpaces != NULL)) {
		    const xmlChar *val;

		    val = (const xmlChar *)
			  xmlHashLookup(ctxt->style->stripSpaces,
				        node->parent->name);
		    if ((val != NULL) &&
			(xmlStrEqual(val, (xmlChar *) "strip")))
			break;
		}
		/* no break on purpose */
	    case XML_CDATA_SECTION_NODE:
		copy = xmlCopyNode(node, 0);
		if (copy != NULL) {
		    xmlAddChild(ctxt->insert, copy);
		} else {
		    xsltGenericError(xsltGenericErrorContext,
			"xsltDefaultProcessOneNode: text copy failed\n");
		}
		break;
	    default:
		TODO
	}
	node = node->next;
    }
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
    xmlChar *prop;

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL))
	return;

#ifdef DEBUG_PROCESS
    xsltGenericError(xsltGenericErrorContext,
	 "xsltApplyTemplates: node: %s\n", node->name);
#endif
    prop = xmlGetNsProp(inst, (const xmlChar *)"select", XSLT_NAMESPACE);
    if (prop != NULL) {
	TODO
    } else {
	xsltDefaultProcessOneNode(ctxt, node);
    }
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
    xmlNodePtr cur, insert, copy;
    xmlNodePtr oldInsert;

    oldInsert = insert = ctxt->insert;
    template = xsltGetTemplate(ctxt->style, node);
    /*
     * If no template is found, apply the deafult rule.
     */
    if (template == NULL) {
#ifdef DEBUG_PROCESS
	xsltGenericError(xsltGenericErrorContext,
	     "xsltProcessOneNode: no template found for %s\n", node->name);
#endif

	xsltDefaultProcessOneNode(ctxt, node);
	return;
    }

    /*
     * Insert all non-XSLT nodes found in the template
     */
    cur = template->content;
    while (cur != NULL) {
	if (IS_XSLT_ELEM(cur)) {
	    if (IS_XSLT_NAME(cur, "apply-templates")) {
		ctxt->insert = insert;
		xsltApplyTemplates(ctxt, node, cur);
		ctxt->insert = oldInsert;
	    } else if (IS_XSLT_NAME(cur, "value-of")) {
		ctxt->insert = insert;
		xsltValueOf(ctxt, node, cur);
		ctxt->insert = oldInsert;
	    } else {
#ifdef DEBUG_PROCESS
		xsltGenericError(xsltGenericErrorContext,
		     "xsltProcessOneNode: found xslt:%s\n", cur->name);
#endif
		TODO
	    }
	} else if (cur->type == XML_TEXT_NODE) {
	    /*
	     * This text comes from the stylesheet
	     * For stylesheets, the set of whitespace-preserving
	     * element names consists of just xsl:text.
	     */
	    if (!(IS_BLANK_NODE(cur))) {
#ifdef DEBUG_PROCESS
		xsltGenericError(xsltGenericErrorContext,
		     "xsltProcessOneNode: copy text %s\n", cur->content);
#endif
		copy = xmlCopyNode(cur, 0);
		if (copy != NULL) {
		    xmlAddChild(insert, copy);
		} else {
		    xsltGenericError(xsltGenericErrorContext,
			    "xsltProcessOneNode: text copy failed\n");
		}
	    }
	} else  {
#ifdef DEBUG_PROCESS
	    xsltGenericError(xsltGenericErrorContext,
		 "xsltProcessOneNode: copy node %s\n", cur->name);
#endif
	    copy = xsltCopyNode(ctxt, cur, insert);
	    /*
	     * all the attributes are directly inherited
	     * TODO: Do the substitution of {} XPath expressions !!!
	     */
	    if (cur->properties != NULL)
		copy->properties = xmlCopyPropList(copy, cur->properties);
	}
	/*
	 * Skip to next node
	 */

	if (cur->children != NULL) {
	    if (cur->children->type != XML_ENTITY_DECL) {
		cur = cur->children;
		insert = copy;
		continue;
	    }
	}
	if (cur->next != NULL) {
	    cur = cur->next;
	    continue;
	}
	
	do {
	    cur = cur->parent;
	    insert = insert->parent;
	    if (cur == NULL)
		break;
	    if (cur == template->content) {
		cur = NULL;
		break;
	    }
	    if (cur->next != NULL) {
		cur = cur->next;
		break;
	    }
	} while (cur != NULL);
    }
    /********
    if (ctxt->style->indent) {
	copy = xmlNewText("\n");
	if (copy != NULL)
	    xmlAddChild(ctxt->insert, copy);
    }
    ********/
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
    ctxt = xsltNewTransformContext();
    if (ctxt == NULL)
	return(NULL);
    ctxt->doc = doc;
    ctxt->style = style;
    if ((style->method != NULL) &&
	(!xmlStrEqual(style->method, (const xmlChar *) "xml"))) {
	if (xmlStrEqual(style->method, (const xmlChar *) "html")) {
	    ctxt->type = XSLT_OUTPUT_HTML;
	    res = htmlNewDoc(style->doctypePublic, style->doctypeSystem);
	    if (res == NULL)
		goto error;
	} else if (xmlStrEqual(style->method, (const xmlChar *) "text")) {
	    ctxt->type = XSLT_OUTPUT_TEXT;
	    TODO
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
    root = xmlDocGetRootElement(doc);
    if (root == NULL) {
	xsltGenericError(xsltGenericErrorContext,
			 "xsltApplyStylesheet: document has no root\n");
	goto error;
    }
    ctxt->output = res;
    ctxt->insert = (xmlNodePtr) res;
    ctxt->node = root;
    ctxt->nodeList = xmlXPathNodeSetCreate(root);
    xsltProcessOneNode(ctxt, root);


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

