/*
 * xslt.c: Implemetation of an XSL Transformation 1.0 engine
 *
 * Reference:
 *   http://www.w3.org/TR/1999/REC-xslt-19991116
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
 */

#include "xsltconfig.h"

#include <string.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/valid.h>
#include <libxml/hash.h>
#include <libxml/xmlerror.h>
#include <libxml/parserInternals.h>
#include "xslt.h"
#include "xsltInternals.h"
#include "pattern.h"
#include "variables.h"
#include "xsltutils.h"

#define DEBUG_PARSING

/*
 * Useful macros
 */

#define IS_BLANK(c) (((c) == 0x20) || ((c) == 0x09) || ((c) == 0xA) ||	\
                     ((c) == 0x0D))

#define IS_BLANK_NODE(n)						\
    (((n)->type == XML_TEXT_NODE) && (xsltIsBlank((n)->content)))


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
    cur->priority = XSLT_PAT_NO_PRIORITY;
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
    if (template->match) xmlFree(template->match);
    if (template->name) xmlFree(template->name);
    if (template->nameURI) xmlFree(template->nameURI);
    if (template->mode) xmlFree(template->mode);
    if (template->modeURI) xmlFree(template->modeURI);
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

    while (template != NULL) {
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
    cur->omitXmlDeclaration = -1;
    cur->standalone = -1;
    cur->indent = -1;
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

    xsltFreeTemplateHashes(sheet);
    xsltFreeVariableHashes(sheet);
    xsltFreeTemplateList(sheet->templates);
    if (sheet->doc != NULL)
	xmlFreeDoc(sheet->doc);
    if (sheet->stripSpaces != NULL)
	xmlHashFree(sheet->stripSpaces, NULL);

    if (sheet->method != NULL) xmlFree(sheet->method);
    if (sheet->methodURI != NULL) xmlFree(sheet->methodURI);
    if (sheet->version != NULL) xmlFree(sheet->version);
    if (sheet->encoding != NULL) xmlFree(sheet->encoding);
    if (sheet->doctypePublic != NULL) xmlFree(sheet->doctypePublic);
    if (sheet->doctypeSystem != NULL) xmlFree(sheet->doctypeSystem);
    if (sheet->mediaType != NULL) xmlFree(sheet->mediaType);

    memset(sheet, -1, sizeof(xsltStylesheet));
    xmlFree(sheet);
}

/************************************************************************
 *									*
 *		Parsing of an XSLT Stylesheet				*
 *									*
 ************************************************************************/

/**
 * xsltParseStylesheetOutput:
 * @style:  the XSLT stylesheet
 * @template:  the "output" element
 *
 * parse an XSLT stylesheet output element and record
 * information related to the stylesheet output
 */

void
xsltParseStylesheetOutput(xsltStylesheetPtr style, xmlNodePtr cur) {
    xmlChar *elements, *prop;
    xmlChar *element, *end;

    if ((cur == NULL) || (style == NULL))
	return;

    prop = xmlGetNsProp(cur, (const xmlChar *)"version", XSLT_NAMESPACE);
    if (prop != NULL) {
	if (style->version != NULL) xmlFree(style->version);
	style->version  = prop;
    }

    prop = xmlGetNsProp(cur, (const xmlChar *)"encoding", XSLT_NAMESPACE);
    if (prop != NULL) {
	if (style->encoding != NULL) xmlFree(style->encoding);
	style->encoding  = prop;
    }

    prop = xmlGetNsProp(cur, (const xmlChar *)"method", XSLT_NAMESPACE);
    if (prop != NULL) {
	xmlChar *ncname;
	xmlChar *prefix = NULL;

	if (style->method != NULL) xmlFree(style->method);
	style->method = NULL;
	if (style->methodURI != NULL) xmlFree(style->methodURI);
	style->methodURI = NULL;

	ncname = xmlSplitQName2(prop, &prefix);
	if (ncname != NULL) {
	    if (prefix != NULL) {
		xmlNsPtr ns;

		ns = xmlSearchNs(cur->doc, cur, prefix);
		if (ns == NULL) {
		    xsltGenericError(xsltGenericErrorContext,
			"no namespace bound to prefix %s\n", prefix);
		    xmlFree(prefix);
		    xmlFree(ncname);
		    style->method = prop;
		} else {
		    style->methodURI = xmlStrdup(ns->href);
		    style->method = ncname;
		    xmlFree(prefix);
		    xmlFree(prop);
		}
	    } else {
		style->method = ncname;
		xmlFree(prop);
	    }
	} else {
	    if ((xmlStrEqual(prop, (const xmlChar *)"xml")) ||
		(xmlStrEqual(prop, (const xmlChar *)"html")) ||
		(xmlStrEqual(prop, (const xmlChar *)"text"))) {
		style->method  = prop;
	    } else {
		xsltGenericError(xsltGenericErrorContext,
		    "invalid value for method: %s\n", prop);
	    }
	}
    }

    prop = xmlGetNsProp(cur, (const xmlChar *)"doctype-system", XSLT_NAMESPACE);
    if (prop != NULL) {
	if (style->doctypeSystem != NULL) xmlFree(style->doctypeSystem);
	style->doctypeSystem  = prop;
    }

    prop = xmlGetNsProp(cur, (const xmlChar *)"doctype-public", XSLT_NAMESPACE);
    if (prop != NULL) {
	if (style->doctypePublic != NULL) xmlFree(style->doctypePublic);
	style->doctypePublic  = prop;
    }

    prop = xmlGetNsProp(cur, (const xmlChar *)"standalone",
	                XSLT_NAMESPACE);
    if (prop != NULL) {
	if (xmlStrEqual(prop, (const xmlChar *)"yes")) {
	    style->standalone = 1;
	} else if (xmlStrEqual(prop, (const xmlChar *)"no")) {
	    style->standalone = 0;
	} else {
	    xsltGenericError(xsltGenericErrorContext,
		"invalid value for standalone: %s\n", prop);
	}
	xmlFree(prop);
    }

    prop = xmlGetNsProp(cur, (const xmlChar *)"indent",
	                XSLT_NAMESPACE);
    if (prop != NULL) {
	if (xmlStrEqual(prop, (const xmlChar *)"yes")) {
	    style->indent = 1;
	} else if (xmlStrEqual(prop, (const xmlChar *)"no")) {
	    style->indent = 0;
	} else {
	    xsltGenericError(xsltGenericErrorContext,
		"invalid value for indent: %s\n", prop);
	}
	xmlFree(prop);
    }

    prop = xmlGetNsProp(cur, (const xmlChar *)"omit-xml-declaration",
	                XSLT_NAMESPACE);
    if (prop != NULL) {
	if (xmlStrEqual(prop, (const xmlChar *)"yes")) {
	    style->omitXmlDeclaration = 1;
	} else if (xmlStrEqual(prop, (const xmlChar *)"no")) {
	    style->omitXmlDeclaration = 0;
	} else {
	    xsltGenericError(xsltGenericErrorContext,
		"invalid value for omit-xml-declaration: %s\n", prop);
	}
	xmlFree(prop);
    }

    elements = xmlGetNsProp(cur, (const xmlChar *)"cdata-section-elements",
	                    XSLT_NAMESPACE);
    if (elements != NULL) {
	if (style->stripSpaces == NULL)
	    style->stripSpaces = xmlHashCreate(10);
	if (style->stripSpaces == NULL)
	    return;

	element = elements;
	while (*element != 0) {
	    while (IS_BLANK(*element)) element++;
	    if (*element == 0)
		break;
	    end = element;
	    while ((*end != 0) && (!IS_BLANK(*end))) end++;
	    element = xmlStrndup(element, end - element);
	    if (element) {
#ifdef DEBUG_PARSING
		xsltGenericDebug(xsltGenericDebugContext,
		    "add cdata section output element %s\n", element);
#endif
		xmlHashAddEntry(style->stripSpaces, element, "cdata");
		xmlFree(element);
	    }
	    element = end;
	}
	xmlFree(elements);
    }
}

/**
 * xsltParseStylesheetPreserveSpace:
 * @style:  the XSLT stylesheet
 * @template:  the "preserve-space" element
 *
 * parse an XSLT stylesheet preserve-space element and record
 * elements needing preserving
 */

void
xsltParseStylesheetPreserveSpace(xsltStylesheetPtr style, xmlNodePtr cur) {
    xmlChar *elements;
    xmlChar *element, *end;

    if ((cur == NULL) || (style == NULL))
	return;

    elements = xmlGetNsProp(cur, (const xmlChar *)"elements", XSLT_NAMESPACE);
    if (elements == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsltParseStylesheetPreserveSpace: missing elements attribute\n");
	return;
    }

    if (style->stripSpaces == NULL)
	style->stripSpaces = xmlHashCreate(10);
    if (style->stripSpaces == NULL)
	return;

    element = elements;
    while (*element != 0) {
	while (IS_BLANK(*element)) element++;
	if (*element == 0)
	    break;
        end = element;
	while ((*end != 0) && (!IS_BLANK(*end))) end++;
	element = xmlStrndup(element, end - element);
	if (element) {
#ifdef DEBUG_PARSING
	    xsltGenericDebug(xsltGenericDebugContext,
		"add preserved space element %s\n", element);
#endif
	    xmlHashAddEntry(style->stripSpaces, element, "preserve");
	    xmlFree(element);
	}
	element = end;
    }
    xmlFree(elements);
}

/**
 * xsltParseStylesheetStripSpace:
 * @style:  the XSLT stylesheet
 * @template:  the "strip-space" element
 *
 * parse an XSLT stylesheet strip-space element and record
 * elements needing stripping
 */

void
xsltParseStylesheetStripSpace(xsltStylesheetPtr style, xmlNodePtr cur) {
    xmlChar *elements;
    xmlChar *element, *end;

    if ((cur == NULL) || (style == NULL))
	return;

    elements = xmlGetNsProp(cur, (const xmlChar *)"elements", XSLT_NAMESPACE);
    if (elements == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsltParseStylesheetStripSpace: missing elements attribute\n");
	return;
    }

    if (style->stripSpaces == NULL)
	style->stripSpaces = xmlHashCreate(10);
    if (style->stripSpaces == NULL)
	return;

    element = elements;
    while (*element != 0) {
	while (IS_BLANK(*element)) element++;
	if (*element == 0)
	    break;
        end = element;
	while ((*end != 0) && (!IS_BLANK(*end))) end++;
	element = xmlStrndup(element, end - element);
	if (element) {
#ifdef DEBUG_PARSING
	    xsltGenericDebug(xsltGenericDebugContext,
		"add stripped space element %s\n", element);
#endif
	    xmlHashAddEntry(style->stripSpaces, element, "strip");
	    xmlFree(element);
	}
	element = end;
    }
    xmlFree(elements);
}

/**
 * xsltParseTemplateContent:
 * @style:  the XSLT stylesheet
 * @ret:  the "template" structure
 * @template:  the container node (can be a document for literal results)
 *
 * parse an XSLT template element content
 * Clean-up the template content from unwanted ignorable blank nodes
 * and process xslt:text
 */

void
xsltParseTemplateContent(xsltStylesheetPtr style, xsltTemplatePtr ret,
	                 xmlNodePtr template) {
    xmlNodePtr cur, delete;
    /*
     * This content comes from the stylesheet
     * For stylesheets, the set of whitespace-preserving
     * element names consists of just xsl:text.
     */
    cur = template->children;
    delete = NULL;
    while (cur != NULL) {
	if (delete != NULL) {
#ifdef DEBUG_PARSING
	    xsltGenericDebug(xsltGenericDebugContext,
	     "xsltParseStylesheetTemplate: removing ignorable blank node\n");
#endif
	    xmlUnlinkNode(delete);
	    xmlFreeNode(delete);
	    delete = NULL;
	}
	if (IS_XSLT_ELEM(cur)) {
	    if (IS_XSLT_NAME(cur, "text")) {
		if (cur->children != NULL) {
		    if ((cur->children->type != XML_TEXT_NODE) ||
			(cur->children->next != NULL)) {
			xsltGenericError(xsltGenericErrorContext,
	     "xsltParseStylesheetTemplate: xslt:text content problem\n");
		    } else {
			xmlChar *prop;
			xmlNodePtr text = cur->children;
			
			prop = xmlGetNsProp(cur,
				(const xmlChar *)"disable-output-escaping",
				            XSLT_NAMESPACE);
			if (prop != NULL) {
			    if (xmlStrEqual(prop, (const xmlChar *)"yes")) {
#if LIBXML_VERSION > 20211
				text->name = xmlStringTextNoenc;
#else
				xsltGenericError(xsltGenericErrorContext,
"xsl:text disable-output-escaping need newer > 20211 libxml version\n");
#endif
			    } else if (!xmlStrEqual(prop,
					            (const xmlChar *)"no")){
				xsltGenericError(xsltGenericErrorContext,
		 "xslt:text: disable-output-escaping allow only yes or no\n");

			    }
			    xmlFree(prop);
			}
			xmlUnlinkNode(text);
			xmlAddPrevSibling(cur, text);
		    }
		}
		delete = cur;
		goto skip_children;
	    }
	} else if (cur->type == XML_TEXT_NODE) {
	    if (IS_BLANK_NODE(cur)) {
		if (xmlNodeGetSpacePreserve(cur) != 1) {
		    delete = cur;
		}
	    }
	} else if (cur->type != XML_ELEMENT_NODE) {
	    delete = cur;
	}

	/*
	 * Skip to next node
	 */
	if (cur->children != NULL) {
	    if (cur->children->type != XML_ENTITY_DECL) {
		cur = cur->children;
		continue;
	    }
	}
skip_children:
	if (cur->next != NULL) {
	    cur = cur->next;
	    continue;
	}
	
	do {
	    cur = cur->parent;
	    if (cur == NULL)
		break;
	    if (cur == template) {
		cur = NULL;
		break;
	    }
	    if (cur->next != NULL) {
		cur = cur->next;
		break;
	    }
	} while (cur != NULL);
    }
    if (delete != NULL) {
#ifdef DEBUG_PARSING
	xsltGenericDebug(xsltGenericDebugContext,
	 "xsltParseStylesheetTemplate: removing ignorable blank node\n");
#endif
	xmlUnlinkNode(delete);
	xmlFreeNode(delete);
	delete = NULL;
    }

    /*
     * Find and handle the params
     */
    cur = template->children;
    while (cur != NULL) {
	/*
	 * Remove Blank nodes found at this level.
	 */
	if (IS_BLANK_NODE(cur)) {
	    xmlNodePtr blank = cur;

            cur = cur->next;
	    xmlUnlinkNode(blank);
	    xmlFreeNode(blank);
	    continue;
	}
	if ((IS_XSLT_ELEM(cur)) && (IS_XSLT_NAME(cur, "param"))) {
	    TODO /* Handle param */
	} else
	    break;
	cur = cur->next;
    }

    /*
     * Browse the remaining of the template
     */
    while (cur != NULL) {
	/*
	 * Remove Blank nodes found at this level.
	 */
	if (IS_BLANK_NODE(cur)) {
	    xmlNodePtr blank = cur;

            cur = cur->next;
	    xmlUnlinkNode(blank);
	    xmlFreeNode(blank);
	    continue;
	}
	if ((IS_XSLT_ELEM(cur)) && (IS_XSLT_NAME(cur, "param"))) {
	    xmlNodePtr param = cur;

            cur = cur->next;
	    xsltGenericError(xsltGenericErrorContext,
		"xsltParseStylesheetTop: ignoring misplaced param element\n");
	    xmlUnlinkNode(param);
	    xmlFreeNode(param);
	    continue;
	} else
	    break;
	cur = cur->next;
    }

    ret->content = template->children;
}

/**
 * xsltParseStylesheetTemplate:
 * @style:  the XSLT stylesheet
 * @template:  the "template" element
 *
 * parse an XSLT stylesheet template building the associated structures
 */

void
xsltParseStylesheetTemplate(xsltStylesheetPtr style, xmlNodePtr template) {
    xsltTemplatePtr ret;
    xmlChar *prop;

    if (template == NULL)
	return;

    /*
     * Create and link the structure
     */
    ret = xsltNewTemplate();
    if (ret == NULL)
	return;
    ret->next = style->templates;
    style->templates = ret;

    /*
     * Get arguments
     */
    prop = xmlGetNsProp(template, (const xmlChar *)"match", XSLT_NAMESPACE);
    if (prop != NULL) {
	if (ret->match != NULL) xmlFree(ret->match);
	ret->match  = prop;
    }

    prop = xmlGetNsProp(template, (const xmlChar *)"name", XSLT_NAMESPACE);
    if (prop != NULL) {
	xmlChar *ncname;
	xmlChar *prefix = NULL;

	if (ret->name != NULL) xmlFree(ret->name);
	ret->name = NULL;
	if (ret->nameURI != NULL) xmlFree(ret->nameURI);
	ret->nameURI = NULL;

	ncname = xmlSplitQName2(prop, &prefix);
	if (ncname != NULL) {
	    if (prefix != NULL) {
		xmlNsPtr ns;

		ns = xmlSearchNs(template->doc, template, prefix);
		if (ns == NULL) {
		    xsltGenericError(xsltGenericErrorContext,
			"no namespace bound to prefix %s\n", prefix);
		    xmlFree(prefix);
		    xmlFree(ncname);
		    ret->name = prop;
		} else {
		    ret->nameURI = xmlStrdup(ns->href);
		    ret->name = ncname;
		    xmlFree(prefix);
		    xmlFree(prop);
		}
	    } else {
		ret->name = ncname;
		xmlFree(prop);
	    }
	} else {
	    ret->name  = prop;
	}
    }

    /*
     * parse the content and register the pattern
     */
    xsltParseTemplateContent(style, ret, template);
    xsltAddTemplate(style, ret);
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
    xmlChar *prop;
#ifdef DEBUG_PARSING
    int templates = 0;
#endif

    if (top == NULL)
	return;

    prop = xmlGetNsProp(cur, (const xmlChar *)"version", XSLT_NAMESPACE);
    if (prop == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsl:version is missing: document may not be a stylesheet\n");
    } else {
	if (!xmlStrEqual(prop, (const xmlChar *)"1.0")) {
	    xsltGenericError(xsltGenericErrorContext,
		"xsl:version: only 1.0 features are supported\n");
	    TODO /* set up compatibility when not XSLT 1.0 */
	}
	xmlFree(prop);
    }

    cur = top->children;

    while (cur != NULL) {
	if (IS_BLANK_NODE(cur)) {
            cur = cur->next;
	    continue;
	}
	if (!(IS_XSLT_ELEM(cur))) {
#ifdef DEBUG_PARSING
	    xsltGenericDebug(xsltGenericDebugContext,
		    "xsltParseStylesheetTop : found foreign element %s\n",
		    cur->name);
#endif
            cur = cur->next;
	    continue;
	}
	if (IS_XSLT_NAME(cur, "import")) {
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
	    xsltGenericDebug(xsltGenericDebugContext,
		    "xsltParseStylesheetTop : found foreign element %s\n",
		    cur->name);
#endif
            cur = cur->next;
	    continue;
	}
	if (IS_XSLT_NAME(cur, "import")) {
	    xsltGenericError(xsltGenericErrorContext,
		"xsltParseStylesheetTop: ignoring misplaced import element\n");
        } else if (IS_XSLT_NAME(cur, "include")) {
	    TODO /* Handle include */
        } else if (IS_XSLT_NAME(cur, "strip-space")) {
	    xsltParseStylesheetStripSpace(style, cur);
        } else if (IS_XSLT_NAME(cur, "preserve-space")) {
	    xsltParseStylesheetPreserveSpace(style, cur);
        } else if (IS_XSLT_NAME(cur, "output")) {
	    xsltParseStylesheetOutput(style, cur);
        } else if (IS_XSLT_NAME(cur, "key")) {
	    TODO /* Handle key */
        } else if (IS_XSLT_NAME(cur, "decimal-format")) {
	    TODO /* Handle decimal-format */
        } else if (IS_XSLT_NAME(cur, "attribute-set")) {
	    TODO /* Handle attribute-set */
        } else if (IS_XSLT_NAME(cur, "variable")) {
	    TODO /* Handle variable */
        } else if (IS_XSLT_NAME(cur, "param")) {
	    TODO /* Handle param */
        } else if (IS_XSLT_NAME(cur, "template")) {
#ifdef DEBUG_PARSING
	    templates++;
#endif
	    xsltParseStylesheetTemplate(style, cur);
        } else if (IS_XSLT_NAME(cur, "namespace-alias")) {
	    TODO /* Handle namespace-alias */
	} else {
	    xsltGenericError(xsltGenericErrorContext,
		"xsltParseStylesheetTop: ignoring unknown %s element\n",
		             cur->name);
	}
	cur = cur->next;
    }
#ifdef DEBUG_PARSING
    xsltGenericDebug(xsltGenericDebugContext,
		    "parsed %d templates\n", templates);
#endif
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

    ret->doc = doc;
    if ((IS_XSLT_ELEM(cur)) && 
	((IS_XSLT_NAME(cur, "stylesheet")) ||
	 (IS_XSLT_NAME(cur, "transform")))) {
#ifdef DEBUG_PARSING
	xsltGenericDebug(xsltGenericDebugContext,
		"xsltParseStylesheetDoc : found stylesheet\n");
#endif

	xsltParseStylesheetTop(ret, cur);
    } else {
	xmlChar *prop;
	xsltTemplatePtr template;

	/*
	 * the document itself might be the template, check xsl:version
	 */
	prop = xmlGetNsProp(cur, (const xmlChar *)"version", XSLT_NAMESPACE);
	if (prop == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
		"xsltParseStylesheetDoc : document is not a stylesheet\n");
	    xsltFreeStylesheet(ret);
	    return(NULL);
	}

#ifdef DEBUG_PARSING
        xsltGenericDebug(xsltGenericDebugContext,
		"xsltParseStylesheetDoc : document is stylesheet\n");
#endif
	
	/* TODO: check the version */
	xmlFree(prop);

	/*
	 * Create and link the template
	 */
	template = xsltNewTemplate();
	if (template == NULL) {
	    xsltFreeStylesheet(ret);
	    return(NULL);
	}
	template->next = ret->templates;
	ret->templates = template;
	template->match = xmlStrdup((const xmlChar *)"/");

	/*
	 * parse the content and register the pattern
	 */
	xsltParseTemplateContent(ret, template, (xmlNodePtr) doc);
	xsltAddTemplate(ret, template);
    }

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
    xsltGenericDebug(xsltGenericDebugContext,
	    "xsltParseStylesheetFile : parse %s\n", filename);
#endif

    doc = xmlParseFile((const char *) filename);
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

