#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <libxslt/xsltutils.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/extensions.h>

#include "exslt.h"
#include "utils.h"

/**
 * exsltSetsDifference:
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
exsltSetsDifference (xmlNodeSetPtr nodes1, xmlNodeSetPtr nodes2) {
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
 * exsltSetsDifferenceFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps #exsltSetsDifference for use by the XPath processor
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

    ret = exsltSetsDifference(arg1, arg2);

    if (ret != arg1)
	xmlXPathFreeNodeSet(arg1);
    xmlXPathFreeNodeSet(arg2);

    xmlXPathReturnNodeSet(ctxt, ret);
}

/**
 * exsltSetsIntersection:
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
exsltSetsIntersection (xmlNodeSetPtr nodes1, xmlNodeSetPtr nodes2) {
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
 * exsltSetsIntersectionFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps #exsltSetsIntersection for use by the XPath processor
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

    ret = exsltSetsIntersection(arg1, arg2);

    xmlXPathFreeNodeSet(arg1);
    xmlXPathFreeNodeSet(arg2);

    xmlXPathReturnNodeSet(ctxt, ret);
}

/**
 * exsltSetsDistinctSorted:
 * @nodes:  a node-set, sorted by document order
 *
 * Implements the EXSLT - Sets distinct() function:
 *    node-set set:distinct (node-set)
 * 
 * Returns a subset of the nodes contained in @nodes, or @nodes if
 *         it is empty
 */
static xmlNodeSetPtr
exsltSetsDistinctSorted (xmlNodeSetPtr nodes) {
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
 * exsltSetsDistinct:
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
exsltSetsDistinct (xmlNodeSetPtr nodes) {
    if (xmlXPathNodeSetIsEmpty(nodes))
	return(nodes);

    xmlXPathNodeSetSort(nodes);
    return(exsltSetsDistinctSorted(nodes));
}

/**
 * exsltSetsDistinctFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps #exsltSetsDistinct for use by the XPath processor
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
    ret = exsltSetsDistinctSorted(ns);

    xmlXPathFreeNodeSet(ns);

    xmlXPathReturnNodeSet(ctxt, ret);
}

/**
 * exsltSetsHasSameNodes:
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
exsltSetsHasSameNodes (xmlNodeSetPtr nodes1, xmlNodeSetPtr nodes2) {
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
 * exsltSetsHasSameNodesFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps #exsltSetsHasSameNodes for use by the XPath processor
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

    ret = exsltSetsHasSameNodes(arg1, arg2);

    xmlXPathFreeNodeSet(arg1);
    xmlXPathFreeNodeSet(arg2);

    xmlXPathReturnBoolean(ctxt, ret);
}

/**
 * exsltSetsNodeLeadingSorted:
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
exsltSetsNodeLeadingSorted (xmlNodeSetPtr nodes, xmlNodePtr node) {
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
 * exsltSetsNodeLeading:
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
exsltSetsNodeLeading (xmlNodeSetPtr nodes, xmlNodePtr node) {
    xmlXPathNodeSetSort(nodes);
    return(exsltSetsNodeLeadingSorted(nodes, node));
}

/**
 * exsltSetsLeadingSorted:
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
exsltSetsLeadingSorted (xmlNodeSetPtr nodes1, xmlNodeSetPtr nodes2) {
    if (xmlXPathNodeSetIsEmpty(nodes2))
	return(nodes1);
    return(exsltSetsNodeLeadingSorted(nodes1,
				     xmlXPathNodeSetItem(nodes2, 1)));
}

/**
 * exsltSetsLeading:
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
exsltSetsLeading (xmlNodeSetPtr nodes1, xmlNodeSetPtr nodes2) {
    if (xmlXPathNodeSetIsEmpty(nodes2))
	return(nodes1);
    if (xmlXPathNodeSetIsEmpty(nodes1))
	return(xmlXPathNodeSetCreate(NULL));
    xmlXPathNodeSetSort(nodes1);
    xmlXPathNodeSetSort(nodes2);
    return(exsltSetsNodeLeadingSorted(nodes1,
				      xmlXPathNodeSetItem(nodes2, 1)));
}

/**
 * exsltSetsLeadingFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps #exsltSetsLeading for use by the XPath processor
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
    ret = exsltSetsLeadingSorted(arg1, arg2);

    xmlXPathFreeNodeSet(arg1);
    xmlXPathFreeNodeSet(arg2);

    xmlXPathReturnNodeSet(ctxt, ret);
}

/**
 * exsltSetsNodeTrailingSorted:
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
exsltSetsNodeTrailingSorted (xmlNodeSetPtr nodes, xmlNodePtr node) {
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
 * exsltSetsNodeTrailing:
 * @nodes:  a node-set
 * @node:  a node
 *
 * Implements the EXSLT - Sets trailing() function:
 *    node-set set:trailing (node-set, node-set)
 * @nodes is sorted by document order, then #exsltSetsNodeTrailingSorted
 * is called.
 *
 * Returns the nodes in @nodes that follow @node in document order,
 *         @nodes if @node is NULL or an empty node-set if @nodes
 *         doesn't contain @node
 */
xmlNodeSetPtr
exsltSetsNodeTrailing (xmlNodeSetPtr nodes, xmlNodePtr node) {
    xmlXPathNodeSetSort(nodes);
    return(exsltSetsNodeTrailingSorted(nodes, node));
}

/**
 * exsltSetsTrailingSorted:
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
exsltSetsTrailingSorted (xmlNodeSetPtr nodes1, xmlNodeSetPtr nodes2) {
    if (xmlXPathNodeSetIsEmpty(nodes2))
	return(nodes1);
    return(exsltSetsNodeTrailingSorted(nodes1,
				       xmlXPathNodeSetItem(nodes2, 0)));
}

/**
 * exsltSetsTrailing:
 * @nodes1:  a node-set
 * @nodes2:  a node-set
 *
 * Implements the EXSLT - Sets trailing() function:
 *    node-set set:trailing (node-set, node-set)
 * @nodes1 and @nodes2 are sorted by document order, then
 * #exsltSetsTrailingSorted is called.
 *
 * Returns the nodes in @nodes1 that follow the first node in @nodes2
 *         in document order, @nodes1 if @nodes2 is NULL or empty or
 *         an empty node-set if @nodes1 doesn't contain @nodes2
 */
xmlNodeSetPtr
exsltSetsTrailing (xmlNodeSetPtr nodes1, xmlNodeSetPtr nodes2) {
    if (xmlXPathNodeSetIsEmpty(nodes2))
	return(nodes1);
    if (xmlXPathNodeSetIsEmpty(nodes1))
	return(xmlXPathNodeSetCreate(NULL));
    xmlXPathNodeSetSort(nodes1);
    xmlXPathNodeSetSort(nodes2);
    return(exsltSetsNodeTrailingSorted(nodes1,
				       xmlXPathNodeSetItem(nodes2, 0)));
}

/**
 * exsltSetsTrailingFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  the number of arguments
 *
 * Wraps #exsltSetsTrailing for use by the XPath processor
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
    ret = exsltSetsNodeTrailingSorted(arg1, xmlXPathNodeSetItem(arg2, 0));

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
