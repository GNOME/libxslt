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
#include <libxml/xpath.h>
#include <libxslt/xslt.h>

#ifdef __cplusplus
extern "C" {
#endif


/*
 * The in-memory structure corresponding to an XSLT Variable
 * or Param
 */

typedef enum {
    XSLT_ELEM_VARIABLE=1,
    XSLT_ELEM_PARAM
} xsltElem;

typedef struct _xsltStackElem xsltStackElem;
typedef xsltStackElem *xsltStackElemPtr;
struct _xsltStackElem {
    struct _xsltStackElem *next;/* chained list */
    xsltElem type;	/* type of the element */
    int computed;	/* was the evaluation done */
    xmlChar *name;	/* the local part of the name QName */
    xmlChar *nameURI;	/* the URI part of the name QName */
    xmlChar *select;	/* the eval string */
    xmlNodePtr tree;	/* the tree if no eval string */
    xmlXPathObjectPtr value; /* The value if computed */
};

/*
 * The in-memory structure corresponding to an XSLT Template
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
 * NOTE: most of the content is simply linked from the doc tree
 *       structure, no specific allocation is made.
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
     * Global variable or parameters
     */
    xsltStackElemPtr variables; /* linked list of param and variables */

    /*
     * Template descriptions
     */
    xsltTemplatePtr templates;	/* the ordered list of templates */
    void *templatesHash;	/* hash table or wherever compiled templates
				   informations are stored */
    void *rootMatch;		/* template based on / */
    void *elemMatch;		/* template based on * */
    void *attrMatch;		/* template based on @* */
    void *parentMatch;		/* template based on .. */
    void *textMatch;		/* template based on text() */
    void *piMatch;		/* template based on processing-instruction() */
    void *commentMatch;		/* template based on comment() */
    
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
 * The in-memory structure corresponding to an XSLT Transformation
 */
typedef enum {
    XSLT_OUTPUT_XML = 0,
    XSLT_OUTPUT_HTML,
    XSLT_OUTPUT_TEXT
} xsltOutputType;

typedef struct _xsltTransformContext xsltTransformContext;
typedef xsltTransformContext *xsltTransformContextPtr;
struct _xsltTransformContext {
    xsltStylesheetPtr style;		/* the stylesheet used */
    xsltOutputType type;		/* the type of output */

    xmlDocPtr doc;			/* the current doc */
    xmlNodePtr node;			/* the current node */
    xmlNodeSetPtr nodeList;		/* the current node list */

    xmlDocPtr output;			/* the resulting document */
    xmlNodePtr insert;			/* the insertion node */

    xmlXPathContextPtr xpathCtxt;	/* the XPath context */
    void *variablesHash;		/* hash table or wherever variables
				   	   informations are stored */
    xmlDocPtr extraDocs;		/* extra docs parsed by document() */
};

/*
 * Functions associated to the internal types
 */
xsltStylesheetPtr	xsltParseStylesheetFile	(const xmlChar* filename);
void			xsltFreeStylesheet	(xsltStylesheetPtr sheet);
int			xsltIsBlank		(xmlChar *str);
void			xsltFreeStackElemList	(xsltStackElemPtr elem);

#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_H__ */

