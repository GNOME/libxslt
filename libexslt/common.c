#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <libxslt/xsltutils.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/extensions.h>
#include <libxslt/transform.h>

#include "exslt.h"
#include "utils.h"

/**
 * exslNodeSetFunction:
 * @ctxt:  an XPath parser context
 *
 * Implements the EXSLT - Common node-set function:
 *    node-set exsl:node-set (result-tree-fragment)
 * for use by the XPath processor.
 */
void
exslNodeSetFunction(xmlXPathParserContextPtr ctxt, int nargs){
    if (nargs != 1) {
	xmlXPathSetArityError(ctxt);
        return;
    }
    if (!xmlXPathStackIsNodeSet(ctxt)) {
	xmlXPathSetTypeError(ctxt);
        return;
    }
    ctxt->value->type = XPATH_NODESET;
    ctxt->value->boolval = 1;
}

void
exslObjectTypeFunction (xmlXPathParserContextPtr ctxt, int nargs) {
    xmlXPathObjectPtr obj, ret;

    if (nargs != 1) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    obj = valuePop(ctxt);

    switch (obj->type) {
    case XPATH_STRING:
	ret = xmlXPathNewCString("string");
	break;
    case XPATH_NUMBER:
	ret = xmlXPathNewCString("number");
	break;
    case XPATH_BOOLEAN:
	ret = xmlXPathNewCString("boolean");
	break;
    case XPATH_NODESET:
	ret = xmlXPathNewCString("node-set");
	break;
    case XPATH_XSLT_TREE:
	ret = xmlXPathNewCString("RTF");
	break;
    case XPATH_USERS:
	ret = xmlXPathNewCString("external");
	break;
    default:
	xsltGenericError(xsltGenericErrorContext,
		"object-type() invalid arg\n");
	ctxt->error = XPATH_INVALID_TYPE;
	xmlXPathFreeObject(obj);
	return;
    }
    xmlXPathFreeObject(obj);
    valuePush(ctxt, ret);
}


static void *
exslCommonInit (xsltTransformContextPtr ctxt, const xmlChar *URI) {
    xsltRegisterExtFunction (ctxt, (const xmlChar *) "node-set",
			     URI, exslNodeSetFunction);
    xsltRegisterExtFunction (ctxt, (const xmlChar *) "object-type",
			     URI, exslObjectTypeFunction);

    xsltRegisterExtElement (ctxt, (const xmlChar *) "document",
			    URI, xsltDocumentElem);

    return(NULL);
}

void
exslCommonRegister (void) {
    xsltRegisterExtModule (EXSLT_COMMON_NAMESPACE,
			   exslCommonInit, NULL);
}
