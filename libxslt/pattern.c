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
#include "xsltutils.h"
#include "imports.h"
#include "templates.h"

/* #define DEBUG_PARSING */

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
 * xsltSwapTopCompMatch:
 * @comp:  the compiled match expression
 *
 * reverse the two top steps.
 */
void
xsltSwapTopCompMatch(xsltCompMatchPtr comp) {
    int i;
    int j = comp->nbStep - 1;

    if (j > 0) {
	register xmlChar *tmp;
	register xsltOp op;
	i = j - 1;
	tmp = comp->steps[i].value;
	comp->steps[i].value = comp->steps[j].value;
	comp->steps[j].value = tmp;
	tmp = comp->steps[i].value2;
	comp->steps[i].value2 = comp->steps[j].value2;
	comp->steps[j].value2 = tmp;
	op = comp->steps[i].op;
	comp->steps[i].op = comp->steps[j].op;
	comp->steps[j].op = op;
    }
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
 * @ctxt:  a XSLT process context
 * @comp: the precompiled pattern
 * @node: a node
 *
 * Test wether the node matches the pattern
 *
 * Returns 1 if it matches, 0 if it doesn't and -1 in case of failure
 */
int
xsltTestCompMatch(xsltTransformContextPtr ctxt, xsltCompMatchPtr comp,
	          xmlNodePtr node) {
    int i;
    xsltStepOpPtr step, select = NULL;

    if ((comp == NULL) || (node == NULL) || (ctxt == NULL)) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltTestCompMatch: null arg\n");
        return(-1);
    }
    for (i = 0;i < comp->nbStep;i++) {
	step = &comp->steps[i];
	if (step->op != XSLT_OP_PREDICATE)
	    select = step;
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

		/* Namespace test */
		if (node->ns == NULL) {
		    if (step->value2 != NULL)
			return(0);
		} else if (node->ns->href != NULL) {
		    if (step->value2 == NULL)
			return(0);
		    if (!xmlStrEqual(step->value2, node->ns->href))
			return(0);
		}
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

		/* Namespace test */
		if (node->ns == NULL) {
		    if (step->value2 != NULL)
			return(0);
		} else if (node->ns->href != NULL) {
		    if (step->value2 == NULL)
			return(0);
		    if (!xmlStrEqual(step->value2, node->ns->href))
			return(0);
		}
		continue;
            case XSLT_OP_PARENT:
		node = node->parent;
		if (node == NULL)
		    return(0);
		if (step->value == NULL)
		    continue;
		if (!xmlStrEqual(step->value, node->name))
		    return(0);
		/* Namespace test */
		if (node->ns == NULL) {
		    if (step->value2 != NULL)
			return(0);
		} else if (node->ns->href != NULL) {
		    if (step->value2 == NULL)
			return(0);
		    if (!xmlStrEqual(step->value2, node->ns->href))
			return(0);
		}
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
			/* Namespace test */
			if (node->ns == NULL) {
			    if (step->value2 == NULL)
				break;
			} else if (node->ns->href != NULL) {
			    if ((step->value2 != NULL) &&
			        (xmlStrEqual(step->value2, node->ns->href)))
				break;
			}
		    }
		    node = node->parent;
		}
		if (node == NULL)
		    return(0);
		continue;
            case XSLT_OP_ID: {
		/* TODO Handle IDs decently, must be done differently */
		xmlAttrPtr id;

		id = xmlGetID(node->doc, step->value);
		if ((id == NULL) || (id->parent != node))
		    return(0);
		break;
	    }
            case XSLT_OP_KEY:
		TODO /* Handle Keys, might be done differently */
		break;
            case XSLT_OP_NS:
		/* Namespace test */
		if (node->ns == NULL) {
		    if (step->value != NULL)
			return(0);
		} else if (node->ns->href != NULL) {
		    if (step->value == NULL)
			return(0);
		    if (!xmlStrEqual(step->value, node->ns->href))
			return(0);
		}
		break;
            case XSLT_OP_ALL:
		switch (node->type) {
		    case XML_DOCUMENT_NODE:
		    case XML_HTML_DOCUMENT_NODE:
		    case XML_ELEMENT_NODE:
			break;
		    default:
			return(0);
		}
		break;
	    case XSLT_OP_PREDICATE: {
		xmlNodePtr oldNode;
		int oldCS, oldCP;
		int pos = 0, len = 0;
		/*
		 * Depending on the last selection, one may need to
		 * recompute contextSize and proximityPosition.
		 */
		if ((select != NULL) &&
		    (select->op == XSLT_OP_ELEM) &&
		    (select->value != NULL) &&
		    (node->type == XML_ELEMENT_NODE) &&
		    (node->parent != NULL)) {

		    /* TODO: cache those informations ?!? */
		    xmlNodePtr siblings = node->parent->children;

		    oldCS = ctxt->xpathCtxt->contextSize;
		    oldCP = ctxt->xpathCtxt->proximityPosition;
		    while (siblings != NULL) {
			if (siblings->type == XML_ELEMENT_NODE) {
			    if (siblings == node) {
				len++;
				pos = len;
			    } else if (xmlStrEqual(node->name,
				       siblings->name)) {
				len++;
			    }
			}
			siblings = siblings->next;
		    }
		    if (pos != 0) {
			ctxt->xpathCtxt->contextSize = len;
			ctxt->xpathCtxt->proximityPosition = pos;
		    }
		}
		oldNode = ctxt->node;
		ctxt->node = node;

		if ((step->value == NULL) ||
		    (!xsltEvalXPathPredicate(ctxt, step->value))) {
		    if (pos != 0) {
			ctxt->xpathCtxt->contextSize = oldCS;
			ctxt->xpathCtxt->proximityPosition = oldCP;
		    }
		    ctxt->node = oldNode;
		    return(0);
		}
		if (pos != 0) {
		    ctxt->xpathCtxt->contextSize = oldCS;
		    ctxt->xpathCtxt->proximityPosition = oldCP;
		}
		ctxt->node = oldNode;
		break;
	    }
            case XSLT_OP_PI:
		if (node->type != XML_PI_NODE)
		    return(0);
		if (step->value != NULL) {
		    if (!xmlStrEqual(step->value, node->name))
			return(0);
		}
		break;
            case XSLT_OP_COMMENT:
		if (node->type != XML_COMMENT_NODE)
		    return(0);
		break;
            case XSLT_OP_TEXT:
		if ((node->type != XML_TEXT_NODE) &&
		    (node->type != XML_CDATA_SECTION_NODE))
		    return(0);
		break;
            case XSLT_OP_NODE:
		switch (node->type) {
		    case XML_DOCUMENT_NODE:
		    case XML_HTML_DOCUMENT_NODE:
		    case XML_ELEMENT_NODE:
		    case XML_CDATA_SECTION_NODE:
		    case XML_PI_NODE:
		    case XML_COMMENT_NODE:
		    case XML_TEXT_NODE:
		    case XML_ATTRIBUTE_NODE:
			break;
		    default:
			return(0);
		}
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

#define SWAP() 						\
    xsltSwapTopCompMatch(ctxt->comp);

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
	PUSH(XSLT_OP_KEY, lit, lit2);
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
    } else if (xmlStrEqual(name, (const xmlChar *)"node")) {
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
    if (name != NULL)
	xmlFree(name);
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
	NEXT;
	if (CUR == '*') {
	    NEXT;
	    PUSH(XSLT_OP_ATTR, NULL, NULL);
	    return;
	}
	token = xsltScanName(ctxt);
	if (token == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		    "xsltCompileStepPattern : Name expected\n");
	    ctxt->error = 1;
	    goto error;
	}
	PUSH(XSLT_OP_ATTR, token, NULL);
	return;
    }
    if (token == NULL)
	token = xsltScanName(ctxt);
    if (token == NULL) {
	if (CUR == '*') {
	    NEXT;
	    PUSH(XSLT_OP_ALL, token, NULL);
	    goto parse_predicate;
	} else {
	    xsltGenericError(xsltGenericErrorContext,
		    "xsltCompileStepPattern : Name expected\n");
	    ctxt->error = 1;
	    goto error;
	}
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
		    "xsltCompileStepPattern : sequence '::' expected\n");
	    ctxt->error = 1;
	    goto error;
	}
	NEXT;
	if (xmlStrEqual(token, (const xmlChar *) "child")) {
	    /* TODO: handle namespace */
	    name = xsltScanName(ctxt);
	    if (name == NULL) {
		xsltGenericError(xsltGenericErrorContext,
			"xsltCompileStepPattern : QName expected\n");
		ctxt->error = 1;
		goto error;
	    }
	    PUSH(XSLT_OP_CHILD, name, NULL);
	} else if (xmlStrEqual(token, (const xmlChar *) "attribute")) {
	    /* TODO: handle namespace */
	    name = xsltScanName(ctxt);
	    if (name == NULL) {
		xsltGenericError(xsltGenericErrorContext,
			"xsltCompileStepPattern : QName expected\n");
		ctxt->error = 1;
		goto error;
	    }
	    PUSH(XSLT_OP_ATTR, name, NULL);
	} else {
	    xsltGenericError(xsltGenericErrorContext,
		"xsltCompileStepPattern : 'child' or 'attribute' expected\n");
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
parse_predicate:
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
		    "xsltCompileStepPattern : ']' expected\n");
	    ctxt->error = 1;
	    goto error;
        }
	ret = xmlStrndup(q, CUR_PTR - q);
	PUSH(XSLT_OP_PREDICATE, ret, NULL);
	/* push the predicate lower than local test */
	SWAP();
	NEXT;
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
    } else if (CUR == '*') {
	xsltCompileRelativePathPattern(ctxt, NULL);
    } else if (CUR == '@') {
	xsltCompileRelativePathPattern(ctxt, NULL);
    } else {
	xmlChar *name;
	name = xsltScanName(ctxt);
	if (name == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		    "xsltCompileLocationPathPattern : Name expected\n");
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
    xsltGenericDebug(xsltGenericDebugContext,
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
    xsltCompMatchPtr pat, list, *top;
    const xmlChar *name = NULL;
    xmlChar *p, *pattern, tmp;

    if ((style == NULL) || (cur == NULL))
	return(-1);

    p = cur->match;
    if (p == NULL)
	return(-1);

next_pattern:
    if (*p == 0)
	return(0);
    /*
     * get a compiled form of the pattern
     */
    pattern = p;
    while ((*p != 0) && (*p != '|')) {
	/* TODO: handle string escaping "a | b" in patterns ... */
	p++;
    }

    tmp = *p;
    *p = 0;
    pat = xsltCompilePattern(pattern);
    *p = tmp;
    if (tmp != 0)
	p++;
    if (pat == NULL)
	return(-1);
    pat->template = cur;
    if (cur->priority != XSLT_PAT_NO_PRIORITY)
	pat->priority = cur->priority;

    /*
     * insert it in the hash table list corresponding to its lookup name
     */
    switch (pat->steps[0].op) {
        case XSLT_OP_ATTR:
	    if (pat->steps[0].value != NULL)
		name = pat->steps[0].value;
	    else
		top = (xsltCompMatchPtr *) &(style->attrMatch);
	    break;
        case XSLT_OP_ELEM:
        case XSLT_OP_CHILD:
        case XSLT_OP_PARENT:
        case XSLT_OP_ANCESTOR:
        case XSLT_OP_NS:
             name = pat->steps[0].value;
	     break;
        case XSLT_OP_ROOT:
             top = (xsltCompMatchPtr *) &(style->rootMatch);
	     break;
        case XSLT_OP_ID:
        case XSLT_OP_KEY:
	     /* TODO optimize ID/KEY !!! */
        case XSLT_OP_ALL:
             top = (xsltCompMatchPtr *) &(style->elemMatch);
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
	    if (pat->steps[0].value != NULL)
		name = pat->steps[0].value;
	    else
		top = (xsltCompMatchPtr *) &(style->piMatch);
	    break;
	case XSLT_OP_COMMENT:
	    top = (xsltCompMatchPtr *) &(style->commentMatch);
	    break;
	case XSLT_OP_TEXT:
	    top = (xsltCompMatchPtr *) &(style->textMatch);
	    break;
	case XSLT_OP_NODE:
	    if (pat->steps[0].value != NULL)
		name = pat->steps[0].value;
	    else
		top = (xsltCompMatchPtr *) &(style->elemMatch);
	    
	    break;
    }
    if (name != NULL) {
	if (style->templatesHash == NULL) {
	    style->templatesHash = xmlHashCreate(0);
	    if (style->templatesHash == NULL) {
		xsltFreeCompMatch(pat);
		return(-1);
	    }
#ifdef DEBUG_PARSING
	    xsltGenericDebug(xsltGenericDebugContext,
		    "xsltAddTemplate: created template hash\n");
#endif
	    xmlHashAddEntry(style->templatesHash, name, pat);
#ifdef DEBUG_PARSING
	    xsltGenericDebug(xsltGenericDebugContext,
		    "xsltAddTemplate: added new hash %s\n", name);
#endif
	} else {
	    list = (xsltCompMatchPtr) xmlHashLookup(style->templatesHash, name);
	    if (list == NULL) {
		xmlHashAddEntry(style->templatesHash, name, pat);
#ifdef DEBUG_PARSING
		xsltGenericDebug(xsltGenericDebugContext,
			"xsltAddTemplate: added new hash %s\n", name);
#endif
	    } else {
		/*
		 * Note '<=' since one must choose among the matching
		 * template rules that are left, the one that occurs
		 * last in the stylesheet
		 */
		if (list->priority <= pat->priority) {
		    pat->next = list;
		    xmlHashUpdateEntry(style->templatesHash, name, pat, NULL);
#ifdef DEBUG_PARSING
		    xsltGenericDebug(xsltGenericDebugContext,
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
    } else if (top != NULL) {
	list = *top;
	if (list == NULL) {
	    *top = pat;
	    pat->next = NULL;
	} else if (list->priority <= pat->priority) {
	    pat->next = list;
	    *top = pat;
	} else {
	    while (list->next != NULL) {
		if (list->next->priority <= pat->priority)
		    break;
	    }
	    pat->next = list->next;
	    list->next = pat;
	}
    } else {
	xsltGenericError(xsltGenericErrorContext,
		"xsltAddTemplate: invalid compiled pattern\n");
	xsltFreeCompMatch(pat);
	return(-1);
    }
    if (*p != 0)
	goto next_pattern;
    return(0);
}

/**
 * xsltGetTemplate:
 * @ctxt:  a XSLT process context
 * @node: an XML Node
 *
 * Finds the template applying to this node
 *
 * Returns the xsltTemplatePtr or NULL if not found
 */
xsltTemplatePtr
xsltGetTemplate(xsltTransformContextPtr ctxt, xmlNodePtr node) {
    xsltStylesheetPtr style;
    xsltTemplatePtr ret = NULL;
    const xmlChar *name = NULL;
    xsltCompMatchPtr list = NULL;

    if ((ctxt == NULL) || (node == NULL))
	return(NULL);

    style = ctxt->style;
    while (style != NULL) {
	/* TODO : handle IDs/keys here ! */
	if (style->templatesHash != NULL) {
	    /*
	     * Use the top name as selector
	     */
	    switch (node->type) {
		case XML_ELEMENT_NODE:
		case XML_ATTRIBUTE_NODE:
		case XML_PI_NODE:
		    name = node->name;
		    break;
		case XML_DOCUMENT_NODE:
		case XML_HTML_DOCUMENT_NODE:
		case XML_TEXT_NODE:
		case XML_CDATA_SECTION_NODE:
		case XML_COMMENT_NODE:
		case XML_ENTITY_REF_NODE:
		case XML_ENTITY_NODE:
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
		    break;
		default:
		    return(NULL);

	    }
	}
	if (name != NULL) {
	    /*
	     * find the list of appliable expressions based on the name
	     */
	    list = (xsltCompMatchPtr) xmlHashLookup(style->templatesHash, name);
	}
	while (list != NULL) {
	    if (xsltTestCompMatch(ctxt, list, node)) {
		ret = list->template;
		break;
	    }
	    list = list->next;
	}
	list = NULL;

	/*
	 * find alternate generic matches
	 */
	switch (node->type) {
	    case XML_ELEMENT_NODE:
		list = style->elemMatch;
		break;
	    case XML_ATTRIBUTE_NODE:
		list = style->attrMatch;
		break;
	    case XML_PI_NODE:
		list = style->piMatch;
		break;
	    case XML_DOCUMENT_NODE:
	    case XML_HTML_DOCUMENT_NODE:
		list = style->rootMatch;
		break;
	    case XML_TEXT_NODE:
	    case XML_CDATA_SECTION_NODE:
		list = style->textMatch;
		break;
	    case XML_COMMENT_NODE:
		list = style->commentMatch;
		break;
	    case XML_ENTITY_REF_NODE:
	    case XML_ENTITY_NODE:
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
		break;
	    default:
		break;

	}
	while ((list != NULL) &&
	       ((ret == NULL)  || (list->priority > ret->priority))) {
	    if (xsltTestCompMatch(ctxt, list, node)) {
		ret = list->template;
		break;
	    }
	    list = list->next;
	}
	if (ret != NULL)
	    return(ret);

	/*
	 * Cycle on next stylesheet import.
	 */
	style = xsltNextImport(style);
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
    if (style->rootMatch != NULL)
        xsltFreeCompMatchList(style->rootMatch);
    if (style->elemMatch != NULL)
        xsltFreeCompMatchList(style->elemMatch);
    if (style->attrMatch != NULL)
        xsltFreeCompMatchList(style->attrMatch);
    if (style->parentMatch != NULL)
        xsltFreeCompMatchList(style->parentMatch);
    if (style->textMatch != NULL)
        xsltFreeCompMatchList(style->textMatch);
    if (style->piMatch != NULL)
        xsltFreeCompMatchList(style->piMatch);
    if (style->commentMatch != NULL)
        xsltFreeCompMatchList(style->commentMatch);
}

