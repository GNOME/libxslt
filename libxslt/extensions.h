/*
 * extension.h: interface for the extension support
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#ifndef __XML_XSLT_EXTENSION_H__
#define __XML_XSLT_EXTENSION_H__

#include "libxml/xpath.h"
#include "xsltInternals.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Extension Modules API
 */

/**
 * xsltStyleExtInitFunction:
 * @ctxt:  an XSLT stylesheet
 * @URI:  the namespace URI for the extension
 *
 * A function called at initialization time of an XSLT extension module
 *
 * Returns a pointer to the module specific data for this transformation
 */
typedef void * (*xsltStyleExtInitFunction)	(xsltStylesheetPtr style,
						 const xmlChar *URI);

/**
 * xsltStyleExtShutdownFunction:
 * @ctxt:  an XSLT stylesheet
 * @URI:  the namespace URI for the extension
 * @data:  the data associated to this module
 *
 * A function called at shutdown time of an XSLT extension module
 */
typedef void (*xsltStyleExtShutdownFunction)	(xsltStylesheetPtr style,
						 const xmlChar *URI,
						 void *data);

/**
 * xsltExtInitFunction:
 * @ctxt:  an XSLT transformation context
 * @URI:  the namespace URI for the extension
 *
 * A function called at initialization time of an XSLT extension module
 *
 * Returns a pointer to the module specific data for this transformation
 */
typedef void * (*xsltExtInitFunction)	(xsltTransformContextPtr ctxt,
					 const xmlChar *URI);

/**
 * xsltExtShutdownFunction:
 * @ctxt:  an XSLT transformation context
 * @URI:  the namespace URI for the extension
 * @data:  the data associated to this module
 *
 * A function called at shutdown time of an XSLT extension module
 */
typedef void (*xsltExtShutdownFunction) (xsltTransformContextPtr ctxt,
					 const xmlChar *URI,
					 void *data);

int		xsltRegisterExtModule	(const xmlChar *URI,
					 xsltExtInitFunction initFunc,
					 xsltExtShutdownFunction shutdownFunc);
int		xsltRegisterExtModuleFull
				(const xmlChar * URI,
				 xsltExtInitFunction initFunc,
				 xsltExtShutdownFunction shutdownFunc,
				 xsltStyleExtInitFunction styleInitFunc,
				 xsltStyleExtShutdownFunction styleShutdownFunc);

int		xsltUnregisterExtModule	(const xmlChar * URI);

void *		xsltGetExtData		(xsltTransformContextPtr ctxt,
					 const xmlChar *URI);

void *		xsltStyleGetExtData	(xsltStylesheetPtr style,
					 const xmlChar *URI);

void		xsltShutdownCtxtExts	(xsltTransformContextPtr ctxt);

void		xsltShutdownExts	(xsltStylesheetPtr style);

xsltTransformContextPtr
    		xsltXPathGetTransformContext
					(xmlXPathParserContextPtr ctxt);

/*
 * extension functions
*/
int	xsltRegisterExtModuleFunction	(const xmlChar *name,
					 const xmlChar *URI,
					 xmlXPathFunction function);
xmlXPathFunction
	xsltExtFunctionLookup		(xsltTransformContextPtr ctxt,
					 const xmlChar *name,
					 const xmlChar *URI);
xmlXPathFunction
	xsltExtModuleFunctionLookup	(const xmlChar *name,
					 const xmlChar *URI);
int	xsltUnregisterExtModuleFunction	(const xmlChar *name,
					 const xmlChar *URI);

/*
 * extension elements
 */
typedef xsltElemPreCompPtr
	(*xsltPreComputeFunction)	(xsltStylesheetPtr style,
					 xmlNodePtr inst,
					 xsltTransformFunction function);

xsltElemPreCompPtr
	xsltNewElemPreComp		(xsltStylesheetPtr style,
					 xmlNodePtr inst,
					 xsltTransformFunction function);
void	xsltInitElemPreComp		(xsltElemPreCompPtr comp,
					 xsltStylesheetPtr style,
					 xmlNodePtr inst,
					 xsltTransformFunction function,
					 xsltElemPreCompDeallocator freeFunc);

int	xsltRegisterExtModuleElement	(const xmlChar *name,
					 const xmlChar *URI,
					 xsltPreComputeFunction precomp,
					 xsltTransformFunction transform);
xsltTransformFunction
	xsltExtElementLookup		(xsltTransformContextPtr ctxt,
					 const xmlChar *name,
					 const xmlChar *URI);
xsltTransformFunction
	xsltExtModuleElementLookup	(const xmlChar *name,
					 const xmlChar *URI);
xsltPreComputeFunction
	xsltExtModuleElementPreComputeLookup
					(const xmlChar *name,
					 const xmlChar *URI);
int	xsltUnregisterExtModuleElement	(const xmlChar *name,
					 const xmlChar *URI);

/*
 * top-level elements
 */
typedef void
	(*xsltTopLevelFunction)		(xsltStylesheetPtr style,
					 xmlNodePtr inst);

int	xsltRegisterExtModuleTopLevel	(const xmlChar *name,
					 const xmlChar *URI,
					 xsltTopLevelFunction function);
xsltTopLevelFunction
	xsltExtModuleTopLevelLookup	(const xmlChar *name,
					 const xmlChar *URI);
int	xsltUnregisterExtModuleTopLevel	(const xmlChar *name,
					 const xmlChar *URI);


/* These 2 functions are deprecated for use within modules */
int		xsltRegisterExtFunction	(xsltTransformContextPtr ctxt,
					 const xmlChar *name,
					 const xmlChar *URI,
					 xmlXPathFunction function);
int		xsltRegisterExtElement	(xsltTransformContextPtr ctxt,
					 const xmlChar *name,
					 const xmlChar *URI,
					 xsltTransformFunction function);

/**
 * Extension Prefix handling API
 * Those are used by the XSLT (pre)processor.
 */

int		xsltRegisterExtPrefix	(xsltStylesheetPtr style,
					 const xmlChar *prefix,
					 const xmlChar *URI);
int		xsltCheckExtPrefix	(xsltStylesheetPtr style,
					 const xmlChar *prefix);
int		xsltInitCtxtExts	(xsltTransformContextPtr ctxt);
void		xsltFreeCtxtExts	(xsltTransformContextPtr ctxt);
void		xsltFreeExts		(xsltStylesheetPtr style);

xsltElemPreCompPtr
	xsltPreComputeExtModuleElement	(xsltStylesheetPtr style,
					 xmlNodePtr inst);

/**
 * Test module http://xmlsoft.org/XSLT/
 */
void	xsltRegisterTestModule		(void);

#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_EXTENSION_H__ */

