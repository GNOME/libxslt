/*
 * variable.h: interface for the variable matching and lookup.
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#ifndef __XML_XSLT_VARIABLES_H__
#define __XML_XSLT_VARIABLES_H__

#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include "xsltInternals.h"
#include "functions.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * XSLT_REGISTER_VARIABLE_LOOKUP:
 *
 * registering macro, not general purpose at all but used in different modules
 */

#define XSLT_REGISTER_VARIABLE_LOOKUP(ctxt)			\
    xmlXPathRegisterVariableLookup((ctxt)->xpathCtxt,		\
	       xsltXPathVariableLookup,	(void *)(ctxt));	\
    xsltRegisterAllFunctions((ctxt)->xpathCtxt);		\
    xsltRegisterAllElement(ctxt);				\
    (ctxt)->xpathCtxt->extra = ctxt

/*
 * Interfaces for the variable module.
 */

int		xsltEvalGlobalVariables		(xsltTransformContextPtr ctxt);
int		xsltEvalUserParams		(xsltTransformContextPtr ctxt,
						 const char **params);
int		xsltQuoteUserParams		(xsltTransformContextPtr ctxt,
						 const char **params);
int		xsltEvalOneUserParam		(xsltTransformContextPtr ctxt,
    						 const xmlChar * name,
						 const xmlChar * value);
int		xsltQuoteOneUserParam		(xsltTransformContextPtr ctxt,
    						 const xmlChar * name,
						 const xmlChar * value);

void		xsltParseGlobalVariable		(xsltStylesheetPtr style,
						 xmlNodePtr cur);
void		xsltParseGlobalParam		(xsltStylesheetPtr style,
						 xmlNodePtr cur);
void		xsltParseStylesheetVariable	(xsltTransformContextPtr ctxt,
						 xmlNodePtr cur);
void		xsltParseStylesheetParam	(xsltTransformContextPtr ctxt,
						 xmlNodePtr cur);
xsltStackElemPtr xsltParseStylesheetCallerParam	(xsltTransformContextPtr ctxt,
						 xmlNodePtr cur);
int		xsltAddStackElemList		(xsltTransformContextPtr ctxt,
						 xsltStackElemPtr elems);
void		xsltFreeGlobalVariables		(xsltTransformContextPtr ctxt);
xmlXPathObjectPtr	xsltVariableLookup	(xsltTransformContextPtr ctxt,
						 const xmlChar *name,
						 const xmlChar *ns_uri);
xmlXPathObjectPtr	xsltXPathVariableLookup	(void *ctxt,
						 const xmlChar *name,
						 const xmlChar *ns_uri);
#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_VARIABLES_H__ */

