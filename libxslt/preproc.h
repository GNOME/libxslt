/*
 * preproc.h: precomputing data interfaces
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
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
void		xsltStylePreCompute	(xsltStylesheetPtr style,
					 xmlNodePtr inst);
void		xsltFreeStylePreComps	(xsltStylesheetPtr style);
#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_PRECOMP_H__ */

