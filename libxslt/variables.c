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
#include "xslt.h"
#include "xsltInternals.h"
#include "xsltutils.h"
#include "variables.h"
#include "transform.h"

#define DEBUG_VARIABLE

/*
 * Types are private:
 */

typedef struct _xsltStack xsltStack;
typedef xsltStack *xsltStackPtr;
struct _xsltStack {
    int cur;
    int max;
    xsltStackElemPtr elems[50];
};


/************************************************************************
 *									*
 *			Module interfaces				*
 *									*
 ************************************************************************/

/**
 * xsltNewParserContext:
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
    xsltStackPtr stack;
    xsltStackElemPtr cur;

    if ((ctxt == NULL) || (elem == NULL))
	return(-1);

    stack = ctxt->variablesHash;
    if (stack == NULL) {
	/* TODO make stack size dynamic !!! */
	stack = xmlMalloc(sizeof (xsltStack));
	if (stack == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		"xsltPushStack : malloc failed\n");
	    return(-1);
	}
	memset(stack, 0, sizeof(xsltStack));
	ctxt->variablesHash = stack;
	stack->cur = 0;
	stack->max = 50;
    }
    /* TODO: check that there is no conflict with existing values
     * at that level */
    cur = stack->elems[stack->cur];
    while (cur != NULL) {
	if (xmlStrEqual(elem->name, cur->name)) {
	    if (((elem->nameURI == NULL) && (cur->nameURI == NULL)) ||
		((elem->nameURI != NULL) && (cur->nameURI != NULL) &&
		 (xmlStrEqual(elem->nameURI, cur->nameURI)))) {
		xsltGenericError(xsltGenericErrorContext,
		    "redefinition of param or variable %s\n", elem->name);
		xsltFreeStackElem(elem);
	    }
	}
	cur = cur->next;
    }

    elem->next = stack->elems[stack->cur];
    stack->elems[stack->cur] = elem;
    return(0);
}

/**
 * xsltPushStack:
 * @ctxt:  xn XSLT transformation context
 *
 * Push a new level on the ctxtsheet interprestation stack
 */
void
xsltPushStack(xsltTransformContextPtr ctxt) {
    xsltStackPtr stack;

    if (ctxt == NULL)
	return;

    stack = ctxt->variablesHash;
    if (stack == NULL) {
	/* TODO make stack size dynamic !!! */
	stack = xmlMalloc(sizeof (xsltStack));
	if (stack == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		"xsltPushStack : malloc failed\n");
	    return;
	}
	memset(stack, 0, sizeof(xsltStack));
	ctxt->variablesHash = stack;
	stack->cur = 0;
	stack->max = 50;
    }
    if (stack->cur >= stack->max + 1) {
	TODO /* make stack size dynamic !!! */
	xsltGenericError(xsltGenericErrorContext,
	    "xsltPushStack : overflow\n");
	return;
    }
    stack->cur++;
    stack->elems[stack->cur] = NULL;
}

/**
 * xsltPopStack:
 * @ctxt:  an XSLT transformation context
 *
 * Pop a level on the ctxtsheet interprestation stack
 */
void
xsltPopStack(xsltTransformContextPtr ctxt) {
    xsltStackPtr stack;

    if (ctxt == NULL)
	return;

    stack = ctxt->variablesHash;
    if (stack == NULL)
	return;

    xsltFreeStackElemList(stack->elems[stack->cur]);
    stack->elems[stack->cur] = NULL;
    stack->cur--;
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
    xsltStackPtr stack;
    int i;
    xsltStackElemPtr cur;

    if ((ctxt == NULL) || (name == NULL))
	return(NULL);

    stack = ctxt->variablesHash;
    if (stack == NULL)
	return(NULL);

    for (i = stack->cur;i >= 0;i--) {
	cur = stack->elems[i];
	while (cur != NULL) {
	    if (xmlStrEqual(cur->name, name)) {
		/* TODO: double check param binding */
		if (nameURI == NULL) {
		    if (cur->nameURI == NULL) {
			if (cur->type == XSLT_ELEM_PARAM)
			    ret = cur;
			else
			    return(cur);
		    }
		} else {
		    if ((cur->nameURI != NULL) &&
			(xmlStrEqual(cur->nameURI, nameURI))) {
			if (cur->type == XSLT_ELEM_PARAM)
			    ret = cur;
			else
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
xsltEvalVariables(xsltTransformContextPtr ctxt, xsltStackElemPtr elem) {
    xmlXPathParserContextPtr xpathParserCtxt;

    if ((ctxt == NULL) || (elem == NULL))
	return(-1);

#ifdef DEBUG_VARIABLE
    xsltGenericDebug(xsltGenericDebugContext,
	"Evaluating variable %s\n", elem->name);
#endif
    if (elem->select != NULL) {
	xmlXPathObjectPtr result, tmp;

	xpathParserCtxt = xmlXPathNewParserContext(elem->select,
						   ctxt->xpathCtxt);
	if (xpathParserCtxt == NULL)
	    return(-1);
	ctxt->xpathCtxt->node = (xmlNodePtr) ctxt->node;
	xmlXPathEvalExpr(xpathParserCtxt);
	result = valuePop(xpathParserCtxt);
	do {
	    tmp = valuePop(xpathParserCtxt);
	    if (tmp != NULL) {
		xmlXPathFreeObject(tmp);
	    }
	} while (tmp != NULL);

	if (result == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		"Evaluating global variable %s failed\n");
	}
	if (xpathParserCtxt != NULL)
	    xmlXPathFreeParserContext(xpathParserCtxt);
	if (result != NULL) {
	    if (elem->value != NULL)
		xmlXPathFreeObject(elem->value);
	    elem->value = result;
	    elem->computed = 1;
	}
    } else {
	if (elem->tree == NULL) {
	    elem->value = xmlXPathNewCString("");
	    elem->computed = 1;
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

	    xsltApplyOneTemplate(ctxt, ctxt->node, elem->tree);

	    ctxt->insert = oldInsert;
	    ctxt->node = oldNode;
	    elem->value = xmlXPathNewValueTree(container);
	    elem->computed = 1;
	}
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
    ctxt->node = (xmlNodePtr) ctxt->doc;
    style = ctxt->style;
    /* TODO: handle the stylesheet cascade */
    if (style != NULL) {
	elem = style->variables;
	
	while (elem != NULL) {
	    xsltEvalVariables(ctxt, elem);
	    elem = elem->next;
	}
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
 * xsltRegisterVariable:
 * @ctxt:  the XSLT transformation context
 * @name:  the variable name
 * @ns_uri:  the variable namespace URI
 * @select:  the expression which need to be evaluated to generate a value
 * @tree:  the tree if select is NULL
 * @param:  this is a parameter actually
 *
 * Register a new variable value. If @value is NULL it unregisters
 * the variable
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

#ifdef DEBUG_VARIABLE
    xsltGenericDebug(xsltGenericDebugContext,
		     "Defineing variable %s\n", name);
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
    xsltAddStackElem(ctxt, elem);
    xsltEvalVariables(ctxt, elem);
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
    xsltStackElemPtr elem;

    style = ctxt->style;
    /* TODO: handle the stylesheet cascade */
    if (style != NULL) {
	elem = style->variables;
	
	while (elem != NULL) {
	    if (xmlStrEqual(elem->name, name)) {
		/* TODO: double check param binding */
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
    }
    if (elem == NULL)
	return(NULL);

    if (!elem->computed) {
#ifdef DEBUG_VARIABLE
	xsltGenericDebug(xsltGenericDebugContext,
		         "uncomputed global variable %s\n", name);
#endif
        xsltEvalVariables(ctxt, elem);
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
        xsltEvalVariables(ctxt, elem);
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
 * xsltFreeVariableHashes:
 * @ctxt: an XSLT transformation context
 *
 * Free up the memory used by xsltAddVariable/xsltGetVariable mechanism
 */
void
xsltFreeVariableHashes(xsltTransformContextPtr ctxt) {
    xsltStackPtr stack;
    int i;

    if (ctxt == NULL)
	return;

    stack = ctxt->variablesHash;
    if (stack == NULL)
	return;

    for (i = 0; i <= stack->cur;i++) {
	xsltFreeStackElemList(stack->elems[i]);
    }
    xmlFree(stack);
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

