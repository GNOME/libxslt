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
#ifdef HAVE_MATH_H
#include <math.h>
#endif
#ifdef HAVE_FLOAT_H
#include <float.h>
#endif
#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#ifdef HAVE_NAN_H
#include <nan.h>
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

#define DEBUG_FUNCTION

#ifndef FALSE
# define FALSE (0 == 1)
# define TRUE (1 == 1)
#endif

#define DIGIT_LIST "0123456789"
#define SYMBOL_QUOTE             ((xmlChar)'\'')
#define ID_STRING "ABCDEFGHIJKLMNOPQRSTUVWXYZ"


/************************************************************************
 *									*
 *			Utility functions				*
 *									*
 ************************************************************************/

#ifndef isnan
static int
isnan(volatile double number)
{
    return (!(number < 0.0 || number > 0.0) && (number != 0.0));
}
#endif

#ifndef isinf
static int
isinf(double number)
{
# ifdef HUGE_VAL
    return ((number == HUGE_VAL) ? 1 : ((number == -HUGE_VAL) ? -1 : 0));
# else
    return FALSE;
# endif
}
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
    xmlDocPtr doc;
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
	    doc = xmlParseDoc(URI);
	    if (doc == NULL)
		valuePush(ctxt, xmlXPathNewNodeSet(NULL));
	    else {
		xsltTransformContextPtr tctxt;

		/*
		 * link it to the context for cleanup when done
		 */
		tctxt = (xsltTransformContextPtr) ctxt->context->extra;
		if (tctxt == NULL) {
		    xsltGenericError(xsltGenericErrorContext,
			"document() : internal error tctxt == NULL\n");
		    xmlFreeDoc(doc);
		    valuePush(ctxt, xmlXPathNewNodeSet(NULL));
		} else {
		    /*
		     * Keep a link from the context to be able to deallocate
		     */
		    doc->next = (xmlNodePtr) tctxt->extraDocs;
		    tctxt->extraDocs = doc;

		    /* TODO: use XPointer of HTML location for fragment ID */
		    /* pbm #xxx can lead to location sets, not nodesets :-) */
                    valuePush(ctxt, xmlXPathNewNodeSet((xmlNodePtr) doc));
		}
	    }
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
    TODO /* function */
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
/*
 * JDK 1.1 DecimalFormat class:
 *
 * http://java.sun.com/products/jdk/1.1/docs/api/java.text.DecimalFormat.html
 *
 * Structure:
 *
 *   pattern    := subpattern{;subpattern}
 *   subpattern := {prefix}integer{.fraction}{suffix}
 *   prefix     := '\\u0000'..'\\uFFFD' - specialCharacters
 *   suffix     := '\\u0000'..'\\uFFFD' - specialCharacters
 *   integer    := '#'* '0'* '0'
 *   fraction   := '0'* '#'*
 *
 *   Notation:
 *    X*       0 or more instances of X
 *    (X | Y)  either X or Y.
 *    X..Y     any character from X up to Y, inclusive.
 *    S - T    characters in S, except those in T
 *
 * Special Characters:
 *
 *   Symbol Meaning
 *   0      a digit
 *   #      a digit, zero shows as absent
 *   .      placeholder for decimal separator
 *   ,      placeholder for grouping separator.
 *   ;      separates formats.
 *   -      default negative prefix.
 *   %      multiply by 100 and show as percentage
 *   ?      multiply by 1000 and show as per mille
 *   X      any other characters can be used in the prefix or suffix
 *   '      used to quote special characters in a prefix or suffix.
 */
/* TODO
 *
 * The JDK description does not tell where they may and may not appear
 * within the format string. Nor does it tell what happens to integer
 * values that does not fit into the format string.
 *
 *  Inf and NaN not tested.
 */
#define IS_SPECIAL(self,letter) \
    (((letter) == (self)->zeroDigit[0]) || \
     ((letter) == (self)->digit[0]) || \
     ((letter) == (self)->decimalPoint[0]) || \
     ((letter) == (self)->grouping[0]) || \
     ((letter) == (self)->minusSign[0]) || \
     ((letter) == (self)->percent[0]) || \
     ((letter) == (self)->permille[0]))

static xmlXPathError
xsltFormatNumberConversion(xsltDecimalFormatPtr self,
			   xmlChar *format,
			   double number,
			   xmlChar **result)
{
    xmlXPathError status = XPATH_EXPRESSION_OK;
    xmlChar *the_format;
    xmlBufferPtr buffer;
    char digit_buffer[2];
    int use_minus;
    int i, j;
    int length;
    int group;
    int integer_digits = 0;
    int integer_zeroes = 0;
    int fraction_digits = 0;
    int fraction_zeroes = 0;
    int decimal_point;
    double divisor;
    int digit;
    int is_percent = FALSE;
    int is_permille = FALSE;

    buffer = xmlBufferCreate();
    if (buffer == NULL) {
	status = XPATH_MEMORY_ERROR;
	goto DECIMAL_FORMAT_END;
    }

    /* Find positive or negative template */
    the_format = (xmlChar *)xmlStrchr(format,
				      self->patternSeparator[0]);
    if ((the_format != NULL) && (number < 0.0)) {
	/* Use negative template */
	the_format++;
	use_minus = FALSE;
    } else {
	/* Use positive template */
	if (the_format)
	    the_format[0] = 0;
	the_format = format;
	use_minus = (number < 0.0) ? TRUE : FALSE;
    }
  
    /* Prefix */
    length = xmlStrlen(the_format);
    for (i = 0; i < length; i++) {
	if (IS_SPECIAL(self, the_format[i])) {
	    break; /* for */
	} else {
	    if (the_format[i] == SYMBOL_QUOTE) {
		/* Quote character */
		i++;
	    }
	    xmlBufferAdd(buffer, &the_format[i], 1);
	}
    }

    if (isinf(number)) {
	xmlBufferCat(buffer, self->infinity);
	/* Skip until suffix */
	for ( ; i < length; i++) {
	    if (! IS_SPECIAL(self, the_format[i]))
		break; /* for */
	}
    } else if (isnan(number)) {
	xmlBufferCat(buffer, self->noNumber);
	/* Skip until suffix */
	for ( ; i < length; i++) {
	    if (! IS_SPECIAL(self, the_format[i]))
		break; /* for */
	}
    } else {
	
	/* Parse the number part of the format string */
	decimal_point = FALSE;
	group = 0;
	for ( ; i < length; i++) {
	    
	    if (the_format[i] == self->digit[0]) {
		if (decimal_point) {
		    if (fraction_zeroes > 0) {
			status = XPATH_EXPR_ERROR;
			goto DECIMAL_FORMAT_END;
		    }
		    fraction_digits++;
		} else {
		    integer_digits++;
		    group++;
		}
		
	    } else if (the_format[i] == self->zeroDigit[0]) {
		if (decimal_point)
		    fraction_zeroes++;
		else {
		    if (integer_digits > 0) {
			status = XPATH_EXPR_ERROR;
			goto DECIMAL_FORMAT_END;
		    }
		    integer_zeroes++;
		    group++;
		}
		
	    } else if (the_format[i] == self->grouping[0]) {
		if (decimal_point) {
		    status = XPATH_EXPR_ERROR;
		    goto DECIMAL_FORMAT_END;
		}
		group = 0;
		
	    } else if (the_format[i] == self->decimalPoint[0]) {
		if (decimal_point) {
		    status = XPATH_EXPR_ERROR;
		    goto DECIMAL_FORMAT_END;
		}
		decimal_point = TRUE;
		
	    } else
		break;
	}
	if (the_format[i] == self->percent[0]) {
	    is_percent = TRUE;
	} else if (the_format[i] == self->permille[0]) {
	    is_permille = TRUE;
	}
	
	/* Format the number */

	if (use_minus)
	    xmlBufferAdd(buffer, self->minusSign, 1);

	number = fabs(number);
	if (is_percent)
	    number /= 100.0;
	else if (is_permille)
	    number /= 1000.0;
	number = floor(0.5 + number * pow(10.0, (double)(fraction_digits + fraction_zeroes)));
	
	/* Integer part */
	digit_buffer[1] = (char)0;
	divisor = pow(10.0, (double)(integer_digits + integer_zeroes + fraction_digits + fraction_zeroes - 1));
	for (j = integer_digits + integer_zeroes; j > 0; j--) {
	    digit = (int)(number / divisor);
	    number -= (double)digit * divisor;
	    divisor /= 10.0;
	    if ((digit > 0) || (j <= integer_digits)) {
		digit_buffer[0] = DIGIT_LIST[digit];
		xmlBufferCCat(buffer, digit_buffer);
	    }
	}
	
	if (decimal_point)
	    xmlBufferAdd(buffer, self->decimalPoint, 1);

	/* Fraction part */
	for (j = fraction_digits + fraction_zeroes; j > 0; j--) {
	    digit = (int)(number / divisor);
	    number -= (double)digit * divisor;
	    divisor /= 10.0;
	    if ((digit > 0) || (j > fraction_zeroes)) {
		digit_buffer[0] = DIGIT_LIST[digit];
		xmlBufferCCat(buffer, digit_buffer);
	    }
	}
    }

    if (is_percent)
	xmlBufferAdd(buffer, self->percent, 1);
    else if (is_permille)
	xmlBufferAdd(buffer, self->permille, 1);
    
    /* Suffix */
    for ( ; i < length; i++) {
	if (the_format[i] == SYMBOL_QUOTE)
	    i++;
	xmlBufferAdd(buffer, &the_format[i], 1);
    }

 DECIMAL_FORMAT_END:
    if (status == XPATH_EXPRESSION_OK)
	*result = xmlStrdup(xmlBufferContent(buffer));
    xmlBufferFree(buffer);
    return status;
}

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
