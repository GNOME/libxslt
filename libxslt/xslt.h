/*
 * xslt.h: Interfaces, constants and types related to the XSLT engine
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
 */

#ifndef __XML_XSLT_H__
#define __XML_XSLT_H__

#include <libxml/tree.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Constants.
 */
#define XSLT_DEFAULT_VERSION     "1.0"
#define XSLT_DEFAULT_VENDOR      "libxslt"
#define XSLT_DEFAULT_URL         "http://xmlsoft.org/XSLT/"
#define XSLT_NAMESPACE ((xmlChar *) "http://www.w3.org/1999/XSL/Transform")

extern int xsltMaxDepth;

#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_H__ */

