/*
 * functions.h: interface for the XSLT extra functions
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 * Bjorn Reese <breese@users.sourceforge.net> for number formatting
 */

#ifndef __XML_XSLT_FUNCTIONS_H__
#define __XML_XSLT_FUNCTIONS_H__

#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include "xsltInternals.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * XSLT_REGISTER_FUNCTION_LOOKUP:
 *
 * registering macro, not general purpose at all but used in different modules
 */
#define XSLT_REGISTER_FUNCTION_LOOKUP(ctxt)			\
    xmlXPathRegisterFuncLookup((ctxt)->xpathCtxt,		\
	(xmlXPathFuncLookupFunc) xsltXPathFunctionLookup,	\
	(void *)(ctxt->xpathCtxt));

xmlXPathFunction
	xsltXPathFunctionLookup	(xmlXPathContextPtr ctxt,
				 const xmlChar *name,
				 const xmlChar *ns_uri);

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

/*
 * And the registration
 */

void	xsltRegisterAllFunctions	(xmlXPathContextPtr ctxt);

#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_FUNCTIONS_H__ */

