/*
 * pattern.h: interface for the pattern matching used in template matches.
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
 */

#ifndef __XML_XSLT_PATTERN_H__
#define __XML_XSLT_H__

#include "xsltInternals.h"

#ifdef __cplusplus
extern "C" {
#endif

int		xsltAddTemplate		(xsltStylesheetPtr style,
					 xsltTemplatePtr cur);
xsltTemplatePtr	xsltGetTemplate		(xsltStylesheetPtr style,
					 xmlNodePtr node);
void		xsltFreeTemplateHashes	(xsltStylesheetPtr style);
xsltTemplatePtr xsltFindTemplate	(xsltStylesheetPtr style,
					 const xmlChar *name,
					 const xmlChar *nameURI);
#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_H__ */

