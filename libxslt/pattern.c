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

/*
 * TODO: handle pathological cases like *[*[@a="b"]]
 * TODO: detect [number] at compilation, optimize accordingly
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
#include "keys.h"
#include "pattern.h"

#ifdef WITH_XSLT_DEBUG
#define WITH_XSLT_DEBUG_PATTERN
#endif

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
    xmlChar *value3;
    xmlXPathCompExprPtr comp;
    /*
     * Optimisations for count
     */
    xmlNodePtr previous;
    int        index;
    int        len;
};

struct _xsltCompMatch {
    struct _xsltCompMatch *next; /* siblings in the name hash */
    float priority;              /* the priority */
    const xmlChar *mode;         /* the mode */
    const xmlChar *modeURI;      /* the mode URI */
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
    xmlDocPtr      doc;			/* the source document */
    xmlNodePtr    elem;			/* the source element */
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
static xsltCompMatchPtr
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
static void
xsltFreeCompMatch(xsltCompMatchPtr comp) {
    xsltStepOpPtr op;
    int i;

    if (comp == NULL)
	return;
    if (comp->mode != NULL)
	xmlFree((xmlChar *)comp->mode);
    if (comp->modeURI != NULL)
	xmlFree((xmlChar *)comp->modeURI);
    for (i = 0;i < comp->nbStep;i++) {
	op = &comp->steps[i];
	if (op->value != NULL)
	    xmlFree(op->value);
	if (op->value2 != NULL)
	    xmlFree(op->value2);
	if (op->value3 != NULL)
	    xmlFree(op->value3);
	if (op->comp != NULL)
	    xmlXPathFreeCompExpr(op->comp);
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
static xsltParserContextPtr
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
static void
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
static int
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
static void
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
static void
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

/**
 * xsltCleanupCompMatch:
 * @comp:  the compiled match expression
 *
 * remove all computation state from the pattern
 */
static void
xsltCleanupCompMatch(xsltCompMatchPtr comp) {
    int i;
    
    for (i = 0;i < comp->nbStep;i++) {
	comp->steps[i].previous = NULL;
    }
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
 * @mode:  the mode name or NULL
 * @modeURI:  the mode URI or NULL
 *
 * Test wether the node matches the pattern
 *
 * Returns 1 if it matches, 0 if it doesn't and -1 in case of failure
 */
static int
xsltTestCompMatch(xsltTransformContextPtr ctxt, xsltCompMatchPtr comp,
	          xmlNodePtr node, const xmlChar *mode,
		  const xmlChar *modeURI) {
    int i;
    xsltStepOpPtr step, select = NULL;

    if ((comp == NULL) || (node == NULL) || (ctxt == NULL)) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltTestCompMatch: null arg\n");
        return(-1);
    }
    if (mode != NULL) {
	if (comp->mode == NULL)
	    return(0);
	if ((comp->mode != mode) && (!xmlStrEqual(comp->mode, mode)))
	    return(0);
    } else {
	if (comp->mode != NULL)
	    return(0);
    }
    if (modeURI != NULL) {
	if (comp->modeURI == NULL)
	    return(0);
	if ((comp->modeURI != modeURI) &&
	    (!xmlStrEqual(comp->modeURI, modeURI)))
	    return(0);
    } else {
	if (comp->modeURI != NULL)
	    return(0);
    }
    for (i = 0;i < comp->nbStep;i++) {
	step = &comp->steps[i];
	if (step->op != XSLT_OP_PREDICATE)
	    select = step;
	switch (step->op) {
            case XSLT_OP_END:
		return(1);
            case XSLT_OP_ROOT:
		if ((node->type == XML_DOCUMENT_NODE) ||
		    (node->type == XML_HTML_DOCUMENT_NODE))
		    continue;
		return(0);
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
            case XSLT_OP_KEY: {
		xmlNodeSetPtr list;
		int indx;

		list = xsltGetKey(ctxt, step->value,
			          step->value3, step->value2);
		if (list == NULL)
		    return(0);
		for (indx = 0;indx < list->nodeNr;indx++)
		    if (list->nodeTab[indx] == node)
			break;
		if (indx >= list->nodeNr)
		    return(0);
		break;
	    }
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
		oldCS = ctxt->xpathCtxt->contextSize;
		oldCP = ctxt->xpathCtxt->proximityPosition;
		if ((select != NULL) &&
		    (select->op == XSLT_OP_ELEM) &&
		    (select->value != NULL) &&
		    (node->type == XML_ELEMENT_NODE) &&
		    (node->parent != NULL)) {

		    if ((select->previous != NULL) &&
			(select->previous->parent == node->parent)) {
			/*
			 * just walk back to adjust the index
			 */
			int indx = 0;
			xmlNodePtr sibling = node;

			while (sibling != NULL) {
			    if (sibling == select->previous)
				break;
			    if (xmlStrEqual(node->name, sibling->name)) {
				if ((select->value2 == NULL) ||
				    ((sibling->ns != NULL) &&
				     (xmlStrEqual(select->value2,
						  sibling->ns->href))))
				    indx++;
			    }
			    sibling = sibling->prev;
			}
			if (sibling == NULL) {
			    /* hum going backward in document order ... */
			    indx = 0;
			    sibling = node;
			    while (sibling != NULL) {
				if (sibling == select->previous)
				    break;
				if ((select->value2 == NULL) ||
				    ((sibling->ns != NULL) &&
				     (xmlStrEqual(select->value2,
						  sibling->ns->href))))
				    indx--;
				sibling = sibling->next;
			    }
			}
			if (sibling != NULL) {
			    pos = select->index + indx;
			    len = select->len;
			    select->previous = node;
			    select->index = pos;
			} else
			    pos = 0;
		    } else {
			/*
			 * recompute the index
			 */
			xmlNodePtr siblings = node->parent->children;

			while (siblings != NULL) {
			    if (siblings->type == XML_ELEMENT_NODE) {
				if (siblings == node) {
				    len++;
				    pos = len;
				} else if (xmlStrEqual(node->name,
					   siblings->name)) {
				    if ((select->value2 == NULL) ||
					((siblings->ns != NULL) &&
					 (xmlStrEqual(select->value2,
						      siblings->ns->href))))
					len++;
				}
			    }
			    siblings = siblings->next;
			}
		    }
		    if (pos != 0) {
			ctxt->xpathCtxt->contextSize = len;
			ctxt->xpathCtxt->proximityPosition = pos;
			select->previous = node;
			select->index = pos;
			select->len = len;
		    }
		} else if ((select != NULL) && (select->op == XSLT_OP_ALL)) {
		    if ((select->previous != NULL) &&
			(select->previous->parent == node->parent)) {
			/*
			 * just walk back to adjust the index
			 */
			int indx = 0;
			xmlNodePtr sibling = node;

			while (sibling != NULL) {
			    if (sibling == select->previous)
				break;
			    if (sibling->type == XML_ELEMENT_NODE)
				indx++;
			    sibling = sibling->prev;
			}
			if (sibling == NULL) {
			    /* hum going backward in document order ... */
			    indx = 0;
			    sibling = node;
			    while (sibling != NULL) {
				if (sibling == select->previous)
				    break;
				if (sibling->type == XML_ELEMENT_NODE)
				    indx--;
				sibling = sibling->next;
			    }
			}
			if (sibling != NULL) {
			    pos = select->index + indx;
			    len = select->len;
			    select->previous = node;
			    select->index = pos;
			} else
			    pos = 0;
		    } else {
			/*
			 * recompute the index
			 */
			xmlNodePtr siblings = node->parent->children;

			while (siblings != NULL) {
			    if (siblings->type == XML_ELEMENT_NODE) {
				len++;
				if (siblings == node) {
				    pos = len;
				}
			    }
			    siblings = siblings->next;
			}
		    }
		    if (pos != 0) {
			ctxt->xpathCtxt->contextSize = len;
			ctxt->xpathCtxt->proximityPosition = pos;
			select->previous = node;
			select->index = pos;
			select->len = len;
		    }
		}
		oldNode = ctxt->node;
		ctxt->node = node;

		if (step->value == NULL)
		    goto wrong_index;

		if (step->comp == NULL) {
		    step->comp = xmlXPathCompile(step->value);
		    if (step->comp == NULL)
			goto wrong_index;
		}
		if (!xsltEvalXPathPredicate(ctxt, step->comp))
		    goto wrong_index;

		if (pos != 0) {
		    ctxt->xpathCtxt->contextSize = oldCS;
		    ctxt->xpathCtxt->proximityPosition = oldCP;
		}
		ctxt->node = oldNode;
		break;
wrong_index:
		if (pos != 0) {
		    ctxt->xpathCtxt->contextSize = oldCS;
		    ctxt->xpathCtxt->proximityPosition = oldCP;
		}
		ctxt->node = oldNode;
		return(0);
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

/**
 * xsltTestCompMatchList:
 * @ctxt:  a XSLT process context
 * @node: a node
 * @comp: the precompiled pattern list
 *
 * Test wether the node matches one of the patterns in the list
 *
 * Returns 1 if it matches, 0 if it doesn't and -1 in case of failure
 */
int
xsltTestCompMatchList(xsltTransformContextPtr ctxt, xmlNodePtr node,
	              xsltCompMatchPtr comp) {
    int ret;

    if ((ctxt == NULL) || (node == NULL))
	return(-1);
    while (comp != NULL) {
	ret = xsltTestCompMatch(ctxt, comp, node, NULL, NULL);
	if (ret == 1)
	    return(1);
	comp = comp->next;
    }
    return(0);
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

static xmlChar *
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

static xmlChar *
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
static void
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
	/* TODO: support namespace in keys */
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

static void
xsltCompileStepPattern(xsltParserContextPtr ctxt, xmlChar *token) {
    xmlChar *name = NULL;
    xmlChar *prefix = NULL;
    xmlChar *ncname = NULL;
    xmlChar *URL = NULL;
    int level;

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
	    name = xsltScanName(ctxt);
	    if (name == NULL) {
		xsltGenericError(xsltGenericErrorContext,
			"xsltCompileStepPattern : QName expected\n");
		ctxt->error = 1;
		goto error;
	    }
	    ncname = xmlSplitQName2(name, &prefix);
	    if (ncname != NULL) {
		if (prefix != NULL) {
		    xmlNsPtr ns;

		    ns = xmlSearchNs(ctxt->doc, ctxt->elem, prefix);
		    if (ns == NULL) {
			xsltGenericError(xsltGenericErrorContext,
			    "xsl: pattern, no namespace bound to prefix %s\n",
			                 prefix);
		    } else {
			URL = xmlStrdup(ns->href);
		    }
		    xmlFree(prefix);
		}
		xmlFree(name);
		name = ncname;
	    }
	    PUSH(XSLT_OP_CHILD, name, URL);
	} else if (xmlStrEqual(token, (const xmlChar *) "attribute")) {
	    name = xsltScanName(ctxt);
	    if (name == NULL) {
		xsltGenericError(xsltGenericErrorContext,
			"xsltCompileStepPattern : QName expected\n");
		ctxt->error = 1;
		goto error;
	    }
	    ncname = xmlSplitQName2(name, &prefix);
	    if (ncname != NULL) {
		if (prefix != NULL) {
		    xmlNsPtr ns;

		    ns = xmlSearchNs(ctxt->doc, ctxt->elem, prefix);
		    if (ns == NULL) {
			xsltGenericError(xsltGenericErrorContext,
			    "xsl: pattern, no namespace bound to prefix %s\n",
			                 prefix);
		    } else {
			URL = xmlStrdup(ns->href);
		    }
		    xmlFree(prefix);
		}
		xmlFree(name);
		name = ncname;
	    }
	    PUSH(XSLT_OP_ATTR, name, URL);
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
	ncname = xmlSplitQName2(token, &prefix);
	if (ncname != NULL) {
	    if (prefix != NULL) {
		xmlNsPtr ns;

		ns = xmlSearchNs(ctxt->doc, ctxt->elem, prefix);
		if (ns == NULL) {
		    xsltGenericError(xsltGenericErrorContext,
			"xsl: pattern, no namespace bound to prefix %s\n",
				     prefix);
		} else {
		    URL = xmlStrdup(ns->href);
		}
		xmlFree(prefix);
	    }
	    xmlFree(token);
	    token = ncname;
	}
	PUSH(XSLT_OP_ELEM, token, URL);
    }
parse_predicate:
    SKIP_BLANKS;
    level = 0;
    while (CUR == '[') {
	const xmlChar *q;
	xmlChar *ret = NULL;

	level++;
	NEXT;
	q = CUR_PTR;
	/* TODO: avoid breaking in strings ... */
	while (IS_CHAR(CUR)) {
	    /* Skip over nested predicates */
	    if (CUR == '[')
		level++;
	    if (CUR == ']') {
		level--;
		if (level == 0)
		    break;
	    }
	    NEXT;
	}
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
static void
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
static void
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
	if ((CUR == '(') && !xmlXPathIsNodeType(name)) {
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
 * @doc:  the containing document
 * @node:  the containing element
 *
 * Compile the XSLT pattern and generates a list of precompiled form suitable
 * for fast matching.
 *
 * [1] Pattern ::= LocationPathPattern | Pattern '|' LocationPathPattern
 *
 * Returns the generated pattern list or NULL in case of failure
 */

xsltCompMatchPtr
xsltCompilePattern(const xmlChar *pattern, xmlDocPtr doc, xmlNodePtr node) {
    xsltParserContextPtr ctxt = NULL;
    xsltCompMatchPtr element, first = NULL, previous = NULL;
    int current, start, end;

    if (pattern == NULL) {
        xsltGenericError(xsltGenericErrorContext,
			 "xsltCompilePattern : NULL pattern\n");
	return(NULL);
    }

#ifdef WITH_XSLT_DEBUG_PATTERN
    xsltGenericDebug(xsltGenericDebugContext,
		     "xsltCompilePattern : parsing '%s'\n", pattern);
#endif

    ctxt = xsltNewParserContext();
    if (ctxt == NULL)
	return(NULL);
    ctxt->doc = doc;
    ctxt->elem = node;
    current = end = 0;
    while (pattern[current] != 0) {
	start = current;
	while (IS_BLANK(pattern[current]))
	    current++;
	end = current;
	while ((pattern[end] != 0) && (pattern[end] != '|'))
	    end++;
	if (current == end) {
	    xsltGenericError(xsltGenericErrorContext,
			     "xsltCompilePattern : NULL pattern\n");
	    goto error;
	}
	element = xsltNewCompMatch();
	if (element == NULL) {
	    goto error;
	}
	if (first == NULL)
	    first = element;
	else if (previous != NULL)
	    previous->next = element;
	previous = element;

	ctxt->comp = element;
	ctxt->base = xmlStrndup(&pattern[start], end - start);
	ctxt->cur = &(ctxt->base)[current - start];
	xsltCompileLocationPathPattern(ctxt);
	if (ctxt->base)
	    xmlFree((xmlChar *)ctxt->base);
	if (ctxt->error)
	    goto error;

	/*
	 * Reverse for faster interpretation.
	 */
	xsltReverseCompMatch(element);

	/*
	 * Set-up the priority
	 */
	if (((element->steps[0].op == XSLT_OP_ELEM) ||
	     (element->steps[0].op == XSLT_OP_ATTR)) &&
	    (element->steps[0].value != NULL) &&
	    (element->steps[1].op == XSLT_OP_END)) {
	    element->priority = 0;
	} else if ((element->steps[0].op == XSLT_OP_ROOT) &&
		   (element->steps[1].op == XSLT_OP_END)) {
	    element->priority = 0;
	} else if ((element->steps[0].op == XSLT_OP_PI) &&
		   (element->steps[0].value != NULL) &&
		   (element->steps[1].op == XSLT_OP_END)) {
	    element->priority = 0;
	} else if ((element->steps[0].op == XSLT_OP_NS) &&
		   (element->steps[0].value != NULL) &&
		   (element->steps[1].op == XSLT_OP_END)) {
	    element->priority = -0.25;
	} else if (((element->steps[0].op == XSLT_OP_PI) ||
		    (element->steps[0].op == XSLT_OP_TEXT) ||
		    (element->steps[0].op == XSLT_OP_ALL) ||
		    (element->steps[0].op == XSLT_OP_NODE) ||
		    (element->steps[0].op == XSLT_OP_COMMENT)) &&
		   (element->steps[1].op == XSLT_OP_END)) {
	    element->priority = -0.5;
	} else {
	    element->priority = 0.5;
	}
	if (pattern[end] == '|')
	    end++;
	current = end;
    }
    if (end == 0) {
        xsltGenericError(xsltGenericErrorContext,
			 "xsltCompilePattern : NULL pattern\n");
	goto error;
    }

    xsltFreeParserContext(ctxt);
    return(first);

error:
    if (ctxt != NULL)
	xsltFreeParserContext(ctxt);
    if (first != NULL)
	xsltFreeCompMatchList(first);
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
 * @mode:  the mode name or NULL
 * @modeURI:  the mode URI or NULL
 *
 * Register the XSLT pattern associated to @cur
 *
 * Returns -1 in case of error, 0 otherwise
 */
int
xsltAddTemplate(xsltStylesheetPtr style, xsltTemplatePtr cur,
	        const xmlChar *mode, const xmlChar *modeURI) {
    xsltCompMatchPtr pat, list, *top = NULL, next;
    const xmlChar *name = NULL;

    if ((style == NULL) || (cur == NULL) || (cur->match == NULL))
	return(-1);

    pat = xsltCompilePattern(cur->match, style->doc, cur->elem);
    while (pat) {
	next = pat->next;
	pat->next = NULL;
	
	pat->template = cur;
	if (mode != NULL)
	    pat->mode = xmlStrdup(mode);
	if (modeURI != NULL)
	    pat->modeURI = xmlStrdup(modeURI);
	if (cur->priority == XSLT_PAT_NO_PRIORITY)
	    cur->priority = pat->priority;
	else
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
        case XSLT_OP_KEY:
	    top = (xsltCompMatchPtr *) &(style->keyMatch);
	    break;
        case XSLT_OP_ID:
	    /* TODO optimize ID !!! */
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
		style->templatesHash = xmlHashCreate(1024);
		if (style->templatesHash == NULL) {
		    xsltFreeCompMatch(pat);
		    return(-1);
		}
		xmlHashAddEntry3(style->templatesHash, name, mode, modeURI, pat);
	    } else {
		list = (xsltCompMatchPtr) xmlHashLookup3(style->templatesHash,
							 name, mode, modeURI);
		if (list == NULL) {
		    xmlHashAddEntry3(style->templatesHash, name,
				     mode, modeURI, pat);
		} else {
		    /*
		     * Note '<=' since one must choose among the matching
		     * template rules that are left, the one that occurs
		     * last in the stylesheet
		     */
		    if (list->priority <= pat->priority) {
			pat->next = list;
			xmlHashUpdateEntry3(style->templatesHash, name,
					    mode, modeURI, pat, NULL);
		    } else {
			while (list->next != NULL) {
			    if (list->next->priority <= pat->priority)
				break;
			    list = list->next;
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
		    list = list->next;
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
#ifdef WITH_XSLT_DEBUG_PATTERN
	if (mode)
	    xsltGenericDebug(xsltGenericDebugContext,
			 "added pattern : '%s' mode '%s' priority %f\n",
			     pat->template->match, pat->mode, pat->priority);
	else
	    xsltGenericDebug(xsltGenericDebugContext,
			 "added pattern : '%s' priority %f\n",
			     pat->template->match, pat->priority);
#endif

	pat = next;
    }
    return(0);
}

/**
 * xsltGetTemplate:
 * @ctxt:  a XSLT process context
 * @mode:  the mode 
 * @style:  the current style
 *
 * Finds the template applying to this node, if @style is non-NULL
 * it means one need to look for the next imported template in scope.
 *
 * Returns the xsltTemplatePtr or NULL if not found
 */
xsltTemplatePtr
xsltGetTemplate(xsltTransformContextPtr ctxt, xmlNodePtr node,
	        xsltStylesheetPtr style) {
    xsltStylesheetPtr curstyle;
    xsltTemplatePtr ret = NULL;
    const xmlChar *name = NULL;
    xsltCompMatchPtr list = NULL;

    if ((ctxt == NULL) || (node == NULL))
	return(NULL);

    if (style == NULL) {
	curstyle = ctxt->style;
    } else {
	curstyle = xsltNextImport(style);
    }

    while ((curstyle != NULL) && (curstyle != style)) {
	/* TODO : handle IDs/keys here ! */
	if (curstyle->templatesHash != NULL) {
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
	    list = (xsltCompMatchPtr) xmlHashLookup3(curstyle->templatesHash,
					     name, ctxt->mode, ctxt->modeURI);
	} else
	    list = NULL;
	while (list != NULL) {
	    if (xsltTestCompMatch(ctxt, list, node,
			          ctxt->mode, ctxt->modeURI)) {
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
		list = curstyle->elemMatch;
		break;
	    case XML_ATTRIBUTE_NODE:
		list = curstyle->attrMatch;
		break;
	    case XML_PI_NODE:
		list = curstyle->piMatch;
		break;
	    case XML_DOCUMENT_NODE:
	    case XML_HTML_DOCUMENT_NODE:
		list = curstyle->rootMatch;
		break;
	    case XML_TEXT_NODE:
	    case XML_CDATA_SECTION_NODE:
		list = curstyle->textMatch;
		break;
	    case XML_COMMENT_NODE:
		list = curstyle->commentMatch;
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
	    if (xsltTestCompMatch(ctxt, list, node,
			          ctxt->mode, ctxt->modeURI)) {
		ret = list->template;
		break;
	    }
	    list = list->next;
	}
	if (node->_private != NULL) {
	    list = curstyle->keyMatch;
	    while ((list != NULL) &&
		   ((ret == NULL)  || (list->priority > ret->priority))) {
		if (xsltTestCompMatch(ctxt, list, node,
				      ctxt->mode, ctxt->modeURI)) {
		    ret = list->template;
		    break;
		}
		list = list->next;
	    }
	}
	if (ret != NULL)
	    return(ret);

	/*
	 * Cycle on next curstylesheet import.
	 */
	curstyle = xsltNextImport(curstyle);
    }
    return(NULL);
}

/**
 * xsltCleanupTemplates:
 * @style: an XSLT stylesheet
 *
 * Cleanup the state of the templates used by the stylesheet and
 * the ones it imports.
 */
void
xsltCleanupTemplates(xsltStylesheetPtr style) {
    while (style != NULL) {
	xmlHashScan((xmlHashTablePtr) style->templatesHash,
		    (xmlHashScanner) xsltCleanupCompMatch, NULL);

	style = xsltNextImport(style);
    }
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
    if (style->keyMatch != NULL)
        xsltFreeCompMatchList(style->keyMatch);
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

#if 0
/**
 * xsltMatchPattern
 * @node: a node in the source tree
 * @pattern: an XSLT pattern
 *
 * Determine if a node matches a pattern.
 */
int
xsltMatchPattern(xsltTransformContextPtr context,
		 xmlNodePtr node,
		 const xmlChar *pattern)
{
    int match = 0;
    xsltCompMatchPtr first, comp;

    if ((context != NULL) && (pattern != NULL)) {
	first = xsltCompilePattern(pattern);
	for (comp = first; comp != NULL; comp = comp->next) {
	    match = xsltTestCompMatch(context, comp, node, NULL, NULL);
	    if (match)
		break; /* for */
	}
	if (first)
	    xsltFreeCompMatchList(first);
    }
    return match;
}
#endif
