/*
 * functions.h: interface for the XSLT extra functions
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
 * Bjorn Reese <breese@mail1.stofanet.dk> for number formatting
 */

#ifndef __XML_XSLT_FUNCTIONS_H__
#define __XML_XSLT_FUNCTIONS_H__

#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include "xsltInternals.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Data structure of decimal-format
 */
typedef struct _xsltDecimalFormat {
    /* Used for interpretation of pattern */
    xmlChar digit;
    xmlChar patternSeparator;
    /* May appear in result */
    xmlChar minusSign;
    xmlChar *infinity;
    xmlChar *noNumber;
    /* Used for interpretation of pattern and may appear in result */
    xmlChar decimalPoint;
    xmlChar grouping;
    xmlChar percent;
    xmlChar permille;
    xmlChar zeroDigit;
} xsltDecimalFormat, *xsltDecimalFormatPtr;

/*
 * Interfaces for the functions implementations
 */

void	xsltDocumentFunction		(xmlXPathParserContextPtr ctxt,
					 int nargs);
void	xsltKeyFunction			(xmlXPathParserContextPtr ctxt,
					 int nargs);
void	xsltUnparsedEntityURIFunction	(xmlXPathParserContextPtr ctxt,
					 int nargs);
void	xsltFormatNumberFunction	(xmlXPathParserContextPtr ctxt,
					 int nargs);
void	xsltGenerateIdFunction		(xmlXPathParserContextPtr ctxt,
					 int nargs);
void	xsltSystemPropertyFunction	(xmlXPathParserContextPtr ctxt,
					 int nargs);
void	xsltElementAvailableFunction	(xmlXPathParserContextPtr ctxt,
					 int nargs);
void	xsltFunctionAvailableFunction	(xmlXPathParserContextPtr ctxt,
					 int nargs);
void	xsltXXXFunction			(xmlXPathParserContextPtr ctxt,
					 int nargs);

/*
 * And the registration
 */

void	xsltRegisterAllFunctions	(xmlXPathContextPtr ctxt);

#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_FUNCTIONS_H__ */

