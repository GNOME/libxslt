/*
 * xsltInternals.h: internal data structures, constants and functions used
 *                  by the XSLT engine
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
 */

#ifndef __XML_XSLT_INTERNALS_H__
#define __XML_XSLT_INTERNALS_H__

#include <libxml/tree.h>
#include <libxml/hash.h>
#include <libxslt/xslt.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _xsltStylesheet xsltStylesheet;
typedef xsltStylesheet *xsltStylesheetPtr;
struct _xsltStylesheet {
};

/*
 * Functions associated to the internal types
 */
xsltStylesheetPtr	xsltParseStylesheetFile	(const xmlChar* filename);

#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_H__ */

