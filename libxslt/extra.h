/*
 * extra.h: interface for the non-standard features
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
 */

#ifndef __XML_XSLT_EXTRA_H__
#define __XML_XSLT_EXTRA_H__

#include "libxml/xpath.h"
#include "xsltInternals.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XSLT_LIBXSLT_NAMESPACE ((xmlChar *) "http://xmlsoft.org/XSLT/namespace")
#define XSLT_SAXON_NAMESPACE ((xmlChar *) "http://icl.com/saxon")
#define XSLT_XT_NAMESPACE ((xmlChar *) "http://www.jclark.com/xt")
#define XSLT_XALAN_NAMESPACE ((xmlChar *)	\
	                        "org.apache.xalan.xslt.extensions.Redirect")

void		xsltFunctionNodeSet	(xmlXPathParserContextPtr ctxt,
					 int nargs);

void		xsltRegisterExtras	(xsltTransformContextPtr ctxt);


void		xsltDebug		(xsltTransformContextPtr ctxt,
					 xmlNodePtr node,
					 xmlNodePtr inst,
					 xsltStylePreCompPtr comp);
#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_EXTRA_H__ */

