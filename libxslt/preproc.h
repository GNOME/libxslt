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
#include "xsltexports.h"
#include "xsltInternals.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Interfaces
 */
extern const xmlChar *xsltExtMarker;

XSLTPUBFUN xsltElemPreCompPtr XSLTCALL 
		xsltDocumentComp	(xsltStylesheetPtr style,
					 xmlNodePtr inst,
					 xsltTransformFunction function);

XSLTPUBFUN void XSLTCALL		
		xsltStylePreCompute	(xsltStylesheetPtr style,
					 xmlNodePtr inst);
XSLTPUBFUN void XSLTCALL		
		xsltFreeStylePreComps	(xsltStylesheetPtr style);

#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_PRECOMP_H__ */

