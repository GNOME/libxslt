/*
 * preproc.h: precomputing data interfaces
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#ifndef __XML_XSLT_PRECOMP_H__
#define __XML_XSLT_PRECOMP_H__

#include <libxml/tree.h>
#include "xsltInternals.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Interfaces
 */
extern const xmlChar *xsltExtMarker;

xsltElemPreCompPtr
		xsltDocumentComp	(xsltStylesheetPtr style,
					 xmlNodePtr inst,
					 xsltTransformFunction function);

void		xsltStylePreCompute	(xsltStylesheetPtr style,
					 xmlNodePtr inst);
void		xsltFreeStylePreComps	(xsltStylesheetPtr style);
#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_PRECOMP_H__ */

