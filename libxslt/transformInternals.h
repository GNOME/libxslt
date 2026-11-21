/*
 * Summary: set of internal interfaces for the XSLT engine transformation part.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: David Kilzer <ddkilzer@apple.com>
 */

#ifndef __XML_XSLT_TRANSFORMINTERNALS_H__
#define __XML_XSLT_TRANSFORMINTERNALS_H__

#include <libxml/tree.h>
#include "xsltexports.h"

#ifdef __cplusplus
extern "C" {
#endif

void xsltCleanupSourceDoc(xmlDocPtr doc);

#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_TRANSFORMINTERNALS_H__ */

