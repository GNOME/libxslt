/*
 * xsltutils.c: Utilities for the XSL Transformation 1.0 engine
 *
 * Reference:
 *   http://www.w3.org/TR/1999/REC-xslt-19991116
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
 */

#include "xsltconfig.h"

#include <stdio.h>
#include <stdarg.h>

#include <libxml/xmlversion.h>
#include <libxml/xmlmemory.h>
#include <libxml/tree.h>
#include <libxml/HTMLtree.h>
#include <libxml/xmlerror.h>
#include <libxml/xmlIO.h>
#include "xsltutils.h"
#include "templates.h"
#include "xsltInternals.h"
#include "imports.h"

/************************************************************************
 * 									*
 * 		Handling of XSLT stylesheets messages			*
 * 									*
 ************************************************************************/

/**
 * xsltMessage:
 * @ctxt:  an XSLT processing context
 * @node:  The current node
 * @inst:  The node containing the message instruction
 *
 * Process and xsl:message construct
 */
void
xsltMessage(xsltTransformContextPtr ctxt, xmlNodePtr node, xmlNodePtr inst) {
    xmlChar *prop, *message;
    int terminate = 0;

    if ((ctxt == NULL) || (inst == NULL))
	return;

    prop = xmlGetNsProp(inst, (const xmlChar *)"terminate", XSLT_NAMESPACE);
    if (prop != NULL) {
	if (xmlStrEqual(prop, (const xmlChar *)"yes")) {
	    terminate = 1;
	} else if (xmlStrEqual(prop, (const xmlChar *)"yes")) {
	    terminate = 0;
	} else {
	    xsltGenericError(xsltGenericErrorContext,
		"xsl:message : terminate expecting 'yes' or 'no'\n");
	}
	xmlFree(prop);
    }
    message = xsltEvalTemplateString(ctxt, node, inst);
    if (message != NULL) {
	int len = xmlStrlen(message);

	xsltGenericError(xsltGenericErrorContext, (const char *)message);
	if ((len > 0) && (message[len - 1] != '\n'))
	    xsltGenericError(xsltGenericErrorContext, "\n");
	xmlFree(message);
    }
    if (terminate)
	ctxt->state = XSLT_STATE_STOPPED;
}

/************************************************************************
 * 									*
 * 		Handling of out of context errors			*
 * 									*
 ************************************************************************/

/**
 * xsltGenericErrorDefaultFunc:
 * @ctx:  an error context
 * @msg:  the message to display/transmit
 * @...:  extra parameters for the message display
 * 
 * Default handler for out of context error messages.
 */
static void
xsltGenericErrorDefaultFunc(void *ctx ATTRIBUTE_UNUSED, const char *msg, ...) {
    va_list args;

    if (xsltGenericErrorContext == NULL)
	xsltGenericErrorContext = (void *) stderr;

    va_start(args, msg);
    vfprintf((FILE *)xsltGenericErrorContext, msg, args);
    va_end(args);
}

xmlGenericErrorFunc xsltGenericError = xsltGenericErrorDefaultFunc;
void *xsltGenericErrorContext = NULL;


/**
 * xsltSetGenericErrorFunc:
 * @ctx:  the new error handling context
 * @handler:  the new handler function
 *
 * Function to reset the handler and the error context for out of
 * context error messages.
 * This simply means that @handler will be called for subsequent
 * error messages while not parsing nor validating. And @ctx will
 * be passed as first argument to @handler
 * One can simply force messages to be emitted to another FILE * than
 * stderr by setting @ctx to this file handle and @handler to NULL.
 */
void
xsltSetGenericErrorFunc(void *ctx, xmlGenericErrorFunc handler) {
    xsltGenericErrorContext = ctx;
    if (handler != NULL)
	xsltGenericError = handler;
    else
	xsltGenericError = xsltGenericErrorDefaultFunc;
}

/**
 * xsltGenericDebugDefaultFunc:
 * @ctx:  an error context
 * @msg:  the message to display/transmit
 * @...:  extra parameters for the message display
 * 
 * Default handler for out of context error messages.
 */
static void
xsltGenericDebugDefaultFunc(void *ctx ATTRIBUTE_UNUSED, const char *msg, ...) {
    va_list args;

    if (xsltGenericDebugContext == NULL)
	return;

    va_start(args, msg);
    vfprintf((FILE *)xsltGenericDebugContext, msg, args);
    va_end(args);
}

xmlGenericErrorFunc xsltGenericDebug = xsltGenericDebugDefaultFunc;
void *xsltGenericDebugContext = NULL;


/**
 * xsltSetGenericDebugFunc:
 * @ctx:  the new error handling context
 * @handler:  the new handler function
 *
 * Function to reset the handler and the error context for out of
 * context error messages.
 * This simply means that @handler will be called for subsequent
 * error messages while not parsing nor validating. And @ctx will
 * be passed as first argument to @handler
 * One can simply force messages to be emitted to another FILE * than
 * stderr by setting @ctx to this file handle and @handler to NULL.
 */
void
xsltSetGenericDebugFunc(void *ctx, xmlGenericErrorFunc handler) {
    xsltGenericDebugContext = ctx;
    if (handler != NULL)
	xsltGenericDebug = handler;
    else
	xsltGenericDebug = xsltGenericDebugDefaultFunc;
}

/************************************************************************
 * 									*
 * 				Sorting					*
 * 									*
 ************************************************************************/

/**
 * xsltDocumentSortFunction:
 * @list:  the node set
 *
 * reorder the current node list @list accordingly to the document order
 */
void
xsltDocumentSortFunction(xmlNodeSetPtr list) {
    int i, j;
    int len, tst;
    xmlNodePtr node;

    if (list == NULL)
	return;
    len = list->nodeNr;
    if (len <= 1)
	return;
    /* TODO: sort is really not optimized, does it needs to ? */
    for (i = 0;i < len -1;i++) {
	for (j = i + 1; j < len; j++) {
	    tst = xmlXPathCmpNodes(list->nodeTab[i], list->nodeTab[j]);
	    if (tst == -1) {
		node = list->nodeTab[i];
		list->nodeTab[i] = list->nodeTab[j];
		list->nodeTab[j] = node;
	    }
	}
    }
}

/**
 * xsltSortFunction:
 * @list:  the node set
 * @results:  the results
 * @descending:  direction of order
 * @number:  the type of the result
 *
 * reorder the current node list @list accordingly to the values
 * present in the array of results @results
 */
void	
xsltSortFunction(xmlNodeSetPtr list, xmlXPathObjectPtr *results,
		 int descending, int number) {
    int i, j, incr;
    int len, tst;
    xmlNodePtr node;
    xmlXPathObjectPtr tmp;

    if ((list == NULL) || (results == NULL))
	return;
    len = list->nodeNr;
    if (len <= 1)
	return;

    /* Shell's sort of node-set */
    for (incr = len / 2; incr > 0; incr /= 2) {
	for (i = incr; i < len; i++) {
	    j = i - incr;
	    if (results[i] == NULL)
		continue;
	    
	    while (j >= 0) {
		if (results[j] == NULL)
		    tst = 1;
		else if (number) {
		    tst = (results[j]->floatval > results[j + incr]->floatval);
		    if (descending)
			tst = !tst;
		} else {
		    tst = (xmlStrcmp(results[j]->stringval,
				     results[j + incr]->stringval)) > 0; 
		    if (descending)
			tst = !tst;
		}
		if (tst) {
		    tmp = results[j];
		    results[j] = results[j + incr];
		    results[j + incr] = tmp;
		    node = list->nodeTab[j];
		    list->nodeTab[j] = list->nodeTab[j + incr];
		    list->nodeTab[j + incr] = node;
		    j -= incr;
		} else
		    break;
	    }
	}
    }
}

/************************************************************************
 * 									*
 * 				Output					*
 * 									*
 ************************************************************************/

/**
 * xsltSaveResultTo:
 * @buf:  an output buffer
 * @result:  the result xmlDocPtr
 * @style:  the stylesheet
 *
 * Save the result @result obtained by applying the @style stylesheet
 * to an I/O output channel @buf
 *
 * Returns the number of byte written or -1 in case of failure.
 */
int
xsltSaveResultTo(xmlOutputBufferPtr buf, xmlDocPtr result,
	       xsltStylesheetPtr style) {
    const xmlChar *encoding;
    xmlNodePtr root;
    int base;
    const xmlChar *method;

    if ((buf == NULL) || (result == NULL) || (style == NULL))
	return(-1);

    if ((style->methodURI != NULL) &&
	((style->method == NULL) ||
	 (!xmlStrEqual(style->method, (const xmlChar *) "xhtml")))) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltSaveResultTo : unknown ouput method\n");
        return(-1);
    }

    /* TODO: when outputing and having imported stylesheets, apply cascade */
    base = buf->written;

    XSLT_GET_IMPORT_PTR(method, style, method)
    XSLT_GET_IMPORT_PTR(encoding, style, encoding)

    if (method == NULL)
	root = xmlDocGetRootElement(result);
    else
	root = NULL;

    if ((method != NULL) &&
	(xmlStrEqual(method, (const xmlChar *) "html"))) {
	if (encoding != NULL) {
	    htmlSetMetaEncoding(result, (const xmlChar *) encoding);
	} else {
	    htmlSetMetaEncoding(result, (const xmlChar *) "UTF-8");
	}
	htmlDocContentDumpOutput(buf, result, (const char *) encoding);
    } else if ((method != NULL) &&
	(xmlStrEqual(method, (const xmlChar *) "xhtml"))) {
	if (encoding != NULL) {
	    htmlSetMetaEncoding(result, (const xmlChar *) encoding);
	} else {
	    htmlSetMetaEncoding(result, (const xmlChar *) "UTF-8");
	}
	htmlDocContentDumpOutput(buf, result, (const char *) encoding);
    } else if ((method != NULL) &&
	       (xmlStrEqual(method, (const xmlChar *) "text"))) {
	xmlNodePtr cur;

	cur = result->children;
	while (cur != NULL) {
	    if (cur->type == XML_TEXT_NODE)
		xmlOutputBufferWriteString(buf, (const char *) cur->content);
	    cur = cur->next;
	}
    } else {
	int omitXmlDecl;
	int standalone;
	int indent;
	const xmlChar *version;
	const xmlChar *doctypePublic;
	const xmlChar *doctypeSystem;

	XSLT_GET_IMPORT_INT(omitXmlDecl, style, omitXmlDeclaration);
	XSLT_GET_IMPORT_INT(standalone, style, standalone);
	XSLT_GET_IMPORT_INT(indent, style, indent);
	XSLT_GET_IMPORT_PTR(version, style, version)
	XSLT_GET_IMPORT_PTR(doctypePublic, style, doctypePublic)
	XSLT_GET_IMPORT_PTR(doctypeSystem, style, doctypeSystem)

	if (omitXmlDecl != 1) {
	    xmlOutputBufferWriteString(buf, "<?xml version=");
	    if (result->version != NULL) 
		xmlBufferWriteQuotedString(buf->buffer, result->version);
	    else
		xmlOutputBufferWriteString(buf, "\"1.0\"");
	    if (encoding == NULL) {
		if (result->encoding != NULL)
		    encoding = result->encoding;
		else if (result->charset != XML_CHAR_ENCODING_UTF8)
		    encoding = (const xmlChar *)
			       xmlGetCharEncodingName((xmlCharEncoding)
			                              result->charset);
	    }
	    if (encoding != NULL) {
		xmlOutputBufferWriteString(buf, " encoding=");
		xmlBufferWriteQuotedString(buf->buffer, (xmlChar *) encoding);
	    }
	    switch (standalone) {
		case 0:
		    xmlOutputBufferWriteString(buf, " standalone=\"no\"");
		    break;
		case 1:
		    xmlOutputBufferWriteString(buf, " standalone=\"yes\"");
		    break;
		default:
		    break;
	    }
	    xmlOutputBufferWriteString(buf, "?>\n");
	}
	if ((doctypePublic != NULL) || (doctypeSystem != NULL)) {
	    xmlNodePtr cur = result->children;

	    while (cur != NULL) {
		if (cur->type == XML_ELEMENT_NODE)
		    break;
		cur = cur->next;
	    }
	    if ((cur != NULL) && (cur->name != NULL)) {
		xmlOutputBufferWriteString(buf, "<!DOCTYPE ");
		xmlOutputBufferWriteString(buf, (const char *) cur->name);
		if (doctypePublic != NULL) {
		    if (doctypeSystem != NULL) {
			xmlOutputBufferWriteString(buf, " PUBLIC ");
			xmlBufferWriteQuotedString(buf->buffer,
				         doctypePublic);
			xmlOutputBufferWriteString(buf, " ");
			xmlBufferWriteQuotedString(buf->buffer,
				         doctypeSystem);
		    } else {
			xmlOutputBufferWriteString(buf, " PUBLIC \"-\" ");
			xmlBufferWriteQuotedString(buf->buffer,
				         doctypeSystem);
		    }

		} else {
		    xmlOutputBufferWriteString(buf, " SYSTEM ");
		    xmlBufferWriteQuotedString(buf->buffer,
				     doctypeSystem);
		}
		xmlOutputBufferWriteString(buf, ">\n");
	    }
	}
	if (result->children != NULL) {
	    xmlNodePtr child = result->children;

	    while (child != NULL) {
		xmlNodeDumpOutput(buf, result, child, 0, (indent == 1),
			          (const char *) encoding);
		xmlOutputBufferWriteString(buf, "\n");
		child = child->next;
	    }
	}
	xmlOutputBufferFlush(buf);
    }
    return(buf->written - base);
}

/**
 * xsltSaveResultToFilename:
 * @URL:  a filename or URL
 * @result:  the result xmlDocPtr
 * @style:  the stylesheet
 * @compression:  the compression factor (0 - 9 included)
 *
 * Save the result @result obtained by applying the @style stylesheet
 * to a file or URL @URL
 *
 * Returns the number of byte written or -1 in case of failure.
 */
int
xsltSaveResultToFilename(const char *URL, xmlDocPtr result,
			 xsltStylesheetPtr style, int compression) {
    xmlOutputBufferPtr buf;
    const xmlChar *encoding;
    int ret;

    if ((URL == NULL) || (result == NULL) || (style == NULL))
	return(-1);

    XSLT_GET_IMPORT_PTR(encoding, style, encoding)
    if (encoding != NULL) {
	xmlCharEncodingHandlerPtr encoder;

	encoder = xmlFindCharEncodingHandler((char *)encoding);
	if ((encoder != NULL) &&
	    (xmlStrEqual((const xmlChar *)encoder->name,
			 (const xmlChar *) "UTF-8")))
	    encoder = NULL;
	buf = xmlOutputBufferCreateFilename(URL, encoder, compression);
    } else {
	buf = xmlOutputBufferCreateFilename(URL, NULL, compression);
    }
    if (buf == NULL)
	return(-1);
    xsltSaveResultTo(buf, result, style);
    ret = xmlOutputBufferClose(buf);
    return(ret);
}

/**
 * xsltSaveResultToFile:
 * @file:  a FILE * I/O
 * @result:  the result xmlDocPtr
 * @style:  the stylesheet
 *
 * Save the result @result obtained by applying the @style stylesheet
 * to an open FILE * I/O.
 * This does not close the FILE @file
 *
 * Returns the number of byte written or -1 in case of failure.
 */
int
xsltSaveResultToFile(FILE *file, xmlDocPtr result, xsltStylesheetPtr style) {
    xmlOutputBufferPtr buf;
    const xmlChar *encoding;
    int ret;

    if ((file == NULL) || (result == NULL) || (style == NULL))
	return(-1);

    XSLT_GET_IMPORT_PTR(encoding, style, encoding)
    if (encoding != NULL) {
	xmlCharEncodingHandlerPtr encoder;

	encoder = xmlFindCharEncodingHandler((char *)encoding);
	if ((encoder != NULL) &&
	    (xmlStrEqual((const xmlChar *)encoder->name,
			 (const xmlChar *) "UTF-8")))
	    encoder = NULL;
	buf = xmlOutputBufferCreateFile(file, encoder);
    } else {
	buf = xmlOutputBufferCreateFile(file, NULL);
    }

    if (buf == NULL)
	return(-1);
    xsltSaveResultTo(buf, result, style);
    ret = xmlOutputBufferClose(buf);
    return(ret);
}

/**
 * xsltSaveResultToFd:
 * @fd:  a file descriptor
 * @result:  the result xmlDocPtr
 * @style:  the stylesheet
 *
 * Save the result @result obtained by applying the @style stylesheet
 * to an open file descriptor
 * This does not close the descriptor.
 *
 * Returns the number of byte written or -1 in case of failure.
 */
int
xsltSaveResultToFd(int fd, xmlDocPtr result, xsltStylesheetPtr style) {
    xmlOutputBufferPtr buf;
    const xmlChar *encoding;
    int ret;

    if ((fd < 0) || (result == NULL) || (style == NULL))
	return(-1);

    XSLT_GET_IMPORT_PTR(encoding, style, encoding)
    if (encoding != NULL) {
	xmlCharEncodingHandlerPtr encoder;

	encoder = xmlFindCharEncodingHandler((char *)encoding);
	if ((encoder != NULL) &&
	    (xmlStrEqual((const xmlChar *)encoder->name,
			 (const xmlChar *) "UTF-8")))
	    encoder = NULL;
	buf = xmlOutputBufferCreateFd(fd, encoder);
    } else {
	buf = xmlOutputBufferCreateFd(fd, NULL);
    }
    if (buf == NULL)
	return(-1);
    xsltSaveResultTo(buf, result, style);
    ret = xmlOutputBufferClose(buf);
    return(ret);
}

