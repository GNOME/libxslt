/*
 * document.h: interface for the document handling
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
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
void		xsltFreeDocuments	(xsltTransformContextPtr ctxt);

#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_DOCUMENTS_H__ */

