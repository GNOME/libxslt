/*
 * attributes.h: interface for the XSLT attribute handling
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#ifndef __XML_XSLT_ATTRIBUTES_H__
#define __XML_XSLT_ATTRIBUTES_H__

#include <libxml/tree.h>
#include "xsltexports.h"

#ifdef __cplusplus
extern "C" {
#endif

XSLTPUBFUN void XSLTCALL
	xsltParseStylesheetAttributeSet	(xsltStylesheetPtr style,
					 xmlNodePtr cur);
XSLTPUBFUN void XSLTCALL    
	xsltFreeAttributeSetsHashes	(xsltStylesheetPtr style);
XSLTPUBFUN void XSLTCALL	
	xsltApplyAttributeSet		(xsltTransformContextPtr ctxt,
					 xmlNodePtr node,
					 xmlNodePtr inst,
					 xmlChar *attributes);
XSLTPUBFUN void XSLTCALL	
	xsltResolveStylesheetAttributeSet(xsltStylesheetPtr style);
#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_ATTRIBUTES_H__ */

