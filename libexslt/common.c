#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <libxslt/xsltconfig.h>
#include <libxslt/xsltutils.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/extensions.h>
#include <libxslt/transform.h>
#include <libxslt/extra.h>
#include <libxslt/preproc.h>

#include "exslt.h"

static void
exsltObjectTypeFunction (xmlXPathParserContextPtr ctxt, int nargs) {
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


/**
 * exsltCommonRegister:
 *
 * Registers the EXSLT - Common module
 */

void
exsltCommonRegister (void) {
    xsltRegisterExtModuleFunction((const xmlChar *) "node-set",
				  EXSLT_COMMON_NAMESPACE,
				  xsltFunctionNodeSet);
    xsltRegisterExtModuleFunction((const xmlChar *) "object-type",
				  EXSLT_COMMON_NAMESPACE,
				  exsltObjectTypeFunction);
    xsltRegisterExtModuleElement((const xmlChar *) "document",
				 EXSLT_COMMON_NAMESPACE,
				 (xsltPreComputeFunction) xsltDocumentComp,
				 (xsltTransformFunction) xsltDocumentElem);
}
