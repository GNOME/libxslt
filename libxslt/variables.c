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
#include "preproc.h"

#ifdef WITH_XSLT_DEBUG
#define WITH_XSLT_DEBUG_VARIABLE
#endif

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
static xsltStackElemPtr
xsltNewStackElem(void) {
    xsltStackElemPtr cur;

    cur = (xsltStackElemPtr) xmlMalloc(sizeof(xsltStackElem));
    if (cur == NULL) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltNewStackElem : malloc failed\n");
	return(NULL);
    }
    cur->computed = 0;
    cur->name = NULL;
    cur->nameURI = NULL;
    cur->select = NULL;
    cur->tree = NULL;
    cur->value = NULL;
    return(cur);
}

/**
 * xsltFreeStackElem:
 * @elem:  an XSLT stack element
 *
 * Free up the memory allocated by @elem
 */
static void
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
static int
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
static int
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
static xsltStackElemPtr
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
static int
xsltEvalVariable(xsltTransformContextPtr ctxt, xsltStackElemPtr elem,
	         xsltStylePreCompPtr precomp) {
    int oldProximityPosition, oldContextSize;
    if ((ctxt == NULL) || (elem == NULL))
	return(-1);

#ifdef WITH_XSLT_DEBUG_VARIABLE
    xsltGenericDebug(xsltGenericDebugContext,
	"Evaluating variable %s\n", elem->name);
#endif
    if (elem->select != NULL) {
	xmlXPathCompExprPtr comp = NULL;
	xmlXPathObjectPtr result;

	if (precomp != NULL) {
	    comp = precomp->comp;
	    if (comp == NULL) {
		comp = xmlXPathCompile(elem->select);
		precomp->comp = comp;
	    }
	} else {
	    comp = xmlXPathCompile(elem->select);
	}
	if (comp == NULL)
	    return(-1);
	oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
	oldContextSize = ctxt->xpathCtxt->contextSize;
	ctxt->xpathCtxt->node = (xmlNodePtr) ctxt->node;
	/* TODO: do we need to propagate the namespaces here ? */
	ctxt->xpathCtxt->namespaces = NULL;
	ctxt->xpathCtxt->nsNr = 0;
	result = xmlXPathCompiledEval(comp, ctxt->xpathCtxt);
	ctxt->xpathCtxt->contextSize = oldContextSize;
	ctxt->xpathCtxt->proximityPosition = oldProximityPosition;
	if (precomp == NULL)
	    xmlXPathFreeCompExpr(comp);
	if (result == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		"Evaluating variable %s failed\n", elem->name);
	} else {
#ifdef WITH_XSLT_DEBUG_VARIABLE
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
#ifdef WITH_XSLT_DEBUG_VARIABLE
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
 
#ifdef WITH_XSLT_DEBUG_VARIABLE
    xsltGenericDebug(xsltGenericDebugContext,
	"Evaluating global variables\n");
#endif
    ctxt->node = (xmlNodePtr) ctxt->document->doc;
    ctxt->xpathCtxt->contextSize = 1;
    ctxt->xpathCtxt->proximityPosition = 1;
    style = ctxt->style;
    while (style != NULL) {
	elem = style->variables;
	
	while (elem != NULL) {
	    if (elem->computed == 0)
		xsltEvalVariable(ctxt, elem, NULL);
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
 * @value:  the string value if available
 *
 * Register a new variable value. If @value is NULL it unregisters
 * the variable
 *
 * Returns 0 in case of success, -1 in case of error
 */
static int
xsltRegisterGlobalVariable(xsltStylesheetPtr style, const xmlChar *name,
		     const xmlChar *ns_uri, const xmlChar *select,
		     xmlNodePtr tree, int param, const xmlChar *value) {
    xsltStackElemPtr elem;
    if (style == NULL)
	return(-1);
    if (name == NULL)
	return(-1);

#ifdef WITH_XSLT_DEBUG_VARIABLE
    if (param)
	xsltGenericDebug(xsltGenericDebugContext,
			 "Defining global param %s\n", name);
    else
	xsltGenericDebug(xsltGenericDebugContext,
			 "Defining global variable %s\n", name);
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
    if (value != NULL) {
	elem->computed = 1;
	elem->value = xmlXPathNewString(value);
    }
    return(0);
}

/**
 * xsltEvalUserParams:
 * @ctxt:  the XSLT transformation context
 * @params:  a NULL terminated arry of parameters names/values tuples
 *
 * Evaluate the global variables of a stylesheet. This need to be
 * done on parsed stylesheets before starting to apply transformations
 *
 * Returns 0 in case of success, -1 in case of error
 */
int
xsltEvalUserParams(xsltTransformContextPtr ctxt, const char **params) {
    xsltStylesheetPtr style;
    int indx = 0;
    const xmlChar *name;
    const xmlChar *value;
    xmlChar *ncname, *prefix;

    if (ctxt == NULL)
	return(-1);
    if (params == NULL)
	return(0);
 
    style = ctxt->style;
    while (params[indx] != NULL) {
	name = (const xmlChar *)params[indx++];
	value = (const xmlChar *)params[indx++];
	if ((name == NULL) || (value == NULL))
	    break;

#ifdef WITH_XSLT_DEBUG_VARIABLE
	xsltGenericDebug(xsltGenericDebugContext,
	    "Evaluating user parameter %s=%s\n", name, value);
#endif
	ncname = xmlSplitQName2(name, &prefix);
	if (ncname != NULL) {
	    if (prefix != NULL) {
		xmlNsPtr ns;

		ns = xmlSearchNs(style->doc, xmlDocGetRootElement(style->doc),
			         prefix);
		if (ns == NULL) {
		    xsltGenericError(xsltGenericErrorContext,
			"user param : no namespace bound to prefix %s\n", prefix);
		} else {
		    xsltRegisterGlobalVariable(style, ncname, ns->href, NULL,
					       NULL, 1, value);
		}
		xmlFree(prefix);
	    } else {
		xsltRegisterGlobalVariable(style, ncname, NULL, NULL, NULL,
			                   1, value);
	    }
	    xmlFree(ncname);
	} else {
	    xsltRegisterGlobalVariable(style, name, NULL, NULL, NULL, 1, value);
	}

    }

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
static xsltStackElemPtr
xsltBuildVariable(xsltTransformContextPtr ctxt, xsltStylePreCompPtr comp,
		  xmlNodePtr tree, int param) {
    xsltStackElemPtr elem;

#ifdef WITH_XSLT_DEBUG_VARIABLE
    xsltGenericDebug(xsltGenericDebugContext,
		     "Building variable %s", comp->name);
    if (comp->select != NULL)
	xsltGenericDebug(xsltGenericDebugContext,
			 " select %s", comp->select);
    xsltGenericDebug(xsltGenericDebugContext, "\n");
#endif
    elem = xsltNewStackElem();
    if (elem == NULL)
	return(NULL);
    if (param)
	elem->type = XSLT_ELEM_PARAM;
    else
	elem->type = XSLT_ELEM_VARIABLE;
    elem->name = xmlStrdup(comp->name);
    if (comp->select != NULL)
	elem->select = xmlStrdup(comp->select);
    else
	elem->select = NULL;
    if (comp->ns)
	elem->nameURI = xmlStrdup(comp->ns);
    elem->tree = tree;
    xsltEvalVariable(ctxt, elem, comp);
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
static int
xsltRegisterVariable(xsltTransformContextPtr ctxt, xsltStylePreCompPtr comp,
		     xmlNodePtr tree, int param) {
    xsltStackElemPtr elem;

    if (xsltCheckStackElem(ctxt, comp->name, comp->ns) != 0) {
	if (!param) {
	    xsltGenericError(xsltGenericErrorContext,
	    "xsl:variable : redefining %s\n", comp->name);
	}
#ifdef WITH_XSLT_DEBUG_VARIABLE
	else
	    xsltGenericDebug(xsltGenericDebugContext,
		     "param %s defined by caller", comp->name);
#endif
	return(0);
    }
    elem = xsltBuildVariable(ctxt, comp, tree, param);
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
static xmlXPathObjectPtr
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
#ifdef WITH_XSLT_DEBUG_VARIABLE
	xsltGenericDebug(xsltGenericDebugContext,
		         "uncomputed global variable %s\n", name);
#endif
        xsltEvalVariable(ctxt, elem, NULL);
    }
    if (elem->value != NULL)
	return(xmlXPathObjectCopy(elem->value));
#ifdef WITH_XSLT_DEBUG_VARIABLE
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
#ifdef WITH_XSLT_DEBUG_VARIABLE
	xsltGenericDebug(xsltGenericDebugContext,
		         "uncomputed variable %s\n", name);
#endif
        xsltEvalVariable(ctxt, elem, NULL);
    }
    if (elem->value != NULL)
	return(xmlXPathObjectCopy(elem->value));
#ifdef WITH_XSLT_DEBUG_VARIABLE
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
    xmlNodePtr tree = NULL;
    xsltStackElemPtr elem = NULL;
    xsltStylePreCompPtr comp;

    if ((cur == NULL) || (ctxt == NULL))
	return(NULL);
    comp = (xsltStylePreCompPtr) cur->_private;
    if (comp == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsl:param : compilation error\n");
	return(NULL);
    }

    if (comp->name == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsl:param : missing name attribute\n");
	return(NULL);
    }
#ifdef WITH_XSLT_DEBUG_VARIABLE
    xsltGenericDebug(xsltGenericDebugContext,
	    "Handling param %s\n", comp->name);
#endif


    if (comp->select == NULL) {
	tree = cur->children;
    } else {
#ifdef WITH_XSLT_DEBUG_VARIABLE
	xsltGenericDebug(xsltGenericDebugContext,
	    "        select %s\n", comp->select);
#endif
	tree = cur;
    }

    elem = xsltBuildVariable(ctxt, comp, tree, 1);

    return(elem);
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
    xsltStylePreCompPtr comp;

    if ((cur == NULL) || (style == NULL))
	return;

    xsltStylePreCompute(style, cur);
    comp = (xsltStylePreCompPtr) cur->_private;
    if (comp == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsl:variable : compilation had failed\n");
	return;
    }

    if (comp->name == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsl:variable : missing name attribute\n");
	return;
    }

#ifdef WITH_XSLT_DEBUG_VARIABLE
    xsltGenericDebug(xsltGenericDebugContext,
	"Registering global variable %s\n", comp->name);
#endif

    xsltRegisterGlobalVariable(style, comp->name, comp->ns, comp->select,
	                       cur->children, 0, NULL);
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
    xsltStylePreCompPtr comp;

    if ((cur == NULL) || (style == NULL))
	return;

    xsltStylePreCompute(style, cur);
    comp = (xsltStylePreCompPtr) cur->_private;
    if (comp == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsl:param : compilation had failed\n");
	return;
    }

    if (comp->name == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsl:param : missing name attribute\n");
	return;
    }

#ifdef WITH_XSLT_DEBUG_VARIABLE
    xsltGenericDebug(xsltGenericDebugContext,
	"Registering global param %s\n", comp->name);
#endif

    xsltRegisterGlobalVariable(style, comp->name, comp->ns, comp->select,
	                       cur->children, 1, NULL);
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
    xsltStylePreCompPtr comp;

    if ((cur == NULL) || (ctxt == NULL))
	return;

    comp = (xsltStylePreCompPtr) cur->_private;
    if (comp == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsl:variable : compilation had failed\n");
	return;
    }

    if (comp->name == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsl:variable : missing name attribute\n");
	return;
    }

#ifdef WITH_XSLT_DEBUG_VARIABLE
    xsltGenericDebug(xsltGenericDebugContext,
	"Registering variable %s\n", comp->name);
#endif

    xsltRegisterVariable(ctxt, comp, cur->children, 0);
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
    xsltStylePreCompPtr comp;

    if ((cur == NULL) || (ctxt == NULL))
	return;

    comp = (xsltStylePreCompPtr) cur->_private;
    if (comp == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	     "xsl:param : compilation had failed\n");
	return;
    }

    if (comp->name == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsl:param : missing name attribute\n");
	return;
    }

#ifdef WITH_XSLT_DEBUG_VARIABLE
    xsltGenericDebug(xsltGenericDebugContext,
	"Registering param %s\n", comp->name);
#endif

    xsltRegisterVariable(ctxt, comp, cur->children, 1);
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

#ifdef WITH_XSLT_DEBUG_VARIABLE
    xsltGenericDebug(xsltGenericDebugContext,
	    "Lookup variable %s\n", name);
#endif
    context = (xsltTransformContextPtr) ctxt;
    ret = xsltVariableLookup(context, name, ns_uri);
    if (ret == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "unregistered variable %s\n", name);
    }
#ifdef WITH_XSLT_DEBUG_VARIABLE
    if (ret != NULL)
	xsltGenericDebug(xsltGenericDebugContext,
	    "found variable %s\n", name);
#endif
    return(ret);
}

