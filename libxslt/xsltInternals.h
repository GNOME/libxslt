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

/*
 * The in-memory structure corresponding to an XSLT Stylesheet
 * NOTE: most of the content is simply linked from the doc tree
 *       structure, no specific allocation is made.
 */

#define XSLT_PAT_NO_PRIORITY -12345789

typedef struct _xsltTemplate xsltTemplate;
typedef xsltTemplate *xsltTemplatePtr;
struct _xsltTemplate {
    struct _xsltTemplate *next;/* chained list sorted by priority */
    xmlChar *match;	/* the matching string */
    int priority;	/* as given from the stylesheet, not computed */
    xmlChar *name;	/* the local part of the name QName */
    xmlChar *nameURI;	/* the URI part of the name QName */
    xmlChar *mode;	/* the local part of the mode QName */
    xmlChar *modeURI;	/* the URI part of the mode QName */
    xmlNodePtr content;	/* the template replacement value */
};

/*
 * The in-memory structure corresponding to an XSLT Stylesheet
 */
typedef struct _xsltStylesheet xsltStylesheet;
typedef xsltStylesheet *xsltStylesheetPtr;
struct _xsltStylesheet {
    /*
     * The stylesheet import relation is kept as a tree
     */
    struct _xsltStylesheet *parent;
    struct _xsltStylesheet *imports;

    /*
     * General data on the style sheet document
     */
    xmlDocPtr doc;		/* the parsed XML stylesheet */
    xmlHashTablePtr stripSpaces;/* the hash table of the strip-space
				   preserve space and cdata-section elements */

    /*
     * Template descriptions
     */
    xsltTemplatePtr templates;	/* the ordered list of templates */
    void *templatesHash;	/* hash table or wherever compiled templates
				   informations are stored */
    /*
     * Variable descriptions
     */
    void *variablesHash;	/* hash table or wherever variables
				   informations are stored */

    /*
     * Output related stuff.
     */
    xmlChar *method;		/* the output method */
    xmlChar *methodURI;		/* associated namespace if any */
    xmlChar *version;		/* version string */
    xmlChar *encoding;		/* encoding string */
    int omitXmlDeclaration;     /* omit-xml-declaration = "yes" | "no" */
    int standalone;             /* standalone = "yes" | "no" */
    xmlChar *doctypePublic;     /* doctype-public string */
    xmlChar *doctypeSystem;     /* doctype-system string */
    int indent;			/* should output being indented */
    xmlChar *mediaType;		/* media-type string */
};


/*
 * Functions associated to the internal types
 */
xsltStylesheetPtr	xsltParseStylesheetFile	(const xmlChar* filename);
void			xsltFreeStylesheet	(xsltStylesheetPtr sheet);
int			xsltIsBlank		(xmlChar *str);

#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_H__ */

