/*
 * key.h: interface for the key matching used in template matches.
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#ifndef __XML_XSLT_KEY_H__
#define __XML_XSLT_KEY_H__

#include "libxml/xpath.h"
#include "xsltInternals.h"

#ifdef __cplusplus
extern "C" {
#endif

int		xsltAddKey		(xsltStylesheetPtr style,
					 const xmlChar *name,
					 const xmlChar *nameURI,
					 const xmlChar *match,
					 const xmlChar *use,
					 xmlNodePtr inst);
xmlNodeSetPtr	xsltGetKey		(xsltTransformContextPtr ctxt,
					 const xmlChar *name,
					 const xmlChar *nameURI,
					 const xmlChar *value);
void		xsltInitCtxtKeys	(xsltTransformContextPtr ctxt,
					 xsltDocumentPtr doc);
void		xsltFreeKeys		(xsltStylesheetPtr style);
void		xsltFreeDocumentKeys	(xsltDocumentPtr doc);

#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_H__ */

