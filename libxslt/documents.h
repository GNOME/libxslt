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
#include "xsltexports.h"
#include "xsltInternals.h"

#ifdef __cplusplus
extern "C" {
#endif

XSLTPUBFUN xsltDocumentPtr XSLTCALL	
		xsltNewDocument		(xsltTransformContextPtr ctxt,
					 xmlDocPtr doc);
XSLTPUBFUN xsltDocumentPtr XSLTCALL	
    		xsltLoadDocument	(xsltTransformContextPtr ctxt,
					 const xmlChar *URI);
XSLTPUBFUN xsltDocumentPtr XSLTCALL	
    		xsltFindDocument	(xsltTransformContextPtr ctxt,
					 xmlDocPtr doc);
XSLTPUBFUN void XSLTCALL		
    		xsltFreeDocuments	(xsltTransformContextPtr ctxt);

XSLTPUBFUN xsltDocumentPtr XSLTCALL	
    		xsltLoadStyleDocument	(xsltStylesheetPtr style,
					 const xmlChar *URI);
XSLTPUBFUN xsltDocumentPtr XSLTCALL	
    		xsltNewStyleDocument	(xsltStylesheetPtr style,
					 xmlDocPtr doc);
XSLTPUBFUN void XSLTCALL		
    		xsltFreeStyleDocuments	(xsltStylesheetPtr style);

#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_DOCUMENTS_H__ */

