#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "utils.h"

/**
 * xmlXPathPopBoolean:
 * @ctxt:  an XPath parser context
 *
 * Pops a boolean from the stack, handling conversion if needed.
 * Check error with #xmlXPathCheckError.
 *
 * Returns the boolean
 */
int
xmlXPathPopBoolean (xmlXPathParserContextPtr ctxt) {
    xmlXPathObjectPtr obj;
    int ret;

    obj = valuePop(ctxt);
    if (obj == NULL) {
	xmlXPathSetError(ctxt, XPATH_INVALID_OPERAND);
	return(0);
    }
    ret = xmlXPathCastToBoolean(obj);
    xmlXPathFreeObject(obj);
    return(ret);
}

/**
 * xmlXPathPopNumber:
 * @ctxt:  an XPath parser context
 *
 * Pops a number from the stack, handling conversion if needed.
 * Check error with #xmlXPathCheckError.
 *
 * Returns the number
 */
double
xmlXPathPopNumber (xmlXPathParserContextPtr ctxt) {
    xmlXPathObjectPtr obj;
    double ret;

    obj = valuePop(ctxt);
    if (obj == NULL) {
	xmlXPathSetError(ctxt, XPATH_INVALID_OPERAND);
	return(0);
    }
    ret = xmlXPathCastToNumber(obj);
    xmlXPathFreeObject(obj);
    return(ret);
}

/**
 * xmlXPathPopString:
 * @ctxt:  an XPath parser context
 *
 * Pops a string from the stack, handling conversion if needed.
 * Check error with #xmlXPathCheckError.
 *
 * Returns the string
 */
xmlChar *
xmlXPathPopString (xmlXPathParserContextPtr ctxt) {
    xmlXPathObjectPtr obj;
    xmlChar * ret;

    obj = valuePop(ctxt);
    if (obj == NULL) {
	xmlXPathSetError(ctxt, XPATH_INVALID_OPERAND);
	return(NULL);
    }
    ret = xmlXPathCastToString(obj);
    /* TODO: needs refactoring somewhere else */
    if (obj->stringval == ret)
	obj->stringval = NULL;
    xmlXPathFreeObject(obj);
    return(ret);
}

/**
 * xmlXPathPopNodeSet:
 * @ctxt:  an XPath parser context
 *
 * Pops a node-set from the stack, handling conversion if needed.
 * Check error with #xmlXPathCheckError.
 *
 * Returns the node-set
 */
xmlNodeSetPtr
xmlXPathPopNodeSet (xmlXPathParserContextPtr ctxt) {
    xmlXPathObjectPtr obj;
    xmlNodeSetPtr ret;

    if (ctxt->value == NULL) {
	xmlXPathSetError(ctxt, XPATH_INVALID_OPERAND);
	return(NULL);
    }
    if (!xmlXPathStackIsNodeSet(ctxt)) {
	xmlXPathSetTypeError(ctxt);
	return(NULL);
    }
    obj = valuePop(ctxt);
    ret = obj->nodesetval;
    xmlXPathFreeNodeSetList(obj);
    return(ret);
}

/**
 * xmlXPathNodeSetContains:
 * @cur:  the node-set
 * @val:  the node
 *
 * checks whether @cur contains @val
 *
 * Returns true (1) if @cur contains @val, false (0) otherwise
 */
int
xmlXPathNodeSetContains (xmlNodeSetPtr cur, xmlNodePtr val) {
    int i;

    for (i = 0; i < cur->nodeNr; i++) {
        if (cur->nodeTab[i] == val)
	    return(1);
    }
    return(0);
}
