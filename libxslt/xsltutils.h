/*
 * xsltutils.h: interfaces for the utilities module of the XSLT engine
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#ifndef __XML_XSLTUTILS_H__
#define __XML_XSLTUTILS_H__

#if defined(WIN32) && defined(_MSC_VER)
#include <libxslt/xsltwin32config.h>
#else
#include <libxslt/xsltconfig.h>
#endif

#include <libxml/xpath.h>
#include <libxml/xmlerror.h>
#include "xsltInternals.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * TODO:
 *
 * macro to flag unimplemented blocks
 */
#define XSLT_TODO 							\
    xsltGenericError(xsltGenericErrorContext,				\
	    "Unimplemented block at %s:%d\n",				\
            __FILE__, __LINE__);

/**
 * STRANGE:
 *
 * macro to flag that a problem was detected internally
 */
#define XSLT_STRANGE 							\
    xsltGenericError(xsltGenericErrorContext,				\
	    "Internal error at %s:%d\n",				\
            __FILE__, __LINE__);

/**
 * IS_XSLT_ELEM:
 *
 * Checks that the element pertains to XSLt namespace
 */
#define IS_XSLT_ELEM(n)							\
    (((n) != NULL) && ((n)->ns != NULL) &&				\
     (xmlStrEqual((n)->ns->href, XSLT_NAMESPACE)))

/**
 * IS_XSLT_NAME:
 *
 * Checks the value of an element in XSLT namespace
 */
#define IS_XSLT_NAME(n, val)						\
    (xmlStrEqual((n)->name, (const xmlChar *) (val)))

/**
 * IS_XSLT_REAL_NODE:
 *
 * check that a node is a 'real' one: document, element, text or attribute
 */
#ifdef LIBXML_DOCB_ENABLED
#define IS_XSLT_REAL_NODE(n)						\
    (((n) != NULL) &&							\
     (((n)->type == XML_ELEMENT_NODE) ||				\
      ((n)->type == XML_TEXT_NODE) ||					\
      ((n)->type == XML_ATTRIBUTE_NODE) ||				\
      ((n)->type == XML_DOCUMENT_NODE) ||				\
      ((n)->type == XML_HTML_DOCUMENT_NODE) ||				\
      ((n)->type == XML_DOCB_DOCUMENT_NODE)))
#else
#define IS_XSLT_REAL_NODE(n)						\
    (((n) != NULL) &&							\
     (((n)->type == XML_ELEMENT_NODE) ||				\
      ((n)->type == XML_TEXT_NODE) ||					\
      ((n)->type == XML_ATTRIBUTE_NODE) ||				\
      ((n)->type == XML_DOCUMENT_NODE) ||				\
      ((n)->type == XML_HTML_DOCUMENT_NODE)))
#endif

/*
 * Our own version of namespaced atributes lookup
 */
xmlChar *	 xsltGetNsProp			(xmlNodePtr node,
						 const xmlChar *name,
						 const xmlChar *nameSpace);

/*
 * XSLT specific error and debug reporting functions
 */
LIBXSLT_PUBLIC extern xmlGenericErrorFunc xsltGenericError;
LIBXSLT_PUBLIC extern void *xsltGenericErrorContext;
LIBXSLT_PUBLIC extern xmlGenericErrorFunc xsltGenericDebug;
LIBXSLT_PUBLIC extern void *xsltGenericDebugContext;

void		xsltPrintErrorContext		(xsltTransformContextPtr ctxt,
	                                         xsltStylesheetPtr style,
						 xmlNodePtr node);
void		xsltMessage			(xsltTransformContextPtr ctxt,
						 xmlNodePtr node,
						 xmlNodePtr inst);
void		xsltSetGenericErrorFunc		(void *ctx,
						 xmlGenericErrorFunc handler);
void		xsltSetGenericDebugFunc		(void *ctx,
						 xmlGenericErrorFunc handler);

/*
 * Sorting
 */

void		xsltDocumentSortFunction	(xmlNodeSetPtr list);
void		xsltDoSortFunction		(xsltTransformContextPtr ctxt,
						 xmlNodePtr *sorts,
						 int nbsorts);

/*
 * QNames handling
 */

const xmlChar * xsltGetQNameURI			(xmlNodePtr node,
						 xmlChar **name);

/*
 * Output, reuse libxml I/O buffers
 */
int		xsltSaveResultTo		(xmlOutputBufferPtr buf,
						 xmlDocPtr result,
						 xsltStylesheetPtr style);
int		xsltSaveResultToFilename	(const char *URI,
						 xmlDocPtr result,
						 xsltStylesheetPtr style,
						 int compression);
int		xsltSaveResultToFile		(FILE *file,
						 xmlDocPtr result,
						 xsltStylesheetPtr style);
int		xsltSaveResultToFd		(int fd,
						 xmlDocPtr result,
						 xsltStylesheetPtr style);

/*
 * profiling
 */
void		xsltSaveProfiling		(xsltTransformContextPtr ctxt,
						 FILE *output);

long		xsltTimestamp			(void);
void		xsltCalibrateAdjust		(long delta);

#define XSLT_TIMESTAMP_TICS_PER_SEC 100000l

/*
 * Hooks for the debugger
 */

typedef enum {
    XSLT_DEBUG_NONE = 0, /* no debugging allowed */
    XSLT_DEBUG_INIT,
    XSLT_DEBUG_STEP,
    XSLT_DEBUG_STEPOUT,
    XSLT_DEBUG_NEXT,
    XSLT_DEBUG_STOP,
    XSLT_DEBUG_CONT,
    XSLT_DEBUG_RUN,
    XSLT_DEBUG_RUN_RESTART,
    XSLT_DEBUG_QUIT
} xsltDebugStatusCodes;

LIBXSLT_PUBLIC extern int xslDebugStatus;

typedef void (*xsltHandleDebuggerCallback) (xmlNodePtr cur, xmlNodePtr node,
			xsltTemplatePtr templ, xsltTransformContextPtr ctxt);
typedef int (*xsltAddCallCallback) (xsltTemplatePtr templ, xmlNodePtr source);
typedef void (*xsltDropCallCallback) (void);

int		xsltSetDebuggerCallbacks	(int no, void *block);
int		xslAddCall			(xsltTemplatePtr templ,
						 xmlNodePtr source);
void		xslDropCall			(void);

#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLTUTILS_H__ */

