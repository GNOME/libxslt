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

#define DEBUG_FUNCTION


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
    xmlXPathObjectPtr obj;
    xmlChar *base, *URI;


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
    if (ctxt->value->type == XPATH_NODESET) {
	TODO
        xsltGenericError(xsltGenericErrorContext,
		"document() : with node-sets args not yet supported\n");
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
	return;
    }
    obj = valuePop(ctxt);
    if (obj->stringval == NULL) {
	valuePush(ctxt, xmlXPathNewNodeSet(NULL));
    } else {
        base = xmlNodeGetBase(ctxt->context->doc, ctxt->context->node);
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
    xmlXPathObjectPtr obj, tmp;
    xmlChar *key, *value;
    xsltTransformContextPtr tctxt;

    tctxt = ((xsltTransformContextPtr)ctxt->context->extra);

    if (nargs != 2) {
        xsltGenericError(xsltGenericErrorContext,
		"key() : expects one string arg\n");
	ctxt->error = XPATH_INVALID_ARITY;
	return;
    }

    obj = valuePop(ctxt);
    if ((obj == NULL) ||
	(ctxt->value == NULL) || (ctxt->value->type != XPATH_STRING)) {
	xsltGenericError(xsltGenericErrorContext,
	    "generate-id() : invalid arg expecting a string\n");
	ctxt->error = XPATH_INVALID_TYPE;
	xmlXPathFreeObject(obj);

	return;
    }
    tmp = valuePop(ctxt);
    key = tmp->stringval;

    /* TODO: find URI when qualified name */
    
    if (obj->type == XPATH_NODESET) {
	TODO /* handle NODE set as 2nd args of key() */
    } else {
	/*
	 * Force conversion of first arg to string
	 */
	valuePush(ctxt, obj);
	xmlXPathStringFunction(ctxt, 1);
	if ((ctxt->value == NULL) || (ctxt->value->type != XPATH_STRING)) {
	    xsltGenericError(xsltGenericErrorContext,
		"generate-id() : invalid arg expecting a string\n");
	    ctxt->error = XPATH_INVALID_TYPE;
	    xmlXPathFreeObject(obj);

	    return;
	}
	obj = valuePop(ctxt);
	value = obj->stringval;

	nodelist = xsltGetKey(tctxt, key, NULL, value);
	valuePush(ctxt, xmlXPathWrapNodeSet(
		        xmlXPathNodeSetMerge(NULL, nodelist)));
    }


    if (obj != NULL)
	xmlXPathFreeObject(obj);
    if (tmp != NULL)
	xmlXPathFreeObject(tmp);
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

    if (nargs != 1) {
        xsltGenericError(xsltGenericErrorContext,
		"system-property() : expects one string arg\n");
	ctxt->error = XPATH_INVALID_ARITY;
	return;
    }
    if ((ctxt->value == NULL) || (ctxt->value->type != XPATH_STRING)) {
	xsltGenericError(xsltGenericErrorContext,
	    "generate-id() : invalid arg expecting a string\n");
	ctxt->error = XPATH_INVALID_TYPE;
	return;
    }
    obj = valuePop(ctxt);
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
		valuePush(ctxt, xmlXPathNewString(
			    xmlStrdup((const xmlChar *)"")));
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
	    ctxt->error = XPATH_INVALID_TYPE;
	    xsltGenericError(xsltGenericErrorContext,
		"generate-id() : got an empty node-set\n");
	    xmlXPathFreeObject(obj);
	    return;
	}
	cur = nodelist->nodeTab[0];
	for (i = 2;i <= nodelist->nodeNr;i++) {
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
    val |= 0xFFFFFF;
    sprintf((char *)str, "id%10ld", val);
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
    xmlChar *str;

    if (nargs != 1) {
        xsltGenericError(xsltGenericErrorContext,
		"system-property() : expects one string arg\n");
	ctxt->error = XPATH_INVALID_ARITY;
	return;
    }
    if ((ctxt->value == NULL) || (ctxt->value->type != XPATH_STRING)) {
	xsltGenericError(xsltGenericErrorContext,
	    "generate-id() : invalid arg expecting a string\n");
	ctxt->error = XPATH_INVALID_TYPE;
	return;
    }
    obj = valuePop(ctxt);
    str = obj->stringval;
    if (str == NULL) {
	valuePush(ctxt, xmlXPathNewString((const xmlChar *)""));
    } else if (!xmlStrcmp(str, (const xmlChar *)"xsl:version")) {
	valuePush(ctxt, xmlXPathNewString(
		(const xmlChar *)XSLT_DEFAULT_VERSION));
    } else if (!xmlStrcmp(str, (const xmlChar *)"xsl:vendor")) {
	valuePush(ctxt, xmlXPathNewString(
		(const xmlChar *)XSLT_DEFAULT_VENDOR));
    } else if (!xmlStrcmp(str, (const xmlChar *)"xsl:vendor-url")) {
	valuePush(ctxt, xmlXPathNewString(
		(const xmlChar *)XSLT_DEFAULT_URL));
    } else {
	/* TODO cheated with the QName resolution */
	valuePush(ctxt, xmlXPathNewString((const xmlChar *)""));
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
    xmlXPathFreeObject(obj);
    valuePush(ctxt, xmlXPathNewBoolean(0));
}

/**
 * xsltCurrentFunction:
 * @ctxt:  the XPath Parser context
 * @nargs:  the number of arguments
 *
 * Implement the current() XSLT function
 *   node-set current()
 */
void
xsltCurrentFunction(xmlXPathParserContextPtr ctxt, int nargs){
    if (nargs != 0) {
        xsltGenericError(xsltGenericErrorContext,
		"document() : function uses no argument\n");
	ctxt->error = XPATH_INVALID_ARITY;
	return;
    }
    valuePush(ctxt, xmlXPathNewNodeSet(ctxt->context->node));
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
