/*
 * transform.h: Interfaces, constants and types related to the XSLT engine
 *            transform part.
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#ifndef __XML_XSLT_TRANSFORM_H__
#define __XML_XSLT_TRANSFORM_H__

#include <libxml/parser.h>
#include <libxml/xmlIO.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * XInclude default processing
 */
void		xsltSetXIncludeDefault	(int xinclude);
int		xsltGetXIncludeDefault	(void);

/**
 * Export context to users.
 */
xsltTransformContextPtr xsltNewTransformContext	(xsltStylesheetPtr style,
						 xmlDocPtr doc);

void			xsltFreeTransformContext(xsltTransformContextPtr ctxt);

xmlDocPtr		xsltApplyStylesheetUser	(xsltStylesheetPtr style,
						 xmlDocPtr doc,
						 const char **params,
						 const char *output,
						 FILE * profile,
					     xsltTransformContextPtr userCtxt);
/**
 * Private Interfaces
 */
void		xsltApplyStripSpaces	(xsltTransformContextPtr ctxt,
					 xmlNodePtr node);
xsltTransformFunction
		xsltExtElementLookup	(xsltTransformContextPtr ctxt,
					 const xmlChar *name,
					 const xmlChar *URI);

xmlDocPtr	xsltApplyStylesheet	(xsltStylesheetPtr style,
					 xmlDocPtr doc,
					 const char **params);
xmlDocPtr	xsltProfileStylesheet	(xsltStylesheetPtr style,
					 xmlDocPtr doc,
					 const char **params,
					 FILE * output);
int		xsltRunStylesheet	(xsltStylesheetPtr style,
					 xmlDocPtr doc,
					 const char **params,
					 const char *output,
					 xmlSAXHandlerPtr SAX,
					 xmlOutputBufferPtr IObuf);
void		xsltApplyOneTemplate	(xsltTransformContextPtr ctxt,
					 xmlNodePtr node,
					 xmlNodePtr list,
					 xsltTemplatePtr templ,
					 xsltStackElemPtr params);
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
void		xsltRegisterAllElement	(xsltTransformContextPtr ctxt);

/*
 * Hook for the debugger if activated.
 */
void		xslHandleDebugger	(xmlNodePtr cur,
					 xmlNodePtr node,
					 xsltTemplatePtr templ,
					 xsltTransformContextPtr ctxt);

#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_TRANSFORM_H__ */

