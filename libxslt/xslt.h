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

/**
 * XSLT_DEFAULT_VERSION:
 *
 * The default version of XSLT supported
 */
#define XSLT_DEFAULT_VERSION     "1.0"

/**
 * XSLT_DEFAULT_VENDOR:
 *
 * The XSLT "vendor" string for this processor
 */
#define XSLT_DEFAULT_VENDOR      "libxslt"

/**
 * XSLT_DEFAULT_URL:
 *
 * The XSLT "vendor" URL for this processor
 */
#define XSLT_DEFAULT_URL         "http://xmlsoft.org/XSLT/"

/**
 * XSLT_NAMESPACE:
 *
 * The XSLT specification namespace
 */
#define XSLT_NAMESPACE ((xmlChar *) "http://www.w3.org/1999/XSL/Transform")

/**
 * xsltMaxDepth:
 *
 * This value is used to detect templates loops
 */
extern int xsltMaxDepth;

#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_H__ */

