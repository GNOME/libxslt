/*
 * transform.h: Interfaces, constants and types related to the XSLT engine
 *            transform part.
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
 */

#ifndef __XML_XSLT_TRANSFORM_H__
#define __XML_XSLT_TRANSFORM_H__

#include <libxml/tree.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Interfaces
 */
xmlDocPtr	xsltApplyStylesheet	(xsltStylesheetPtr style,
					 xmlDocPtr doc);
void		xsltApplyOneTemplate	(xsltTransformContextPtr ctxt,
					 xmlNodePtr node,
					 xmlNodePtr list,
					 int real);
void 		xsltDocumentElem	(xsltTransformContextPtr ctxt,
	                                 xmlNodePtr node,
					 xmlNodePtr inst,
					 xsltStylePreCompPtr comp);
void 		xsltSort		(xsltTransformContextPtr ctxt,
	                                 xmlNodePtr node,
					 xmlNodePtr inst,
					 xsltStylePreCompPtr comp);
void 		xsltCopy		(xsltTransformContextPtr ctxt,
	                                 xmlNodePtr node,
					 xmlNodePtr inst,
					 xsltStylePreCompPtr comp);
void 		xsltText		(xsltTransformContextPtr ctxt,
	                                 xmlNodePtr node,
					 xmlNodePtr inst,
					 xsltStylePreCompPtr comp);
void 		xsltElement		(xsltTransformContextPtr ctxt,
	                                 xmlNodePtr node,
					 xmlNodePtr inst,
					 xsltStylePreCompPtr comp);
void 		xsltComment		(xsltTransformContextPtr ctxt,
	                                 xmlNodePtr node,
					 xmlNodePtr inst,
					 xsltStylePreCompPtr comp);
void 		xsltAttribute		(xsltTransformContextPtr ctxt,
	                                 xmlNodePtr node,
					 xmlNodePtr inst,
					 xsltStylePreCompPtr comp);
void 		xsltProcessingInstruction(xsltTransformContextPtr ctxt,
	                                 xmlNodePtr node,
					 xmlNodePtr inst,
					 xsltStylePreCompPtr comp);
void 		xsltCopyOf		(xsltTransformContextPtr ctxt,
	                                 xmlNodePtr node,
					 xmlNodePtr inst,
					 xsltStylePreCompPtr comp);
void 		xsltValueOf		(xsltTransformContextPtr ctxt,
	                                 xmlNodePtr node,
					 xmlNodePtr inst,
					 xsltStylePreCompPtr comp);
void 		xsltNumber		(xsltTransformContextPtr ctxt,
	                                 xmlNodePtr node,
					 xmlNodePtr inst,
					 xsltStylePreCompPtr comp);
void 		xsltApplyImports	(xsltTransformContextPtr ctxt,
	                                 xmlNodePtr node,
					 xmlNodePtr inst,
					 xsltStylePreCompPtr comp);
void 		xsltCallTemplate	(xsltTransformContextPtr ctxt,
	                                 xmlNodePtr node,
					 xmlNodePtr inst,
					 xsltStylePreCompPtr comp);
void 		xsltApplyTemplates	(xsltTransformContextPtr ctxt,
	                                 xmlNodePtr node,
					 xmlNodePtr inst,
					 xsltStylePreCompPtr comp);
void 		xsltChoose		(xsltTransformContextPtr ctxt,
	                                 xmlNodePtr node,
					 xmlNodePtr inst,
					 xsltStylePreCompPtr comp);
void 		xsltIf			(xsltTransformContextPtr ctxt,
	                                 xmlNodePtr node,
					 xmlNodePtr inst,
					 xsltStylePreCompPtr comp);
void 		xsltForEach		(xsltTransformContextPtr ctxt,
	                                 xmlNodePtr node,
					 xmlNodePtr inst,
					 xsltStylePreCompPtr comp);
#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_TRANSFORM_H__ */

