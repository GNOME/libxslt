/*
 * variables.c: Implementation of the variable storage and lookup
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
#include <libxml/valid.h>
#include <libxml/hash.h>
#include <libxml/xmlerror.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/parserInternals.h>
#include <libxml/xmlversion.h>
#include "xslt.h"
#include "xsltInternals.h"
#include "xsltutils.h"
#include "variables.h"
#include "transform.h"
#include "imports.h"

#define DEBUG_VARIABLE

/************************************************************************
 *									*
 *			Module interfaces				*
 *									*
 ************************************************************************/

/**
 * xsltNewStackElem:
 *
 * Create a new XSLT ParserContext
 *
 * Returns the newly allocated xsltParserStackElem or NULL in case of error
 */
xsltStackElemPtr
xsltNewStackElem(void) {
    xsltStackElemPtr cur;

    cur = (xsltStackElemPtr) xmlMalloc(sizeof(xsltStackElem));
    if (cur == NULL) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltNewStackElem : malloc failed\n");
	return(NULL);
    }
    memset(cur, 0, sizeof(xsltStackElem));
    cur->computed = 0;
    return(cur);
}

/**
 * xsltFreeStackElem:
 * @elem:  an XSLT stack element
 *
 * Free up the memory allocated by @elem
 */
void
xsltFreeStackElem(xsltStackElemPtr elem) {
    if (elem == NULL)
	return;
    if (elem->name != NULL)
	xmlFree(elem->name);
    if (elem->nameURI != NULL)
	xmlFree(elem->nameURI);
    if (elem->select != NULL)
	xmlFree(elem->select);
    if (elem->value != NULL)
	xmlXPathFreeObject(elem->value);

    memset(elem, -1, sizeof(xsltStackElem));
    xmlFree(elem);
}

/**
 * xsltFreeStackElemList:
 * @elem:  an XSLT stack element
 *
 * Free up the memory allocated by @elem
 */
void
xsltFreeStackElemList(xsltStackElemPtr elem) {
    xsltStackElemPtr next;

    while(elem != NULL) {
	next = elem->next;
	xsltFreeStackElem(elem);
	elem = next;
    }
}

/**
 * xsltCheckStackElem:
 * @ctxt:  xn XSLT transformation context
 * @name:  the variable name
 * @nameURI:  the variable namespace URI
 *
 * check wether the variable or param is already defined
 *
 * Returns 1 if present, 0 if not, -1 in case of failure.
 */
int
xsltCheckStackElem(xsltTransformContextPtr ctxt, const xmlChar *name,
	           const xmlChar *nameURI) {
    xsltStackElemPtr cur;

    if ((ctxt == NULL) || (name == NULL))
	return(-1);

    cur = ctxt->vars;
    while (cur != NULL) {
	if (xmlStrEqual(name, cur->name)) {
	    if (((nameURI == NULL) && (cur->nameURI == NULL)) ||
		((nameURI != NULL) && (cur->nameURI != NULL) &&
		 (xmlStrEqual(nameURI, cur->nameURI)))) {
		return(1);
	    }
	}
	cur = cur->next;
    }
    return(0);
}

/**
 * xsltAddStackElem:
 * @ctxt:  xn XSLT transformation context
 * @elem:  a stack element
 *
 * add a new element at this level of the stack.
 *
 * Returns 0 in case of success, -1 in case of failure.
 */
int
xsltAddStackElem(xsltTransformContextPtr ctxt, xsltStackElemPtr elem) {
    if ((ctxt == NULL) || (elem == NULL))
	return(-1);

    elem->next = ctxt->varsTab[ctxt->varsNr - 1];
    ctxt->varsTab[ctxt->varsNr - 1] = elem;
    ctxt->vars = elem;
    return(0);
}

/**
 * xsltAddStackElemList:
 * @ctxt:  xn XSLT transformation context
 * @elems:  a stack element list
 *
 * add the new element list at this level of the stack.
 *
 * Returns 0 in case of success, -1 in case of failure.
 */
int
xsltAddStackElemList(xsltTransformContextPtr ctxt, xsltStackElemPtr elems) {
    xsltStackElemPtr cur;

    if ((ctxt == NULL) || (elems == NULL))
	return(-1);

    /* TODO: check doublons */
    if (ctxt->varsTab[ctxt->varsNr - 1] != NULL) {
	cur = ctxt->varsTab[ctxt->varsNr - 1];
	while (cur->next != NULL)
	    cur = cur->next;
	cur->next = elems;
    } else {
	elems->next = ctxt->varsTab[ctxt->varsNr - 1];
	ctxt->varsTab[ctxt->varsNr - 1] = elems;
	ctxt->vars = elems;
    }
    return(0);
}

/**
 * xsltStackLookup:
 * @ctxt:  an XSLT transformation context
 * @name:  the local part of the name
 * @nameURI:  the URI part of the name
 *
 * Locate an element in the stack based on its name.
 */
xsltStackElemPtr
xsltStackLookup(xsltTransformContextPtr ctxt, const xmlChar *name,
	        const xmlChar *nameURI) {
    xsltStackElemPtr ret = NULL;
    int i;
    xsltStackElemPtr cur;

    if ((ctxt == NULL) || (name == NULL))
	return(NULL);

    /*
     * Do the lookup from the top of the stack, but
     * don't use params being computed in a call-param
     */
    i = ctxt->varsNr - 1;

    for (;i >= 0;i--) {
	cur = ctxt->varsTab[i];
	while (cur != NULL) {
	    if (xmlStrEqual(cur->name, name)) {
		if (nameURI == NULL) {
		    if (cur->nameURI == NULL) {
			return(cur);
		    }
		} else {
		    if ((cur->nameURI != NULL) &&
			(xmlStrEqual(cur->nameURI, nameURI))) {
			return(cur);
		    }
		}

	    }
	    cur = cur->next;
	}
    }
    return(ret);
}

/************************************************************************
 *									*
 *			Module interfaces				*
 *									*
 ************************************************************************/

/**
 * xsltEvalVariable:
 * @ctxt:  the XSLT transformation context
 * @elem:  the variable or parameter.
 *
 * Evaluate a variable value.
 *
 * Returns 0 in case of success, -1 in case of error
 */
int
xsltEvalVariable(xsltTransformContextPtr ctxt, xsltStackElemPtr elem) {
    if ((ctxt == NULL) || (elem == NULL))
	return(-1);

#ifdef DEBUG_VARIABLE
    xsltGenericDebug(xsltGenericDebugContext,
	"Evaluating variable %s\n", elem->name);
#endif
    if (elem->select != NULL) {
	xmlXPathCompExprPtr comp;
	xmlXPathObjectPtr result;

	comp = xmlXPathCompile(elem->select);
	if (comp == NULL)
	    return(-1);
	ctxt->xpathCtxt->node = (xmlNodePtr) ctxt->node;
	result = xmlXPathCompiledEval(comp, ctxt->xpathCtxt);
	xmlXPathFreeCompExpr(comp);
	if (result == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		"Evaluating global variable %s failed\n");
	} else {
#ifdef DEBUG_VARIABLE
#ifdef LIBXML_DEBUG_ENABLED
	    if ((xsltGenericDebugContext == stdout) ||
		(xsltGenericDebugContext == stderr))
		xmlXPathDebugDumpObject((FILE *)xsltGenericDebugContext,
					result, 0);
#endif
#endif
	    if (elem->value != NULL)
		xmlXPathFreeObject(elem->value);
	    elem->value = result;
	}
	elem->computed = 1;
    } else {
	if (elem->tree == NULL) {
	    elem->value = xmlXPathNewCString("");
	} else {
	    /*
	     * This is a result tree fragment.
	     */
	    xmlNodePtr container;
	    xmlNodePtr oldInsert, oldNode;

	    container = xmlNewDocNode(ctxt->output, NULL,
		                      (const xmlChar *) "fake", NULL);
	    if (container == NULL)
		return(-1);

	    oldInsert = ctxt->insert;
	    oldNode = ctxt->node;
	    ctxt->insert = container;
	    xsltApplyOneTemplate(ctxt, ctxt->node, elem->tree, 0);
	    ctxt->insert = oldInsert;
	    ctxt->node = oldNode;

	    if (elem->value != NULL)
		xmlXPathFreeObject(elem->value);
	    elem->value = xmlXPathNewValueTree(container);
	    if (elem->value == NULL) {
		elem->value = xmlXPathNewCString("");
	    }
#ifdef DEBUG_VARIABLE
#ifdef LIBXML_DEBUG_ENABLED
	    if ((xsltGenericDebugContext == stdout) ||
		(xsltGenericDebugContext == stderr))
		xmlXPathDebugDumpObject((FILE *)xsltGenericDebugContext,
					elem->value, 0);
#endif
#endif
	}
	elem->computed = 1;
    }
    return(0);
}
/**
 * xsltEvalGlobalVariables:
 * @ctxt:  the XSLT transformation context
 *
 * Evaluate the global variables of a stylesheet. This need to be
 * done on parsed stylesheets before starting to apply transformations
 *
 * Returns 0 in case of success, -1 in case of error
 */
int
xsltEvalGlobalVariables(xsltTransformContextPtr ctxt) {
    xsltStackElemPtr elem;
    xsltStylesheetPtr style;

    if (ctxt == NULL)
	return(-1);
 
#ifdef DEBUG_VARIABLE
    xsltGenericDebug(xsltGenericDebugContext,
	"Evaluating global variables\n");
#endif
    ctxt->node = (xmlNodePtr) ctxt->document->doc;
    style = ctxt->style;
    while (style != NULL) {
	elem = style->variables;
	
	while (elem != NULL) {
	    xsltEvalVariable(ctxt, elem);
	    elem = elem->next;
	}

	style = xsltNextImport(style);
    }
    return(0);
}

/**
 * xsltRegisterGlobalVariable:
 * @style:  the XSLT transformation context
 * @name:  the variable name
 * @ns_uri:  the variable namespace URI
 * @select:  the expression which need to be evaluated to generate a value
 * @tree:  the subtree if select is NULL
 * @param:  this is a parameter actually
 *
 * Register a new variable value. If @value is NULL it unregisters
 * the variable
 *
 * Returns 0 in case of success, -1 in case of error
 */
int
xsltRegisterGlobalVariable(xsltStylesheetPtr style, const xmlChar *name,
		     const xmlChar *ns_uri, const xmlChar *select,
		     xmlNodePtr tree, int param) {
    xsltStackElemPtr elem;
    if (style == NULL)
	return(-1);
    if (name == NULL)
	return(-1);

#ifdef DEBUG_VARIABLE
    if (param)
	xsltGenericDebug(xsltGenericDebugContext,
			 "Defineing global param %s\n", name);
    else
	xsltGenericDebug(xsltGenericDebugContext,
			 "Defineing global variable %s\n", name);
#endif
    elem = xsltNewStackElem();
    if (elem == NULL)
	return(-1);
    if (param)
	elem->type = XSLT_ELEM_PARAM;
    else
	elem->type = XSLT_ELEM_VARIABLE;
    elem->name = xmlStrdup(name);
    elem->select = xmlStrdup(select);
    if (ns_uri)
	elem->nameURI = xmlStrdup(ns_uri);
    elem->tree = tree;
    elem->next = style->variables;
    style->variables = elem;
    return(0);
}

/**
 * xsltBuildVariable:
 * @ctxt:  the XSLT transformation context
 * @name:  the variable name
 * @ns_uri:  the variable namespace URI
 * @select:  the expression which need to be evaluated to generate a value
 * @tree:  the tree if select is NULL
 * @param:  this is a parameter actually
 *
 * Computes a new variable value.
 *
 * Returns the xsltStackElemPtr or NULL in case of error
 */
xsltStackElemPtr
xsltBuildVariable(xsltTransformContextPtr ctxt, const xmlChar *name,
		     const xmlChar *ns_uri, const xmlChar *select,
		     xmlNodePtr tree, int param) {
    xsltStackElemPtr elem;
    if (ctxt == NULL)
	return(NULL);
    if (name == NULL)
	return(NULL);

#ifdef DEBUG_VARIABLE
    xsltGenericDebug(xsltGenericDebugContext,
		     "Building variable %s", name);
    if (select != NULL)
	xsltGenericDebug(xsltGenericDebugContext,
			 " select %s",  select);
    xsltGenericDebug(xsltGenericDebugContext, "\n");
#endif
    elem = xsltNewStackElem();
    if (elem == NULL)
	return(NULL);
    if (param)
	elem->type = XSLT_ELEM_PARAM;
    else
	elem->type = XSLT_ELEM_VARIABLE;
    elem->name = xmlStrdup(name);
    if (select != NULL)
	elem->select = xmlStrdup(select);
    else
	elem->select = NULL;
    if (ns_uri)
	elem->nameURI = xmlStrdup(ns_uri);
    elem->tree = tree;
    xsltEvalVariable(ctxt, elem);
    return(elem);
}

/**
 * xsltRegisterVariable:
 * @ctxt:  the XSLT transformation context
 * @name:  the variable name
 * @ns_uri:  the variable namespace URI
 * @select:  the expression which need to be evaluated to generate a value
 * @tree:  the tree if select is NULL
 * @param:  this is a parameter actually
 *
 * Computes and register a new variable value. 
 *
 * Returns 0 in case of success, -1 in case of error
 */
int
xsltRegisterVariable(xsltTransformContextPtr ctxt, const xmlChar *name,
		     const xmlChar *ns_uri, const xmlChar *select,
		     xmlNodePtr tree, int param) {
    xsltStackElemPtr elem;
    if (ctxt == NULL)
	return(-1);
    if (name == NULL)
	return(-1);

    if (xsltCheckStackElem(ctxt, name, ns_uri) != 0) {
	if (!param) {
	    xsltGenericError(xsltGenericErrorContext,
	    "xsl:variable : redefining %s\n", name);
	}
#ifdef DEBUG_VARIABLE
	else
	    xsltGenericDebug(xsltGenericDebugContext,
		     "param %s defined by caller", name);
#endif
	return(0);
    }
    elem = xsltBuildVariable(ctxt, name, ns_uri, select, tree, param);
    xsltAddStackElem(ctxt, elem);
    return(0);
}

/**
 * xsltGlobalVariableLookup:
 * @ctxt:  the XSLT transformation context
 * @name:  the variable name
 * @ns_uri:  the variable namespace URI
 *
 * Search in the Variable array of the context for the given
 * variable value.
 *
 * Returns the value or NULL if not found
 */
xmlXPathObjectPtr
xsltGlobalVariableLookup(xsltTransformContextPtr ctxt, const xmlChar *name,
		         const xmlChar *ns_uri) {
    xsltStylesheetPtr style;
    xsltStackElemPtr elem = NULL;

    style = ctxt->style;
    while (style != NULL) {
	elem = style->variables;
	
	while (elem != NULL) {
	    if (xmlStrEqual(elem->name, name)) {
		if (ns_uri == NULL) {
		    if (elem->nameURI == NULL)
			break;
		} else {
		    if ((elem->nameURI != NULL) &&
			(xmlStrEqual(elem->nameURI, ns_uri)))
			break;
		}

	    }
	    elem = elem->next;
	}
	if (elem != NULL) break;

	style = xsltNextImport(style);
    }
    if (elem == NULL)
	return(NULL);

    if (!elem->computed) {
#ifdef DEBUG_VARIABLE
	xsltGenericDebug(xsltGenericDebugContext,
		         "uncomputed global variable %s\n", name);
#endif
        xsltEvalVariable(ctxt, elem);
    }
    if (elem->value != NULL)
	return(xmlXPathObjectCopy(elem->value));
#ifdef DEBUG_VARIABLE
    xsltGenericDebug(xsltGenericDebugContext,
		     "global variable not found %s\n", name);
#endif
    return(NULL);
}

/**
 * xsltVariableLookup:
 * @ctxt:  the XSLT transformation context
 * @name:  the variable name
 * @ns_uri:  the variable namespace URI
 *
 * Search in the Variable array of the context for the given
 * variable value.
 *
 * Returns the value or NULL if not found
 */
xmlXPathObjectPtr
xsltVariableLookup(xsltTransformContextPtr ctxt, const xmlChar *name,
		   const xmlChar *ns_uri) {
    xsltStackElemPtr elem;

    if (ctxt == NULL)
	return(NULL);

    elem = xsltStackLookup(ctxt, name, ns_uri);
    if (elem == NULL) {
	return(xsltGlobalVariableLookup(ctxt, name, ns_uri));
    }
    if (!elem->computed) {
#ifdef DEBUG_VARIABLE
	xsltGenericDebug(xsltGenericDebugContext,
		         "uncomputed variable %s\n", name);
#endif
        xsltEvalVariable(ctxt, elem);
    }
    if (elem->value != NULL)
	return(xmlXPathObjectCopy(elem->value));
#ifdef DEBUG_VARIABLE
    xsltGenericDebug(xsltGenericDebugContext,
		     "variable not found %s\n", name);
#endif
    return(NULL);
}

/**
 * xsltParseStylesheetCallerParam:
 * @ctxt:  the XSLT transformation context
 * @cur:  the "param" element
 *
 * parse an XSLT transformation param declaration, compute
 * its value but doesn't record it.
 *
 * It returns the new xsltStackElemPtr or NULL
 */

xsltStackElemPtr
xsltParseStylesheetCallerParam(xsltTransformContextPtr ctxt, xmlNodePtr cur) {
    xmlChar *name, *ncname, *prefix;
    xmlChar *select;
    xmlNodePtr tree = NULL;
    xsltStackElemPtr elem = NULL;

    if ((cur == NULL) || (ctxt == NULL))
	return(NULL);

    name = xmlGetNsProp(cur, (const xmlChar *)"name", XSLT_NAMESPACE);
    if (name == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsl:param : missing name attribute\n");
	return(NULL);
    }

#ifdef DEBUG_VARIABLE
    xsltGenericDebug(xsltGenericDebugContext,
	"Parsing param %s\n", name);
#endif

    select = xmlGetNsProp(cur, (const xmlChar *)"select", XSLT_NAMESPACE);
    if (select == NULL) {
	tree = cur->children;
    } else {
#ifdef DEBUG_VARIABLE
	xsltGenericDebug(xsltGenericDebugContext,
	    "        select %s\n", select);
#endif
	if (cur->children != NULL)
	    xsltGenericError(xsltGenericErrorContext,
	    "xsl:param : content shuld be empty since select is present \n");
    }

    ncname = xmlSplitQName2(name, &prefix);

    if (ncname != NULL) {
	if (prefix != NULL) {
	    xmlNsPtr ns;

	    ns = xmlSearchNs(cur->doc, cur, prefix);
	    if (ns == NULL) {
		xsltGenericError(xsltGenericErrorContext,
		    "xsl:param : no namespace bound to prefix %s\n", prefix);
	    } else {
		elem = xsltBuildVariable(ctxt, ncname, ns->href, select,
			                 tree, 1);
	    }
	    xmlFree(prefix);
	} else {
	    elem = xsltBuildVariable(ctxt, ncname, NULL, select, tree, 1);
	}
	xmlFree(ncname);
    } else {
	elem = xsltBuildVariable(ctxt, name, NULL, select, tree, 1);
    }

    xmlFree(name);
    if (select != NULL)
	xmlFree(select);
    return(elem);
}

/**
 * xsltParseStylesheetParam:
 * @ctxt:  the XSLT transformation context
 * @cur:  the "param" element
 *
 * parse an XSLT transformation param declaration and record
 * its value.
 */

void
xsltParseStylesheetParam(xsltTransformContextPtr ctxt, xmlNodePtr cur) {
    xmlChar *name, *ncname, *prefix;
    xmlChar *select;
    xmlNodePtr tree = NULL;

    if ((cur == NULL) || (ctxt == NULL))
	return;

    name = xmlGetNsProp(cur, (const xmlChar *)"name", XSLT_NAMESPACE);
    if (name == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsl:param : missing name attribute\n");
	return;
    }

#ifdef DEBUG_VARIABLE
    xsltGenericDebug(xsltGenericDebugContext,
	"Parsing param %s\n", name);
#endif

    select = xmlGetNsProp(cur, (const xmlChar *)"select", XSLT_NAMESPACE);
    if (select == NULL) {
	tree = cur->children;
    } else {
#ifdef DEBUG_VARIABLE
	xsltGenericDebug(xsltGenericDebugContext,
	    "        select %s\n", select);
#endif
	if (cur->children != NULL)
	    xsltGenericError(xsltGenericErrorContext,
	    "xsl:param : content shuld be empty since select is present \n");
    }

    ncname = xmlSplitQName2(name, &prefix);

    if (ncname != NULL) {
	if (prefix != NULL) {
	    xmlNsPtr ns;

	    ns = xmlSearchNs(cur->doc, cur, prefix);
	    if (ns == NULL) {
		xsltGenericError(xsltGenericErrorContext,
		    "xsl:param : no namespace bound to prefix %s\n", prefix);
	    } else {
		xsltRegisterVariable(ctxt, ncname, ns->href, select, tree, 1);
	    }
	    xmlFree(prefix);
	} else {
	    xsltRegisterVariable(ctxt, ncname, NULL, select, tree, 1);
	}
	xmlFree(ncname);
    } else {
	xsltRegisterVariable(ctxt, name, NULL, select, tree, 1);
    }

    xmlFree(name);
    if (select != NULL)
	xmlFree(select);

}

/**
 * xsltParseGlobalVariable:
 * @style:  the XSLT stylesheet
 * @cur:  the "variable" element
 *
 * parse an XSLT transformation variable declaration and record
 * its value.
 */

void
xsltParseGlobalVariable(xsltStylesheetPtr style, xmlNodePtr cur) {
    xmlChar *name, *ncname, *prefix;
    xmlChar *select;
    xmlNodePtr tree = NULL;

    if ((cur == NULL) || (style == NULL))
	return;

    name = xmlGetNsProp(cur, (const xmlChar *)"name", XSLT_NAMESPACE);
    if (name == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsl:variable : missing name attribute\n");
	return;
    }

#ifdef DEBUG_VARIABLE
    xsltGenericDebug(xsltGenericDebugContext,
	"Parsing global variable %s\n", name);
#endif

    select = xmlGetNsProp(cur, (const xmlChar *)"select", XSLT_NAMESPACE);
    if (select == NULL) {
	tree = cur->children;
    } else {
	if (cur->children != NULL)
	    xsltGenericError(xsltGenericErrorContext,
	    "xsl:variable : content shuld be empty since select is present \n");
    }

    ncname = xmlSplitQName2(name, &prefix);

    if (ncname != NULL) {
	if (prefix != NULL) {
	    xmlNsPtr ns;

	    ns = xmlSearchNs(cur->doc, cur, prefix);
	    if (ns == NULL) {
		xsltGenericError(xsltGenericErrorContext,
		    "xsl:variable : no namespace bound to prefix %s\n", prefix);
	    } else {
		xsltRegisterGlobalVariable(style, ncname, ns->href, select, tree, 0);
	    }
	    xmlFree(prefix);
	} else {
	    xsltRegisterGlobalVariable(style, ncname, NULL, select, tree, 0);
	}
	xmlFree(ncname);
    } else {
	xsltRegisterGlobalVariable(style, name, NULL, select, tree, 0);
    }

    xmlFree(name);
    if (select != NULL)
	xmlFree(select);

}

/**
 * xsltParseGlobalParam:
 * @style:  the XSLT stylesheet
 * @cur:  the "param" element
 *
 * parse an XSLT transformation param declaration and record
 * its value.
 */

void
xsltParseGlobalParam(xsltStylesheetPtr style, xmlNodePtr cur) {
    xmlChar *name, *ncname, *prefix;
    xmlChar *select;
    xmlNodePtr tree = NULL;

    if ((cur == NULL) || (style == NULL))
	return;

    name = xmlGetNsProp(cur, (const xmlChar *)"name", XSLT_NAMESPACE);
    if (name == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsl:param : missing name attribute\n");
	return;
    }

#ifdef DEBUG_VARIABLE
    xsltGenericDebug(xsltGenericDebugContext,
	"Parsing global param %s\n", name);
#endif

    select = xmlGetNsProp(cur, (const xmlChar *)"select", XSLT_NAMESPACE);
    if (select == NULL) {
	tree = cur->children;
    } else {
	if (cur->children != NULL)
	    xsltGenericError(xsltGenericErrorContext,
	    "xsl:param : content shuld be empty since select is present \n");
    }

    ncname = xmlSplitQName2(name, &prefix);

    if (ncname != NULL) {
	if (prefix != NULL) {
	    xmlNsPtr ns;

	    ns = xmlSearchNs(cur->doc, cur, prefix);
	    if (ns == NULL) {
		xsltGenericError(xsltGenericErrorContext,
		    "xsl:param : no namespace bound to prefix %s\n", prefix);
	    } else {
		xsltRegisterGlobalVariable(style, ncname, ns->href, select, tree, 1);
	    }
	    xmlFree(prefix);
	} else {
	    xsltRegisterGlobalVariable(style, ncname, NULL, select, tree, 1);
	}
	xmlFree(ncname);
    } else {
	xsltRegisterGlobalVariable(style, name, NULL, select, tree, 1);
    }

    xmlFree(name);
    if (select != NULL)
	xmlFree(select);
}

/**
 * xsltParseStylesheetVariable:
 * @ctxt:  the XSLT transformation context
 * @cur:  the "variable" element
 *
 * parse an XSLT transformation variable declaration and record
 * its value.
 */

void
xsltParseStylesheetVariable(xsltTransformContextPtr ctxt, xmlNodePtr cur) {
    xmlChar *name, *ncname, *prefix;
    xmlChar *select;
    xmlNodePtr tree = NULL;

    if ((cur == NULL) || (ctxt == NULL))
	return;

    name = xmlGetNsProp(cur, (const xmlChar *)"name", XSLT_NAMESPACE);
    if (name == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsl:variable : missing name attribute\n");
	return;
    }

#ifdef DEBUG_VARIABLE
    xsltGenericDebug(xsltGenericDebugContext,
	"Parsing variable %s\n", name);
#endif

    select = xmlGetNsProp(cur, (const xmlChar *)"select", XSLT_NAMESPACE);
    if (select == NULL) {
	tree = cur->children;
    } else {
	if (cur->children != NULL)
	    xsltGenericError(xsltGenericErrorContext,
	"xsl:variable : content should be empty since select is present \n");
    }

    ncname = xmlSplitQName2(name, &prefix);

    if (ncname != NULL) {
	if (prefix != NULL) {
	    xmlNsPtr ns;

	    ns = xmlSearchNs(cur->doc, cur, prefix);
	    if (ns == NULL) {
		xsltGenericError(xsltGenericErrorContext,
		    "xsl:variable : no namespace bound to prefix %s\n", prefix);
	    } else {
		xsltRegisterVariable(ctxt, ncname, ns->href, select, tree, 0);
	    }
	    xmlFree(prefix);
	} else {
	    xsltRegisterVariable(ctxt, ncname, NULL, select, tree, 0);
	}
	xmlFree(ncname);
    } else {
	xsltRegisterVariable(ctxt, name, NULL, select, tree, 0);
    }

    xmlFree(name);
    if (select != NULL)
	xmlFree(select);

}

/**
 * xsltVariableLookup:
 * @ctxt:  a void * but the the XSLT transformation context actually
 * @name:  the variable name
 * @ns_uri:  the variable namespace URI
 *
 * This is the entry point when a varibale is needed by the XPath
 * interpretor.
 *
 * Returns the value or NULL if not found
 */
xmlXPathObjectPtr
xsltXPathVariableLookup(void *ctxt, const xmlChar *name,
	                const xmlChar *ns_uri) {
    xsltTransformContextPtr context;
    xmlXPathObjectPtr ret;

    if ((ctxt == NULL) || (name == NULL))
	return(NULL);

#ifdef DEBUG_VARIABLE
    xsltGenericDebug(xsltGenericDebugContext,
	    "Lookup variable %s\n", name);
#endif
    context = (xsltTransformContextPtr) ctxt;
    ret = xsltVariableLookup(context, name, ns_uri);
    if (ret == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "unregistered variable %s\n", name);
    }
#ifdef DEBUG_VARIABLE
    if (ret != NULL)
	xsltGenericDebug(xsltGenericDebugContext,
	    "found variable %s\n", name);
#endif
    return(ret);
}

