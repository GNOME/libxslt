/*
 * imports.h: interface for the XSLT import support
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#ifndef __XML_XSLT_IMPORTS_H__
#define __XML_XSLT_IMPORTS_H__

#include <libxml/tree.h>
#include "xsltInternals.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * XSLT_GET_IMPORT_PTR:
 *
 * a macro to import pointers from the stylesheet cascading order
 */
#define XSLT_GET_IMPORT_PTR(res, style, name) {			\
    xsltStylesheetPtr st = style;				\
    res = NULL;							\
    while (st != NULL) {					\
	if (st->name != NULL) { res = st->name; break; }	\
	st = xsltNextImport(st);				\
    }}

/**
 * XSLT_GET_IMPORT_INT:
 *
 * a macro to import intergers from the stylesheet cascading order
 */
#define XSLT_GET_IMPORT_INT(res, style, name) {			\
    xsltStylesheetPtr st = style;				\
    res = -1;							\
    while (st != NULL) {					\
	if (st->name != -1) { res = st->name; break; }	\
	st = xsltNextImport(st);				\
    }}

/*
 * Module interfaces
 */
void			xsltParseStylesheetImport(xsltStylesheetPtr style,
						  xmlNodePtr cur);
void			xsltParseStylesheetInclude(xsltStylesheetPtr style,
						  xmlNodePtr cur);
xsltStylesheetPtr	xsltNextImport		 (xsltStylesheetPtr style);
int			xsltNeedElemSpaceHandling(xsltTransformContextPtr ctxt);
int			xsltFindElemSpaceHandling(xsltTransformContextPtr ctxt,
						  xmlNodePtr node);
xsltTemplatePtr		xsltFindTemplate	 (xsltTransformContextPtr ctxt,
						  const xmlChar *name,
						  const xmlChar *nameURI);

#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_IMPORTS_H__ */

