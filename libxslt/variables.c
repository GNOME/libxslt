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
#include <libxml/xpathInternals.h>
#include <libxml/parserInternals.h>
#include "xslt.h"
#include "xsltInternals.h"
#include "xsltutils.h"
#include "variables.h"

#define DEBUG_VARIABLE

/*
 * Types are private:
 */


typedef enum {
    XSLT_ELEM_VARIABLE=1,
    XSLT_ELEM_PARAM
} xsltElem;

typedef struct _xsltStackElem xsltStackElem;
typedef xsltStackElem *xsltStackElemPtr;
struct _xsltStackElem {
    struct _xsltStackElem *next;/* chained list */
    xsltElem elem;	/* type of the element */
    int computed;	/* was the evaluation done */
    xmlChar *name;	/* the local part of the name QName */
    xmlChar *nameURI;	/* the URI part of the name QName */
    xmlXPathObjectPtr value; /* The value if computed */
    xmlChar *select;	/* the eval string */
};

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
		if (nameURI == NULL) {
		    if (cur->nameURI == NULL)
			return(cur);
		} else {
		    if ((cur->nameURI != NULL) &&
			(xmlStrEqual(cur->nameURI, nameURI)))
			return(cur);
		}

	    }
	    cur = cur->next;
	}
    }
    return(NULL);
}

/************************************************************************
 *									*
 *			Module interfaces				*
 *									*
 ************************************************************************/

/**
 * xsltRegisterVariable:
 * @ctxt:  the XSLT transformation context
 * @name:  the variable name
 * @ns_uri:  the variable namespace URI
 * @select:  the expression which need to be evaluated to generate a value
 * @value:  the variable value if select is NULL
 *
 * Register a new variable value. If @value is NULL it unregisters
 * the variable
 *
 * Returns 0 in case of success, -1 in case of error
 */
int
xsltRegisterVariable(xsltTransformContextPtr ctxt, const xmlChar *name,
		     const xmlChar *ns_uri, const xmlChar *select,
		     xmlXPathObjectPtr value) {
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
    elem->name = xmlStrdup(name);
    elem->select = xmlStrdup(select);
    if (ns_uri)
	elem->nameURI = xmlStrdup(ns_uri);
    elem->value = value;
    xsltAddStackElem(ctxt, elem);
    if (elem->select != NULL) {
	xmlXPathObjectPtr result, tmp;
	xmlXPathParserContextPtr xpathParserCtxt;

	xpathParserCtxt = xmlXPathNewParserContext(elem->select,
						   ctxt->xpathCtxt);
	if (xpathParserCtxt == NULL)
	    goto error;
	ctxt->xpathCtxt->node = ctxt->node;
	xmlXPathEvalExpr(xpathParserCtxt);
	result = valuePop(xpathParserCtxt);
	do {
	    tmp = valuePop(xpathParserCtxt);
	    if (tmp != NULL) {
		xmlXPathFreeObject(tmp);
	    }
	} while (tmp != NULL);

	if (result == NULL) {
#ifdef DEBUG_PROCESS
	    xsltGenericDebug(xsltGenericDebugContext,
		"Evaluating variable %s failed\n");
#endif
	}
	if (xpathParserCtxt != NULL)
	    xmlXPathFreeParserContext(xpathParserCtxt);
	if (result != NULL) {
	    if (elem->value != NULL)
		xmlXPathFreeObject(elem->value);
	    elem->value = result;
	    elem->computed = 1;
	}
    }
error:
    return(0);
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
	TODO /* searching on other ctxtsheets ? */
	return(NULL);
    }
    if (!elem->computed) {
#ifdef DEBUG_VARIABLE
	xsltGenericDebug(xsltGenericDebugContext,
		         "uncomputed variable %s\n", name);
#endif
	    TODO /* Variable value computation needed */
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
 * xsltParseStylesheetVariable:
 * @ctxt:  the XSLT transformation context
 * @template:  the "variable" name
 *
 * parse an XSLT transformation context variable name and record
 * its value.
 */

void
xsltParseStylesheetVariable(xsltTransformContextPtr ctxt, xmlNodePtr cur) {
    xmlChar *name, *ncname, *prefix;
    xmlChar *select;
    xmlXPathObjectPtr value = NULL;

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
	if (cur->children == NULL)
	    value = xmlXPathNewCString("");
	else
	    value = xmlXPathNewNodeSet(cur->children);
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
		xsltRegisterVariable(ctxt, ncname, ns->href, select, value);
	    }
	    xmlFree(prefix);
	} else {
	    xsltRegisterVariable(ctxt, ncname, NULL, select, value);
	}
	xmlFree(ncname);
    } else {
	xsltRegisterVariable(ctxt, name, NULL, select, value);
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

