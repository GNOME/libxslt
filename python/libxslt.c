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

