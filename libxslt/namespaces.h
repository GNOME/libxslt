/*
 * namespaces.h: interface for the XSLT namespace handling
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#ifndef __XML_XSLT_NAMESPACES_H__
#define __XML_XSLT_NAMESPACES_H__

#include <libxml/tree.h>
#include "xsltexports.h"

#ifdef __cplusplus
extern "C" {
#endif

XSLTPUBFUN void XSLTCALL
		xsltNamespaceAlias	(xsltStylesheetPtr style,
					 xmlNodePtr node);
XSLTPUBFUN xmlNsPtr XSLTCALL	
		xsltGetNamespace	(xsltTransformContextPtr ctxt,
					 xmlNodePtr cur,
					 xmlNsPtr ns,
					 xmlNodePtr out);
XSLTPUBFUN xmlNsPtr XSLTCALL	
		xsltGetSpecialNamespace	(xsltTransformContextPtr ctxt,
					 xmlNodePtr cur,
					 const xmlChar *URI,
					 const xmlChar *prefix,
					 xmlNodePtr out);
XSLTPUBFUN xmlNsPtr XSLTCALL	
		xsltCopyNamespace	(xsltTransformContextPtr ctxt,
					 xmlNodePtr node,
					 xmlNsPtr cur);
XSLTPUBFUN xmlNsPtr XSLTCALL	
		xsltCopyNamespaceList	(xsltTransformContextPtr ctxt,
					 xmlNodePtr node,
					 xmlNsPtr cur);
XSLTPUBFUN void XSLTCALL		
		xsltFreeNamespaceAliasHashes
					(xsltStylesheetPtr style);

#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_NAMESPACES_H__ */

