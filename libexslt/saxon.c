#define IN_LIBEXSLT
#include "libexslt/libexslt.h"

#if defined(WIN32) && !defined (__CYGWIN__) && (!__MINGW32__)
#include <win32config.h>
#else
#include "config.h"
#endif

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/parser.h>
#include <libxml/hash.h>

#include <libxslt/xsltconfig.h>
#include <libxslt/xsltutils.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/extensions.h>

#include "exslt.h"

/**
 * exsltSaxonInit:
 * @ctxt: an XSLT transformation context
 * @URI: the namespace URI for the extension
 *
 * Initializes the SAXON module.
 *
 * Returns the data for this transformation
 */
static xmlHashTablePtr
exsltSaxonInit (xsltTransformContextPtr ctxt ATTRIBUTE_UNUSED,
		const xmlChar *URI ATTRIBUTE_UNUSED) {
    return xmlHashCreate(1);
}

/**
 * exsltSaxonShutdown:
 * @ctxt: an XSLT transformation context
 * @URI: the namespace URI for the extension
 * @data: the module data to free up
 *
 * Shutdown the SAXON extension module
 */
static void
exsltSaxonShutdown (xsltTransformContextPtr ctxt ATTRIBUTE_UNUSED,
		    const xmlChar *URI ATTRIBUTE_UNUSED,
		    xmlHashTablePtr data) {
    xmlHashFree(data, (xmlHashDeallocator) xmlXPathFreeCompExpr);
}


/**
 * exsltSaxonExpressionFunction:
 * @ctxt: an XPath parser context
 * @nargs: the number of arguments
 *
 * The supplied string must contain an XPath expression. The result of
 * the function is a stored expression, which may be supplied as an
 * argument to other extension functions such as saxon:eval(),
 * saxon:sum() and saxon:distinct(). The result of the expression will
 * usually depend on the current node. The expression may contain
 * references to variables that are in scope at the point where
 * saxon:expression() is called: these variables will be replaced in
 * the stored expression with the values they take at the time
 * saxon:expression() is called, not the values of the variables at
 * the time the stored expression is evaluated.  Similarly, if the
 * expression contains namespace prefixes, these are interpreted in
 * terms of the namespace declarations in scope at the point where the
 * saxon:expression() function is called, not those in scope where the
 * stored expression is evaluated.
 *
 * TODO: current implementation doesn't conform to SAXON behaviour
 * regarding context and namespaces.
 */
static void
exsltSaxonExpressionFunction (xmlXPathParserContextPtr ctxt, int nargs) {
    xmlChar *arg;
    xmlXPathCompExprPtr ret;
    xmlHashTablePtr hash;
    xsltTransformContextPtr tctxt = xsltXPathGetTransformContext(ctxt);

    if (nargs != 1) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    arg = xmlXPathPopString(ctxt);
    if (xmlXPathCheckError(ctxt) || (arg == NULL)) {
	xmlXPathSetTypeError(ctxt);
	return;
    }

    hash = (xmlHashTablePtr) xsltGetExtData(tctxt,
					    ctxt->context->functionURI);

    ret = xmlHashLookup(hash, arg);

    if (ret == NULL) {
	 ret = xmlXPathCompile(arg);
	 if (ret == NULL) {
	      xmlFree(arg);
	      xsltGenericError(xsltGenericErrorContext,
			"{%s}:%s: argument is not an XPath expression\n",
			ctxt->context->functionURI, ctxt->context->function);
	      return;
	 }
	 xmlHashAddEntry(hash, arg, (void *) ret);
    }

    xmlFree(arg);

    xmlXPathReturnExternal(ctxt, ret);
}

/**
 * exsltSaxonEvalFunction:
 * @ctxt:  an XPath parser context
 * @nargs:  number of arguments
 *
 * Implements de SAXON eval() function:
 *    object saxon:eval (saxon:stored-expression)
 * Returns the result of evaluating the supplied stored expression.
 * A stored expression may be obtained as the result of calling
 * the saxon:expression() function.
 * The stored expression is evaluated in the current context, that
 * is, the context node is the current node, and the context position
 * and context size are the same as the result of calling position()
 * or last() respectively.
 */
static void
exsltSaxonEvalFunction (xmlXPathParserContextPtr ctxt, int nargs) {
     xmlXPathCompExprPtr expr;
     xmlXPathObjectPtr ret;

     if (nargs != 1) {
	  xmlXPathSetArityError(ctxt);
	  return;
     }

     if (!xmlXPathStackIsExternal(ctxt)) {
	  xmlXPathSetTypeError(ctxt);
	  return;
     }

     expr = (xmlXPathCompExprPtr) xmlXPathPopExternal(ctxt);

     ret = xmlXPathCompiledEval(expr, ctxt->context);

     valuePush(ctxt, ret);
}

/**
 * exsltSaxonEvaluateFunction:
 * @ctxt:  an XPath parser context
 * @nargs: number of arguments
 *
 * Implements the SAXON evaluate() function
 *     object saxon:evaluate (string)
 * The supplied string must contain an XPath expression. The result of
 * the function is the result of evaluating the XPath expression. This
 * is useful where an expression needs to be constructed at run-time or
 * passed to the stylesheet as a parameter, for example where the sort
 * key is determined dynamically. The context for the expression (e.g.
 * which variables and namespaces are available) is exactly the same as
 * if the expression were written explicitly at this point in the
 * stylesheet. The function saxon:evaluate(string) is shorthand for
 * saxon:eval(saxon:expression(string)).
 */
static void
exsltSaxonEvaluateFunction (xmlXPathParserContextPtr ctxt, int nargs) {
     if (nargs != 1) {
	  xmlXPathSetArityError(ctxt);
	  return;
     }

     exsltSaxonExpressionFunction(ctxt, 1);
     exsltSaxonEvalFunction(ctxt, 1);
}

/**
 * exsltSaxonRegister:
 *
 * Registers the SAXON extension module
 */
void
exsltSaxonRegister (void) {
     xsltRegisterExtModule (SAXON_NAMESPACE,
			    (xsltExtInitFunction) exsltSaxonInit,
			    (xsltExtShutdownFunction) exsltSaxonShutdown);
     xsltRegisterExtModuleFunction((const xmlChar *) "expression",
				   SAXON_NAMESPACE,
				   exsltSaxonExpressionFunction);
     xsltRegisterExtModuleFunction((const xmlChar *) "eval",
				   SAXON_NAMESPACE,
				   exsltSaxonEvalFunction);
     xsltRegisterExtModuleFunction((const xmlChar *) "evaluate",
				   SAXON_NAMESPACE,
				   exsltSaxonEvaluateFunction);
}
