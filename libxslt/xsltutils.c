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

#include <libxml/xmlmemory.h>
#include <libxml/tree.h>
#include <libxml/xmlerror.h>
#include "xsltutils.h"


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
void
xsltGenericErrorDefaultFunc(void *ctx, const char *msg, ...) {
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
void
xsltGenericDebugDefaultFunc(void *ctx, const char *msg, ...) {
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

