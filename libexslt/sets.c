#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <libxslt/xsltutils.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/extensions.h>

#include "exslt.h"
#include "utils.h"

/**
 * exslSetsDifference:
 * @nodes1:  a node-set
 * @nodes2:  a node-set
 *
 * Implements the EXSLT - Sets difference() function:
 *    node-set set:difference (node-set, node-set)
 *
 * Returns the difference between the two node sets, or nodes1 if
 *         nodes2 is empty
 */
static xmlNodeSetPtr
exslSetsDifference (xmlNodeSetPtr nodes1, xmlNodeSetPtr nodes2) {
    xmlNodeSetPtr ret;
    int i, l1;
    xmlNodePtr cur;

    if (xmlXPathNodeSetIsEmpty(nodes2))
	return(nodes1);

    ret = xmlXPathNodeSetCreate(NULL);
    if (xmlXPathNodeSetIsEmpty(nodes1))
	return(ret);

    l1 = xmlXPathNodeSetGetLength(nodes1);

    for (i = 0; i < l1; i++) {
	cur = xmlXPathNodeSetItem(nodes1, i);
	if (!xmlXPathNodeSetContains(nodes2, cur))
	    xmlXPathNodeSetAddUnique(ret, cur);
    }
    return(ret);
}

/**
 * exslSetsDifferenceFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps #exslSetsDifference for use by the XPath processor
 */
static void
exslSetsDifferenceFunction (xmlXPathParserContextPtr ctxt, int nargs) {
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

    ret = exslSetsDifference(arg1, arg2);

    if (ret != arg1)
	xmlXPathFreeNodeSet(arg1);
    xmlXPathFreeNodeSet(arg2);

    xmlXPathReturnNodeSet(ctxt, ret);
}

/**
 * exslSetsIntersection:
 * @nodes1:  a node-set
 * @nodes2:  a node-set
 *
 * Implements the EXSLT - Sets intersection() function:
 *    node-set set:intersection (node-set, node-set)
 *
 * Returns a node set comprising the nodes that are within both the
 *         node sets passed as arguments
 */
static xmlNodeSetPtr
exslSetsIntersection (xmlNodeSetPtr nodes1, xmlNodeSetPtr nodes2) {
    xmlNodeSetPtr ret = xmlXPathNodeSetCreate(NULL);
    int i, l1;
    xmlNodePtr cur;

    if (xmlXPathNodeSetIsEmpty(nodes1))
	return(ret);
    if (xmlXPathNodeSetIsEmpty(nodes2))
	return(ret);

    l1 = xmlXPathNodeSetGetLength(nodes1);

    for (i = 0; i < l1; i++) {
	cur = xmlXPathNodeSetItem(nodes1, i);
	if (xmlXPathNodeSetContains(nodes2, cur))
	    xmlXPathNodeSetAddUnique(ret, cur);
    }
    return(ret);
}

/**
 * exslSetsIntersectionFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps #exslSetsIntersection for use by the XPath processor
 */
static void
exslSetsIntersectionFunction (xmlXPathParserContextPtr ctxt, int nargs) {
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

    ret = exslSetsIntersection(arg1, arg2);

    xmlXPathFreeNodeSet(arg1);
    xmlXPathFreeNodeSet(arg2);

    xmlXPathReturnNodeSet(ctxt, ret);
}

/**
 * exslSetsDistinctSorted:
 * @nodes:  a node-set, sorted by document order
 *
 * Implements the EXSLT - Sets distinct() function:
 *    node-set set:distinct (node-set)
 * 
 * Returns a subset of the nodes contained in @nodes, or @nodes if
 *         it is empty
 */
static xmlNodeSetPtr
exslSetsDistinctSorted (xmlNodeSetPtr nodes) {
    xmlNodeSetPtr ret;
    xmlHashTablePtr hash;
    int i, l;
    xmlChar * strval;
    xmlNodePtr cur;

    if (xmlXPathNodeSetIsEmpty(nodes))
	return(nodes);

    ret = xmlXPathNodeSetCreate(NULL);
    l = xmlXPathNodeSetGetLength(nodes);
    hash = xmlHashCreate (l);
    for (i = 0; i < l; i++) {
	cur = xmlXPathNodeSetItem(nodes, i);
	strval = xmlXPathCastNodeToString(cur);
	if (xmlHashLookup(hash, strval) == NULL) {
	    xmlHashAddEntry(hash, strval, strval);
	    xmlXPathNodeSetAddUnique(ret, cur);
	} else {
	    xmlFree(strval);
	}
    }
    xmlHashFree(hash, (xmlHashDeallocator) xmlFree);
    return(ret);
}

/**
 * exslSetsDistinct:
 * @nodes:  a node-set
 *
 * Implements the EXSLT - Sets distinct() function:
 *    node-set set:distinct (node-set)
 * @nodes is sorted by document order, then #exslSetsDistinctSorted
 * is called with the sorted node-set
 *
 * Returns a subset of the nodes contained in @nodes, or @nodes if
 *         it is empty
 */
xmlNodeSetPtr
exslSetsDistinct (xmlNodeSetPtr nodes) {
    if (xmlXPathNodeSetIsEmpty(nodes))
	return(nodes);

    xmlXPathNodeSetSort(nodes);
    return(exslSetsDistinctSorted(nodes));
}

/**
 * exslSetsDistinctFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps #exslSetsDistinct for use by the XPath processor
 */
static void
exslSetsDistinctFunction (xmlXPathParserContextPtr ctxt, int nargs) {
    xmlNodeSetPtr ns, ret;

    if (nargs != 1) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    ns = xmlXPathPopNodeSet(ctxt);
    if (xmlXPathCheckError(ctxt))
	return;

    /* !!! must be sorted !!! */
    ret = exslSetsDistinctSorted(ns);

    xmlXPathFreeNodeSet(ns);

    xmlXPathReturnNodeSet(ctxt, ret);
}

/**
 * exslSetsHasSameNodes:
 * @nodes1:  a node-set
 * @nodes2:  a node-set
 *
 * Implements the EXSLT - Sets has-same-nodes function:
 *    boolean set:has-same-node(node-set, node-set)
 *
 * Returns true (1) if @nodes1 shares any node with @nodes2, false (0)
 *         otherwise
 */
static int
exslSetsHasSameNodes (xmlNodeSetPtr nodes1, xmlNodeSetPtr nodes2) {
    int i, l;
    xmlNodePtr cur;

    if (xmlXPathNodeSetIsEmpty(nodes1) ||
	xmlXPathNodeSetIsEmpty(nodes2))
	return(0);

    l = xmlXPathNodeSetGetLength(nodes1);
    for (i = 0; i < l; i++) {
	cur = xmlXPathNodeSetItem(nodes1, i);
	if (xmlXPathNodeSetContains(nodes2, cur))
	    return(1);
    }
    return(0);
}

/**
 * exslSetsHasSameNodesFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps #exslSetsHasSameNodes for use by the XPath processor
 */
static void
exslSetsHasSameNodesFunction (xmlXPathParserContextPtr ctxt,
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

    ret = exslSetsHasSameNodes(arg1, arg2);

    xmlXPathFreeNodeSet(arg1);
    xmlXPathFreeNodeSet(arg2);

    xmlXPathReturnBoolean(ctxt, ret);
}

/**
 * exslSetsNodeLeadingSorted:
 * @nodes: a node-set, sorted by document order
 * @node: a node
 *
 * Implements the EXSLT - Sets leading() function:
 *    node-set set:leading (node-set, node-set)
 *
 * Returns the nodes in @nodes that precede @node in document order,
 *         @nodes if @node is NULL or an empty node-set if @nodes
 *         doesn't contain @node
 */
static xmlNodeSetPtr
exslSetsNodeLeadingSorted (xmlNodeSetPtr nodes, xmlNodePtr node) {
    int i, l;
    xmlNodePtr cur;
    xmlNodeSetPtr ret;

    if (node == NULL)
	return(nodes);

    ret = xmlXPathNodeSetCreate(NULL);
    if (xmlXPathNodeSetIsEmpty(nodes) ||
	(!xmlXPathNodeSetContains(nodes, node)))
	return(ret);

    l = xmlXPathNodeSetGetLength(nodes);
    for (i = 0; i < l; i++) {
	cur = xmlXPathNodeSetItem(nodes, i);
	if (cur == node)
	    break;
	xmlXPathNodeSetAddUnique(ret, cur);
    }
    return(ret);
}

/**
 * exslSetsNodeLeading:
 * @nodes:  a node-set
 * @node:  a node
 *
 * Implements the EXSLT - Sets leading() function:
 *    node-set set:leading (node-set, node-set)
 * @nodes is sorted by document order, then #exslSetsNodeLeadingSorted
 * is called.
 *
 * Returns the nodes in @nodes that precede @node in document order,
 *         @nodes if @node is NULL or an empty node-set if @nodes
 *         doesn't contain @node
 */
xmlNodeSetPtr
exslSetsNodeLeading (xmlNodeSetPtr nodes, xmlNodePtr node) {
    xmlXPathNodeSetSort(nodes);
    return(exslSetsNodeLeadingSorted(nodes, node));
}

/**
 * exslSetsLeadingSorted:
 * @nodes1:  a node-set, sorted by document order
 * @nodes2:  a node-set, sorted by document order
 *
 * Implements the EXSLT - Sets leading() function:
 *    node-set set:leading (node-set, node-set)
 *
 * Returns the nodes in @nodes1 that precede the first node in @nodes2
 *         in document order, @nodes1 if @nodes2 is NULL or empty or
 *         an empty node-set if @nodes1 doesn't contain @nodes2
 */
static xmlNodeSetPtr
exslSetsLeadingSorted (xmlNodeSetPtr nodes1, xmlNodeSetPtr nodes2) {
    if (xmlXPathNodeSetIsEmpty(nodes2))
	return(nodes1);
    return(exslSetsNodeLeadingSorted(nodes1,
				     xmlXPathNodeSetItem(nodes2, 1)));
}

/**
 * exslSetsLeading:
 * @nodes1:  a node-set
 * @nodes2:  a node-set
 *
 * Implements the EXSLT - Sets leading() function:
 *    node-set set:leading (node-set, node-set)
 * @nodes1 and @nodes2 are sorted by document order, then
 * #exslSetsLeadingSorted is called.
 *
 * Returns the nodes in @nodes1 that precede the first node in @nodes2
 *         in document order, @nodes1 if @nodes2 is NULL or empty or
 *         an empty node-set if @nodes1 doesn't contain @nodes2
 */
xmlNodeSetPtr
exslSetsLeading (xmlNodeSetPtr nodes1, xmlNodeSetPtr nodes2) {
    if (xmlXPathNodeSetIsEmpty(nodes2))
	return(nodes1);
    if (xmlXPathNodeSetIsEmpty(nodes1))
	return(xmlXPathNodeSetCreate(NULL));
    xmlXPathNodeSetSort(nodes1);
    xmlXPathNodeSetSort(nodes2);
    return(exslSetsNodeLeadingSorted(nodes1,
				     xmlXPathNodeSetItem(nodes2, 1)));
}

/**
 * exslSetsLeadingFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps #exslSetsLeading for use by the XPath processor
 */
static void
exslSetsLeadingFunction (xmlXPathParserContextPtr ctxt, int nargs) {
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

    ret = exslSetsLeadingSorted(arg1, arg2);

    xmlXPathFreeNodeSet(arg1);
    xmlXPathFreeNodeSet(arg2);

    xmlXPathReturnNodeSet(ctxt, ret);
}

/**
 * exslSetsNodeTrailingSorted:
 * @nodes: a node-set, sorted by document order
 * @node: a node
 *
 * Implements the EXSLT - Sets trailing() function:
 *    node-set set:trailing (node-set, node-set)
 *
 * Returns the nodes in @nodes that follow @node in document order,
 *         @nodes if @node is NULL or an empty node-set if @nodes
 *         doesn't contain @node
 */
static xmlNodeSetPtr
exslSetsNodeTrailingSorted (xmlNodeSetPtr nodes, xmlNodePtr node) {
    int i, l;
    xmlNodePtr cur;
    xmlNodeSetPtr ret;

    if (node == NULL)
	return(nodes);

    ret = xmlXPathNodeSetCreate(NULL);
    if (xmlXPathNodeSetIsEmpty(nodes) ||
	(!xmlXPathNodeSetContains(nodes, node)))
	return(ret);

    l = xmlXPathNodeSetGetLength(nodes);
    for (i = 0; i < l; i++) {
	cur = xmlXPathNodeSetItem(nodes, i);
	if (cur == node)
	    break;
	xmlXPathNodeSetAddUnique(ret, cur);
    }
    return(ret);
}

/**
 * exslSetsNodeTrailing:
 * @nodes:  a node-set
 * @node:  a node
 *
 * Implements the EXSLT - Sets trailing() function:
 *    node-set set:trailing (node-set, node-set)
 * @nodes is sorted by document order, then #exslSetsNodeLeadingSorted
 * is called.
 *
 * Returns the nodes in @nodes that follow @node in document order,
 *         @nodes if @node is NULL or an empty node-set if @nodes
 *         doesn't contain @node
 */
xmlNodeSetPtr
exslSetsNodeTrailing (xmlNodeSetPtr nodes, xmlNodePtr node) {
    xmlXPathNodeSetSort(nodes);
    return(exslSetsNodeTrailingSorted(nodes, node));
}

/**
 * exslSetsTrailingSorted:
 * @nodes1:  a node-set, sorted by document order
 * @nodes2:  a node-set, sorted by document order
 *
 * Implements the EXSLT - Sets trailing() function:
 *    node-set set:trailing (node-set, node-set)
 *
 * Returns the nodes in @nodes1 that follow the first node in @nodes2
 *         in document order, @nodes1 if @nodes2 is NULL or empty or
 *         an empty node-set if @nodes1 doesn't contain @nodes2
 */
xmlNodeSetPtr
exslSetsTrailingSorted (xmlNodeSetPtr nodes1, xmlNodeSetPtr nodes2) {
    if (xmlXPathNodeSetIsEmpty(nodes2))
	return(nodes1);
    return(exslSetsNodeTrailingSorted(nodes1,
				     xmlXPathNodeSetItem(nodes2, 0)));
}

/**
 * exslSetsTrailing:
 * @nodes1:  a node-set
 * @nodes2:  a node-set
 *
 * Implements the EXSLT - Sets trailing() function:
 *    node-set set:trailing (node-set, node-set)
 * @nodes1 and @nodes2 are sorted by document order, then
 * #exslSetsLeadingSorted is called.
 *
 * Returns the nodes in @nodes1 that follow the first node in @nodes2
 *         in document order, @nodes1 if @nodes2 is NULL or empty or
 *         an empty node-set if @nodes1 doesn't contain @nodes2
 */
xmlNodeSetPtr
exslSetsTrailing (xmlNodeSetPtr nodes1, xmlNodeSetPtr nodes2) {
    if (xmlXPathNodeSetIsEmpty(nodes2))
	return(nodes1);
    if (xmlXPathNodeSetIsEmpty(nodes1))
	return(xmlXPathNodeSetCreate(NULL));
    xmlXPathNodeSetSort(nodes1);
    xmlXPathNodeSetSort(nodes2);
    return(exslSetsNodeTrailingSorted(nodes1,
				     xmlXPathNodeSetItem(nodes2, 0)));
}

/**
 * exslSetsTrailingFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps #exslSetsTrailing for use by the XPath processor
 */
static void
exslSetsTrailingFunction (xmlXPathParserContextPtr ctxt, int nargs) {
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

    ret = exslSetsNodeTrailingSorted(arg1, xmlXPathNodeSetItem(arg2, 0));

    xmlXPathFreeNodeSet(arg1);
    xmlXPathFreeNodeSet(arg2);

    xmlXPathReturnNodeSet(ctxt, ret);
}

static void *
exslSetsInit (xsltTransformContextPtr ctxt, const xmlChar *URI) {
    xsltRegisterExtFunction (ctxt, (const xmlChar *) "difference",
			     URI, exslSetsDifferenceFunction);
    xsltRegisterExtFunction (ctxt, (const xmlChar *) "intersection",
			     URI, exslSetsIntersectionFunction);
    xsltRegisterExtFunction (ctxt, (const xmlChar *) "distinct",
			     URI, exslSetsDistinctFunction);
    xsltRegisterExtFunction (ctxt, (const xmlChar *) "has-same-nodes",
			     URI, exslSetsHasSameNodesFunction);
    xsltRegisterExtFunction (ctxt, (const xmlChar *) "leading",
			     URI, exslSetsLeadingFunction);
    xsltRegisterExtFunction (ctxt, (const xmlChar *) "trailing",
			     URI, exslSetsTrailingFunction);

    return(NULL);
}

/**
 * exslCommonRegister:
 *
 * Registers the EXSLT - Sets module
 */

void
exslSetsRegister (void) {
    xsltRegisterExtModule (EXSLT_SETS_NAMESPACE, exslSetsInit, NULL);
}
