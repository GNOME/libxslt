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

#ifdef __cplusplus
extern "C" {
#endif

void	xsltParseStylesheetAttributeSet	(xsltStylesheetPtr style,
					 xmlNodePtr cur);
void	xsltFreeAttributeSetsHashes	(xsltStylesheetPtr style);
void	xsltApplyAttributeSet		(xsltTransformContextPtr ctxt,
					 xmlNodePtr node,
					 xmlNodePtr inst,
					 xmlChar *attributes);
#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_ATTRIBUTES_H__ */

