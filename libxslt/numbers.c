/*
 * numbers.c: Implementation of the XSLT number functions
 *
 * Reference:
 *   http://www.w3.org/TR/1999/REC-xslt-19991116
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
 * Bjorn Reese <breese@users.sourceforge.net>
 */

#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <float.h>

#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#ifdef HAVE_NAN_H
#include <nan.h>
#endif

#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include "xsltutils.h"
#include "numbersInternals.h"

#ifndef FALSE
# define FALSE (0 == 1)
# define TRUE (1 == 1)
#endif

#define SYMBOL_QUOTE             ((xmlChar)'\'')

static char digit_list[] = "0123456789";
static char alpha_upper_list[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static char alpha_lower_list[] = "abcdefghijklmnopqrstuvwxyz";

/************************************************************************
 *									*
 *			Utility functions				*
 *									*
 ************************************************************************/

#define IS_SPECIAL(self,letter) \
    (((letter) == (self)->zeroDigit[0]) || \
     ((letter) == (self)->digit[0]) || \
     ((letter) == (self)->decimalPoint[0]) || \
     ((letter) == (self)->grouping[0]) || \
     ((letter) == (self)->minusSign[0]) || \
     ((letter) == (self)->percent[0]) || \
     ((letter) == (self)->permille[0]))

#define IS_DIGIT_ZERO(x) xsltIsDigitZero(x)
#define IS_DIGIT_ONE(x) xsltIsDigitZero((x)-1)

static int
xsltIsDigitZero(xmlChar ch)
{
    /*
     * Reference: ftp://ftp.unicode.org/Public/UNIDATA/UnicodeData.txt
     */
    switch (ch) {
    case 0x0030: case 0x0660: case 0x06F0: case 0x0966:
    case 0x09E6: case 0x0A66: case 0x0AE6: case 0x0B66:
    case 0x0C66: case 0x0CE6: case 0x0D66: case 0x0E50:
    case 0x0E60: case 0x0F20: case 0x1040: case 0x17E0:
    case 0x1810: case 0xFF10:
	return TRUE;
    default:
	return FALSE;
    }
}

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
 *
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
xmlXPathError
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
    int group = 0;
    int integer_digits = 0;
    int integer_hash = 0;
    int fraction_digits = 0;
    int fraction_hash = 0;
    int decimal_point;
    double divisor;
    int digit;
    int is_percent;
    int is_permille;

    buffer = xmlBufferCreate();
    if (buffer == NULL) {
	return XPATH_MEMORY_ERROR;
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
  
    /*
     * Prefix
     */
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

    /* Handle infinity and not-a-number first */
    switch (isinf(number)) {
    case -1:
	xmlBufferCat(buffer, self->minusSign);
	/* Intentional fall-through */
    case 1:
	xmlBufferCat(buffer, self->infinity);
	for ( ; i < length; i++) {
	    if (! IS_SPECIAL(self, the_format[i]))
		break; /* for */
	}
	goto DECIMAL_FORMAT_SUFFIX;
	
    default:
	if (isnan(number)) {
	    xmlBufferCat(buffer, self->noNumber);
	    /* Skip until suffix */
	    for ( ; i < length; i++) {
		if (! IS_SPECIAL(self, the_format[i]))
		    break; /* for */
	    }
	    goto DECIMAL_FORMAT_SUFFIX;
	}
	break;
    }
	
    /*
     * Parse the number part of the format string
     */
    for ( ; i < length; i++) {
	if (the_format[i] == self->zeroDigit[0]) {
	    integer_hash++;
	    group++;
	} else if (the_format[i] == self->grouping[0]) {
	    group = 0; /* Reset count */
	} else
	    break; /* for */
    }
    for ( ; i < length; i++) {
	if (the_format[i] == self->digit[0]) {
	    integer_digits++;
	    group++;
	} else if (the_format[i] == self->grouping[0]) {
	    group = 0; /* Reset count */
	} else
	    break; /* for */
    }
    decimal_point = (the_format[i] == self->decimalPoint[0]) ? TRUE : FALSE;
    if (decimal_point) {
	for ( ; i < length; i++) {
	    if (the_format[i] == self->digit[0]) {
		fraction_digits++;
	    } else
		break; /* for */
	}
	for ( ; i < length; i++) {
	    if (the_format[i] == self->zeroDigit[0]) {
		fraction_hash++;
	    } else
		break; /* for */
	}
    }
    is_percent = (the_format[i] == self->percent[0]) ? TRUE : FALSE;
    is_permille = (the_format[i] == self->permille[0]) ? TRUE : FALSE;
    
    /*
     * Format the number
     */

    if (use_minus)
	xmlBufferAdd(buffer, self->minusSign, 1);

    number = fabs(number);
    if (is_percent)
	number *= 100.0;
    else if (is_permille)
	number *= 1000.0;
    number = floor(0.5 + number * pow(10.0, (double)(fraction_digits + fraction_hash)));
	
    /* Integer part */
    digit_buffer[1] = (char)0;
    divisor = pow(10.0, (double)(integer_digits + integer_hash + fraction_digits + fraction_hash - 1));
    for (j = integer_digits + integer_hash; j > 0; j--) {
	digit = (int)(number / divisor);
	number -= (double)digit * divisor;
	divisor /= 10.0;
	if ((digit > 0) || (j <= integer_digits)) {
	    digit_buffer[0] = digit_list[digit];
	    xmlBufferCCat(buffer, digit_buffer);
	}
    }
	
    if (decimal_point)
	xmlBufferAdd(buffer, self->decimalPoint, 1);

    /* Fraction part */
    for (j = fraction_digits + fraction_hash; j > 0; j--) {
	digit = (int)(number / divisor);
	number -= (double)digit * divisor;
	divisor /= 10.0;
	if ((digit > 0) || (j > fraction_hash)) {
	    digit_buffer[0] = digit_list[digit];
	    xmlBufferCCat(buffer, digit_buffer);
	}
    }

    if (is_percent)
	xmlBufferAdd(buffer, self->percent, 1);
    else if (is_permille)
	xmlBufferAdd(buffer, self->permille, 1);

 DECIMAL_FORMAT_SUFFIX:
    /*
     * Suffix
     */
    for ( ; i < length; i++) {
	if (the_format[i] == SYMBOL_QUOTE)
	    i++;
	xmlBufferAdd(buffer, &the_format[i], 1);
    }

    *result = xmlStrdup(xmlBufferContent(buffer));
    xmlBufferFree(buffer);
    return status;
}

/************************************************************************
 *
 * xsltNumberFormat
 *
 * Convert one number.
 */
static void
xsltNumberFormatDecimal(xmlBufferPtr buffer,
			double number,
			xmlChar digit_zero,
			int width,
			int digitsPerGroup,
			xmlChar groupingCharacter)
{
    xmlChar temp_string[sizeof(double) * CHAR_BIT * sizeof(xmlChar) + 1];
    xmlChar *pointer;
    int i;

    /* Build buffer from back */
    pointer = &temp_string[sizeof(temp_string) - 1];
    *pointer-- = 0;
    for (i = 1; i < (int)sizeof(temp_string); i++) {
	*pointer-- = digit_zero + (int)fmod(number, 10.0);
	number /= 10.0;
	width--;
	if ((width <= 0) && (fabs(number) < 1.0))
	    break; /* for */
	if ((groupingCharacter != 0) &&
	    ((i % digitsPerGroup) == 0)) {
	    *pointer-- = groupingCharacter;
	}
    }
    xmlBufferCat(buffer, pointer + 1);
}

static void
xsltNumberFormatAlpha(xmlBufferPtr buffer,
		      double number,
		      int is_upper)
{
    char temp_string[sizeof(double) * CHAR_BIT * sizeof(xmlChar) + 1];
    char *pointer;
    int i;
    char *alpha_list;
    double alpha_size = (double)(sizeof(alpha_upper_list) - 1);

    /* Build buffer from back */
    pointer = &temp_string[sizeof(temp_string) - 1];
    *pointer-- = 0;
    alpha_list = (is_upper) ? alpha_upper_list : alpha_lower_list;
    
    for (i = 1; i < (int)sizeof(buffer); i++) {
	number--;
	*pointer-- = alpha_list[((int)fmod(number, alpha_size))];
	number /= alpha_size;
	if (fabs(number) < 1.0)
	    break; /* for */
    }
    xmlBufferCCat(buffer, pointer + 1);
}

static void
xsltNumberFormatRoman(xmlBufferPtr buffer,
		      double number,
		      int is_upper)
{
    /*
     * Based on an example by Jim Walsh
     */
    while (number >= 1000.0) {
	xmlBufferCCat(buffer, (is_upper) ? "M" : "m");
	number -= 1000.0;
    }
    if (number >= 900.0) {
	xmlBufferCCat(buffer, (is_upper) ? "CM" : "cm");
	number -= 900.0;
    }
    while (number >= 500.0) {
	xmlBufferCCat(buffer, (is_upper) ? "D" : "d");
	number -= 500.0;
    }
    if (number >= 400.0) {
	xmlBufferCCat(buffer, (is_upper) ? "CD" : "cd");
	number -= 400.0;
    }
    while (number >= 100.0) {
	xmlBufferCCat(buffer, (is_upper) ? "C" : "c");
	number -= 100.0;
    }
    if (number >= 90.0) {
	xmlBufferCCat(buffer, (is_upper) ? "XC" : "xc");
	number -= 90.0;
    }
    while (number >= 50.0) {
	xmlBufferCCat(buffer, (is_upper) ? "L" : "l");
	number -= 50.0;
    }
    if (number >= 40.0) {
	xmlBufferCCat(buffer, (is_upper) ? "XL" : "xl");
	number -= 40.0;
    }
    while (number >= 10.0) {
	xmlBufferCCat(buffer, (is_upper) ? "X" : "x");
	number -= 10.0;
    }
    if (number >= 9.0) {
	xmlBufferCCat(buffer, (is_upper) ? "IX" : "ix");
	number -= 9.0;
    }
    while (number >= 5.0) {
	xmlBufferCCat(buffer, (is_upper) ? "V" : "v");
	number -= 5.0;
    }
    if (number >= 4.0) {
	xmlBufferCCat(buffer, (is_upper) ? "IV" : "iv");
	number -= 4.0;
    }
    while (number >= 1.0) {
	xmlBufferCCat(buffer, (is_upper) ? "I" : "i");
	number--;
    }
}

void
xsltNumberFormat(xsltTransformContextPtr ctxt,
		 xsltNumberDataPtr data,
		 xmlNodePtr node)
{
    xmlXPathParserContextPtr parser = NULL;
    xmlXPathObjectPtr res;
    xmlBufferPtr buffer;
    xmlNodePtr copy = NULL;
    xmlChar *format;
    int i, j;
    int width;
    
    buffer = xmlBufferCreate();
    if (buffer == NULL)
	goto XSLT_NUMBER_FORMAT_END;

    format = data->format;
    
    /*
     * Evaluate the XPath expression to find the value(s)
     */
    parser = xmlXPathNewParserContext(data->value, ctxt->xpathCtxt);
    if (parser == NULL)
	goto XSLT_NUMBER_FORMAT_END;
    ctxt->xpathCtxt->node = node;
    valuePush(parser, xmlXPathNewNodeSet(node));
    xmlXPathEvalExpr(parser);

    /*
     * Parse format string
     */
    i = 0;
    for (;;) {
	/*
	 * Workaround:
	 *  There always seem to be a superfluos object on the stack.
	 *  We handle this by exiting the loop if there is only one
	 *  object back.
	 */
	if (parser->valueNr == 1)
	    break; /* for */
	
	/* Cast to number if necessary */
	if ((parser->value != NULL) &&
	    (parser->value->type != XPATH_NUMBER))
	    xmlXPathNumberFunction(parser, 1);
	
	res = valuePop(parser);
	if (res == NULL)
	    break; /* for */

	if (res->type == XPATH_NUMBER) {

	    switch (isinf(res->floatval)) {
	    case -1:
		xmlBufferCCat(buffer, "-Infinity");
		break;
	    case 1:
		xmlBufferCCat(buffer, "Infinity");
		break;
	    default:
		if (isnan(res->floatval)) {
		    xmlBufferCCat(buffer, "NaN");
		} else {
	    
		    /* Find formatting token */
		    if (IS_DIGIT_ONE(format[i]) || IS_DIGIT_ZERO(format[i])) {
			width = 1;
			while (IS_DIGIT_ZERO(format[i])) {
			    width++;
			    i++;
			}
			if (IS_DIGIT_ONE(format[i])) {
			    xsltNumberFormatDecimal(buffer,
						    res->floatval,
						    format[i] - 1,
						    width,
						    data->digitsPerGroup,
						    data->groupingCharacter);
			    i++;
			}
		    } else if (format[i] == 'A') {
			xsltNumberFormatAlpha(buffer,
					      res->floatval,
					      TRUE);
			i++;
		    } else if (format[i] == 'a') {
			xsltNumberFormatAlpha(buffer,
					      res->floatval,
					      FALSE);
			i++;
		    } else if (format[i] == 'I') {
			xsltNumberFormatRoman(buffer,
					      res->floatval,
					      TRUE);
			i++;
		    } else if (format[i] == 'i') {
			xsltNumberFormatRoman(buffer,
					      res->floatval,
					      FALSE);
			i++;
		    }
		}
		break;
	    }
	    /* Insert separator */
	    for (j = i; !isalnum(format[i]); i++)
		;
	    if (i > j)
		xmlBufferAdd(buffer, &format[j], i - j);
	}
	xmlXPathFreeObject(res);
    }
    /* Insert number as text */
    copy = xmlNewText(xmlBufferContent(buffer));
    if (copy != NULL) {
	xmlAddChild(ctxt->insert, copy);
    }
    
    if (parser != NULL) {
	while ((res = valuePop(parser)) != NULL) {
	    xmlXPathFreeObject(res);
	}
    }
    
 XSLT_NUMBER_FORMAT_END:
    if (parser != NULL)
	xmlXPathFreeParserContext(parser);
    if (buffer != NULL)
	xmlBufferFree(buffer);
}
