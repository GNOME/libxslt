/*
 * extension.h: interface for the extension support
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#ifndef __XML_XSLT_EXTENSION_H__
#define __XML_XSLT_EXTENSION_H__

#include "libxml/xpath.h"
#include "xsltInternals.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Extension Modules API
 */

/**
 * xsltExtInitFunction:
 * @ctxt:  an XSLT transformation context
 * @URI:  the namespace URI for the extension
 *
 * A function called at initialization time of an XSLT extension module
 *
 * Returns a pointer to the module specific data for this transformation
 */
typedef void * (*xsltExtInitFunction)	(xsltTransformContextPtr ctxt,
					 const xmlChar *URI);

/**
 * xsltExtShutdownFunction:
 * @ctxt:  an XSLT transformation context
 * @URI:  the namespace URI for the extension
 * @data:  the data associated to this module
 *
 * A function called at shutdown time of an XSLT extension module
 */
typedef void (*xsltExtShutdownFunction) (xsltTransformContextPtr ctxt,
					 const xmlChar *URI,
					 void *data);

int		xsltRegisterExtModule	(const xmlChar *URI,
					 xsltExtInitFunction initFunc,
					 xsltExtShutdownFunction shutdownFunc);

int		xsltUnregisterExtModule	(const xmlChar * URI);

void		xsltUnregisterAllExtModules(void);

void *		xsltGetExtData		(xsltTransformContextPtr ctxt,
					 const xmlChar *URI);

void		xsltShutdownCtxtExts	(xsltTransformContextPtr ctxt);

xsltTransformContextPtr
    		xsltXPathGetTransformContext
					(xmlXPathParserContextPtr ctxt);

int		xsltRegisterExtFunction	(xsltTransformContextPtr ctxt,
					 const xmlChar *name,
					 const xmlChar *URI,
					 xmlXPathEvalFunc function);
int		xsltRegisterExtElement	(xsltTransformContextPtr ctxt,
					 const xmlChar *name,
					 const xmlChar *URI,
					 xsltTransformFunction function);

/**
 * Extension Prefix handling API
 * Those are used by the XSLT (pre)processor.
 */

int		xsltRegisterExtPrefix	(xsltStylesheetPtr style,
					 const xmlChar *prefix,
					 const xmlChar *URI);
int		xsltCheckExtPrefix	(xsltStylesheetPtr style,
					 const xmlChar *prefix);
int		xsltInitCtxtExts	(xsltTransformContextPtr ctxt);
void		xsltFreeCtxtExts	(xsltTransformContextPtr ctxt);
void		xsltFreeExts		(xsltStylesheetPtr style);


#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_EXTENSION_H__ */

