/*
 * variable.h: interface for the variable matching and lookup.
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
 */

#ifndef __XML_XSLT_VARIABLES_H__
#define __XML_XSLT_VARIABLES_H__

#include <libxml/xpath.h>
#include "xsltInternals.h"

#ifdef __cplusplus
extern "C" {
#endif

void			xsltFreeVariableHashes	(xsltStylesheetPtr style);
xmlXPathObjectPtr	xsltVariableLookup	(xsltStylesheetPtr style,
						 const xmlChar *name,
						 const xmlChar *ns_uri);
int			xsltRegisterVariable	(xsltStylesheetPtr style,
						 const xmlChar *name,
						 const xmlChar *ns_uri,
						 xmlXPathObjectPtr value);
#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_VARIABLES_H__ */

