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

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>

/************************************************************************
 *									*
 *		Routines to handle XSLT data structures			*
 *									*
 ************************************************************************/

/**
 * xsltFreeStylesheet:
 * @sheet:  an XSLT stylesheet
 *
 * Free up the memory allocated by @sheet
 */
void
xsltFreeStylesheet(xsltStylesheetPtr sheet) {
}

/************************************************************************
 *									*
 *		Parsing of an XSLT Stylesheet				*
 *									*
 ************************************************************************/

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

    return(ret);
}

