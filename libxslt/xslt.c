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
#include <libxml/uri.h>
#include <libxml/xmlerror.h>
#include <libxml/parserInternals.h>
#include "xslt.h"
#include "xsltInternals.h"
#include "pattern.h"
#include "variables.h"
#include "namespaces.h"
#include "attributes.h"
#include "xsltutils.h"
#include "imports.h"
#include "keys.h"
#include "documents.h"
#include "extensions.h"
#include "preproc.h"

#ifdef WITH_XSLT_DEBUG
#define WITH_XSLT_DEBUG_PARSING
/* #define WITH_XSLT_DEBUG_BLANKS */
#endif

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
 * Check if a string is ignorable
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
static xsltDecimalFormatPtr
xsltNewDecimalFormat(xmlChar *name)
{
    xsltDecimalFormatPtr self;

    self = xmlMalloc(sizeof(xsltDecimalFormat));
    if (self != NULL) {
	self->next = NULL;
	self->name = name;
	
	/* Default values */
	self->digit = xmlStrdup(BAD_CAST("#"));
	self->patternSeparator = xmlStrdup(BAD_CAST(";"));
	self->decimalPoint = xmlStrdup(BAD_CAST("."));
	self->grouping = xmlStrdup(BAD_CAST(","));
	self->percent = xmlStrdup(BAD_CAST("%"));
	self->permille = xmlStrdup(BAD_CAST("?"));
	self->zeroDigit = xmlStrdup(BAD_CAST("0"));
	self->minusSign = xmlStrdup(BAD_CAST("-"));
	self->infinity = xmlStrdup(BAD_CAST("Infinity"));
	self->noNumber = xmlStrdup(BAD_CAST("NaN"));
    }
    return self;
}

static void
xsltFreeDecimalFormat(xsltDecimalFormatPtr self)
{
    if (self != NULL) {
	if (self->digit)
	    xmlFree(self->digit);
	if (self->patternSeparator)
	    xmlFree(self->patternSeparator);
	if (self->decimalPoint)
	    xmlFree(self->decimalPoint);
	if (self->grouping)
	    xmlFree(self->grouping);
	if (self->percent)
	    xmlFree(self->percent);
	if (self->permille)
	    xmlFree(self->permille);
	if (self->zeroDigit)
	    xmlFree(self->zeroDigit);
	if (self->minusSign)
	    xmlFree(self->minusSign);
	if (self->infinity)
	    xmlFree(self->infinity);
	if (self->noNumber)
	    xmlFree(self->noNumber);
	if (self->name)
	    xmlFree(self->name);
	xmlFree(self);
    }
}

static void
xsltFreeDecimalFormatList(xsltStylesheetPtr self)
{
    xsltDecimalFormatPtr iter;
    xsltDecimalFormatPtr tmp;

    if (self == NULL)
	return;
    
    iter = self->decimalFormat;
    while (iter != NULL) {
	tmp = iter->next;
	xsltFreeDecimalFormat(iter);
	iter = tmp;
    }
}

/**
 * xsltDecimalFormatGetByName:
 * @sheet: the XSLT stylesheet
 * @name: the decimal-format name to find
 *
 * Find decimal-format by name
 */
xsltDecimalFormatPtr
xsltDecimalFormatGetByName(xsltStylesheetPtr sheet, xmlChar *name)
{
    xsltDecimalFormatPtr result;

    if (name == NULL)
	return sheet->decimalFormat;
    
    for (result = sheet->decimalFormat->next;
	 result != NULL;
	 result = result->next) {
	if (xmlStrEqual(name, result->name))
	    break; /* for */
    }
    return result;
}


/**
 * xsltNewTemplate:
 *
 * Create a new XSLT Template
 *
 * Returns the newly allocated xsltTemplatePtr or NULL in case of error
 */
static xsltTemplatePtr
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
static void
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
static void
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
    cur->decimalFormat = xsltNewDecimalFormat(NULL);
    cur->indent = -1;
    cur->errors = 0;
    cur->warnings = 0;
    return(cur);
}

/**
 * xsltFreeStylesheetList:
 * @sheet:  an XSLT stylesheet list
 *
 * Free up the memory allocated by the list @sheet
 */
static void
xsltFreeStylesheetList(xsltStylesheetPtr sheet) {
    xsltStylesheetPtr next;

    while (sheet != NULL) {
	next = sheet->next;
	xsltFreeStylesheet(sheet);
	sheet = next;
    }
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

    xsltFreeKeys(sheet);
    xsltFreeExts(sheet);
    xsltFreeTemplateHashes(sheet);
    xsltFreeDecimalFormatList(sheet);
    xsltFreeTemplateList(sheet->templates);
    xsltFreeAttributeSetsHashes(sheet);
    xsltFreeNamespaceAliasHashes(sheet);
    xsltFreeStyleDocuments(sheet);
    xsltFreeStylePreComps(sheet);
    if (sheet->doc != NULL)
	xmlFreeDoc(sheet->doc);
    if (sheet->variables != NULL)
	xsltFreeStackElemList(sheet->variables);
    if (sheet->stripSpaces != NULL)
	xmlHashFree(sheet->stripSpaces, NULL);
    if (sheet->nsHash != NULL) 
	xmlHashFree(sheet->nsHash, NULL);

    if (sheet->method != NULL) xmlFree(sheet->method);
    if (sheet->methodURI != NULL) xmlFree(sheet->methodURI);
    if (sheet->version != NULL) xmlFree(sheet->version);
    if (sheet->encoding != NULL) xmlFree(sheet->encoding);
    if (sheet->doctypePublic != NULL) xmlFree(sheet->doctypePublic);
    if (sheet->doctypeSystem != NULL) xmlFree(sheet->doctypeSystem);
    if (sheet->mediaType != NULL) xmlFree(sheet->mediaType);

    if (sheet->imports != NULL)
	xsltFreeStylesheetList(sheet->imports);

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

    /* relaxed to support xt:document */
    prop = xmlGetProp(cur, (const xmlChar *)"method");
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
		    style->warnings++;
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
		style->warnings++;
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
	    style->warnings++;
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
	    style->warnings++;
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
	    style->warnings++;
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
#ifdef WITH_XSLT_DEBUG_PARSING
		xsltGenericDebug(xsltGenericDebugContext,
		    "add cdata section output element %s\n", element);
#endif
		xmlHashAddEntry(style->stripSpaces, element,
			        (xmlChar *) "cdata");
		xmlFree(element);
	    }
	    element = end;
	}
	xmlFree(elements);
    }
}

/**
 * xsltParseStylesheetDecimalFormat:
 * @style:  the XSLT stylesheet
 * @cur:  the "decimal-format" element
 *
 * parse an XSLT stylesheet decimal-format element and
 * and record the formatting characteristics
 */
static void
xsltParseStylesheetDecimalFormat(xsltStylesheetPtr style, xmlNodePtr cur)
{
    xmlChar *prop;
    xsltDecimalFormatPtr format;
    xsltDecimalFormatPtr iter;
    
    if ((cur == NULL) || (style == NULL))
	return;

    format = style->decimalFormat;
    
    prop = xmlGetNsProp(cur, BAD_CAST("name"), XSLT_NAMESPACE);
    if (prop != NULL) {
	format = xsltDecimalFormatGetByName(style, prop);
	if (format != NULL) {
	    xsltGenericError(xsltGenericErrorContext,
	 "xsltParseStylestyleDecimalFormat: %s already exists\n", prop);
	    style->warnings++;
	    return;
	}
	format = xsltNewDecimalFormat(prop);
	if (format == NULL) {
	    xsltGenericError(xsltGenericErrorContext,
     "xsltParseStylestyleDecimalFormat: failed creating new decimal-format\n");
	    style->errors++;
	    return;
	}
	/* Append new decimal-format structure */
	for (iter = style->decimalFormat; iter->next; iter = iter->next)
	    ;
	if (iter)
	    iter->next = format;
    }

    prop = xmlGetNsProp(cur, (const xmlChar *)"decimal-separator",
	                XSLT_NAMESPACE);
    if (prop != NULL) {
	if (format->decimalPoint != NULL) xmlFree(format->decimalPoint);
	format->decimalPoint  = prop;
    }
    
    prop = xmlGetNsProp(cur, (const xmlChar *)"grouping-separator",
	                XSLT_NAMESPACE);
    if (prop != NULL) {
	if (format->grouping != NULL) xmlFree(format->grouping);
	format->grouping  = prop;
    }

    prop = xmlGetNsProp(cur, (const xmlChar *)"infinity", XSLT_NAMESPACE);
    if (prop != NULL) {
	if (format->infinity != NULL) xmlFree(format->infinity);
	format->infinity  = prop;
    }
    
    prop = xmlGetNsProp(cur, (const xmlChar *)"minus-sign", XSLT_NAMESPACE);
    if (prop != NULL) {
	if (format->minusSign != NULL) xmlFree(format->minusSign);
	format->minusSign  = prop;
    }
    
    prop = xmlGetNsProp(cur, (const xmlChar *)"NaN", XSLT_NAMESPACE);
    if (prop != NULL) {
	if (format->noNumber != NULL) xmlFree(format->noNumber);
	format->noNumber  = prop;
    }
    
    prop = xmlGetNsProp(cur, (const xmlChar *)"percent", XSLT_NAMESPACE);
    if (prop != NULL) {
	if (format->percent != NULL) xmlFree(format->percent);
	format->percent  = prop;
    }
    
    prop = xmlGetNsProp(cur, (const xmlChar *)"per-mille", XSLT_NAMESPACE);
    if (prop != NULL) {
	if (format->permille != NULL) xmlFree(format->permille);
	format->permille  = prop;
    }
    
    prop = xmlGetNsProp(cur, (const xmlChar *)"zero-digit", XSLT_NAMESPACE);
    if (prop != NULL) {
	if (format->zeroDigit != NULL) xmlFree(format->zeroDigit);
	format->zeroDigit  = prop;
    }
    
    prop = xmlGetNsProp(cur, (const xmlChar *)"digit", XSLT_NAMESPACE);
    if (prop != NULL) {
	if (format->digit != NULL) xmlFree(format->digit);
	format->digit  = prop;
    }
    
    prop = xmlGetNsProp(cur, (const xmlChar *)"pattern-separator",
	                XSLT_NAMESPACE);
    if (prop != NULL) {
	if (format->patternSeparator != NULL) xmlFree(format->patternSeparator);
	format->patternSeparator  = prop;
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

static void
xsltParseStylesheetPreserveSpace(xsltStylesheetPtr style, xmlNodePtr cur) {
    xmlChar *elements;
    xmlChar *element, *end;

    if ((cur == NULL) || (style == NULL))
	return;

    elements = xmlGetNsProp(cur, (const xmlChar *)"elements", XSLT_NAMESPACE);
    if (elements == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsltParseStylesheetPreserveSpace: missing elements attribute\n");
	style->warnings++;
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
#ifdef WITH_XSLT_DEBUG_PARSING
	    xsltGenericDebug(xsltGenericDebugContext,
		"add preserved space element %s\n", element);
#endif
	    if (xmlStrEqual(element, (const xmlChar *)"*")) {
		style->stripAll = -1;
	    } else {
		xmlHashAddEntry(style->stripSpaces, element,
				(xmlChar *) "preserve");
	    }
	    xmlFree(element);
	}
	element = end;
    }
    xmlFree(elements);
}

/**
 * xsltParseStylesheetExtPrefix:
 * @style:  the XSLT stylesheet
 * @template:  the "strip-space" prefix
 *
 * parse an XSLT stylesheet extension prefix and record
 * prefixes needing stripping
 */

static void
xsltParseStylesheetExtPrefix(xsltStylesheetPtr style, xmlNodePtr cur) {
    xmlChar *prefixes;
    xmlChar *prefix, *end;

    if ((cur == NULL) || (style == NULL))
	return;

    prefixes = xmlGetNsProp(cur, (const xmlChar *)"extension-element-prefixes",
	                    XSLT_NAMESPACE);
    if (prefixes == NULL) {
	return;
    }

    prefix = prefixes;
    while (*prefix != 0) {
	while (IS_BLANK(*prefix)) prefix++;
	if (*prefix == 0)
	    break;
        end = prefix;
	while ((*end != 0) && (!IS_BLANK(*end))) end++;
	prefix = xmlStrndup(prefix, end - prefix);
	if (prefix) {
	    xmlNsPtr ns;

	    if (xmlStrEqual(prefix, (const xmlChar *)"#default"))
		ns = xmlSearchNs(style->doc, cur, NULL);
	    else
		ns = xmlSearchNs(style->doc, cur, prefix);
	    if (ns == NULL) {
		xsltGenericError(xsltGenericErrorContext,
	    "xsl:extension-element-prefix : undefined namespace %s\n",
	                         prefix);
		style->warnings++;
	    } else {
#ifdef WITH_XSLT_DEBUG_PARSING
		xsltGenericDebug(xsltGenericDebugContext,
		    "add extension prefix %s\n", prefix);
#endif
		xsltRegisterExtPrefix(style, prefix, ns->href);
	    }
	    xmlFree(prefix);
	}
	prefix = end;
    }
    xmlFree(prefixes);
}

/**
 * xsltParseStylesheetStripSpace:
 * @style:  the XSLT stylesheet
 * @template:  the "strip-space" element
 *
 * parse an XSLT stylesheet strip-space element and record
 * elements needing stripping
 */

static void
xsltParseStylesheetStripSpace(xsltStylesheetPtr style, xmlNodePtr cur) {
    xmlChar *elements;
    xmlChar *element, *end;

    if ((cur == NULL) || (style == NULL))
	return;

    elements = xmlGetNsProp(cur, (const xmlChar *)"elements", XSLT_NAMESPACE);
    if (elements == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsltParseStylesheetStripSpace: missing elements attribute\n");
	style->warnings++;
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
#ifdef WITH_XSLT_DEBUG_PARSING
	    xsltGenericDebug(xsltGenericDebugContext,
		"add stripped space element %s\n", element);
#endif
	    if (xmlStrEqual(element, (const xmlChar *)"*")) {
		style->stripAll = 1;
	    } else {
		xmlHashAddEntry(style->stripSpaces, element,
			        (xmlChar *) "strip");
	    }
	    xmlFree(element);
	}
	element = end;
    }
    xmlFree(elements);
}

/**
 * xsltPrecomputeStylesheet:
 * @style:  the XSLT stylesheet
 *
 * Clean-up the stylesheet content from unwanted ignorable blank nodes
 * and run the preprocessing of all XSLT constructs.
 *
 * and process xslt:text
 */
static void
xsltPrecomputeStylesheet(xsltStylesheetPtr style) {
    xmlNodePtr cur, delete;

    /*
     * This content comes from the stylesheet
     * For stylesheets, the set of whitespace-preserving
     * element names consists of just xsl:text.
     */
    cur = (xmlNodePtr) style->doc;
    if (cur == NULL)
	return;
    cur = cur->children;
    delete = NULL;
    while (cur != NULL) {
	if (delete != NULL) {
#ifdef WITH_XSLT_DEBUG_BLANKS
	    xsltGenericDebug(xsltGenericDebugContext,
	     "xsltPrecomputeStylesheet: removing ignorable blank node\n");
#endif
	    xmlUnlinkNode(delete);
	    xmlFreeNode(delete);
	    delete = NULL;
	}
	if ((cur->type == XML_ELEMENT_NODE) && (IS_XSLT_ELEM(cur))) {
	    xsltStylePreCompute(style, cur);
	    if (IS_XSLT_NAME(cur, "text")) {
		goto skip_children;
	    }
	} else if (cur->type == XML_TEXT_NODE) {
	    if (IS_BLANK_NODE(cur)) {
		if (xmlNodeGetSpacePreserve(cur) != 1) {
		    delete = cur;
		}
	    }
	} else if ((cur->type != XML_ELEMENT_NODE) &&
		   (cur->type != XML_CDATA_SECTION_NODE)) {
	    delete = cur;
	    goto skip_children;
	}

	/*
	 * Skip to next node
	 */
	if (cur->children != NULL) {
	    if ((cur->children->type != XML_ENTITY_DECL) &&
		(cur->children->type != XML_ENTITY_REF_NODE) &&
		(cur->children->type != XML_ENTITY_NODE)) {
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
	    if (cur == (xmlNodePtr) style->doc) {
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
#ifdef WITH_XSLT_DEBUG_PARSING
	xsltGenericDebug(xsltGenericDebugContext,
	 "xsltPrecomputeStylesheet: removing ignorable blank node\n");
#endif
	xmlUnlinkNode(delete);
	xmlFreeNode(delete);
	delete = NULL;
    }
}

/**
 * xsltGatherNamespaces:
 * @style:  the XSLT stylesheet
 *
 * Browse the stylesheet and buit the namspace hash table which
 * will be used for XPath interpretation. If needed do a bit of normalization
 */

static void
xsltGatherNamespaces(xsltStylesheetPtr style) {
    xmlNodePtr cur;
    const xmlChar *URI;

    /* 
     * TODO: basically if the stylesheet uses the same prefix for different
     *       patterns, well they may be in problem, hopefully they will get
     *       a warning first.
     */
    cur = xmlDocGetRootElement(style->doc);
    while (cur != NULL) {
	if (cur->type == XML_ELEMENT_NODE) {
	    xmlNsPtr ns = cur->nsDef;
	    while (ns != NULL) {
		if (ns->prefix != NULL) {
		    if (style->nsHash == NULL) {
			style->nsHash = xmlHashCreate(10);
			if (style->nsHash == NULL) {
			    xsltGenericError(xsltGenericErrorContext,
		 "xsltGatherNamespaces: failed to create hash table\n");
			    style->errors++;
			    return;
			}
		    }
		    URI = xmlHashLookup(style->nsHash, ns->prefix);
		    if ((URI != NULL) && (!xmlStrEqual(URI, ns->href))) {
			xsltGenericError(xsltGenericErrorContext,
	     "Namespaces prefix %s used for multiple namespaces\n");
			style->warnings++;
		    } else if (URI == NULL) {
			xmlHashUpdateEntry(style->nsHash, ns->prefix,
			    (void *) ns->href, (xmlHashDeallocator)xmlFree);

#ifdef WITH_XSLT_DEBUG_PARSING
			xsltGenericDebug(xsltGenericDebugContext,
		 "Added namespace: %s mapped to %s\n", ns->prefix, ns->href);
#endif
		    }
		}
		ns = ns->next;
	    }
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
	if (cur->next != NULL) {
	    cur = cur->next;
	    continue;
	}
	
	do {
	    cur = cur->parent;
	    if (cur == NULL)
		break;
	    if (cur == (xmlNodePtr) style->doc) {
		cur = NULL;
		break;
	    }
	    if (cur->next != NULL) {
		cur = cur->next;
		break;
	    }
	} while (cur != NULL);
    }
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

static void
xsltParseTemplateContent(xsltStylesheetPtr style, xsltTemplatePtr ret,
	                 xmlNodePtr template) {
    xmlNodePtr cur, delete;
    /*
     * This content comes from the stylesheet
     * For stylesheets, the set of whitespace-preserving
     * element names consists of just xsl:text.
     */
    ret->elem = template;
    cur = template->children;
    delete = NULL;
    while (cur != NULL) {
	if (delete != NULL) {
#ifdef WITH_XSLT_DEBUG_BLANKS
	    xsltGenericDebug(xsltGenericDebugContext,
	     "xsltParseTemplateContent: removing text\n");
#endif
	    xmlUnlinkNode(delete);
	    xmlFreeNode(delete);
	    delete = NULL;
	}
	if (IS_XSLT_ELEM(cur)) {
	    if (IS_XSLT_NAME(cur, "text")) {
		if (cur->children != NULL) {
		    xmlChar *prop;
		    xmlNodePtr text = cur->children, next;
		    int noesc = 0;
			
		    prop = xmlGetNsProp(cur,
			    (const xmlChar *)"disable-output-escaping",
					XSLT_NAMESPACE);
		    if (prop != NULL) {
#ifdef WITH_XSLT_DEBUG_PARSING
			xsltGenericDebug(xsltGenericDebugContext,
			     "Disable escaping: %s\n", text->content);
#endif
			if (xmlStrEqual(prop, (const xmlChar *)"yes")) {
			    noesc = 1;
			} else if (!xmlStrEqual(prop,
						(const xmlChar *)"no")){
			    xsltGenericError(xsltGenericErrorContext,
	     "xslt:text: disable-output-escaping allow only yes or no\n");
			    style->warnings++;

			}
			xmlFree(prop);
		    }

		    while (text != NULL) {
			if (((text->type != XML_TEXT_NODE) &&
			     (text->type != XML_CDATA_SECTION_NODE)) ||
			    (text->next != NULL)) {
			    xsltGenericError(xsltGenericErrorContext,
		 "xsltParseTemplateContent: xslt:text content problem\n");
			    style->errors++;
			    break;
			}
			if (noesc)
			    text->name = xmlStringTextNoenc;
			text = text->next;
		    }

		    /*
		     * replace xsl:text by the list of childs
		     */
		    if (text == NULL) {
			text = cur->children;
			while (text != NULL) {
			    next = text->next;
			    xmlUnlinkNode(text);
			    xmlAddPrevSibling(cur, text);
			    text = next;
			}
		    }
		}
		delete = cur;
		goto skip_children;
	    }
	} else if ((cur->ns != NULL) && (style->nsDefs != NULL)) {
	    if (xsltCheckExtPrefix(style, cur->ns->prefix)) {
		/*
		 * okay this is an extension element compile it too
		 */
		xsltStylePreCompute(style, cur);
	    }
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
#ifdef WITH_XSLT_DEBUG_PARSING
	xsltGenericDebug(xsltGenericDebugContext,
	 "xsltParseStylesheetTemplate: removing text\n");
#endif
	xmlUnlinkNode(delete);
	xmlFreeNode(delete);
	delete = NULL;
    }

    /*
     * Skip the first params
     */
    cur = template->children;
    while (cur != NULL) {
	if ((IS_XSLT_ELEM(cur)) && (!(IS_XSLT_NAME(cur, "param"))))
	    break;
	cur = cur->next;
    }

    /*
     * Browse the remaining of the template
     */
    while (cur != NULL) {
	if ((IS_XSLT_ELEM(cur)) && (IS_XSLT_NAME(cur, "param"))) {
	    xmlNodePtr param = cur;

            cur = cur->next;
	    xsltGenericError(xsltGenericErrorContext,
		"xsltParseStylesheetTop: ignoring misplaced param element\n");
	    style->warnings++;
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
 * xsltParseStylesheetKey:
 * @style:  the XSLT stylesheet
 * @key:  the "key" element
 *
 * parse an XSLT stylesheet key definition and register it
 */

static void
xsltParseStylesheetKey(xsltStylesheetPtr style, xmlNodePtr key) {
    xmlChar *prop = NULL;
    xmlChar *use = NULL;
    xmlChar *match = NULL;
    xmlChar *name = NULL;
    xmlChar *nameURI = NULL;

    if (key == NULL)
	return;

    /*
     * Get arguments
     */
    prop = xmlGetNsProp(key, (const xmlChar *)"name", XSLT_NAMESPACE);
    if (prop != NULL) {
	xmlChar *prefix = NULL;

	name = xmlSplitQName2(prop, &prefix);
	if (name != NULL) {
	    if (prefix != NULL) {
		xmlNsPtr ns;

		ns = xmlSearchNs(key->doc, key, prefix);
		if (ns == NULL) {
		    xsltGenericError(xsltGenericErrorContext,
			"no namespace bound to prefix %s\n", prefix);
		    style->warnings++;
		    xmlFree(prefix);
		    xmlFree(name);
		    name = prop;
		    nameURI = NULL;
		} else {
		    nameURI = xmlStrdup(ns->href);
		    xmlFree(prefix);
		    xmlFree(prop);
		}
	    } else {
		xmlFree(prop);
		nameURI = NULL;
	    }
	} else {
	    name = prop;
	    nameURI = NULL;
	}
#ifdef WITH_XSLT_DEBUG_PARSING
	xsltGenericDebug(xsltGenericDebugContext,
	     "xslt:key: name %s\n", name);
#endif
    } else {
	xsltGenericError(xsltGenericErrorContext,
	    "xsl:key : error missing name\n");
	style->errors++;
	goto error;
    }

    match = xmlGetNsProp(key, (const xmlChar *)"match", XSLT_NAMESPACE);
    if (match == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsl:key : error missing match\n");
	style->errors++;
	goto error;
    }

    use = xmlGetNsProp(key, (const xmlChar *)"use", XSLT_NAMESPACE);
    if (use == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsl:key : error missing use\n");
	style->errors++;
	goto error;
    }

    /*
     * register the key
     */
    xsltAddKey(style, name, nameURI, match, use, key);

error:
    if (use != NULL)
	xmlFree(use);
    if (match != NULL)
	xmlFree(match);
    if (name != NULL)
	xmlFree(name);
    if (nameURI != NULL)
	xmlFree(nameURI);
}

/**
 * xsltParseStylesheetTemplate:
 * @style:  the XSLT stylesheet
 * @template:  the "template" element
 *
 * parse an XSLT stylesheet template building the associated structures
 */

static void
xsltParseStylesheetTemplate(xsltStylesheetPtr style, xmlNodePtr template) {
    xsltTemplatePtr ret;
    xmlChar *prop;
    xmlChar *mode;
    xmlChar *modeURI;
    double  priority;

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
    ret->style = style;

    /*
     * Get arguments
     */
    prop = xmlGetNsProp(template, (const xmlChar *)"mode", XSLT_NAMESPACE);
    if (prop != NULL) {
	xmlChar *prefix = NULL;

	mode = xmlSplitQName2(prop, &prefix);
	if (mode != NULL) {
	    if (prefix != NULL) {
		xmlNsPtr ns;

		ns = xmlSearchNs(template->doc, template, prefix);
		if (ns == NULL) {
		    xsltGenericError(xsltGenericErrorContext,
			"no namespace bound to prefix %s\n", prefix);
		    style->warnings++;
		    xmlFree(prefix);
		    xmlFree(mode);
		    mode = prop;
		    modeURI = NULL;
		} else {
		    modeURI = xmlStrdup(ns->href);
		    xmlFree(prefix);
		    xmlFree(prop);
		}
	    } else {
		xmlFree(prop);
		modeURI = NULL;
	    }
	} else {
	    mode = prop;
	    modeURI = NULL;
	}
#ifdef WITH_XSLT_DEBUG_PARSING
	xsltGenericDebug(xsltGenericDebugContext,
	     "xslt:template: mode %s\n", mode);
#endif
    } else {
	mode = NULL;
	modeURI = NULL;
    }
    prop = xmlGetNsProp(template, (const xmlChar *)"match", XSLT_NAMESPACE);
    if (prop != NULL) {
	if (ret->match != NULL) xmlFree(ret->match);
	ret->match  = prop;
    }

    prop = xmlGetNsProp(template, (const xmlChar *)"priority", XSLT_NAMESPACE);
    if (prop != NULL) {
	priority = xmlXPathStringEvalNumber(prop);
	ret->priority = priority;
	xmlFree(prop);
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
		    style->warnings++;
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
    xsltAddTemplate(style, ret, mode, modeURI);

    if (mode != NULL)
	xmlFree(mode);
    if (modeURI != NULL)
	xmlFree(modeURI);
}

/**
 * xsltParseStylesheetTop:
 * @style:  the XSLT stylesheet
 * @top:  the top level "stylesheet" element
 *
 * scan the top level elements of an XSL stylesheet
 */

static void
xsltParseStylesheetTop(xsltStylesheetPtr style, xmlNodePtr top) {
    xmlNodePtr cur;
    xmlChar *prop;
#ifdef WITH_XSLT_DEBUG_PARSING
    int templates = 0;
#endif

    if (top == NULL)
	return;

    prop = xmlGetNsProp(top, (const xmlChar *)"version", XSLT_NAMESPACE);
    if (prop == NULL) {
	xsltGenericError(xsltGenericErrorContext,
	    "xsl:version is missing: document may not be a stylesheet\n");
	style->warnings++;
    } else {
	if (!xmlStrEqual(prop, (const xmlChar *)"1.0")) {
	    xsltGenericError(xsltGenericErrorContext,
		"xsl:version: only 1.0 features are supported\n");
	     /* TODO set up compatibility when not XSLT 1.0 */
	    style->warnings++;
	}
	xmlFree(prop);
    }

    xsltParseStylesheetExtPrefix(style, top);

    cur = top->children;

    while (cur != NULL) {
	if (IS_BLANK_NODE(cur)) {
            cur = cur->next;
	    continue;
	}
	if (!(IS_XSLT_ELEM(cur))) {
#ifdef WITH_XSLT_DEBUG_PARSING
	    xsltGenericDebug(xsltGenericDebugContext,
		    "xsltParseStylesheetTop : found foreign element %s\n",
		    cur->name);
#endif
            cur = cur->next;
	    continue;
	}
	if (IS_XSLT_NAME(cur, "import")) {
	    xsltParseStylesheetImport(style, cur);
	} else
	    break;
	cur = cur->next;
    }
    while (cur != NULL) {
	if (!(IS_XSLT_ELEM(cur))) {
#ifdef WITH_XSLT_DEBUG_PARSING
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
	    style->errors++;
        } else if (IS_XSLT_NAME(cur, "include")) {
	    xsltParseStylesheetInclude(style, cur);
        } else if (IS_XSLT_NAME(cur, "strip-space")) {
	    xsltParseStylesheetStripSpace(style, cur);
        } else if (IS_XSLT_NAME(cur, "preserve-space")) {
	    xsltParseStylesheetPreserveSpace(style, cur);
        } else if (IS_XSLT_NAME(cur, "output")) {
	    xsltParseStylesheetOutput(style, cur);
        } else if (IS_XSLT_NAME(cur, "key")) {
	    xsltParseStylesheetKey(style, cur);
        } else if (IS_XSLT_NAME(cur, "decimal-format")) {
	    xsltParseStylesheetDecimalFormat(style, cur);
        } else if (IS_XSLT_NAME(cur, "attribute-set")) {
	    xsltParseStylesheetAttributeSet(style, cur);
        } else if (IS_XSLT_NAME(cur, "variable")) {
	    xsltParseGlobalVariable(style, cur);
        } else if (IS_XSLT_NAME(cur, "param")) {
	    xsltParseGlobalParam(style, cur);
        } else if (IS_XSLT_NAME(cur, "template")) {
#ifdef WITH_XSLT_DEBUG_PARSING
	    templates++;
#endif
	    xsltParseStylesheetTemplate(style, cur);
        } else if (IS_XSLT_NAME(cur, "namespace-alias")) {
	    xsltNamespaceAlias(style, cur);
	} else {
	    xsltGenericError(xsltGenericErrorContext,
		"xsltParseStylesheetTop: ignoring unknown %s element\n",
		             cur->name);
	    style->warnings++;
	}
	cur = cur->next;
    }
#ifdef WITH_XSLT_DEBUG_PARSING
    xsltGenericDebug(xsltGenericDebugContext,
		    "parsed %d templates\n", templates);
#endif
}

/**
 * xsltParseStylesheetProcess:
 * @ret:  the XSLT stylesheet
 * @doc:  and xmlDoc parsed XML
 *
 * parse an XSLT stylesheet adding the associated structures
 *
 * Returns a new XSLT stylesheet structure.
 */

xsltStylesheetPtr
xsltParseStylesheetProcess(xsltStylesheetPtr ret, xmlDocPtr doc) {
    xmlNodePtr cur;

    if (doc == NULL)
	return(NULL);
    if (ret == NULL)
	return(ret);
    
    /*
     * First steps, remove blank nodes,
     * locate the xsl:stylesheet element and the
     * namespace declaration.
     */
    cur = xmlDocGetRootElement(doc);
    if (cur == NULL) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltParseStylesheetProcess : empty stylesheet\n");
	ret->doc = NULL;
	xsltFreeStylesheet(ret);
	return(NULL);
    }

    xsltPrecomputeStylesheet(ret);
    if ((IS_XSLT_ELEM(cur)) && 
	((IS_XSLT_NAME(cur, "stylesheet")) ||
	 (IS_XSLT_NAME(cur, "transform")))) {
#ifdef WITH_XSLT_DEBUG_PARSING
	xsltGenericDebug(xsltGenericDebugContext,
		"xsltParseStylesheetProcess : found stylesheet\n");
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
		"xsltParseStylesheetProcess : document is not a stylesheet\n");
	    ret->doc = NULL;
	    xsltFreeStylesheet(ret);
	    return(NULL);
	}

#ifdef WITH_XSLT_DEBUG_PARSING
        xsltGenericDebug(xsltGenericDebugContext,
		"xsltParseStylesheetProcess : document is stylesheet\n");
#endif
	
	if (!xmlStrEqual(prop, (const xmlChar *)"1.0")) {
	    xsltGenericError(xsltGenericErrorContext,
		"xsl:version: only 1.0 features are supported\n");
	     /* TODO set up compatibility when not XSLT 1.0 */
	    ret->warnings++;
	}
	xmlFree(prop);

	/*
	 * Create and link the template
	 */
	template = xsltNewTemplate();
	if (template == NULL) {
	    ret->doc = NULL;
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
	xsltAddTemplate(ret, template, NULL, NULL);
    }

    return(ret);
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

    if (doc == NULL)
	return(NULL);

    ret = xsltNewStylesheet();
    if (ret == NULL)
	return(NULL);
    
    ret->doc = doc;
    xsltGatherNamespaces(ret);
    ret = xsltParseStylesheetProcess(ret, doc);

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

#ifdef WITH_XSLT_DEBUG_PARSING
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

