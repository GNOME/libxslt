#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <libxslt/xsltutils.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/extensions.h>

#include "exslt.h"
#include "utils.h"

/**
 * exslMathMin:
 * @ns:  a node-set
 *
 * Implements the EXSLT - Math min() function:
 *    number math:min (node-set)
 *
 * Returns the minimum value of the nodes passed as the argument, or
 *         xmlXPathNAN if @ns is NULL or empty or if one of the nodes
 *         turns into NaN.
 */
double
exslMathMin (xmlNodeSetPtr ns) {
    double ret, cur;
    int i;

    if ((ns == NULL) || (ns->nodeNr == 0))
	return(xmlXPathNAN);
    ret = xmlXPathCastNodeToNumber(ns->nodeTab[0]);
    if (ret == xmlXPathNAN)
	return(xmlXPathNAN);
    for (i = 1; i < ns->nodeNr; i++) {
	cur = xmlXPathCastNodeToNumber(ns->nodeTab[i]);
	if (cur == xmlXPathNAN)
	    return(xmlXPathNAN);
	if (cur < ret)
	    ret = cur;
    }
    return(ret);
}

/**
 * exslMathMinFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps #exslMathMin for use by the XPath processor.
 */
void
exslMathMinFunction (xmlXPathParserContextPtr ctxt, int nargs) {
    xmlNodeSetPtr ns;
    double ret;

    if (nargs != 1) {
	xsltGenericError(xsltGenericErrorContext,
			 "math:min: invalid number of arguments\n");
	ctxt->error = XPATH_INVALID_ARITY;
	return;
    }
    ns = xmlXPathPopNodeSet(ctxt);
    if (xmlXPathCheckError(ctxt))
	return;

    ret = exslMathMin(ns);

    xmlXPathFreeNodeSet(ns);

    xmlXPathReturnNumber(ctxt, ret);
}


/**
 * exslMathMax:
 * @ns:  a node-set
 *
 * Implements the EXSLT - Math max() function:
 *    number math:max (node-set)
 *
 * Returns the maximum value of the nodes passed as arguments, or
 *         xmlXPathNAN if @ns is NULL or empty or if one of the nodes
 *         turns into NaN.
 */
double
exslMathMax (xmlNodeSetPtr ns) {
    double ret, cur;
    int i;

    if ((ns == NULL) || (ns->nodeNr == 0))
	return(xmlXPathNAN);
    ret = xmlXPathCastNodeToNumber(ns->nodeTab[0]);
    if (ret == xmlXPathNAN)
	return(xmlXPathNAN);
    for (i = 1; i < ns->nodeNr; i++) {
	cur = xmlXPathCastNodeToNumber(ns->nodeTab[i]);
	if (cur == xmlXPathNAN)
	    return(xmlXPathNAN);
	if (cur > ret)
	    ret = cur;
    }
    return(ret);
}

/**
 * exslMathMaxFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps #exslMathMax for use by the XPath processor.
 */
void
exslMathMaxFunction (xmlXPathParserContextPtr ctxt, int nargs) {
    xmlNodeSetPtr ns;
    double ret;

    if (nargs != 1) {
	xmlXPathSetArityError(ctxt);
	return;
    }
    ns = xmlXPathPopNodeSet(ctxt);
    if (xmlXPathCheckError(ctxt))
	return;

    ret = exslMathMax(ns);

    xmlXPathFreeNodeSet(ns);

    xmlXPathReturnNumber(ctxt, ret);
}

/**
 * exslMathHighest:
 * @ns:  a node-set
 *
 * Implements the EXSLT - Math highest() function:
 *    node-set math:highest (node-set)
 *
 * Returns the nodes in the node-set whose value is the maximum value
 *         for the node-set.
 */
xmlNodeSetPtr
exslMathHighest (xmlNodeSetPtr ns) {
    xmlNodeSetPtr ret = xmlXPathNodeSetCreate(NULL);
    double max, cur;
    int i;

    if ((ns == NULL) || (ns->nodeNr == 0))
	return(ret);

    max = xmlXPathCastNodeToNumber(ns->nodeTab[0]);
    if (max == xmlXPathNAN)
	return(ret);
    else
	xmlXPathNodeSetAddUnique(ret, ns->nodeTab[0]);

    for (i = 1; i < ns->nodeNr; i++) {
	cur = xmlXPathCastNodeToNumber(ns->nodeTab[i]);
	if (cur == xmlXPathNAN) {
	    xmlXPathEmptyNodeSet(ret);
	    return(ret);
	}
	if (cur < max)
	    continue;
	if (cur > max) {
	    max = cur;
	    xmlXPathEmptyNodeSet(ret);
	    xmlXPathNodeSetAddUnique(ret, ns->nodeTab[i]);
	    continue;
	}
	xmlXPathNodeSetAddUnique(ret, ns->nodeTab[i]);
    }
    return(ret);
}

/**
 * exslMathHighestFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps #exslMathHighest for use by the XPath processor
 */
void
exslMathHighestFunction (xmlXPathParserContextPtr ctxt, int nargs) {
    xmlNodeSetPtr ns, ret;

    if (nargs != 1) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    ns = xmlXPathPopNodeSet(ctxt);
    if (xmlXPathCheckError(ctxt))
	return;

    ret = exslMathHighest(ns);

    xmlXPathFreeNodeSet(ns);

    xmlXPathReturnNodeSet(ctxt, ret);
}

/**
 * exslMathLowest:
 * @ns:  a node-set
 *
 * Implements the EXSLT - Math lowest() function
 *    node-set math:lowest (node-set)
 *
 * Returns the nodes in the node-set whose value is the minimum value
 *         for the node-set.
 */
xmlNodeSetPtr
exslMathLowest (xmlNodeSetPtr ns) {
    xmlNodeSetPtr ret = xmlXPathNodeSetCreate(NULL);
    double min, cur;
    int i;

    if ((ns == NULL) || (ns->nodeNr == 0))
	return(ret);

    min = xmlXPathCastNodeToNumber(ns->nodeTab[0]);
    if (min == xmlXPathNAN)
	return(ret);
    else
	xmlXPathNodeSetAddUnique(ret, ns->nodeTab[0]);

    for (i = 1; i < ns->nodeNr; i++) {
	cur = xmlXPathCastNodeToNumber(ns->nodeTab[i]);
	if (cur == xmlXPathNAN) {
	    xmlXPathEmptyNodeSet(ret);
	    return(ret);
	}
        if (cur > min)
	    continue;
	if (cur < min) {
	    min = cur;
	    xmlXPathEmptyNodeSet(ret);
	    xmlXPathNodeSetAddUnique(ret, ns->nodeTab[i]);
            continue;
	}
	xmlXPathNodeSetAddUnique(ret, ns->nodeTab[i]);
    }
    return(ret);
}

/**
 * exslMathLowestFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps #exslMathLowest for use by the XPath processor
 */
void
exslMathLowestFunction (xmlXPathParserContextPtr ctxt, int nargs) {
    xmlNodeSetPtr ns, ret;

    if (nargs != 1) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    ns = xmlXPathPopNodeSet(ctxt);
    if (xmlXPathCheckError(ctxt))
	return;

    ret = exslMathLowest(ns);

    xmlXPathFreeNodeSet(ns);

    xmlXPathReturnNodeSet(ctxt, ret);
}

static void *
exslMathInit (xsltTransformContextPtr ctxt, const xmlChar *URI) {
    xsltRegisterExtFunction (ctxt, (const xmlChar *) "min",
			     URI, exslMathMinFunction);
    xsltRegisterExtFunction (ctxt, (const xmlChar *) "max",
			     URI, exslMathMaxFunction);
    xsltRegisterExtFunction (ctxt, (const xmlChar *) "highest",
			     URI, exslMathHighestFunction);
    xsltRegisterExtFunction (ctxt, (const xmlChar *) "lowest",
			     URI, exslMathLowestFunction);
}

void
exslMathRegister (void) {
    xsltRegisterExtModule (EXSLT_MATH_NAMESPACE, exslMathInit, NULL);
}
