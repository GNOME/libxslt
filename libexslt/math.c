#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <libxslt/xsltconfig.h>
#include <libxslt/xsltutils.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/extensions.h>

#include "exslt.h"

/* extern double xmlXPathNAN; */

/**
 * exsltMathMin:
 * @ns:  a node-set
 *
 * Implements the EXSLT - Math min() function:
 *    number math:min (node-set)
 *
 * Returns the minimum value of the nodes passed as the argument, or
 *         xmlXPathNAN if @ns is NULL or empty or if one of the nodes
 *         turns into NaN.
 */
static double
exsltMathMin (xmlNodeSetPtr ns) {
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
 * exsltMathMinFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps #exsltMathMin for use by the XPath processor.
 */
static void
exsltMathMinFunction (xmlXPathParserContextPtr ctxt, int nargs) {
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

    ret = exsltMathMin(ns);

    xmlXPathFreeNodeSet(ns);

    xmlXPathReturnNumber(ctxt, ret);
}


/**
 * exsltMathMax:
 * @ns:  a node-set
 *
 * Implements the EXSLT - Math max() function:
 *    number math:max (node-set)
 *
 * Returns the maximum value of the nodes passed as arguments, or
 *         xmlXPathNAN if @ns is NULL or empty or if one of the nodes
 *         turns into NaN.
 */
static double
exsltMathMax (xmlNodeSetPtr ns) {
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
 * exsltMathMaxFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps #exsltMathMax for use by the XPath processor.
 */
static void
exsltMathMaxFunction (xmlXPathParserContextPtr ctxt, int nargs) {
    xmlNodeSetPtr ns;
    double ret;

    if (nargs != 1) {
	xmlXPathSetArityError(ctxt);
	return;
    }
    ns = xmlXPathPopNodeSet(ctxt);
    if (xmlXPathCheckError(ctxt))
	return;

    ret = exsltMathMax(ns);

    xmlXPathFreeNodeSet(ns);

    xmlXPathReturnNumber(ctxt, ret);
}

/**
 * exsltMathHighest:
 * @ns:  a node-set
 *
 * Implements the EXSLT - Math highest() function:
 *    node-set math:highest (node-set)
 *
 * Returns the nodes in the node-set whose value is the maximum value
 *         for the node-set.
 */
static xmlNodeSetPtr
exsltMathHighest (xmlNodeSetPtr ns) {
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
 * exsltMathHighestFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps #exsltMathHighest for use by the XPath processor
 */
static void
exsltMathHighestFunction (xmlXPathParserContextPtr ctxt, int nargs) {
    xmlNodeSetPtr ns, ret;

    if (nargs != 1) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    ns = xmlXPathPopNodeSet(ctxt);
    if (xmlXPathCheckError(ctxt))
	return;

    ret = exsltMathHighest(ns);

    xmlXPathFreeNodeSet(ns);

    xmlXPathReturnNodeSet(ctxt, ret);
}

/**
 * exsltMathLowest:
 * @ns:  a node-set
 *
 * Implements the EXSLT - Math lowest() function
 *    node-set math:lowest (node-set)
 *
 * Returns the nodes in the node-set whose value is the minimum value
 *         for the node-set.
 */
static xmlNodeSetPtr
exsltMathLowest (xmlNodeSetPtr ns) {
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
 * exsltMathLowestFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps #exsltMathLowest for use by the XPath processor
 */
static void
exsltMathLowestFunction (xmlXPathParserContextPtr ctxt, int nargs) {
    xmlNodeSetPtr ns, ret;

    if (nargs != 1) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    ns = xmlXPathPopNodeSet(ctxt);
    if (xmlXPathCheckError(ctxt))
	return;

    ret = exsltMathLowest(ns);

    xmlXPathFreeNodeSet(ns);

    xmlXPathReturnNodeSet(ctxt, ret);
}

/**
 * exsltMathRegister:
 *
 * Registers the EXSLT - Math module
 */

void
exsltMathRegister (void) {
    xsltRegisterExtModuleFunction ((const xmlChar *) "min",
				   EXSLT_MATH_NAMESPACE,
				   exsltMathMinFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "max",
				   EXSLT_MATH_NAMESPACE,
				   exsltMathMaxFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "highest",
				   EXSLT_MATH_NAMESPACE,
				   exsltMathHighestFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "lowest",
				   EXSLT_MATH_NAMESPACE,
				   exsltMathLowestFunction);
}
