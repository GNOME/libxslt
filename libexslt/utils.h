    /**
     * Many of these macros may later turn into functions. They
     * shouldn't be used in #ifdef's preprocessor instructions.
     */
 /**
 * xmlXPathSetError:
 * @ctxt:  an XPath parser context
 * @err:  an xmlXPathError code
 *
 * Raises an error.
 */
#define xmlXPathSetError(ctxt, err)					\
    { xmlXPathParserContextPtr tmpctxt = ctxt;				\
      xmlXPatherror((tmpctxt), __FILE__, __LINE__, (err));		\
      (tmpctxt)->error = (err); }
/**
 * xmlXPathSetArityError:
 * @ctxt:  an XPath parser context
 *
 * Raises an XPATH_INVALID_ARITY error
 */
#define xmlXPathSetArityError(ctxt)					\
    xmlXPathSetError((ctxt), XPATH_INVALID_ARITY)
/**
 * xmlXPathSetTypeError:
 * @ctxt:  an XPath parser context
 *
 * Raises an XPATH_INVALID_TYPE error
 */
#define xmlXPathSetTypeError(ctxt)					\
    xmlXPathSetError((ctxt), XPATH_INVALID_TYPE)
/**
 * xmlXPathGetError:
 * @ctxt:  an XPath parser context
 *
 * Returns the context error
 */
#define xmlXPathGetError(ctxt)	  ((ctxt)->error)
/**
 * xmlXPathCheckError:
 * @ctxt:  an XPath parser context
 *
 * Returns true if an error has been raised, false otherwise.
 */
#define xmlXPathCheckError(ctxt)  ((ctxt)->error != XPATH_EXPRESSION_OK)

/**
 * xmlXPathGetDocument:
 * @ctxt:  an XPath parser context
 *
 * Returns the context document
 */
#define xmlXPathGetDocument(ctxt)	((ctxt)->context->doc)
/**
 * xmlXPathGetContextNode:
 * @ctxt: an XPath parser context
 *
 * Returns the context node
 */
#define xmlXPathGetContextNode(ctxt)	((ctxt)->context->node)

int		xmlXPathPopBoolean	(xmlXPathParserContextPtr ctxt);
double		xmlXPathPopNumber	(xmlXPathParserContextPtr ctxt);
xmlChar *	xmlXPathPopString	(xmlXPathParserContextPtr ctxt);
xmlNodeSetPtr	xmlXPathPopNodeSet	(xmlXPathParserContextPtr ctxt);

/**
 * xmlXPathReturnBoolean:
 * @ctxt:  an XPath parser context
 * @val:  a boolean
 *
 * Pushes the boolean @val on the context stack
 */
#define xmlXPathReturnBoolean(ctxt, val)				\
    valuePush((ctxt), xmlXPathNewBoolean(val))
/**
 * xmlXPathReturnTrue:
 * @ctxt:  an XPath parser context
 *
 * Pushes true on the context stack
 */
#define xmlXPathReturnTrue(ctxt)   xmlXPathReturnBoolean((ctxt), 1)
/**
 * xmlXPathReturnFalse:
 * @ctxt:  an XPath parser context
 *
 * Pushes false on the context stack
 */
#define xmlXPathReturnFalse(ctxt)  xmlXPathReturnBoolean((ctxt), 0)
/**
 * xmlXPathReturnNumber:
 * @ctxt:  an XPath parser context
 * @val:  a double
 *
 * Pushes the double @val on the context stack
 */
#define xmlXPathReturnNumber(ctxt, val)					\
    valuePush((ctxt), xmlXPathNewFloat(val))
/**
 * xmlXPathReturnString:
 * @ctxt:  an XPath parser context
 * @str:  a string
 *
 * Pushes the string @str on the context stack
 */
#define xmlXPathReturnString(ctxt, str)					\
    valuePush((ctxt), xmlXPathWrapString(str))
/**
 * xmlXPathReturnEmptyString:
 * @ctxt: an XPath parser context
 *
 * Pushes an empty string on the context stack
 */
#define xmlXPathReturnEmptyString(ctxt)					\
    valuePush((ctxt), xmlXPathNewCString(""))
/**
 * xmlXPathReturnNodeSet:
 * @ctxt:  an XPath parser context
 * @ns:  a node-set
 *
 * Pushes the node-set @ns on the context stack
 */
#define xmlXPathReturnNodeSet(ctxt, ns)					\
    valuePush((ctxt), xmlXPathWrapNodeSet(ns))

/**
 * xmlXPathStackIsNodeSet:
 * @ctxt: an XPath parser context
 *
 * Returns true if the current object on the stack is a node-set
 */
#define xmlXPathStackIsNodeSet(ctxt)					\
    (((ctxt)->value != NULL) &&						\
     (((ctxt)->value->type == XPATH_NODESET) ||				\
      ((ctxt)->value->type == XPATH_XSLT_TREE)))

/**
 * xmlXPathNodeSetIsEmpty:
 * @ns: a node-set
 *
 * Returns #TRUE if @ns is an empty node-set
 */
#define xmlXPathNodeSetIsEmpty(ns)					\
    (((ns) == NULL) || ((ns)->nodeNr == 0) || ((ns)->nodeTab == NULL))

/**
 * xmlXPathEmptyNodeSet:
 * @ns: a node-set
 *
 * Empties a node-set
 */
#define xmlXPathEmptyNodeSet(ns)					\
    { while ((ns)->nodeNr > 0) (ns)->nodeTab[(ns)->nodeNr--] = NULL; }
