/*
 * imports.h: interface for the XSLT import support
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
 */

#ifndef __XML_XSLT_IMPORTS_H__
#define __XML_XSLT_IMPORTS_H__

#include <libxml/tree.h>
#include "xsltInternals.h"

#ifdef __cplusplus
extern "C" {
#endif

void			xsltParseStylesheetImport(xsltStylesheetPtr style,
						  xmlNodePtr cur);
void			xsltParseStylesheetInclude(xsltStylesheetPtr style,
						  xmlNodePtr cur);
xsltStylesheetPtr	xsltNextImport		 (xsltStylesheetPtr style);
int			xsltFindElemSpaceHandling(xsltTransformContextPtr ctxt,
						  xmlNodePtr node);
xsltTemplatePtr		xsltFindTemplate	 (xsltTransformContextPtr ctxt,
						  const xmlChar *name,
						  const xmlChar *nameURI);

#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_IMPORTS_H__ */

