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
#include <libxml/parserInternals.h>
#include "xslt.h"
#include "xsltInternals.h"

/* #define DEBUG_PARSING */

#define TODO 								\
    xsltGenericError(xsltGenericErrorContext,				\
	    "Unimplemented block at %s:%d\n",				\
            __FILE__, __LINE__);

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
    XSLT_OP_PI,
    XSLT_OP_COMMENT,
    XSLT_OP_TEXT,
    XSLT_OP_NODE,
    XSLT_OP_PREDICATE
} xsltOp;


typedef struct _xsltStepOp xsltStepOp;
typedef xsltStepOp *xsltStepOpPtr;
struct _xsltStepOp {
    xsltOp op;
    xmlChar *value;
    xmlChar *value2;
};

typedef struct _xsltCompMatch xsltCompMatch;
typedef xsltCompMatch *xsltCompMatchPtr;
struct _xsltCompMatch {
    struct _xsltCompMatch *next; /* siblings in the name hash */
    float priority;                /* the priority */
    xsltTemplatePtr template;    /* the associated template */

    /* TODO fix the statically allocated size steps[] */
    int nbStep;
    int maxStep;
    xsltStepOp steps[20];        /* ops for computation */
};

typedef struct _xsltParserContext xsltParserContext;
typedef xsltParserContext *xsltParserContextPtr;
struct _xsltParserContext {
    const xmlChar *cur;			/* the current char being parsed */
    const xmlChar *base;		/* the full expression */
    int error;				/* error code */
    xsltCompMatchPtr comp;		/* the result */
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
    xsltStepOpPtr op;
    int i;

    if (comp == NULL)
	return;
    for (i = 0;i < comp->nbStep;i++) {
	op = &comp->steps[i];
	if (op->value != NULL)
	    xmlFree(op->value);
	if (op->value2 != NULL)
	    xmlFree(op->value2);
    }
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
 * xsltNewParserContext:
 *
 * Create a new XSLT ParserContext
 *
 * Returns the newly allocated xsltParserContextPtr or NULL in case of error
 */
xsltParserContextPtr
xsltNewParserContext(void) {
    xsltParserContextPtr cur;

    cur = (xsltParserContextPtr) xmlMalloc(sizeof(xsltParserContext));
    if (cur == NULL) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltNewParserContext : malloc failed\n");
	return(NULL);
    }
    memset(cur, 0, sizeof(xsltParserContext));
    return(cur);
}

/**
 * xsltFreeParserContext:
 * @ctxt:  an XSLT parser context
 *
 * Free up the memory allocated by @ctxt
 */
void
xsltFreeParserContext(xsltParserContextPtr ctxt) {
    if (ctxt == NULL)
	return;
    memset(ctxt, -1, sizeof(xsltParserContext));
    xmlFree(ctxt);
}

/**
 * xsltCompMatchAdd:
 * @comp:  the compiled match expression
 * @op:  an op
 * @value:  the first value
 * @value2:  the second value
 *
 * Add an step to an XSLT Compiled Match
 *
 * Returns -1 in case of failure, 0 otherwise.
 */
int
xsltCompMatchAdd(xsltCompMatchPtr comp, xsltOp op, xmlChar *value,
	           xmlChar *value2) {
    if (comp->nbStep >= 20) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltCompMatchAddOp: overflow\n");
        return(-1);
    }
    comp->steps[comp->nbStep].op = op;
    comp->steps[comp->nbStep].value = value;
    comp->steps[comp->nbStep].value2 = value2;
    comp->nbStep++;
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
	register xsltOp op;
	tmp = comp->steps[i].value;
	comp->steps[i].value = comp->steps[j].value;
	comp->steps[j].value = tmp;
	tmp = comp->steps[i].value2;
	comp->steps[i].value2 = comp->steps[j].value2;
	comp->steps[j].value2 = tmp;
	op = comp->steps[i].op;
	comp->steps[i].op = comp->steps[j].op;
	comp->steps[j].op = op;
	j--;
	i++;
    }
    comp->steps[comp->nbStep++].op = XSLT_OP_END;
}

/************************************************************************
 * 									*
 * 		The interpreter for the precompiled patterns		*
 * 									*
 ************************************************************************/

/**
 * xsltTestCompMatch:
 * @comp: the precompiled pattern
 * @node: a node
 *
 * Test wether the node matches the pattern
 *
 * Returns 1 if it matches, 0 if it doesn't and -1 in case of failure
 */
int
xsltTestCompMatch(xsltCompMatchPtr comp, xmlNodePtr node) {
    int i;
    xsltStepOpPtr step;

    if ((comp == NULL) || (node == NULL)) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltTestCompMatch: null arg\n");
        return(-1);
    }
    for (i = 0;i < comp->nbStep;i++) {
	step = &comp->steps[i];
	switch (step->op) {
            case XSLT_OP_END:
		return(1);
            case XSLT_OP_ROOT:
		if ((node->type != XML_DOCUMENT_NODE) &&
		    (node->type != XML_HTML_DOCUMENT_NODE))
		    return(0);
		continue;
            case XSLT_OP_ELEM:
		if (node->type != XML_ELEMENT_NODE)
		    return(0);
		if (step->value == NULL)
		    continue;
		if (!xmlStrEqual(step->value, node->name))
		    return(0);
		/* TODO: Handle namespace ... */
		continue;
            case XSLT_OP_CHILD:
		TODO /* Handle OP_CHILD */
		return(0);
            case XSLT_OP_ATTR:
		if (node->type != XML_ATTRIBUTE_NODE)
		    return(0);
		if (step->value == NULL)
		    continue;
		if (!xmlStrEqual(step->value, node->name))
		    return(0);
		/* TODO: Handle namespace ... */
		continue;
            case XSLT_OP_PARENT:
		node = node->parent;
		if (node == NULL)
		    return(0);
		if (step->value == NULL)
		    continue;
		if (!xmlStrEqual(step->value, node->name))
		    return(0);
		/* TODO: Handle namespace ... */
		continue;
            case XSLT_OP_ANCESTOR:
		/* TODO: implement coalescing of ANCESTOR/NODE ops */
		if (step->value == NULL) {
		    i++;
		    step = &comp->steps[i];
		    if (step->op == XSLT_OP_ROOT)
			return(1);
		    if (step->op != XSLT_OP_ELEM)
			return(0);
		    if (step->value == NULL)
			return(-1);
		}
		if (node == NULL)
		    return(0);
		node = node->parent;
		while (node != NULL) {
		    if (node == NULL)
			return(0);
		    if (xmlStrEqual(step->value, node->name)) {
			/* TODO: Handle namespace ... */
			break;
		    }
		}
		if (node == NULL)
		    return(0);
		continue;
            case XSLT_OP_ID:
		TODO /* Handle IDs, might be done differently */
		break;
            case XSLT_OP_KEY:
		TODO /* Handle Keys, might be done differently */
		break;
            case XSLT_OP_NS:
		TODO /* Handle Namespace */
		break;
            case XSLT_OP_ALL:
		TODO /* Handle * */
		break;
	    case XSLT_OP_PREDICATE:
		TODO /* Handle Predicate */
		break;
            case XSLT_OP_PI:
		TODO /* Handle PI() */
		break;
            case XSLT_OP_COMMENT:
		TODO /* Handle Comments() */
		break;
            case XSLT_OP_TEXT:
		TODO /* Handle Text() */
		break;
            case XSLT_OP_NODE:
		TODO /* Handle Node() */
		break;
	}
    }
    return(1);
}

/************************************************************************
 *									*
 *			Dedicated parser for templates			*
 *									*
 ************************************************************************/

#define CUR (*ctxt->cur)
#define SKIP(val) ctxt->cur += (val)
#define NXT(val) ctxt->cur[(val)]
#define CUR_PTR ctxt->cur

#define SKIP_BLANKS 							\
    while (IS_BLANK(CUR)) NEXT

#define CURRENT (*ctxt->cur)
#define NEXT ((*ctxt->cur) ?  ctxt->cur++: ctxt->cur)


#define PUSH(op, val, val2) 						\
    if (xsltCompMatchAdd(ctxt->comp, (op), (val), (val2))) goto error;

#define XSLT_ERROR(X)							\
    { xsltError(ctxt, __FILE__, __LINE__, X);			\
      ctxt->error = (X); return; }

#define XSLT_ERROR0(X)							\
    { xsltError(ctxt, __FILE__, __LINE__, X);			\
      ctxt->error = (X); return(0); }

/**
 * xsltScanLiteral:
 * @ctxt:  the XPath Parser context
 *
 * Parse an XPath Litteral:
 *
 * [29] Literal ::= '"' [^"]* '"'
 *                | "'" [^']* "'"
 *
 * Returns the Literal parsed or NULL
 */

xmlChar *
xsltScanLiteral(xsltParserContextPtr ctxt) {
    const xmlChar *q;
    xmlChar *ret = NULL;

    SKIP_BLANKS;
    if (CUR == '"') {
        NEXT;
	q = CUR_PTR;
	while ((IS_CHAR(CUR)) && (CUR != '"'))
	    NEXT;
	if (!IS_CHAR(CUR)) {
	    /* XP_ERROR(XPATH_UNFINISHED_LITERAL_ERROR); */
	    ctxt->error = 1;
	    return(NULL);
	} else {
	    ret = xmlStrndup(q, CUR_PTR - q);
	    NEXT;
        }
    } else if (CUR == '\'') {
        NEXT;
	q = CUR_PTR;
	while ((IS_CHAR(CUR)) && (CUR != '\''))
	    NEXT;
	if (!IS_CHAR(CUR)) {
	    /* XP_ERROR(XPATH_UNFINISHED_LITERAL_ERROR); */
	    ctxt->error = 1;
	    return(NULL);
	} else {
	    ret = xmlStrndup(q, CUR_PTR - q);
	    NEXT;
        }
    } else {
	/* XP_ERROR(XPATH_START_LITERAL_ERROR); */
	ctxt->error = 1;
	return(NULL);
    }
    return(ret);
}

/**
 * xsltScanName:
 * @ctxt:  the XPath Parser context
 *
 * Trickery: parse an XML name but without consuming the input flow
 * Needed to avoid insanity in the parser state.
 *
 * [4] NameChar ::= Letter | Digit | '.' | '-' | '_' | ':' |
 *                  CombiningChar | Extender
 *
 * [5] Name ::= (Letter | '_' | ':') (NameChar)*
 *
 * [6] Names ::= Name (S Name)*
 *
 * Returns the Name parsed or NULL
 */

xmlChar *
xsltScanName(xsltParserContextPtr ctxt) {
    xmlChar buf[XML_MAX_NAMELEN];
    int len = 0;

    SKIP_BLANKS;
    if (!IS_LETTER(CUR) && (CUR != '_') &&
        (CUR != ':')) {
	return(NULL);
    }

    while ((IS_LETTER(NXT(len))) || (IS_DIGIT(NXT(len))) ||
           (NXT(len) == '.') || (NXT(len) == '-') ||
	   (NXT(len) == '_') || (NXT(len) == ':') || 
	   (IS_COMBINING(NXT(len))) ||
	   (IS_EXTENDER(NXT(len)))) {
	buf[len] = NXT(len);
	len++;
	if (len >= XML_MAX_NAMELEN) {
	    xmlGenericError(xmlGenericErrorContext, 
	       "xmlScanName: reached XML_MAX_NAMELEN limit\n");
	    while ((IS_LETTER(NXT(len))) || (IS_DIGIT(NXT(len))) ||
		   (NXT(len) == '.') || (NXT(len) == '-') ||
		   (NXT(len) == '_') || (NXT(len) == ':') || 
		   (IS_COMBINING(NXT(len))) ||
		   (IS_EXTENDER(NXT(len))))
		 len++;
	    break;
	}
    }
    SKIP(len);
    return(xmlStrndup(buf, len));
}
/*
 * xsltCompileIdKeyPattern:
 * @comp:  the compilation context
 * @name:  a preparsed name
 * @aid:  whether id/key are allowed there
 *
 * Compile the XSLT LocationIdKeyPattern
 * [3] IdKeyPattern ::= 'id' '(' Literal ')'
 *                    | 'key' '(' Literal ',' Literal ')'
 *
 * also handle NodeType and PI from:
 *
 * [7]  NodeTest ::= NameTest
 *                 | NodeType '(' ')'
 *                 | 'processing-instruction' '(' Literal ')'
 */
void
xsltCompileIdKeyPattern(xsltParserContextPtr ctxt, xmlChar *name, int aid) {
    xmlChar *lit = NULL;
    xmlChar *lit2 = NULL;

    if (CUR != '(') {
	xsltGenericError(xsltGenericErrorContext,
		"xsltCompileIdKeyPattern : ( expected\n");
	ctxt->error = 1;
	return;
    }
    if ((aid) && (xmlStrEqual(name, (const xmlChar *)"id"))) {
	NEXT;
	SKIP_BLANKS;
        lit = xsltScanLiteral(ctxt);
	if (ctxt->error)
	    return;
	SKIP_BLANKS;
	if (CUR != ')') {
	    xsltGenericError(xsltGenericErrorContext,
		    "xsltCompileIdKeyPattern : ) expected\n");
	    ctxt->error = 1;
	    return;
	}
	NEXT;
	PUSH(XSLT_OP_ID, lit, NULL);
    } else if ((aid) && (xmlStrEqual(name, (const xmlChar *)"key"))) {
	NEXT;
	SKIP_BLANKS;
        lit = xsltScanLiteral(ctxt);
	if (ctxt->error)
	    return;
	SKIP_BLANKS;
	if (CUR != ',') {
	    xsltGenericError(xsltGenericErrorContext,
		    "xsltCompileIdKeyPattern : , expected\n");
	    ctxt->error = 1;
	    return;
	}
	NEXT;
	SKIP_BLANKS;
        lit2 = xsltScanLiteral(ctxt);
	if (ctxt->error)
	    return;
	SKIP_BLANKS;
	if (CUR != ')') {
	    xsltGenericError(xsltGenericErrorContext,
		    "xsltCompileIdKeyPattern : ) expected\n");
	    ctxt->error = 1;
	    return;
	}
	NEXT;
	PUSH(XSLT_OP_KEY, lit, NULL);
    } else if (xmlStrEqual(name, (const xmlChar *)"processing-instruction")) {
	NEXT;
	SKIP_BLANKS;
	if (CUR != ')') {
	    lit = xsltScanLiteral(ctxt);
	    if (ctxt->error)
		return;
	    SKIP_BLANKS;
	    if (CUR != ')') {
		xsltGenericError(xsltGenericErrorContext,
			"xsltCompileIdKeyPattern : ) expected\n");
		ctxt->error = 1;
		return;
	    }
	}
	NEXT;
	PUSH(XSLT_OP_PI, lit, NULL);
    } else if (xmlStrEqual(name, (const xmlChar *)"text")) {
	NEXT;
	SKIP_BLANKS;
	if (CUR != ')') {
	    xsltGenericError(xsltGenericErrorContext,
		    "xsltCompileIdKeyPattern : ) expected\n");
	    ctxt->error = 1;
	    return;
	}
	NEXT;
	PUSH(XSLT_OP_TEXT, NULL, NULL);
    } else if (xmlStrEqual(name, (const xmlChar *)"comment")) {
	NEXT;
	SKIP_BLANKS;
	if (CUR != ')') {
	    xsltGenericError(xsltGenericErrorContext,
		    "xsltCompileIdKeyPattern : ) expected\n");
	    ctxt->error = 1;
	    return;
	}
	NEXT;
	PUSH(XSLT_OP_COMMENT, NULL, NULL);
    } else if (xmlStrEqual(name, (const xmlChar *)"comment")) {
	NEXT;
	SKIP_BLANKS;
	if (CUR != ')') {
	    xsltGenericError(xsltGenericErrorContext,
		    "xsltCompileIdKeyPattern : ) expected\n");
	    ctxt->error = 1;
	    return;
	}
	NEXT;
	PUSH(XSLT_OP_NODE, NULL, NULL);
    } else if (aid) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsltCompileIdKeyPattern : expecting 'key' or 'id' or node type\n");
	ctxt->error = 1;
	return;
    } else {
	xsltGenericError(xsltGenericErrorContext,
	    "xsltCompileIdKeyPattern : node type\n");
	ctxt->error = 1;
	return;
    }
error:
    if (lit != NULL)
	xmlFree(lit);
    if (lit2 != NULL)
	xmlFree(lit2);
}

/**
 * xsltCompileStepPattern:
 * @comp:  the compilation context
 * @token:  a posible precompiled name
 *
 * Compile the XSLT StepPattern and generates a precompiled
 * form suitable for fast matching.
 *
 * [5] StepPattern ::= ChildOrAttributeAxisSpecifier NodeTest Predicate* 
 * [6] ChildOrAttributeAxisSpecifier ::= AbbreviatedAxisSpecifier
 *                                     | ('child' | 'attribute') '::'
 * from XPath
 * [7]  NodeTest ::= NameTest
 *                 | NodeType '(' ')'
 *                 | 'processing-instruction' '(' Literal ')'
 * [8] Predicate ::= '[' PredicateExpr ']'
 * [9] PredicateExpr ::= Expr
 * [13] AbbreviatedAxisSpecifier ::= '@'?
 * [37] NameTest ::= '*' | NCName ':' '*' | QName
 */

void
xsltCompileStepPattern(xsltParserContextPtr ctxt, xmlChar *token) {
    xmlChar *name = NULL;

    SKIP_BLANKS;
    if ((token == NULL) && (CUR == '@')) {
	token = xsltScanName(ctxt);
	if (token == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		    "xsltCompilePattern : Name expected\n");
	    ctxt->error = 1;
	    goto error;
	}
	PUSH(XSLT_OP_ATTR, token, NULL);
	return;
    }
    if (token == NULL)
	token = xsltScanName(ctxt);
    if (token == NULL) {
	xsltGenericError(xsltGenericErrorContext,
		"xsltCompilePattern : Name expected\n");
        ctxt->error = 1;
	goto error;
    }
    SKIP_BLANKS;
    if (CUR == '(') {
	xsltCompileIdKeyPattern(ctxt, token, 0);
	if (ctxt->error)
	    goto error;
    } else if (CUR == ':') {
	NEXT;
	if (NXT(1) != ':') {
	    xsltGenericError(xsltGenericErrorContext,
		    "xsltCompilePattern : sequence '::' expected\n");
	    ctxt->error = 1;
	    goto error;
	}
	NEXT;
	if (xmlStrEqual(token, (const xmlChar *) "child")) {
	    /* TODO: handle namespace */
	    name = xsltScanName(ctxt);
	    if (name == NULL) {
		xsltGenericError(xsltGenericErrorContext,
			"xsltCompilePattern : QName expected\n");
		ctxt->error = 1;
		goto error;
	    }
	    PUSH(XSLT_OP_CHILD, name, NULL);
	} else if (xmlStrEqual(token, (const xmlChar *) "attribute")) {
	    /* TODO: handle namespace */
	    name = xsltScanName(ctxt);
	    if (name == NULL) {
		xsltGenericError(xsltGenericErrorContext,
			"xsltCompilePattern : QName expected\n");
		ctxt->error = 1;
		goto error;
	    }
	    PUSH(XSLT_OP_ATTR, name, NULL);
	} else {
	    xsltGenericError(xsltGenericErrorContext,
		    "xsltCompilePattern : 'child' or 'attribute' expected\n");
	    ctxt->error = 1;
	    goto error;
	}
	xmlFree(token);
    } else if (CUR == '*') {
	NEXT;
	PUSH(XSLT_OP_ALL, token, NULL);
    } else {
	/* TODO: handle namespace */
	PUSH(XSLT_OP_ELEM, token, NULL);
    }
    SKIP_BLANKS;
    while (CUR == '[') {
	const xmlChar *q;
	xmlChar *ret = NULL;

	NEXT;
	q = CUR_PTR;
	/* TODO: avoid breaking in strings ... */
	while ((IS_CHAR(CUR)) && (CUR != ']'))
	    NEXT;
	if (!IS_CHAR(CUR)) {
	    xsltGenericError(xsltGenericErrorContext,
		    "xsltCompilePattern : ']' expected\n");
	    ctxt->error = 1;
	    goto error;
        }
	NEXT;
	ret = xmlStrndup(q, CUR_PTR - q);
	PUSH(XSLT_OP_PREDICATE, ret, NULL);
    }
    return;
error:
    if (token != NULL)
	xmlFree(token);
    if (name != NULL)
	xmlFree(name);
}

/**
 * xsltCompileRelativePathPattern:
 * @comp:  the compilation context
 * @token:  a posible precompiled name
 *
 * Compile the XSLT RelativePathPattern and generates a precompiled
 * form suitable for fast matching.
 *
 * [4] RelativePathPattern ::= StepPattern
 *                           | RelativePathPattern '/' StepPattern
 *                           | RelativePathPattern '//' StepPattern
 */
void
xsltCompileRelativePathPattern(xsltParserContextPtr ctxt, xmlChar *token) {
    xsltCompileStepPattern(ctxt, token);
    if (ctxt->error)
	goto error;
    SKIP_BLANKS;
    while ((CUR != 0) && (CUR != '|')) {
	if ((CUR == '/') && (NXT(1) == '/')) {
	    PUSH(XSLT_OP_ANCESTOR, NULL, NULL);
	    NEXT;
	    NEXT;
	    SKIP_BLANKS;
	    xsltCompileStepPattern(ctxt, NULL);
	} else if (CUR == '/') {
	    PUSH(XSLT_OP_PARENT, NULL, NULL);
	    NEXT;
	    SKIP_BLANKS;
	    if ((CUR != 0) || (CUR == '|')) {
		xsltCompileRelativePathPattern(ctxt, NULL);
	    }
	} else {
	    ctxt->error = 1;
	}
	if (ctxt->error)
	    goto error;
	SKIP_BLANKS;
    }
error:
    return;
}

/**
 * xsltCompileLocationPathPattern:
 * @comp:  the compilation context
 *
 * Compile the XSLT LocationPathPattern and generates a precompiled
 * form suitable for fast matching.
 *
 * [2] LocationPathPattern ::= '/' RelativePathPattern?
 *                           | IdKeyPattern (('/' | '//') RelativePathPattern)?
 *                           | '//'? RelativePathPattern
 */
void
xsltCompileLocationPathPattern(xsltParserContextPtr ctxt) {
    SKIP_BLANKS;
    if ((CUR == '/') && (NXT(1) == '/')) {
	/*
	 * since we reverse the query
	 * a leading // can be safely ignored
	 */
	NEXT;
	NEXT;
	xsltCompileRelativePathPattern(ctxt, NULL);
    } else if (CUR == '/') {
	/*
	 * We need to find root as the parent
	 */
	NEXT;
	SKIP_BLANKS;
	PUSH(XSLT_OP_ROOT, NULL, NULL);
	if ((CUR != 0) || (CUR == '|')) {
	    PUSH(XSLT_OP_PARENT, NULL, NULL);
	    xsltCompileRelativePathPattern(ctxt, NULL);
	}
    } else {
	xmlChar *name;
	name = xsltScanName(ctxt);
	if (name == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		    "xsltCompilePattern : Name expected\n");
	    ctxt->error = 1;
	    return;
	}
	SKIP_BLANKS;
	if (CUR == '(') {
	    xsltCompileIdKeyPattern(ctxt, name, 1);
	    if ((CUR == '/') && (NXT(1) == '/')) {
		PUSH(XSLT_OP_ANCESTOR, NULL, NULL);
		NEXT;
		NEXT;
		SKIP_BLANKS;
		xsltCompileRelativePathPattern(ctxt, NULL);
	    } else if (CUR == '/') {
		PUSH(XSLT_OP_PARENT, NULL, NULL);
		NEXT;
		SKIP_BLANKS;
		xsltCompileRelativePathPattern(ctxt, NULL);
	    }
	    return;
	}
	xsltCompileRelativePathPattern(ctxt, name);
    }
error:
    return;
}

/**
 * xsltCompilePattern:
 * @pattern an XSLT pattern
 *
 * Compile the XSLT pattern and generates a precompiled form suitable
 * for fast matching.
 * Note that the splitting as union of patterns is expected to be handled
 * by the caller
 *
 * [1] Pattern ::= LocationPathPattern | Pattern '|' LocationPathPattern
 *
 * Returns the generated xsltCompMatchPtr or NULL in case of failure
 */

xsltCompMatchPtr
xsltCompilePattern(const xmlChar *pattern) {
    xsltParserContextPtr ctxt;
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
    while (IS_BLANK(*cur)) cur++;
    if (*cur == 0) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltCompilePattern : NULL pattern\n");
	return(NULL);
    }
    ctxt = xsltNewParserContext();
    if (ctxt == NULL)
	return(NULL);
    ret = xsltNewCompMatch();
    if (ret == NULL) {
	xsltFreeParserContext(ctxt);
	return(NULL);
    }

    ctxt->comp = ret;
    ctxt->base = pattern;
    ctxt->cur = cur;
    xsltCompileLocationPathPattern(ctxt);
    if (ctxt->error)
	goto error;

    /*
     * Reverse for faster interpretation.
     */
    xsltReverseCompMatch(ret);

    /*
     * Set-up the priority
     */
    if (((ret->steps[0].op == XSLT_OP_ELEM) ||
	 (ret->steps[0].op == XSLT_OP_ATTR)) &&
	(ret->steps[0].value != NULL) &&
	(ret->steps[1].op == XSLT_OP_END)) {
	ret->priority = 0;
    } else if ((ret->steps[0].op == XSLT_OP_PI) &&
	       (ret->steps[0].value != NULL) &&
	       (ret->steps[1].op == XSLT_OP_END)) {
	ret->priority = 0;
    } else if ((ret->steps[0].op == XSLT_OP_NS) &&
	       (ret->steps[0].value != NULL) &&
	       (ret->steps[1].op == XSLT_OP_END)) {
	ret->priority = -0.25;
    } else if (((ret->steps[0].op == XSLT_OP_PI) ||
		(ret->steps[0].op == XSLT_OP_TEXT) ||
		(ret->steps[0].op == XSLT_OP_NODE) ||
	        (ret->steps[0].op == XSLT_OP_COMMENT)) &&
	       (ret->steps[1].op == XSLT_OP_END)) {
	ret->priority = -0.5;
    } else {
	ret->priority = 0.5;
    }

    xsltFreeParserContext(ctxt);
    return(ret);

error:
    xsltFreeParserContext(ctxt);
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
    pat->template = cur;
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
             name = pat->steps[0].value;
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
	/*
	 * TODO: some flags at the top level about type based patterns
	 *       would be faster than inclusion in the hash table.
	 */
	case XSLT_OP_PI:
	    name = (const xmlChar *) "processing-instruction()";
	    break;
	case XSLT_OP_COMMENT:
	    name = (const xmlChar *) "comment()";
	    break;
	case XSLT_OP_TEXT:
	    name = (const xmlChar *) "text()";
	    break;
	case XSLT_OP_NODE:
	    name = (const xmlChar *) "node()";
	    break;
    }
    if (name == NULL) {
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
		xmlHashUpdateEntry(style->templatesHash, name, pat, NULL);
#ifdef DEBUG_PARSING
		xsltGenericError(xsltGenericErrorContext,
			"xsltAddTemplate: added head hash for %s\n", name);
#endif
	    } else {
		while (list->next != NULL) {
		    if (list->next->priority <= pat->priority)
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
 * xsltGetTemplate:
 * @style: an XSLT stylesheet
 * @node: an XML Node
 *
 * Finds the template applying to this node
 *
 * Returns the xsltTemplatePtr or NULL if not found
 */
xsltTemplatePtr
xsltGetTemplate(xsltStylesheetPtr style, xmlNodePtr node) {
    const xmlChar *name;
    xsltCompMatchPtr list;

    if ((style == NULL) || (node == NULL))
	return(NULL);

    /* TODO : handle IDs/keys here ! */
    if (style->templatesHash == NULL)
	return(NULL);

    /*
     * Use a name as selector
     */
    switch (node->type) {
        case XML_ELEMENT_NODE:
        case XML_ATTRIBUTE_NODE:
        case XML_PI_NODE:
	    name = node->name;
	    break;
        case XML_DOCUMENT_NODE:
        case XML_HTML_DOCUMENT_NODE:
	    name = (const xmlChar *)"/";
	    break;
        case XML_TEXT_NODE:
        case XML_CDATA_SECTION_NODE:
        case XML_ENTITY_REF_NODE:
        case XML_ENTITY_NODE:
        case XML_COMMENT_NODE:
        case XML_DOCUMENT_TYPE_NODE:
        case XML_DOCUMENT_FRAG_NODE:
        case XML_NOTATION_NODE:
        case XML_DTD_NODE:
        case XML_ELEMENT_DECL:
        case XML_ATTRIBUTE_DECL:
        case XML_ENTITY_DECL:
        case XML_NAMESPACE_DECL:
        case XML_XINCLUDE_START:
        case XML_XINCLUDE_END:
	    return(NULL);
	default:
	    return(NULL);

    }
    if (name == NULL)
	return(NULL);

    /*
     * find the list of appliable expressions based on the name
     */
    list = (xsltCompMatchPtr) xmlHashLookup(style->templatesHash, name);
    if (list == NULL) {
#ifdef DEBUG_MATCHING
	xsltGenericError(xsltGenericErrorContext,
		"xsltGetTemplate: empty set for %s\n", name);
#endif
	return(NULL);
    }
    while (list != NULL) {
	if (xsltTestCompMatch(list, node))
	    return(list->template);
	list = list->next;
    }

    return(NULL);
}


/**
 * xsltFreeTemplateHashes:
 * @style: an XSLT stylesheet
 *
 * Free up the memory used by xsltAddTemplate/xsltGetTemplate mechanism
 */
void
xsltFreeTemplateHashes(xsltStylesheetPtr style) {
    if (style->templatesHash != NULL)
	xmlHashFree((xmlHashTablePtr) style->templatesHash,
		    (xmlHashDeallocator) xsltFreeCompMatchList);
}

