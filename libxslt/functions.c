/*
 * functions.c: Implementation of the XSLT extra functions
 *
 * Reference:
 *   http://www.w3.org/TR/1999/REC-xslt-19991116
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
 * Bjorn Reese <breese@mail1.stofanet.dk> for number formatting
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
#define SYMBOL_PATTERN_SEPARATOR ((xmlChar)';')
#define SYMBOL_ZERO_DIGIT        ((xmlChar)'#')
#define SYMBOL_DIGIT             ((xmlChar)'0')
#define SYMBOL_DECIMAL_POINT     ((xmlChar)'.')
#define SYMBOL_GROUPING          ((xmlChar)',')
#define SYMBOL_MINUS             ((xmlChar)'-')
#define SYMBOL_PERCENT           ((xmlChar)'%')
#define SYMBOL_PERMILLE          ((xmlChar)'?')

static struct _xsltDecimalFormat globalDecimalFormat;

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
    TODO /* function */
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
    TODO /* function */
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
 *  The JDK description does not tell where percent and permille may
 *  and may not appear within the format string.
 *
 *  Error handling.
 *
 *  Inf and NaN not tested.
 */
#define IS_SPECIAL(self,letter) \
    ((letter == self->zeroDigit) || \
     (letter == self->digit) || \
     (letter == self->decimalPoint) || \
     (letter == self->grouping) || \
     (letter == self->minusSign) || \
     (letter == self->percent) || \
     (letter == self->permille)) \

static xmlChar *
PrivateDecimalFormat(xsltDecimalFormatPtr self,
		     xmlChar *format,
		     double number)
{
    xmlChar *the_format;
    xmlBufferPtr buffer;
    xmlChar *result;
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

    buffer = xmlBufferCreate();

    /* Find positive or negative template */
    the_format = (xmlChar *)xmlStrchr(format,
				      self->patternSeparator);
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
	    
	    if (the_format[i] == self->digit) {
		if (decimal_point) {
		    if (fraction_zeroes > 0)
			; /* Error in format */
		    fraction_digits++;
		} else {
		    integer_digits++;
		    group++;
		}
		
	    } else if (the_format[i] == self->zeroDigit) {
		if (decimal_point)
		    fraction_zeroes++;
		else {
		    if (integer_digits > 0)
			; /* Error in format */
		    integer_zeroes++;
		    group++;
		}
		
	    } else if (the_format[i] == self->grouping) {
		if (decimal_point)
		    ; /* Error in format */
		group = 0;
		
	    } else if (the_format[i] == self->decimalPoint) {
		if (decimal_point)
		    ; /* Error in format */
		decimal_point = TRUE;
		
	    } else
		break;
	}
	
	/* Format the number */

	if (use_minus)
	    xmlBufferAdd(buffer, &(self->minusSign), 1);

	number = fabs(number);
	number = floor(0.5 + number * pow(10.0, (double)(fraction_digits + fraction_zeroes)));
	
	/* Integer part */
	digit_buffer[1] = (char)0;
	j = integer_digits + integer_zeroes;
	divisor = pow(10.0, (double)(integer_digits + integer_zeroes + fraction_digits + fraction_zeroes - 1));
	for ( ; j > 0; j--) {
	    digit = (int)(number / divisor);
	    number -= (double)digit * divisor;
	    divisor /= 10.0;
	    if ((digit > 0) || (j <= integer_digits)) {
		digit_buffer[0] = DIGIT_LIST[digit];
		xmlBufferCCat(buffer, digit_buffer);
	    }
	}
	
	if (decimal_point)
	    xmlBufferAdd(buffer, &(self->decimalPoint), 1);

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

    /* Suffix */
    for ( ; i < length; i++) {
	if (the_format[i] == SYMBOL_QUOTE)
	    i++;
	xmlBufferAdd(buffer, &the_format[i], 1);
    }
    
    result = xmlStrdup(xmlBufferContent(buffer));
    xmlBufferFree(buffer);
    return result;
}

void
xsltFormatNumberFunction(xmlXPathParserContextPtr ctxt, int nargs)
{
    xmlXPathObjectPtr numberObj = NULL;
    xmlXPathObjectPtr formatObj = NULL;
    xmlXPathObjectPtr decimalObj = NULL;
    
    switch (nargs) {
    case 3:
	CAST_TO_STRING;
	decimalObj = valuePop(ctxt);
	globalDecimalFormat.decimalPoint = decimalObj->stringval[0]; /* hack */
	/* Intentional fall-through */
    case 2:
	CAST_TO_STRING;
	formatObj = valuePop(ctxt);
	CAST_TO_NUMBER;
	numberObj = valuePop(ctxt);
	break;
    default:
	XP_ERROR(XPATH_INVALID_ARITY);
    }
    
    valuePush(ctxt,
	      xmlXPathNewString(PrivateDecimalFormat(&globalDecimalFormat,
						     formatObj->stringval,
						     numberObj->floatval)));
    
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
    TODO /* function */
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
    TODO /* function */
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
    TODO /* function */
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
    TODO /* function */
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
    TODO /* function */
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
