/*
 * pattern.h: interface for the pattern matching used in template matches.
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#ifndef __XML_XSLT_PATTERN_H__
#define __XML_XSLT_PATTERN_H__

#include "xsltInternals.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * xsltCompMatch:
 *
 * Data structure used for the implementation of patterns.
 * It is kept private (in pattern.c)
 */
typedef struct _xsltCompMatch xsltCompMatch;
typedef xsltCompMatch *xsltCompMatchPtr;

/*
 * Pattern related interfaces
 */

xsltCompMatchPtr xsltCompilePattern	(const xmlChar *pattern,
					 xmlDocPtr doc,
					 xmlNodePtr node);
void		 xsltFreeCompMatchList	(xsltCompMatchPtr comp);
int		 xsltTestCompMatchList	(xsltTransformContextPtr ctxt,
					 xmlNodePtr node,
					 xsltCompMatchPtr comp);

/*
 * Template related interfaces
 */
int		xsltAddTemplate		(xsltStylesheetPtr style,
					 xsltTemplatePtr cur,
					 const xmlChar *mode,
					 const xmlChar *modeURI);
xsltTemplatePtr	xsltGetTemplate		(xsltTransformContextPtr ctxt,
					 xmlNodePtr node,
					 xsltStylesheetPtr style);
void		xsltFreeTemplateHashes	(xsltStylesheetPtr style);
void		xsltCleanupTemplates	(xsltStylesheetPtr style);

#if 0
int		xsltMatchPattern	(xsltTransformContextPtr ctxt,
					 xmlNodePtr node,
					 const xmlChar *pattern);
#endif
#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_PATTERN_H__ */

