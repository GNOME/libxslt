/*
 * extensions.c: Implemetation of the extensions support
 *
 * Reference:
 *   http://www.w3.org/TR/1999/REC-xslt-19991116
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#include "libxslt.h"

#include <string.h>

#include <libxml/xmlmemory.h>
#include <libxml/tree.h>
#include <libxml/hash.h>
#include <libxml/xmlerror.h>
#include <libxml/parserInternals.h>
#include <libxml/xpathInternals.h>
#include "xslt.h"
#include "xsltInternals.h"
#include "xsltutils.h"
#include "imports.h"
#include "extensions.h"

#ifdef WITH_XSLT_DEBUG
#define WITH_XSLT_DEBUG_EXTENSIONS
#endif

/************************************************************************
 * 									*
 * 			Private Types and Globals			*
 * 									*
 ************************************************************************/

typedef struct _xsltExtDef xsltExtDef;
typedef xsltExtDef *xsltExtDefPtr;
struct _xsltExtDef {
    struct _xsltExtDef *next;
    xmlChar *prefix;
    xmlChar *URI;
    void    *data;
};

typedef struct _xsltExtModule xsltExtModule;
typedef xsltExtModule *xsltExtModulePtr;
struct _xsltExtModule {
    xsltExtInitFunction initFunc;
    xsltExtShutdownFunction shutdownFunc;
};

typedef struct _xsltExtData xsltExtData;
typedef xsltExtData *xsltExtDataPtr;
struct _xsltExtData {
    xsltExtModulePtr extModule;
    void *extData;
};

static xmlHashTablePtr xsltExtensionsHash = NULL;

/************************************************************************
 * 									*
 * 			Type functions 					*
 * 									*
 ************************************************************************/

/**
 * xsltNewExtDef:
 * @prefix:  the extension prefix
 * @URI:  the namespace URI
 *
 * Create a new XSLT ExtDef
 *
 * Returns the newly allocated xsltExtDefPtr or NULL in case of error
 */
static xsltExtDefPtr
xsltNewExtDef(const xmlChar * prefix, const xmlChar * URI)
{
    xsltExtDefPtr cur;

    cur = (xsltExtDefPtr) xmlMalloc(sizeof(xsltExtDef));
    if (cur == NULL) {
        xsltGenericError(xsltGenericErrorContext,
                         "xsltNewExtDef : malloc failed\n");
        return (NULL);
    }
    memset(cur, 0, sizeof(xsltExtDef));
    if (prefix != NULL)
        cur->prefix = xmlStrdup(prefix);
    if (URI != NULL)
        cur->URI = xmlStrdup(URI);
    return (cur);
}

/**
 * xsltFreeExtDef:
 * @extensiond:  an XSLT extension definition
 *
 * Free up the memory allocated by @extensiond
 */
static void
xsltFreeExtDef(xsltExtDefPtr extensiond) {
    if (extensiond == NULL)
	return;
    if (extensiond->prefix != NULL)
	xmlFree(extensiond->prefix);
    if (extensiond->URI != NULL)
	xmlFree(extensiond->URI);
    xmlFree(extensiond);
}

/**
 * xsltFreeExtDefList:
 * @extensiond:  an XSLT extension definition list
 *
 * Free up the memory allocated by all the elements of @extensiond
 */
static void
xsltFreeExtDefList(xsltExtDefPtr extensiond) {
    xsltExtDefPtr cur;

    while (extensiond != NULL) {
	cur = extensiond;
	extensiond = extensiond->next;
	xsltFreeExtDef(cur);
    }
}

/**
 * xsltNewExtModule:
 * @initFunc:  the module initialization function
 * @shutdownFunc:  the module shutdown function
 *
 * Create a new XSLT extension module
 *
 * Returns the newly allocated xsltExtModulePtr or NULL in case of error
 */
static xsltExtModulePtr
xsltNewExtModule(xsltExtInitFunction initFunc,
                 xsltExtShutdownFunction shutdownFunc)
{
    xsltExtModulePtr cur;

    cur = (xsltExtModulePtr) xmlMalloc(sizeof(xsltExtModule));
    if (cur == NULL) {
        xsltGenericError(xsltGenericErrorContext,
                         "xsltNewExtModule : malloc failed\n");
        return (NULL);
    }
    cur->initFunc = initFunc;
    cur->shutdownFunc = shutdownFunc;
    return (cur);
}

/**
 * xsltFreeExtModule:
 * @ext:  an XSLT extension module
 *
 * Free up the memory allocated by @ext
 */
static void
xsltFreeExtModule(xsltExtModulePtr ext) {
    if (ext == NULL)
	return;
    xmlFree(ext);
}

/**
 * xsltNewExtData:
 * @extModule:  the module
 * @extData:  the associated data
 *
 * Create a new XSLT extension module data wrapper
 *
 * Returns the newly allocated xsltExtDataPtr or NULL in case of error
 */
static xsltExtDataPtr
xsltNewExtData(xsltExtModulePtr extModule, void *extData)
{
    xsltExtDataPtr cur;

    if (extModule == NULL)
	return(NULL);
    cur = (xsltExtDataPtr) xmlMalloc(sizeof(xsltExtData));
    if (cur == NULL) {
        xsltGenericError(xsltGenericErrorContext,
                         "xsltNewExtData : malloc failed\n");
        return (NULL);
    }
    cur->extModule = extModule;
    cur->extData = extData;
    return (cur);
}

/**
 * xsltFreeExtData:
 * @ext:  an XSLT extension module data wrapper
 *
 * Free up the memory allocated by @ext
 */
static void
xsltFreeExtData(xsltExtDataPtr ext) {
    if (ext == NULL)
	return;
    xmlFree(ext);
}


/************************************************************************
 * 									*
 * 		The stylesheet extension prefixes handling		*
 * 									*
 ************************************************************************/


/**
 * xsltFreeExts:
 * @style: an XSLT stylesheet
 *
 * Free up the memory used by XSLT extensions in a stylesheet
 */
void
xsltFreeExts(xsltStylesheetPtr style) {
    if (style->nsDefs != NULL)
	xsltFreeExtDefList((xsltExtDefPtr) style->nsDefs);
}

/**
 * xsltRegisterExtPrefix:
 * @style: an XSLT stylesheet
 * @prefix: the prefix used
 * @URI: the URI associated to the extension
 *
 * Registers an extension namespace
 *
 * Returns 0 in case of success, -1 in case of failure
 */
int
xsltRegisterExtPrefix(xsltStylesheetPtr style,
		      const xmlChar *prefix, const xmlChar *URI) {
    xsltExtDefPtr def, ret;

    if ((style == NULL) || (prefix == NULL) | (URI == NULL))
	return(-1);

    def = (xsltExtDefPtr) style->nsDefs;
    while (def != NULL) {
	if (xmlStrEqual(prefix, def->prefix))
	    return(-1);
	def = def->next;
    }
    ret = xsltNewExtDef(prefix, URI);
    if (ret == NULL)
	return(-1);
    ret->next = (xsltExtDefPtr) style->nsDefs;
    style->nsDefs = ret;
    return(0);
}

/************************************************************************
 * 									*
 * 		The extensions modules interfaces			*
 * 									*
 ************************************************************************/

/**
 * xsltRegisterExtFunction:
 * @ctxt: an XSLT transformation context
 * @name: the name of the element
 * @URI: the URI associated to the element
 * @function: the actual implementation which should be called 
 *
 * Registers an extension function
 *
 * Returns 0 in case of success, -1 in case of failure
 */
int
xsltRegisterExtFunction(xsltTransformContextPtr ctxt, const xmlChar *name,
	                const xmlChar *URI, xmlXPathEvalFunc function) {
    if ((ctxt == NULL) || (name == NULL) ||
	(URI == NULL) || (function == NULL))
	return(-1);
    if (ctxt->xpathCtxt != NULL) {
	xmlXPathRegisterFuncNS(ctxt->xpathCtxt, name, URI, function);
    }
    if (ctxt->extFunctions == NULL)
	ctxt->extFunctions = xmlHashCreate(10);
    return(xmlHashAddEntry2(ctxt->extFunctions, name, URI, (void *) function));
}

/**
 * xsltRegisterExtElement:
 * @ctxt: an XSLT transformation context
 * @name: the name of the element
 * @URI: the URI associated to the element
 * @function: the actual implementation which should be called 
 *
 * Registers an extension element
 *
 * Returns 0 in case of success, -1 in case of failure
 */
int	
xsltRegisterExtElement(xsltTransformContextPtr ctxt, const xmlChar *name,
		       const xmlChar *URI, xsltTransformFunction function) {
    if ((ctxt == NULL) || (name == NULL) ||
	(URI == NULL) || (function == NULL))
	return(-1);
    if (ctxt->extElements == NULL)
	ctxt->extElements = xmlHashCreate(10);
    return(xmlHashAddEntry2(ctxt->extElements, name, URI, (void *) function));
}

/**
 * xsltFreeCtxtExts:
 * @ctxt: an XSLT transformation context
 *
 * Free the XSLT extension data
 */
void
xsltFreeCtxtExts(xsltTransformContextPtr ctxt) {
    if (ctxt->extElements != NULL)
	xmlHashFree(ctxt->extElements, NULL);
    if (ctxt->extFunctions != NULL)
	xmlHashFree(ctxt->extFunctions, NULL);
}

/**
 * xsltGetExtData:
 * @ctxt: an XSLT transformation context
 * @URI:  the URI associated to the exension module
 *
 * Retrieve the data associated to the extension module in this given
 * transformation.
 *
 * Returns the pointer or NULL if not present
 */
void *
xsltGetExtData(xsltTransformContextPtr ctxt, const xmlChar * URI)
{
    xsltExtDataPtr data;

    if ((ctxt == NULL) || (ctxt->extInfos == NULL) || (URI == NULL))
        return (NULL);
    data = (xsltExtDataPtr) xmlHashLookup(ctxt->extInfos, URI);
    if (data == NULL)
        return (NULL);
    return (data->extData);
}

/**
 * xsltInitCtxtExts:
 * @ctxt: an XSLT transformation context
 *
 * Initialize the set of modules associated to the extension prefixes
 *
 * Returns the number of modules initialized or -1 in case of error
 */
int
xsltInitCtxtExts(xsltTransformContextPtr ctxt)
{
    int ret = 0;
    xsltStylesheetPtr style;
    xsltExtDefPtr def;
    xsltExtModulePtr module;
    xsltExtDataPtr data;
    void *extData;

    if (ctxt == NULL)
        return (-1);

    style = ctxt->style;
    if (style == NULL)
        return (-1);
    while (style != NULL) {
        def = (xsltExtDefPtr) style->nsDefs;
        while (def != NULL) {
            if (def->URI != NULL) {
                if (ctxt->extInfos == NULL) {
                    ctxt->extInfos = xmlHashCreate(10);
                    if (ctxt->extInfos == NULL)
                        return (-1);
                    data = NULL;
                } else {
                    data =
                        (xsltExtDataPtr) xmlHashLookup(ctxt->extInfos,
                                                       def->URI);
                }
                if (data == NULL) {
                    /*
                     * Register this module
                     */
                    module = xmlHashLookup(xsltExtensionsHash, def->URI);
                    if (module == NULL) {
#ifdef WITH_XSLT_DEBUG_EXTENSIONS
                        xsltGenericDebug(xsltGenericDebugContext,
				     "Not registered extension module : %s\n",
                                         def->URI);
#endif
                    } else {
                        if (module->initFunc != NULL) {
#ifdef WITH_XSLT_DEBUG_EXTENSIONS
                            xsltGenericDebug(xsltGenericDebugContext,
                                             "Initializing module : %s\n",
                                             def->URI);
#endif
                            extData = module->initFunc(ctxt, def->URI);
                        } else {
                            extData = NULL;
                        }
                        data = xsltNewExtData(module, extData);
                        if (data == NULL)
                            return (-1);
                        if (xmlHashAddEntry(ctxt->extInfos, def->URI,
                                            (void *) data) < 0) {
                            xsltGenericError(xsltGenericErrorContext,
                                             "Failed to register module : %s\n",
                                             def->URI);
                        } else {
			    ret++;
#ifdef WITH_XSLT_DEBUG_EXTENSIONS
                        xsltGenericDebug(xsltGenericDebugContext,
                                         "Registered module : %s\n",
                                         def->URI);
#endif
			}

                    }
                }
            }
            def = def->next;
        }
        style = xsltNextImport(style);
    }
#ifdef WITH_XSLT_DEBUG_EXTENSIONS
    xsltGenericDebug(xsltGenericDebugContext, "Registered %d modules\n", ret);
#endif
    return (ret);
}

/**
 * xsltShutdownCtxtExt:
 * @data:  the registered data for the module
 * @ctxt:  the XSLT transformation context
 * @URI:  the extension URI
 *
 * Shutdown an extension module loaded
 */
static void
xsltShutdownCtxtExt(xsltExtDataPtr data, xsltTransformContextPtr ctxt,
                    const xmlChar * URI)
{
    xsltExtModulePtr module;

    if ((data == NULL) || (ctxt == NULL) || (URI == NULL))
        return;
    module = data->extModule;
    if ((module == NULL) || (module->shutdownFunc == NULL))
        return;

#ifdef WITH_XSLT_DEBUG_EXTENSIONS
    xsltGenericDebug(xsltGenericDebugContext,
                     "Shutting down module : %s\n", URI);
#endif
    module->shutdownFunc(ctxt, URI, data->extData);
    xmlHashRemoveEntry(ctxt->extInfos, URI,
                       (xmlHashDeallocator) xsltFreeExtData);
}

/**
 * xsltShutdownCtxtExts:
 * @ctxt: an XSLT transformation context
 *
 * Shutdown the set of modules loaded
 */
void
xsltShutdownCtxtExts(xsltTransformContextPtr ctxt)
{
    if (ctxt == NULL)
	return;
    if (ctxt->extInfos == NULL)
	return;
    xmlHashScan(ctxt->extInfos, (xmlHashScanner) xsltShutdownCtxtExt, ctxt);
    xmlHashFree(ctxt->extInfos, (xmlHashDeallocator) xsltFreeExtData);
    ctxt->extInfos = NULL;
}

/**
 * xsltCheckExtPrefix:
 * @style: the stylesheet
 * @prefix: the namespace prefix (possibly NULL)
 *
 * Check if the given prefix is one of the declared extensions
 *
 * Returns 1 if this is an extension, 0 otherwise
 */
int
xsltCheckExtPrefix(xsltStylesheetPtr style, const xmlChar *prefix) {
    xsltExtDefPtr cur;

    if ((style == NULL) || (style->nsDefs == NULL))
	return(0);

    cur = (xsltExtDefPtr) style->nsDefs;
    while (cur != NULL) {
	if (xmlStrEqual(prefix, cur->prefix))
	    return(1);
	cur = cur->next;
    }
    return(0);
}

/**
 * xsltRegisterExtModule:
 * @URI:  URI associated to this module
 * @initFunc:  the module initialization function
 * @shutdownFunc:  the module shutdown function
 *
 * Register an XSLT extension module to the library.
 *
 * Returns 0 if sucessful, -1 in case of error
 */
int
xsltRegisterExtModule(const xmlChar * URI,
                      xsltExtInitFunction initFunc,
                      xsltExtShutdownFunction shutdownFunc)
{
    int ret;
    xsltExtModulePtr module;

    if ((URI == NULL) || (initFunc == NULL))
        return (-1);
    if (xsltExtensionsHash == NULL)
        xsltExtensionsHash = xmlHashCreate(10);

    if (xsltExtensionsHash == NULL)
        return (-1);

    module = xmlHashLookup(xsltExtensionsHash, URI);
    if (module != NULL) {
        if ((module->initFunc == initFunc) &&
            (module->shutdownFunc == shutdownFunc))
            return (0);
        return (-1);
    }
    module = xsltNewExtModule(initFunc, shutdownFunc);
    if (module == NULL)
        return (-1);
    ret = xmlHashAddEntry(xsltExtensionsHash, URI, (void *) module);
    return (ret);
}

/**
 * xsltUnregisterExtModule:
 * @URI:  URI associated to this module
 *
 * Unregister an XSLT extension module from the library.
 *
 * Returns 0 if sucessful, -1 in case of error
 */
int
xsltUnregisterExtModule(const xmlChar * URI)
{
    int ret;

    if (URI == NULL)
        return (-1);
    if (xsltExtensionsHash == NULL)
        return (-1);

    ret =
        xmlHashRemoveEntry(xsltExtensionsHash, URI,
                           (xmlHashDeallocator) xsltFreeExtModule);
    return (ret);
}

/**
 * xsltUnregisterExtModule:
 *
 * Unregister all the XSLT extension module from the library.
 */
void
xsltUnregisterAllExtModules(void)
{
    if (xsltExtensionsHash == NULL)
	return;

    xmlHashFree(xsltExtensionsHash, (xmlHashDeallocator) xsltFreeExtModule);
    xsltExtensionsHash = NULL;
}

/**
 * xsltXPathGetTransformContext:
 * @ctxt:  an XPath transformation context
 *
 * Returns the XSLT transformation context from the XPath transformation
 * context. This is useful when an XPath function in the extension module
 * is called by the XPath interpreter and that the XSLT context is needed
 * for example to retrieve the associated data pertaining to this XSLT
 * transformation.
 *
 * Returns the XSLT transformation context or NULL in case of error.
 */
xsltTransformContextPtr
xsltXPathGetTransformContext(xmlXPathParserContextPtr ctxt)
{
    if ((ctxt == NULL) || (ctxt->context == NULL))
	return(NULL);
    return(ctxt->context->extra);
}


