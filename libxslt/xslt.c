/*
 * xslt.h: Implemetation of an XSL Transformation 1.0 engine
 *
 * Reference:
 *   http://www.w3.org/TR/1999/REC-xslt-19991116
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
 */

#include <string.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlerror.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>

#define DEBUG_PARSING

/*
 * There is no XSLT specific error reporting module yet
 */
#define xsltGenericError xmlGenericError
#define xsltGenericErrorContext xmlGenericErrorContext

/*
 * Useful macros
 */

#define IS_XSLT_ELEM(n)							\
    ((n)->ns != NULL) && (xmlStrEqual(cur->ns->href, XSLT_NAMESPACE))

#define IS_BLANK(c) (((c) == 0x20) || ((c) == 0x09) || ((c) == 0xA) ||	\
                     ((c) == 0x0D))

#define IS_BLANK_NODE(n)						\
    (((n)->type == XML_TEXT_NODE) && (xsltIsBlank((n)->content)))

#define TODO 								\
    xsltGenericError(xsltGenericErrorContext,				\
	    "Unimplemented block at %s:%d\n",				\
            __FILE__, __LINE__);

#define STRANGE 							\
    xsltGenericError(xsltGenericErrorContext,				\
	    "Internal error at %s:%d\n",				\
            __FILE__, __LINE__);

/************************************************************************
 *									*
 *			Helper functions				*
 *									*
 ************************************************************************/

/**
 * xsltIsBlank:
 * @str:  a string
 *
 * Returns 1 if the string is NULL or made of blanks chars, 0 otherwise
 */
int
xsltIsBlank(xmlChar *str) {
    if (str == NULL)
	return(1);
    while (*str != 0) {
	if (!(IS_BLANK(*str))) return(0);
	str++;
    }
    return(1);
}

/************************************************************************
 *									*
 *		Routines to handle XSLT data structures			*
 *									*
 ************************************************************************/

/**
 * xsltNewTemplate:
 *
 * Create a new XSLT Template
 *
 * Returns the newly allocated xsltTemplatePtr or NULL in case of error
 */
xsltTemplatePtr
xsltNewTemplate(void) {
    xsltTemplatePtr cur;

    cur = (xsltTemplatePtr) xmlMalloc(sizeof(xsltTemplate));
    if (cur == NULL) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltNewTemplate : malloc failed\n");
	return(NULL);
    }
    memset(cur, 0, sizeof(xsltTemplate));
    return(cur);
}

/**
 * xsltFreeTemplate:
 * @template:  an XSLT template
 *
 * Free up the memory allocated by @template
 */
void
xsltFreeTemplate(xsltTemplatePtr template) {
    if (template == NULL)
	return;
    memset(template, -1, sizeof(xsltTemplate));
    xmlFree(template);
}

/**
 * xsltFreeTemplateList:
 * @template:  an XSLT template list
 *
 * Free up the memory allocated by all the elements of @template
 */
void
xsltFreeTemplateList(xsltTemplatePtr template) {
    xsltTemplatePtr cur;

    while (template == NULL) {
	cur = template;
	template = template->next;
	xsltFreeTemplate(cur);
    }
}

/**
 * xsltNewStylesheet:
 *
 * Create a new XSLT Stylesheet
 *
 * Returns the newly allocated xsltStylesheetPtr or NULL in case of error
 */
xsltStylesheetPtr
xsltNewStylesheet(void) {
    xsltStylesheetPtr cur;

    cur = (xsltStylesheetPtr) xmlMalloc(sizeof(xsltStylesheet));
    if (cur == NULL) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltNewStylesheet : malloc failed\n");
	return(NULL);
    }
    memset(cur, 0, sizeof(xsltStylesheet));
    return(cur);
}

/**
 * xsltFreeStylesheet:
 * @sheet:  an XSLT stylesheet
 *
 * Free up the memory allocated by @sheet
 */
void
xsltFreeStylesheet(xsltStylesheetPtr sheet) {
    if (sheet == NULL)
	return;
    xsltFreeTemplateList(sheet->templates);
    if (sheet->doc != NULL)
	xmlFreeDoc(sheet->doc);
    memset(sheet, -1, sizeof(xsltStylesheet));
    xmlFree(sheet);
}

/************************************************************************
 *									*
 *		Parsing of an XSLT Stylesheet				*
 *									*
 ************************************************************************/

/**
 * xsltParseStylesheetTemplate:
 * @style:  the XSLT stylesheet
 * @template:  the "template" element
 *
 * parse an XSLT stylesheet building the associated structures
 */

void
xsltParseStylesheetTemplate(xsltStylesheetPtr style, xmlNodePtr template) {
    xsltTemplatePtr ret;
    xmlNodePtr cur;

    if (template == NULL)
	return;

    ret = xsltNewTemplate();
    if (ret == NULL)
	return;
    ret->next = style->templates;
    style->templates = ret;

    cur = template->children;

    /*
     * Find and handle the params
     */
    while (cur != NULL) {
	if (IS_BLANK_NODE(cur)) {
            cur = cur->next;
	    continue;
	}
	if (!(IS_XSLT_ELEM(cur))) {
#ifdef DEBUG_PARSING
	    xsltGenericError(xsltGenericErrorContext,
		    "xsltParseStylesheetTop : found foreign element %s\n",
		    cur->name);
#endif
            cur = cur->next;
	    continue;
	}
	if (xmlStrEqual(cur->name, "param")) {
	    TODO /* Handle param */
	} else
	    break;
	cur = cur->next;
    }

    ret->content = template->children;
}

/**
 * xsltParseStylesheetTop:
 * @style:  the XSLT stylesheet
 * @top:  the top level "stylesheet" element
 *
 * scan the top level elements of an XSL stylesheet
 */

void
xsltParseStylesheetTop(xsltStylesheetPtr style, xmlNodePtr top) {
    xmlNodePtr cur;

    if (top == NULL)
	return;
    cur = top->children;

    while (cur != NULL) {
	if (IS_BLANK_NODE(cur)) {
            cur = cur->next;
	    continue;
	}
	if (!(IS_XSLT_ELEM(cur))) {
#ifdef DEBUG_PARSING
	    xsltGenericError(xsltGenericErrorContext,
		    "xsltParseStylesheetTop : found foreign element %s\n",
		    cur->name);
#endif
            cur = cur->next;
	    continue;
	}
	if (xmlStrEqual(cur->name, "import")) {
	    TODO /* Handle import */
	} else
	    break;
	cur = cur->next;
    }
    while (cur != NULL) {
	if (IS_BLANK_NODE(cur)) {
            cur = cur->next;
	    continue;
	}
	if (!(IS_XSLT_ELEM(cur))) {
#ifdef DEBUG_PARSING
	    xsltGenericError(xsltGenericErrorContext,
		    "xsltParseStylesheetTop : found foreign element %s\n",
		    cur->name);
#endif
            cur = cur->next;
	    continue;
	}
	if (xmlStrEqual(cur->name, "import")) {
	    xsltGenericError(xsltGenericErrorContext,
		"xsltParseStylesheetTop: ignoring misplaced import element\n");
        } else if (xmlStrEqual(cur->name, "include")) {
	    TODO /* Handle include */
        } else if (xmlStrEqual(cur->name, "strip-space")) {
	    TODO /* Handle strip-space */
        } else if (xmlStrEqual(cur->name, "preserve-space")) {
	    TODO /* Handle preserve-space */
        } else if (xmlStrEqual(cur->name, "output")) {
	    TODO /* Handle output */
        } else if (xmlStrEqual(cur->name, "key")) {
	    TODO /* Handle key */
        } else if (xmlStrEqual(cur->name, "decimal-format")) {
	    TODO /* Handle decimal-format */
        } else if (xmlStrEqual(cur->name, "attribute-set")) {
	    TODO /* Handle attribute-set */
        } else if (xmlStrEqual(cur->name, "variable")) {
	    TODO /* Handle variable */
        } else if (xmlStrEqual(cur->name, "param")) {
	    TODO /* Handle param */
        } else if (xmlStrEqual(cur->name, "template")) {
	    xsltParseStylesheetTemplate(style, cur);
        } else if (xmlStrEqual(cur->name, "namespace-alias")) {
	    TODO /* Handle namespace-alias */
	} else {
	    xsltGenericError(xsltGenericErrorContext,
		"xsltParseStylesheetTop: ignoring unknown %s element\n",
		             cur->name);
	}
	cur = cur->next;
    }
}

/**
 * xsltParseStylesheetDoc:
 * @doc:  and xmlDoc parsed XML
 *
 * parse an XSLT stylesheet building the associated structures
 *
 * Returns a new XSLT stylesheet structure.
 */

xsltStylesheetPtr
xsltParseStylesheetDoc(xmlDocPtr doc) {
    xsltStylesheetPtr ret;
    xmlNodePtr cur;

    if (doc == NULL)
	return(NULL);

    ret = xsltNewStylesheet();
    if (ret == NULL)
	return(NULL);

    /*
     * First step, locate the xsl:stylesheet element and the
     * namespace declaration.
     */
    cur = xmlDocGetRootElement(doc);
    if (cur == NULL) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltParseStylesheetDoc : empty stylesheet\n");
	xsltFreeStylesheet(ret);
	return(NULL);
    }

    if ((IS_XSLT_ELEM(cur)) && (xmlStrEqual(cur->name, "stylesheet"))) {
#ifdef DEBUG_PARSING
	xsltGenericError(xsltGenericErrorContext,
		"xsltParseStylesheetDoc : found stylesheet\n");
#endif
    } else {

	TODO /* lookup the stylesheet element down in the tree */
        xsltGenericError(xsltGenericErrorContext,
		"xsltParseStylesheetDoc : root is not stylesheet\n");
	xsltFreeStylesheet(ret);
	return(NULL);
    }
    ret->doc = doc;

    xsltParseStylesheetTop(ret, cur);

    return(ret);
}

/**
 * xsltParseStylesheetFile:
 * @filename:  the filename/URL to the stylesheet
 *
 * Load and parse an XSLT stylesheet
 *
 * Returns a new XSLT stylesheet structure.
 */

xsltStylesheetPtr
xsltParseStylesheetFile(const xmlChar* filename) {
    xsltStylesheetPtr ret;
    xmlDocPtr doc;
    

    if (filename == NULL)
	return(NULL);

#ifdef DEBUG_PARSING
    xsltGenericError(xsltGenericErrorContext,
	    "xsltParseStylesheetFile : parse %s\n", filename);
#endif

    doc = xmlParseFile(filename);
    if (doc == NULL) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltParseStylesheetFile : cannot parse %s\n", filename);
	return(NULL);
    }
    ret = xsltParseStylesheetDoc(doc);
    if (ret == NULL) {
	xmlFreeDoc(doc);
	return(NULL);
    }

    return(ret);
}

