/*
 * document.h: interface for the document handling
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#ifndef __XML_XSLT_DOCUMENTS_H__
#define __XML_XSLT_DOCUMENTS_H__

#include <libxml/tree.h>
#include "xsltInternals.h"

#ifdef __cplusplus
extern "C" {
#endif

xsltDocumentPtr	xsltNewDocument		(xsltTransformContextPtr ctxt,
					 xmlDocPtr doc);
xsltDocumentPtr	xsltLoadDocument	(xsltTransformContextPtr ctxt,
					 const xmlChar *URI);
xsltDocumentPtr	xsltFindDocument	(xsltTransformContextPtr ctxt,
					 xmlDocPtr doc);
void		xsltFreeDocuments	(xsltTransformContextPtr ctxt);

xsltDocumentPtr	xsltLoadStyleDocument	(xsltStylesheetPtr style,
					 const xmlChar *URI);
xsltDocumentPtr	xsltNewStyleDocument	(xsltStylesheetPtr style,
					 xmlDocPtr doc);
void		xsltFreeStyleDocuments	(xsltStylesheetPtr style);

#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_DOCUMENTS_H__ */

