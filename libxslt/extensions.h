/*
 * extension.h: interface for the extension support
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
 */

#ifndef __XML_XSLT_EXTENSION_H__
#define __XML_XSLT_EXTENSION_H__

#include "libxml/xpath.h"
#include "xsltInternals.h"

#ifdef __cplusplus
extern "C" {
#endif

int		xsltRegisterExtPrefix	(xsltStylesheetPtr style,
					 const xmlChar *prefix,
					 const xmlChar *URI);
int		xsltCheckExtPrefix	(xsltStylesheetPtr style,
					 const xmlChar *prefix);
int		xsltRegisterExtFunction	(xsltTransformContextPtr ctxt,
					 const xmlChar *name,
					 const xmlChar *URI,
					 xmlXPathEvalFunc function);
int		xsltRegisterExtElement	(xsltTransformContextPtr ctxt,
					 const xmlChar *name,
					 const xmlChar *URI,
					 xsltTransformFunction function);
void		xsltFreeCtxtExts	(xsltTransformContextPtr ctxt);
void		xsltFreeExts		(xsltStylesheetPtr style);

extern const xmlChar *xsltExtMarker;
#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_EXTENSION_H__ */

