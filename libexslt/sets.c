#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <libxslt/xsltutils.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/extensions.h>

#include "exslt.h"

/**
 * exsltSetsDifferenceFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps #xmlXPathDifference for use by the XPath processor
 */
static void
exsltSetsDifferenceFunction (xmlXPathParserContextPtr ctxt, int nargs) {
    xmlNodeSetPtr arg1, arg2, ret;

    if (nargs != 2) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    arg2 = xmlXPathPopNodeSet(ctxt);
    if (xmlXPathCheckError(ctxt)) {
	xmlXPathSetTypeError(ctxt);
	return;
    }

    arg1 = xmlXPathPopNodeSet(ctxt);
    if (xmlXPathCheckError(ctxt)) {
	xmlXPathSetTypeError(ctxt);
	return;
    }

    ret = xmlXPathDifference(arg1, arg2);

    if (ret != arg1)
	xmlXPathFreeNodeSet(arg1);
    xmlXPathFreeNodeSet(arg2);

    xmlXPathReturnNodeSet(ctxt, ret);
}

/**
 * exsltSetsIntersectionFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps #xmlXPathIntersection for use by the XPath processor
 */
static void
exsltSetsIntersectionFunction (xmlXPathParserContextPtr ctxt, int nargs) {
    xmlNodeSetPtr arg1, arg2, ret;

    if (nargs != 2) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    arg2 = xmlXPathPopNodeSet(ctxt);
    if (xmlXPathCheckError(ctxt)) {
	xmlXPathSetTypeError(ctxt);
	return;
    }

    arg1 = xmlXPathPopNodeSet(ctxt);
    if (xmlXPathCheckError(ctxt)) {
	xmlXPathSetTypeError(ctxt);
	return;
    }

    ret = xmlXPathIntersection(arg1, arg2);

    xmlXPathFreeNodeSet(arg1);
    xmlXPathFreeNodeSet(arg2);

    xmlXPathReturnNodeSet(ctxt, ret);
}

/**
 * exsltSetsDistinctFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps #xmlXPathDistinct for use by the XPath processor
 */
static void
exsltSetsDistinctFunction (xmlXPathParserContextPtr ctxt, int nargs) {
    xmlNodeSetPtr ns, ret;

    if (nargs != 1) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    ns = xmlXPathPopNodeSet(ctxt);
    if (xmlXPathCheckError(ctxt))
	return;

    /* !!! must be sorted !!! */
    ret = xmlXPathDistinctSorted(ns);

    xmlXPathFreeNodeSet(ns);

    xmlXPathReturnNodeSet(ctxt, ret);
}

/**
 * exsltSetsHasSameNodesFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps #xmlXPathHasSameNodes for use by the XPath processor
 */
static void
exsltSetsHasSameNodesFunction (xmlXPathParserContextPtr ctxt,
			      int nargs) {
    xmlNodeSetPtr arg1, arg2;
    int ret;

    if (nargs != 2) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    arg2 = xmlXPathPopNodeSet(ctxt);
    if (xmlXPathCheckError(ctxt)) {
	xmlXPathSetTypeError(ctxt);
	return;
    }

    arg1 = xmlXPathPopNodeSet(ctxt);
    if (xmlXPathCheckError(ctxt)) {
	xmlXPathSetTypeError(ctxt);
	return;
    }

    ret = xmlXPathHasSameNodes(arg1, arg2);

    xmlXPathFreeNodeSet(arg1);
    xmlXPathFreeNodeSet(arg2);

    xmlXPathReturnBoolean(ctxt, ret);
}

/**
 * exsltSetsLeadingFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps #xmlXPathLeading for use by the XPath processor
 */
static void
exsltSetsLeadingFunction (xmlXPathParserContextPtr ctxt, int nargs) {
    xmlNodeSetPtr arg1, arg2, ret;

    if (nargs != 2) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    arg2 = xmlXPathPopNodeSet(ctxt);
    if (xmlXPathCheckError(ctxt)) {
	xmlXPathSetTypeError(ctxt);
	return;
    }

    arg1 = xmlXPathPopNodeSet(ctxt);
    if (xmlXPathCheckError(ctxt)) {
	xmlXPathSetTypeError(ctxt);
	return;
    }

    /* !!! must be sorted */
    ret = xmlXPathNodeLeadingSorted(arg1, xmlXPathNodeSetItem(arg2, 0));

    xmlXPathFreeNodeSet(arg1);
    xmlXPathFreeNodeSet(arg2);

    xmlXPathReturnNodeSet(ctxt, ret);
}

/**
 * exsltSetsTrailingFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps #xmlXPathTrailing for use by the XPath processor
 */
static void
exsltSetsTrailingFunction (xmlXPathParserContextPtr ctxt, int nargs) {
    xmlNodeSetPtr arg1, arg2, ret;

    if (nargs != 2) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    arg2 = xmlXPathPopNodeSet(ctxt);
    if (xmlXPathCheckError(ctxt)) {
	xmlXPathSetTypeError(ctxt);
	return;
    }

    arg1 = xmlXPathPopNodeSet(ctxt);
    if (xmlXPathCheckError(ctxt)) {
	xmlXPathSetTypeError(ctxt);
	return;
    }

    /* !!! mist be sorted */
    ret = xmlXPathNodeTrailingSorted(arg1, xmlXPathNodeSetItem(arg2, 0));

    xmlXPathFreeNodeSet(arg1);
    xmlXPathFreeNodeSet(arg2);

    xmlXPathReturnNodeSet(ctxt, ret);
}

static void *
exsltSetsInit (xsltTransformContextPtr ctxt, const xmlChar *URI) {
    xsltRegisterExtFunction (ctxt, (const xmlChar *) "difference",
			     URI, exsltSetsDifferenceFunction);
    xsltRegisterExtFunction (ctxt, (const xmlChar *) "intersection",
			     URI, exsltSetsIntersectionFunction);
    xsltRegisterExtFunction (ctxt, (const xmlChar *) "distinct",
			     URI, exsltSetsDistinctFunction);
    xsltRegisterExtFunction (ctxt, (const xmlChar *) "has-same-nodes",
			     URI, exsltSetsHasSameNodesFunction);
    xsltRegisterExtFunction (ctxt, (const xmlChar *) "leading",
			     URI, exsltSetsLeadingFunction);
    xsltRegisterExtFunction (ctxt, (const xmlChar *) "trailing",
			     URI, exsltSetsTrailingFunction);

    return(NULL);
}

/**
 * exsltCommonRegister:
 *
 * Registers the EXSLT - Sets module
 */

void
exsltSetsRegister (void) {
    xsltRegisterExtModule (EXSLT_SETS_NAMESPACE, exsltSetsInit, NULL);
}
