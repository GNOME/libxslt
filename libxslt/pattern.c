/*
 * pattern.c: Implemetation of the template match compilation and lookup
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
#include "xslt.h"
#include "xsltInternals.h"

#define DEBUG_PARSING

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
 * Types are private:
 */

typedef enum {
    XSLT_OP_END=0,
    XSLT_OP_ROOT,
    XSLT_OP_ELEM,
    XSLT_OP_CHILD,
    XSLT_OP_ATTR,
    XSLT_OP_PARENT,
    XSLT_OP_ANCESTOR,
    XSLT_OP_ID,
    XSLT_OP_KEY,
    XSLT_OP_NS,
    XSLT_OP_ALL,
    XSLT_OP_PREDICATE
} xsltOp;

typedef union _xsltStepOp xsltStepOp;
typedef xsltStepOp *xsltStepOpPtr;
union _xsltStepOp {
    xsltOp op;
    xmlChar *value;
};

typedef struct _xsltCompMatch xsltCompMatch;
typedef xsltCompMatch *xsltCompMatchPtr;
struct _xsltCompMatch {
    struct _xsltCompMatch *next; /* siblings in the name hash */
    int priority;                /* the priority */

    /* TODO fix the statically allocated size */
    int nbStep;
    int maxStep;
    xsltStepOp steps[20];        /* ops for computation */
};


/************************************************************************
 * 									*
 * 			Type functions 					*
 * 									*
 ************************************************************************/

/**
 * xsltNewCompMatch:
 *
 * Create a new XSLT CompMatch
 *
 * Returns the newly allocated xsltCompMatchPtr or NULL in case of error
 */
xsltCompMatchPtr
xsltNewCompMatch(void) {
    xsltCompMatchPtr cur;

    cur = (xsltCompMatchPtr) xmlMalloc(sizeof(xsltCompMatch));
    if (cur == NULL) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltNewCompMatch : malloc failed\n");
	return(NULL);
    }
    memset(cur, 0, sizeof(xsltCompMatch));
    cur->maxStep = 20;
    return(cur);
}

/**
 * xsltFreeCompMatch:
 * @comp:  an XSLT comp
 *
 * Free up the memory allocated by @comp
 */
void
xsltFreeCompMatch(xsltCompMatchPtr comp) {
    if (comp == NULL)
	return;
    memset(comp, -1, sizeof(xsltCompMatch));
    xmlFree(comp);
}

/**
 * xsltFreeCompMatchList:
 * @comp:  an XSLT comp list
 *
 * Free up the memory allocated by all the elements of @comp
 */
void
xsltFreeCompMatchList(xsltCompMatchPtr comp) {
    xsltCompMatchPtr cur;

    while (comp != NULL) {
	cur = comp;
	comp = comp->next;
	xsltFreeCompMatch(cur);
    }
}

/**
 * xsltCompMatchAddOp:
 * @comp:  the compiled match expression
 * @op:  an op
 *
 * Add an step to an XSLT Compiled Match
 *
 * Returns -1 in case of failure, 0 otherwise.
 */
int
xsltCompMatchAddOp(xsltCompMatchPtr comp, xsltOp op) {
    if (comp->nbStep >= 20) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltCompMatchAddOp: overflow\n");
        return(-1);
    }
    comp->steps[comp->nbStep++].op = op;
    return(0);
}

/**
 * xsltCompMatchAddValue:
 * @comp:  the compiled match expression
 * @val:  a name
 *
 * Add an step to an XSLT Compiled Match
 *
 * Returns -1 in case of failure, 0 otherwise.
 */
int
xsltCompMatchAddValue(xsltCompMatchPtr comp, xmlChar *val) {
    if (comp->nbStep >= 20) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltCompMatchAddOp: overflow\n");
        return(-1);
    }
    comp->steps[comp->nbStep++].value = val;
    return(0);
}

/**
 * xsltReverseCompMatch:
 * @comp:  the compiled match expression
 *
 * reverse all the stack of expressions
 */
void
xsltReverseCompMatch(xsltCompMatchPtr comp) {
    int i = 0;
    int j = comp->nbStep - 1;

    while (j > i) {
	register xmlChar *tmp;
	tmp = comp->steps[i].value;
	comp->steps[i].value = comp->steps[j].value;
	comp->steps[j].value = tmp;
	j--;
	i++;
    }
    comp->steps[comp->nbStep].op = XSLT_OP_END;
}

/************************************************************************
 *									*
 *			Dedicated parser for templates			*
 *									*
 ************************************************************************/

#define IS_BLANK(c) (((c) == 0x20) || ((c) == 0x09) || ((c) == 0xA) ||	\
                     ((c) == 0x0D))

#define SKIP_BLANKS while (IS_BLANK(*cur)) cur++;

#define CUR (*(cur))
#define NXT (*(cur + 1))
#define NEXT cur++

#define PUSH(comp, step) 						\
    if (xsltCompMatchAddOp((comp), (xsltOp) step)) goto error;

#define PUSHSTR(comp, step)						\
    if (xsltCompMatchAddValue((comp), (xmlChar *) step)) goto error;

/*
 * Compile the XSLT LocationPathPattern
 * [2] LocationPathPattern ::= '/' RelativePathPattern?
 *                           | IdKeyPattern (('/' | '//') RelativePathPattern)?
 *                           | '//'? RelativePathPattern
 * [3] IdKeyPattern ::= 'id' '(' Literal ')'
 *                    | 'key' '(' Literal ',' Literal ')'
 * [4] RelativePathPattern ::= StepPattern
 *                           | RelativePathPattern '/' StepPattern
 *                           | RelativePathPattern '//' StepPattern
 * [5] StepPattern ::= ChildOrAttributeAxisSpecifier NodeTest Predicate* 
 * [6] ChildOrAttributeAxisSpecifier ::= AbbreviatedAxisSpecifier
 *                                     | ('child' | 'attribute') '::'
 */

/**
 * xsltCompilePattern:
 * @pattern an XSLT pattern
 *
 * Compile the XSLT pattern and generates a precompiled form suitable
 * for fast matching.
 *
 * [1] Pattern ::= LocationPathPattern | Pattern '|' LocationPathPattern
 * Returns the generated xsltCompMatchPtr or NULL in case of failure
 */

xsltCompMatchPtr
xsltCompilePattern(const xmlChar *pattern) {
    xsltCompMatchPtr ret;
    const xmlChar *cur;

    if (pattern == NULL) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltCompilePattern : NULL pattern\n");
	return(NULL);
    }

#ifdef DEBUG_PARSING
    xsltGenericError(xsltGenericErrorContext,
	    "xsltCompilePattern : parsing '%s'\n", pattern);
#endif

    cur = pattern;
    SKIP_BLANKS;
    if (*cur == 0) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltCompilePattern : NULL pattern\n");
	return(NULL);
    }
    ret = xsltNewCompMatch();
    if (ret == NULL)
	return(NULL);

    if ((CUR == '/') && (NXT == '/')) {
    } else if (CUR == '/') {
	PUSH(ret, XSLT_OP_ROOT);
    }

    /*
     * Reverse for faster interpretation.
     */
    xsltReverseCompMatch(ret);

    return(ret);

error:
    xsltFreeCompMatch(ret);
    return(NULL);

}


/************************************************************************
 *									*
 *			Module interfaces				*
 *									*
 ************************************************************************/

/**
 * xsltAddTemplate:
 * @style: an XSLT stylesheet
 * @cur: an XSLT template
 *
 * Register the XSLT pattern associated to @cur
 *
 * Returns -1 in case of error, 0 otherwise
 */
int
xsltAddTemplate(xsltStylesheetPtr style, xsltTemplatePtr cur) {
    xsltCompMatchPtr pat, list;
    const xmlChar *name;

    /*
     * get a compiled form of the pattern
     */
    /* TODO : handle | in patterns as multple pat !!! */
    pat = xsltCompilePattern(cur->match);
    if (pat == NULL)
	return(-1);
    if (cur->priority != XSLT_PAT_NO_PRIORITY)
	pat->priority = cur->priority;

    /*
     * insert it in the hash table list corresponding to its lookup name
     */
    switch (pat->steps[0].op) {
        case XSLT_OP_ELEM:
        case XSLT_OP_CHILD:
        case XSLT_OP_ATTR:
        case XSLT_OP_PARENT:
        case XSLT_OP_ANCESTOR:
        case XSLT_OP_ID:
        case XSLT_OP_KEY:
        case XSLT_OP_NS:
             name = pat->steps[1].value;
	     break;
        case XSLT_OP_ROOT:
             name = (const xmlChar *) "/";
	     break;
        case XSLT_OP_ALL:
             name = (const xmlChar *) "*";
	     break;
        case XSLT_OP_END:
	case XSLT_OP_PREDICATE:
	    xsltGenericError(xsltGenericErrorContext,
		    "xsltAddTemplate: invalid compiled pattern\n");
	    xsltFreeCompMatch(pat);
	    return(-1);
    }
    if (style->templatesHash == NULL) {
	style->templatesHash = xmlHashCreate(0);
        if (style->templatesHash == NULL) {
	    xsltFreeCompMatch(pat);
	    return(-1);
	}
#ifdef DEBUG_PARSING
	xsltGenericError(xsltGenericErrorContext,
		"xsltAddTemplate: created template hash\n");
#endif
	xmlHashAddEntry(style->templatesHash, name, pat);
#ifdef DEBUG_PARSING
	xsltGenericError(xsltGenericErrorContext,
		"xsltAddTemplate: added new hash %s\n", name);
#endif
    } else {
	list = (xsltCompMatchPtr) xmlHashLookup(style->templatesHash, name);
	if (list == NULL) {
	    xmlHashAddEntry(style->templatesHash, name, pat);
#ifdef DEBUG_PARSING
	    xsltGenericError(xsltGenericErrorContext,
		    "xsltAddTemplate: added new hash %s\n", name);
#endif
	} else {
	    /*
	     * Note '<=' since one must choose among the matching template
	     * rules that are left, the one that occurs last in the stylesheet
	     */
	    if (list->priority <= pat->priority) {
		pat->next = list;
		xmlHashAddEntry(style->templatesHash, name, pat);
#ifdef DEBUG_PARSING
		xsltGenericError(xsltGenericErrorContext,
			"xsltAddTemplate: added head hash for %s\n", name);
#endif
	    } else {
		while (list->next != NULL) {
		    if (list->next->priority < pat->priority)
			break;
		}
		pat->next = list->next;
		list->next = pat;
	    }
	}
    }
    return(0);
}

/**
 * xsltAddTemplate:
 * @style: an XSLT stylesheet
 * @node: an XML Node
 *
 * Finds the template applying to this node
 *
 * Returns the xsltTemplatePtr or NULL if not found
 */
xsltTemplatePtr
xsltGetTemplate(xsltStylesheetPtr style, xmlNodePtr node) {
    return(NULL);
}

