/*
 * functions.c: Implementation of the XSLT extra functions
 *
 * Reference:
 *   http://www.w3.org/TR/1999/REC-xslt-19991116
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
 * Bjorn Reese <breese@users.sourceforge.net> for number formatting
 */

#include "xsltconfig.h"

#include <string.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/valid.h>
#include <libxml/hash.h>
#include <libxml/xmlerror.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/parserInternals.h>
#include <libxml/uri.h>
#include "xslt.h"
#include "xsltInternals.h"
#include "xsltutils.h"
#include "functions.h"
#include "numbersInternals.h"
#include "keys.h"
#include "documents.h"

#ifdef WITH_XSLT_DEBUG
#define WITH_XSLT_DEBUG_FUNCTION
#endif


/************************************************************************
 *									*
 *			Module interfaces				*
 *									*
 ************************************************************************/

/**
 * xsltDocumentFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the document() XSLT function
 *   node-set document(object, node-set?)
 */
void
xsltDocumentFunction(xmlXPathParserContextPtr ctxt, int nargs){
    xsltDocumentPtr doc;
    xmlXPathObjectPtr obj, obj2 = NULL;
    xmlChar *base = NULL, *URI;


    if ((nargs < 1) || (nargs > 2)) {
        xsltGenericError(xsltGenericErrorContext,
		"document() : invalid number of args %d\n", nargs);
	ctxt->error = XPATH_INVALID_ARITY;
	return;
    }
    if (ctxt->value == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "document() : invalid arg value\n");
	ctxt->error = XPATH_INVALID_TYPE;
	return;
    }

    if (nargs == 2) {
	if (ctxt->value->type != XPATH_NODESET) {
	    xsltGenericError(xsltGenericErrorContext,
		"document() : invalid arg expecting a nodeset\n");
	    ctxt->error = XPATH_INVALID_TYPE;
	    return;
	}

	obj2 = valuePop(ctxt);
    }

    if (ctxt->value->type == XPATH_NODESET) {
	int i;
	xmlXPathObjectPtr newobj, ret;

	obj = valuePop(ctxt);
	ret = xmlXPathNewNodeSet(NULL);

	if (obj->nodesetval) {
	    for (i = 0; i < obj->nodesetval->nodeNr; i++) {
		valuePush(ctxt,
			  xmlXPathNewNodeSet(obj->nodesetval->nodeTab[i]));
		xmlXPathStringFunction(ctxt, 1);
		if (nargs == 2) {
		    valuePush(ctxt, xmlXPathObjectCopy(obj2));
		} else {
		    valuePush(ctxt,
			      xmlXPathNewNodeSet(obj->nodesetval->nodeTab[i]));
		}
		xsltDocumentFunction(ctxt, 2);
		newobj = valuePop(ctxt);
		ret->nodesetval = xmlXPathNodeSetMerge(ret->nodesetval,
						       newobj->nodesetval);
		xmlXPathFreeObject(newobj);
	    }
	}

	xmlXPathFreeObject(obj);
	if (obj2 != NULL)
	    xmlXPathFreeObject(obj2);
	valuePush(ctxt, ret);
	return;
    }
    /*
     * Make sure it's converted to a string
     */
    xmlXPathStringFunction(ctxt, 1);
    if (ctxt->value->type != XPATH_STRING) {
	xsltGenericError(xsltGenericErrorContext,
	    "document() : invalid arg expecting a string\n");
	ctxt->error = XPATH_INVALID_TYPE;
	if (obj2 != NULL)
	    xmlXPathFreeObject(obj2);
	return;
    }
    obj = valuePop(ctxt);
    if (obj->stringval == NULL) {
	valuePush(ctxt, xmlXPathNewNodeSet(NULL));
    } else {
	if (obj2 != NULL) {
	    /* obj2 should be ordered in document order !!!!! */
	    base = xmlNodeGetBase(obj2->nodesetval->nodeTab[0]->doc,
				  obj->nodesetval->nodeTab[0]);
	} else {
	    xsltTransformContextPtr tctxt = ctxt->context->extra;
	    if ((tctxt != NULL) && (tctxt->inst != NULL)) {
		base = xmlNodeGetBase(tctxt->inst->doc, tctxt->inst);
	    } else if ((tctxt != NULL) && (tctxt->style != NULL) &&
		       (tctxt->style->doc != NULL)) {
		base = xmlNodeGetBase(tctxt->style->doc, 
			              (xmlNodePtr) tctxt->style->doc);
	    }
	}
	URI = xmlBuildURI(obj->stringval, base);
	if (base != NULL)
	    xmlFree(base);
        if (URI == NULL) {
	    valuePush(ctxt, xmlXPathNewNodeSet(NULL));
	} else {
	    xsltTransformContextPtr tctxt;

	    tctxt = (xsltTransformContextPtr) ctxt->context->extra;
	    if (tctxt == NULL) {
		xsltGenericError(xsltGenericErrorContext,
			"document() : internal error tctxt == NULL\n");
		valuePush(ctxt, xmlXPathNewNodeSet(NULL));
	    } else {
		doc = xsltLoadDocument(tctxt, URI);
		if (doc == NULL)
		    valuePush(ctxt, xmlXPathNewNodeSet(NULL));
		else {
		    /* TODO: use XPointer of HTML location for fragment ID */
		    /* pbm #xxx can lead to location sets, not nodesets :-) */
		    valuePush(ctxt, xmlXPathNewNodeSet((xmlNodePtr) doc->doc));
		}
	    }
	    xmlFree(URI);
	}
    }
    xmlXPathFreeObject(obj);
    if (obj2 != NULL)
	xmlXPathFreeObject(obj2);
}

/**
 * xsltKeyFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the key() XSLT function
 *   node-set key(string, object)
 */
void
xsltKeyFunction(xmlXPathParserContextPtr ctxt, int nargs){
    xmlNodeSetPtr nodelist;
    xmlXPathObjectPtr obj1, obj2;
    xmlChar *key = NULL, *value;
    const xmlChar *keyURI;
    xsltTransformContextPtr tctxt;

    tctxt = ((xsltTransformContextPtr)ctxt->context->extra);

    if (nargs != 2) {
        xsltGenericError(xsltGenericErrorContext,
		"key() : expects two arguments\n");
	ctxt->error = XPATH_INVALID_ARITY;
	return;
    }

    obj2 = valuePop(ctxt);
    if ((obj2 == NULL) ||
	(ctxt->value == NULL) || (ctxt->value->type != XPATH_STRING)) {
	xsltGenericError(xsltGenericErrorContext,
	    "key() : invalid arg expecting a string\n");
	ctxt->error = XPATH_INVALID_TYPE;
	xmlXPathFreeObject(obj2);

	return;
    }
    obj1 = valuePop(ctxt);

    if (obj2->type == XPATH_NODESET) {
	int i;
	xmlXPathObjectPtr newobj, ret;

	ret = xmlXPathNewNodeSet(NULL);

	if (obj2->nodesetval != NULL) {
	    for (i = 0; i < obj2->nodesetval->nodeNr; i++) {
		valuePush(ctxt, xmlXPathObjectCopy(obj1));
		valuePush(ctxt,
			  xmlXPathNewNodeSet(obj2->nodesetval->nodeTab[i]));
		xmlXPathStringFunction(ctxt, 1);
		xsltKeyFunction(ctxt, 2);
		newobj = valuePop(ctxt);
		ret->nodesetval = xmlXPathNodeSetMerge(ret->nodesetval,
						       newobj->nodesetval);
		xmlXPathFreeObject(newobj);
	    }
	}
	valuePush(ctxt, ret);
    } else {
	xmlChar *qname, *prefix;

	/*
	 * Get the associated namespace URI if qualified name
	 */
	qname = obj1->stringval;
	key = xmlSplitQName2(qname, &prefix);
	if (key == NULL) {
	    key = xmlStrdup(obj1->stringval);
	    keyURI = NULL;
	    if (prefix != NULL)
		xmlFree(prefix);
	} else {
	    if (prefix != NULL) {
		keyURI = xmlXPathNsLookup(ctxt->context, prefix);
		if (keyURI == NULL) {
		    xsltGenericError(xsltGenericErrorContext,
			"key() : prefix %s is not bound\n", prefix);
		}
		xmlFree(prefix);
	    } else {
		keyURI = NULL;
	    }
	}

	/*
	 * Force conversion of first arg to string
	 */
	valuePush(ctxt, obj2);
	xmlXPathStringFunction(ctxt, 1);
	if ((ctxt->value == NULL) || (ctxt->value->type != XPATH_STRING)) {
	    xsltGenericError(xsltGenericErrorContext,
		"key() : invalid arg expecting a string\n");
	    ctxt->error = XPATH_INVALID_TYPE;
	    xmlXPathFreeObject(obj1);

	    return;
	}
	obj2 = valuePop(ctxt);
	value = obj2->stringval;

	nodelist = xsltGetKey(tctxt, key, keyURI, value);
	valuePush(ctxt, xmlXPathWrapNodeSet(
		        xmlXPathNodeSetMerge(NULL, nodelist)));
    }


    if (obj1 != NULL)
	xmlXPathFreeObject(obj1);
    if (obj2 != NULL)
	xmlXPathFreeObject(obj2);
    if (key != NULL)
	xmlFree(key);
}

/**
 * xsltUnparsedEntityURIFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the unparsed-entity-uri() XSLT function
 *   string unparsed-entity-uri(string)
 */
void
xsltUnparsedEntityURIFunction(xmlXPathParserContextPtr ctxt, int nargs){
    xmlXPathObjectPtr obj;
    xmlChar *str;

    if ((nargs != 1) || (ctxt->value == NULL)) {
        xsltGenericError(xsltGenericErrorContext,
		"unparsed-entity-uri() : expects one string arg\n");
	ctxt->error = XPATH_INVALID_ARITY;
	return;
    }
    obj = valuePop(ctxt);
    if (obj->type != XPATH_STRING) {
	obj = xmlXPathConvertString(obj);
    }

    str = obj->stringval;
    if (str == NULL) {
	valuePush(ctxt, xmlXPathNewString((const xmlChar *)""));
    } else {
	xmlEntityPtr entity;

	entity = xmlGetDocEntity(ctxt->context->doc, str);
	if (entity == NULL) {
	    valuePush(ctxt, xmlXPathNewString((const xmlChar *)""));
	} else {
	    if (entity->URI != NULL)
		valuePush(ctxt, xmlXPathNewString(entity->URI));
	    else
		valuePush(ctxt, xmlXPathNewString((const xmlChar *)""));
	}
    }
    xmlXPathFreeObject(obj);
}

/**
 * xsltFormatNumberFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the format-number() XSLT function
 *   string format-number(number, string, string?)
 */
void
xsltFormatNumberFunction(xmlXPathParserContextPtr ctxt, int nargs)
{
    xmlXPathObjectPtr numberObj = NULL;
    xmlXPathObjectPtr formatObj = NULL;
    xmlXPathObjectPtr decimalObj = NULL;
    xsltStylesheetPtr sheet;
    xsltDecimalFormatPtr formatValues;
    xmlChar *result;

    sheet = ((xsltTransformContextPtr)ctxt->context->extra)->style;
    formatValues = sheet->decimalFormat;
    
    switch (nargs) {
    case 3:
	CAST_TO_STRING;
	decimalObj = valuePop(ctxt);
	formatValues = xsltDecimalFormatGetByName(sheet, decimalObj->stringval);
	/* Intentional fall-through */
    case 2:
	CAST_TO_STRING;
	formatObj = valuePop(ctxt);
	CAST_TO_NUMBER;
	numberObj = valuePop(ctxt);
	break;
    default:
	XP_ERROR(XPATH_INVALID_ARITY);
	return;
    }

    if (xsltFormatNumberConversion(formatValues,
				   formatObj->stringval,
				   numberObj->floatval,
				   &result) == XPATH_EXPRESSION_OK) {
	valuePush(ctxt, xmlXPathNewString(result));
	xmlFree(result);
    }
    
    xmlXPathFreeObject(numberObj);
    xmlXPathFreeObject(formatObj);
    xmlXPathFreeObject(decimalObj);
}

/**
 * xsltGenerateIdFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the generate-id() XSLT function
 *   string generate-id(node-set?)
 */
void
xsltGenerateIdFunction(xmlXPathParserContextPtr ctxt, int nargs){
    xmlNodePtr cur = NULL;
    unsigned long val;
    xmlChar str[20];

    if (nargs == 0) {
	cur = ctxt->context->node;
    } else if (nargs == 1) {
	xmlXPathObjectPtr obj;
	xmlNodeSetPtr nodelist;
	int i, ret;

	if ((ctxt->value == NULL) || (ctxt->value->type != XPATH_NODESET)) {
	    ctxt->error = XPATH_INVALID_TYPE;
	    xsltGenericError(xsltGenericErrorContext,
		"generate-id() : invalid arg expecting a node-set\n");
	    return;
	}
	obj = valuePop(ctxt);
	nodelist = obj->nodesetval;
	if ((nodelist == NULL) || (nodelist->nodeNr <= 0)) {
	    xmlXPathFreeObject(obj);
	    valuePush(ctxt, xmlXPathNewCString(""));
	    return;
	}
	cur = nodelist->nodeTab[0];
	for (i = 1;i < nodelist->nodeNr;i++) {
	    ret = xmlXPathCmpNodes(cur, nodelist->nodeTab[i]);
	    if (ret == -1)
	        cur = nodelist->nodeTab[i];
	}
	xmlXPathFreeObject(obj);
    } else {
        xsltGenericError(xsltGenericErrorContext,
		"generate-id() : invalid number of args %d\n", nargs);
	ctxt->error = XPATH_INVALID_ARITY;
	return;
    }
    /*
     * Okay this is ugly but should work, use the NodePtr address
     * to forge the ID
     */
    val = (unsigned long)((char *)cur - (char *)0);
    val /= sizeof(xmlNode);
    sprintf((char *)str, "id%ld", val);
    valuePush(ctxt, xmlXPathNewString(str));
}

/**
 * xsltSystemPropertyFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the system-property() XSLT function
 *   object system-property(string)
 */
void
xsltSystemPropertyFunction(xmlXPathParserContextPtr ctxt, int nargs){
    xmlXPathObjectPtr obj;
    xmlChar *prefix, *name;
    const xmlChar *nsURI = NULL;

    if (nargs != 1) {
        xsltGenericError(xsltGenericErrorContext,
		"system-property() : expects one string arg\n");
	ctxt->error = XPATH_INVALID_ARITY;
	return;
    }
    if ((ctxt->value == NULL) || (ctxt->value->type != XPATH_STRING)) {
	xsltGenericError(xsltGenericErrorContext,
	    "system-property() : invalid arg expecting a string\n");
	ctxt->error = XPATH_INVALID_TYPE;
	return;
    }
    obj = valuePop(ctxt);
    if (obj->stringval == NULL) {
	valuePush(ctxt, xmlXPathNewString((const xmlChar *)""));
    } else {
	name = xmlSplitQName2(obj->stringval, &prefix);
	if (name == NULL) {
	    name = xmlStrdup(obj->stringval);
	} else {
	    nsURI = xmlXPathNsLookup(ctxt->context, prefix);
	    if (nsURI == NULL) {
		xsltGenericError(xsltGenericErrorContext,
		    "system-property(): prefix %s is not bound\n", prefix);
	    }
	}

	if (!xmlStrcmp(nsURI, XSLT_NAMESPACE)) {
	    if (!xmlStrcmp(name, (const xmlChar *)"version")) {
		valuePush(ctxt, xmlXPathNewString(
		    (const xmlChar *)XSLT_DEFAULT_VERSION));
	    } else if (!xmlStrcmp(name, (const xmlChar *)"vendor")) {
		valuePush(ctxt, xmlXPathNewString(
		    (const xmlChar *)XSLT_DEFAULT_VENDOR));
	    } else if (!xmlStrcmp(name, (const xmlChar *)"vendor-url")) {
		valuePush(ctxt, xmlXPathNewString(
		    (const xmlChar *)XSLT_DEFAULT_URL));
	    } else {
		valuePush(ctxt, xmlXPathNewString((const xmlChar *)""));
	    }
	}
	if (name != NULL)
	    xmlFree(name);
	if (prefix != NULL)
	    xmlFree(prefix);
    }
    xmlXPathFreeObject(obj);
}

/**
 * xsltElementAvailableFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the element-available() XSLT function
 *   boolean element-available(string)
 */
void
xsltElementAvailableFunction(xmlXPathParserContextPtr ctxt, int nargs){
    xmlXPathObjectPtr obj;

    if (nargs != 1) {
        xsltGenericError(xsltGenericErrorContext,
		"element-available() : expects one string arg\n");
	ctxt->error = XPATH_INVALID_ARITY;
	return;
    }
    if ((ctxt->value == NULL) || (ctxt->value->type != XPATH_STRING)) {
	xsltGenericError(xsltGenericErrorContext,
	    "element-available invalid arg expecting a string\n");
	ctxt->error = XPATH_INVALID_TYPE;
	return;
    }
    obj = valuePop(ctxt);
    xmlXPathFreeObject(obj);
    valuePush(ctxt, xmlXPathNewBoolean(0));
}

/**
 * xsltFunctionAvailableFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the function-available() XSLT function
 *   boolean function-available(string)
 */
void
xsltFunctionAvailableFunction(xmlXPathParserContextPtr ctxt, int nargs){
    xmlXPathObjectPtr obj;
    xmlChar *prefix, *name;
    const xmlChar *nsURI = NULL;

    if (nargs != 1) {
        xsltGenericError(xsltGenericErrorContext,
		"function-available() : expects one string arg\n");
	ctxt->error = XPATH_INVALID_ARITY;
	return;
    }
    if ((ctxt->value == NULL) || (ctxt->value->type != XPATH_STRING)) {
	xsltGenericError(xsltGenericErrorContext,
	    "function-available invalid arg expecting a string\n");
	ctxt->error = XPATH_INVALID_TYPE;
	return;
    }
    obj = valuePop(ctxt);

    name = xmlSplitQName2(obj->stringval, &prefix);
    if (name == NULL) {
	name = xmlStrdup(obj->stringval);
    } else {
	nsURI = xmlXPathNsLookup(ctxt->context, prefix);
	if (nsURI == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		"function-available(): prefix %s is not bound\n", prefix);
	}
    }

    if (xmlXPathFunctionLookupNS(ctxt->context, name, nsURI) != NULL) {
	valuePush(ctxt, xmlXPathNewBoolean(1));
    } else {
	valuePush(ctxt, xmlXPathNewBoolean(0));
    }

    xmlXPathFreeObject(obj);
    if (name != NULL)
	xmlFree(name);
    if (prefix != NULL)
	xmlFree(prefix);
}

/**
 * xsltCurrentFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the current() XSLT function
 *   node-set current()
 */
static void
xsltCurrentFunction(xmlXPathParserContextPtr ctxt, int nargs){
    xsltTransformContextPtr tctxt;

    if (nargs != 0) {
        xsltGenericError(xsltGenericErrorContext,
		"document() : function uses no argument\n");
	ctxt->error = XPATH_INVALID_ARITY;
	return;
    }
    tctxt = (xsltTransformContextPtr) ctxt->context->extra;
    if (tctxt == NULL) {
	xsltGenericError(xsltGenericErrorContext,
		"current() : internal error tctxt == NULL\n");
	valuePush(ctxt, xmlXPathNewNodeSet(NULL));
    } else {
	valuePush(ctxt, xmlXPathNewNodeSet(tctxt->node)); /* current */
    }
}

/**
 * xsltRegisterAllFunctions:
 * @ctxt:  the XPath context
 *
 * Registers all default XSLT functions in this context
 */
void
xsltRegisterAllFunctions(xmlXPathContextPtr ctxt) {
    xmlXPathRegisterFunc(ctxt, (const xmlChar *)"current",
                         xsltCurrentFunction);
    xmlXPathRegisterFunc(ctxt, (const xmlChar *)"document",
                         xsltDocumentFunction);
    xmlXPathRegisterFunc(ctxt, (const xmlChar *)"key",
                         xsltKeyFunction);
    xmlXPathRegisterFunc(ctxt, (const xmlChar *)"unparsed-entity-uri",
                         xsltUnparsedEntityURIFunction);
    xmlXPathRegisterFunc(ctxt, (const xmlChar *)"format-number",
                         xsltFormatNumberFunction);
    xmlXPathRegisterFunc(ctxt, (const xmlChar *)"generate-id",
                         xsltGenerateIdFunction);
    xmlXPathRegisterFunc(ctxt, (const xmlChar *)"system-property",
                         xsltSystemPropertyFunction);
    xmlXPathRegisterFunc(ctxt, (const xmlChar *)"element-available",
                         xsltElementAvailableFunction);
    xmlXPathRegisterFunc(ctxt, (const xmlChar *)"function-available",
                         xsltFunctionAvailableFunction);
}
