/*
 * numbers.h: Implementation of the XSLT number functions
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
 * Bjorn Reese <breese@users.sourceforge.net>
 */

#ifndef __XML_XSLT_NUMBERSINTERNALS_H__
#define __XML_XSLT_NUMBERSINTERNALS_H__

#include <libxml/tree.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This data structure is just a wrapper to pass data in
 */
typedef struct _xsltNumberData {
    xmlChar *level;
    xmlChar *count;
    xmlChar *from;
    xmlChar *value;
    xmlChar *format;
    int digitsPerGroup;
    xmlChar groupingCharacter;
    xmlDocPtr doc;
    xmlNodePtr node;
} xsltNumberData, *xsltNumberDataPtr;

#ifdef __cplusplus
}
#endif
#endif /* __XML_XSLT_NUMBERSINTERNALS_H__ */
