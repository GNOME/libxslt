/*
 * xslt.h: Interfaces, constants and types related to the XSLT engine
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
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
 * LIBXSLT_PUBLIC:
 *
 * Macro used on Windows to tag public identifiers from shared libraries
 */
#ifndef LIBXSLT_PUBLIC
#define LIBXSLT_PUBLIC
#endif

/**
 * xsltMaxDepth:
 *
 * This value is used to detect templates loops
 */
LIBXSLT_PUBLIC extern int xsltMaxDepth;

/**
 * xsltEngineVersion:
 *
 * The version string for libxslt
 */
LIBXSLT_PUBLIC extern const char *xsltEngineVersion;

/**
 * xsltLibxsltVersion:
 *
 * The version of libxslt compiled
 */
LIBXSLT_PUBLIC extern const int xsltLibxsltVersion;

/**
 * xsltLibxmlVersion:
 *
 * The version of libxml libxslt was compiled against
 */
LIBXSLT_PUBLIC extern const int xsltLibxmlVersion;

/*
 * Global cleanup function
 */
void	xsltCleanupGlobals	(void);

#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_H__ */

