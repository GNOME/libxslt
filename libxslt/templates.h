/*
 * templates.h: interface for the template processing
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#ifndef __XML_XSLT_TEMPLATES_H__
#define __XML_XSLT_TEMPLATES_H__

#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include "xsltInternals.h"

#ifdef __cplusplus
extern "C" {
#endif

int		xsltEvalXPathPredicate		(xsltTransformContextPtr ctxt,
						 xmlXPathCompExprPtr comp,
		                                 xmlNsPtr *nsList,
						 int nsNr);
xmlChar *	xsltEvalTemplateString		(xsltTransformContextPtr ctxt,
						 xmlNodePtr node,
						 xmlNodePtr parent);
xmlChar *	xsltEvalAttrValueTemplate	(xsltTransformContextPtr ctxt,
						 xmlNodePtr node,
						 const xmlChar *name,
						 const xmlChar *ns);
xmlChar *	xsltEvalStaticAttrValueTemplate	(xsltStylesheetPtr style,
						 xmlNodePtr node,
						 const xmlChar *name,
						 const xmlChar *ns,
						 int *found);

/* TODO: this is obviously broken ... the namespaces should be passed too ! */
xmlChar *	xsltEvalXPathString		(xsltTransformContextPtr ctxt,
						 xmlXPathCompExprPtr comp);

xmlNodePtr *	xsltTemplateProcess		(xsltTransformContextPtr ctxt,
						 xmlNodePtr node);
xmlAttrPtr	xsltAttrListTemplateProcess	(xsltTransformContextPtr ctxt,
						 xmlNodePtr target,
						 xmlAttrPtr cur);
xmlAttrPtr	xsltAttrTemplateProcess		(xsltTransformContextPtr ctxt,
						 xmlNodePtr target,
						 xmlAttrPtr attr);
xmlChar *	xsltAttrTemplateValueProcess	(xsltTransformContextPtr ctxt,
						 const xmlChar* attr);
#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_TEMPLATES_H__ */

