/*
 * libxslt.c: this modules implements the main part of the glue of the
 *           libxslt library and the Python interpreter. It provides the
 *           entry points where an automatically generated stub is either
 *           unpractical or would not match cleanly the Python model.
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */
#include <Python.h>
#include <libxml/xmlmemory.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include "libxslt_wrap.h"
#include "libxslt-py.h"

/* #define DEBUG */
/* #define DEBUG_XPATH */
/* #define DEBUG_ERROR */
/* #define DEBUG_MEMORY */

/************************************************************************
 *									*
 *			Per type specific glue				*
 *									*
 ************************************************************************/

PyObject *
libxslt_xsltStylesheetPtrWrap(xsltStylesheetPtr style) {
    PyObject *ret;

#ifdef DEBUG
    printf("libxslt_xsltStylesheetPtrWrap: style = %p\n", style);
#endif
    if (style == NULL) {
	Py_INCREF(Py_None);
	return(Py_None);
    }
    ret = PyCObject_FromVoidPtrAndDesc((void *) style, "xsltStylesheetPtr", NULL);
    return(ret);
}

PyObject *
libxslt_xsltTransformContextPtrWrap(xsltTransformContextPtr ctxt) {
    PyObject *ret;

#ifdef DEBUG
    printf("libxslt_xsltTransformContextPtrWrap: ctxt = %p\n", ctxt);
#endif
    if (ctxt == NULL) {
	Py_INCREF(Py_None);
	return(Py_None);
    }
    ret = PyCObject_FromVoidPtrAndDesc((void *) ctxt, "xsltTransformContextPtr", NULL);
    return(ret);
}

/************************************************************************
 *									*
 *		Memory debug interface					*
 *									*
 ************************************************************************/

extern void xmlMemFree(void *ptr);
extern void *xmlMemMalloc(size_t size);
extern void *xmlMemRealloc(void *ptr,size_t size);
extern char *xmlMemoryStrdup(const char *str);

static int libxsltMemoryDebugActivated = 0;
static long libxsltMemoryAllocatedBase = 0;

static int libxsltMemoryDebug = 0;
static xmlFreeFunc freeFunc = NULL;
static xmlMallocFunc mallocFunc = NULL;
static xmlReallocFunc reallocFunc = NULL;
static xmlStrdupFunc strdupFunc = NULL;

PyObject *
libxslt_xmlDebugMemory(PyObject *self, PyObject *args) {
    int activate;
    PyObject *py_retval;
    long ret;

    if (!PyArg_ParseTuple(args, "i:xmlDebugMemory", &activate))
        return(NULL);

#ifdef DEBUG_MEMORY
    printf("libxslt_xmlDebugMemory(%d) called\n", activate);
#endif

    if (activate != 0) {
	if (libxsltMemoryDebug == 0) {
	    /*
	     * First initialize the library and grab the old memory handlers
	     * and switch the library to memory debugging
	     */
	    xmlMemGet((xmlFreeFunc *) &freeFunc,
		      (xmlMallocFunc *)&mallocFunc,
		      (xmlReallocFunc *)&reallocFunc,
		      (xmlStrdupFunc *) &strdupFunc);
	    if ((freeFunc == xmlMemFree) && (mallocFunc == xmlMemMalloc) &&
		(reallocFunc == xmlMemRealloc) &&
		(strdupFunc == xmlMemoryStrdup)) {
		libxsltMemoryAllocatedBase = xmlMemUsed();
	    } else {
		ret = (long) xmlMemSetup(xmlMemFree, xmlMemMalloc,
			                 xmlMemRealloc, xmlMemoryStrdup);
		if (ret < 0)
		    goto error;
		libxsltMemoryAllocatedBase = xmlMemUsed();
	    }
	    xmlInitParser();
	    ret = 0;
	} else if (libxsltMemoryDebugActivated == 0) {
	    libxsltMemoryAllocatedBase = xmlMemUsed();
	    ret = 0;
	} else {
	    ret = xmlMemUsed() - libxsltMemoryAllocatedBase;
	}
	libxsltMemoryDebug = 1;
	libxsltMemoryDebugActivated = 1;
    } else {
	if (libxsltMemoryDebugActivated == 1)
	    ret = xmlMemUsed() - libxsltMemoryAllocatedBase;
	else
	    ret = 0;
	libxsltMemoryDebugActivated = 0;
    }
error:
    py_retval = libxml_longWrap(ret);
    return(py_retval);
}

PyObject *
libxslt_xmlDumpMemory(PyObject *self, PyObject *args) {

    if (libxsltMemoryDebug != 0)
	xmlMemoryDump();
    Py_INCREF(Py_None);
    return(Py_None);
}

/************************************************************************
 *									*
 *			Some customized front-ends			*
 *									*
 ************************************************************************/

PyObject *
libxslt_xsltApplyStylesheet(PyObject *self, PyObject *args) {
    PyObject *py_retval;
    xmlDocPtr c_retval;
    xsltStylesheetPtr style;
    PyObject *pyobj_style;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    PyObject *pyobj_params;
    char **params;

    if (!PyArg_ParseTuple(args, "OOO:xsltApplyStylesheet", &pyobj_style, &pyobj_doc, &pyobj_params))
        return(NULL);

    if (pyobj_params != Py_None) {
	printf("libxslt_xsltApplyStylesheet: parameters not yet supported\n");
	Py_INCREF(Py_None);
	return(Py_None);
    } else {
	params = NULL;
    }
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xsltApplyStylesheet(style, doc, params);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

/************************************************************************
 *									*
 *			The registration stuff				*
 *									*
 ************************************************************************/
static PyMethodDef libxsltMethods[] = {
#include "libxslt-export.c"
};

void init_libxslt(void) {
    PyObject *m;
    m = Py_InitModule("_libxslt", libxsltMethods);
    /* libxslt_xmlErrorInitialize(); */
}

