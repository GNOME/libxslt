/*
 * pattern.h: interface for the pattern matching used in template matches.
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
 */

#ifndef __XML_XSLT_PATTERN_H__
#define __XML_XSLT_PATTERN_H__

#include "xsltInternals.h"

#ifdef __cplusplus
extern "C" {
#endif

int		xsltAddTemplate		(xsltStylesheetPtr style,
					 xsltTemplatePtr cur,
					 const xmlChar *mode,
					 const xmlChar *modeURI);
xsltTemplatePtr	xsltGetTemplate		(xsltTransformContextPtr ctxt,
					 xmlNodePtr node);
void		xsltFreeTemplateHashes	(xsltStylesheetPtr style);
#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_PATTERN_H__ */

