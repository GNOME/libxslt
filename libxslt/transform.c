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
#include "xsltutils.h"
#include "pattern.h"
#include "transform.h"

#define DEBUG_PROCESS

/*
 * Useful macros
 */

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
void xsltForEach(xsltTransformContextPtr ctxt, xmlNodePtr node,
	         xmlNodePtr inst);
void xsltIf(xsltTransformContextPtr ctxt, xmlNodePtr node, xmlNodePtr inst);

/**
 * xsltAttribute:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt attribute node
 *
 * Process the xslt attribute node on the source node
 */
void
xsltAttribute(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst) {
    xmlChar *prop = NULL;
    xmlChar *ncname = NULL;
    xmlChar *prefix = NULL;
    xmlChar *value = NULL;
    xmlNsPtr ns = NULL;
    xmlAttrPtr attr;


    if (ctxt->insert == NULL)
	return;
    if (ctxt->insert->children != NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:attribute : node has already children\n");
	return;
    }
    prop = xmlGetNsProp(inst, (const xmlChar *)"namespace", XSLT_NAMESPACE);
    if (prop != NULL) {
	/* TODO: attribute value template */
	TODO
	xmlFree(prop);
	return;
    }
    prop = xmlGetNsProp(inst, (const xmlChar *)"name", XSLT_NAMESPACE);
    if (prop == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:attribute : name is missing\n");
	goto error;
    }

    ncname = xmlSplitQName2(prop, &prefix);
    if (ncname == NULL) {
	ncname = prop;
	prop = NULL;
	prefix = NULL;
    }
    if (xmlStrEqual(ncname, (const xmlChar *) "xmlns")) {
	xsltGenericError(xsltGenericErrorContext,
	     "xslt:attribute : xmlns forbidden\n");
	goto error;
    }
    if ((prefix != NULL) && (ns == NULL)) {
	ns = xmlSearchNs(ctxt->insert->doc, ctxt->insert, prefix);
	if (ns == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		"no namespace bound to prefix %s\n", prefix);
	}
    }

    value = xmlNodeListGetString(inst->doc, inst->children, 1);
    if (value == NULL) {
	if (ns) {
#if LIBXML_VERSION > 202111
	    attr = xmlSetNsProp(ctxt->insert, ncname, ns->href,
		                (const xmlChar *)"");
#else
	    xsltGenericError(xsltGenericErrorContext,
		"xsl:attribute: recompile against newer libxml version\n");
	    attr = xmlSetProp(ctxt->insert, ncname, (const xmlChar *)"");
#endif
	} else
	    attr = xmlSetProp(ctxt->insert, ncname, (const xmlChar *)"");
    } else {
	/* TODO: attribute value template */
	if (ns) {
#if LIBXML_VERSION > 202111
	    attr = xmlSetNsProp(ctxt->insert, ncname, ns->href, value);
#else
	    xsltGenericError(xsltGenericErrorContext,
		"xsl:attribute: recompile against newer libxml version\n");
	    attr = xmlSetProp(ctxt->insert, ncname, value);
#endif
	} else
	    attr = xmlSetProp(ctxt->insert, ncname, value);
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
    xmlXPathParserContextPtr xpathParserCtxt;
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
	     "xsltValueOf: select is not defined\n");
	return;
    }
#ifdef DEBUG_PROCESS
    xsltGenericDebug(xsltGenericDebugContext,
	 "xsltValueOf: select %s\n", prop);
#endif

    if (ctxt->xpathCtxt == NULL) {
	xmlXPathInit();
	ctxt->xpathCtxt = xmlXPathNewContext(ctxt->doc);
	if (ctxt->xpathCtxt == NULL)
	    goto error;
    }
    xpathParserCtxt =
	xmlXPathNewParserContext(prop, ctxt->xpathCtxt);
    if (xpathParserCtxt == NULL)
	goto error;
    ctxt->xpathCtxt->node = node;
    valuePush(xpathParserCtxt, xmlXPathNewNodeSet(node));
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
		"xsltCopyNode: copy %s failed\n", node->name);
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
    xmlNodePtr delete = NULL;

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
			(xmlStrEqual(val, (xmlChar *) "strip"))) {
			delete = node;
			break;
		    }
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
#ifdef DEBUG_PROCESS
		xsltGenericDebug(xsltGenericDebugContext,
		 "xsltDefaultProcessOneNode: skipping node type %d\n",
		                 node->type);
#endif
		delete = node;
	}
	node = node->next;
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
    xsltGenericDebug(xsltGenericDebugContext,
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
    xmlNodePtr cur, insert, copy, delete = NULL;
    xmlNodePtr oldInsert;

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

	/*
	 * Cleanup of ignorable blank node detected
	 */
	if (delete != NULL) {
#ifdef DEBUG_PROCESS
	    xsltGenericDebug(xsltGenericDebugContext,
		 "xsltApplyOneTemplate: removing ignorable blank node\n");
#endif
	    xmlUnlinkNode(delete);
	    xmlFreeNode(delete);
	    delete = NULL;
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
	    } else if (IS_XSLT_NAME(cur, "if")) {
		ctxt->insert = insert;
		xsltIf(ctxt, node, cur);
		ctxt->insert = oldInsert;
	    } else if (IS_XSLT_NAME(cur, "for-each")) {
		ctxt->insert = insert;
		xsltForEach(ctxt, node, cur);
		ctxt->insert = oldInsert;
	    } else if (IS_XSLT_NAME(cur, "attribute")) {
		ctxt->insert = insert;
		xsltAttribute(ctxt, node, cur);
		ctxt->insert = oldInsert;
	    } else if (IS_XSLT_NAME(cur, "element")) {
		ctxt->insert = insert;
		xsltAttribute(ctxt, node, cur);
		ctxt->insert = oldInsert;
	    } else {
#ifdef DEBUG_PROCESS
		xsltGenericDebug(xsltGenericDebugContext,
		     "xsltApplyOneTemplate: found xslt:%s\n", cur->name);
#endif
		TODO
	    }
	    goto skip_children;
	} else if (cur->type == XML_TEXT_NODE) {
	    /*
	     * This text comes from the stylesheet
	     * For stylesheets, the set of whitespace-preserving
	     * element names consists of just xsl:text.
	     */
	    if (!(IS_BLANK_NODE(cur))) {
#ifdef DEBUG_PROCESS
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
	    } else {
		delete = cur;
	    }
	} else if (cur->type == XML_ELEMENT_NODE) {
#ifdef DEBUG_PROCESS
	    xsltGenericDebug(xsltGenericDebugContext,
		 "xsltApplyOneTemplate: copy node %s\n", cur->name);
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
    xmlXPathObjectPtr res, tmp;
    xmlXPathParserContextPtr xpathParserCtxt;
    int doit;

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

    if (ctxt->xpathCtxt == NULL) {
	xmlXPathInit();
	ctxt->xpathCtxt = xmlXPathNewContext(ctxt->doc);
	if (ctxt->xpathCtxt == NULL)
	    goto error;
    }
    xpathParserCtxt = xmlXPathNewParserContext(prop, ctxt->xpathCtxt);
    if (xpathParserCtxt == NULL)
	goto error;
    ctxt->xpathCtxt->node = node;
    valuePush(xpathParserCtxt, xmlXPathNewNodeSet(node));
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
	xsltApplyOneTemplate(ctxt, ctxt->node, inst->children);
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
    xmlXPathObjectPtr res, tmp;
    xmlNodePtr replacement;
    xmlNodeSetPtr list = NULL, oldlist;
    xmlXPathParserContextPtr xpathParserCtxt;
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

    if (ctxt->xpathCtxt == NULL) {
	xmlXPathInit();
	ctxt->xpathCtxt = xmlXPathNewContext(ctxt->doc);
	if (ctxt->xpathCtxt == NULL)
	    goto error;
    }
    xpathParserCtxt = xmlXPathNewParserContext(prop, ctxt->xpathCtxt);
    if (xpathParserCtxt == NULL)
	goto error;
    ctxt->xpathCtxt->node = node;
    valuePush(xpathParserCtxt, xmlXPathNewNodeSet(node));
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
    /* TODO: handle and skip the xsl:sort */
    replacement = inst->children;

    oldlist = ctxt->nodeList;
    ctxt->nodeList = list;
    oldContextSize = ctxt->xpathCtxt->contextSize;
    oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
    ctxt->xpathCtxt->contextSize = list->nodeNr;
    for (i = 0;i < list->nodeNr;i++) {
	ctxt->node = list->nodeTab[i];
	ctxt->xpathCtxt->proximityPosition = i + 1;
	xsltApplyOneTemplate(ctxt, list->nodeTab[i], replacement);
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
    template = xsltGetTemplate(ctxt->style, node);

    /*
     * If no template is found, apply the default rule.
     */
    if (template == NULL) {
#ifdef DEBUG_PROCESS
	if (node->type == XML_DOCUMENT_NODE)
	    xsltGenericDebug(xsltGenericDebugContext,
	     "xsltProcessOneNode: no template found for /\n");
	else 
	    xsltGenericDebug(xsltGenericDebugContext,
	     "xsltProcessOneNode: no template found for %s\n", node->name);
#endif

	xsltDefaultProcessOneNode(ctxt, node);
	return;
    }

    xsltApplyOneTemplate(ctxt, node, template->content);
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
    xsltProcessOneNode(ctxt, ctxt->node);


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

