/*
 * deprecated.c: remains of the now deprecated libxsltbreakpoint.so.1
 *               shared library
 *
 * See Copyright for the status of this software.
 *
 * (C) 2001 by Keith Isdale
 * k_isdale@tpg.com.au
 * daniel@veillard.com
 */

#include "config.h"
#include "libxslt/xsltutils.h"

int xslDebugStatus = 0;

int xslActiveBreakPoint(void);
int xslSetActiveBreakPoint(int breakPointNumber);
int xslAddBreakPoint(const xmlChar * url, long lineNumber,
                     const xmlChar * templateName, int type);
int xslDeleteBreakPoint(int breakPointNumber);
int xslEnableBreakPoint(int breakPointNumber, int enable);
int xslIsBreakPointEnabled(int breakPointNumber);
int xslBreakPointCount(void);
void *xslGetBreakPoint(int breakPointNumber);
int xslPrintBreakPoint(FILE * file, int breakPointNumber);
int xslIsBreakPoint(const xmlChar * url, long lineNumber);
int xslIsBreakPointNode(xmlNodePtr node);
void *xslAddCallInfo(const xmlChar * templateName, const xmlChar * url);
int xslAddCall(xsltTemplatePtr templ, xmlNodePtr source);
void xslDropCall(void);
int xslStepupToDepth(int depth);
int xslStepdownToDepth(int depth);
void *xslGetCall(int depth);
void *xslGetCallStackTop(void);
int xslCallDepth(void);
void xslDebugInit(void);
void xslDebugFree(void);
void xslDebugBreak(xmlNodePtr templ, xmlNodePtr node, xsltTemplatePtr root,
                   xsltTransformContextPtr ctxt);
int xslDebugGotControl(int reached);
xmlNodePtr xslFindTemplateNode(xsltStylesheetPtr style,
                               const xmlChar * name);
xmlNodePtr xslFindNodeByLineNo(xsltTransformContextPtr ctxt,
                               const xmlChar * url, long lineNumber);
int xslFindBreakPointById(int id);
int xslFindBreakPointByLineNo(const xmlChar * url, long lineNumber);
int xslFindBreakPointByName(const xmlChar * templateName);

/**
 * xslActiveBreakPoint(void);
 *
 * DEPRECATED
 *
 * Returns 0
 */
int
xslActiveBreakPoint(void)
{
    return 0;
}

/**
 * xslSetActiveBreakPoint:
 * @breakPointNumber : 0 < breakPointNumber <= xslBreakPointCount(void)
 *
 * DEPRECATED
 *
 * Returns 0
 */
int
xslSetActiveBreakPoint(int breakPointNumber ATTRIBUTE_UNUSED)
{
    return 0;
}

/**
 * xslAddBreakPoint:
 * @url : url non-null, non-empty file name that has been loaded by
 *                    debugger
 * @lineNumber : number >= 0 and is available in url specified and points to 
 *               an xml element
 * @temlateName : the template name of breakpoint or NULL
 * @type : DEBUG_BREAK_SOURCE if are we stopping at a xsl source line
 *         DEBUG_BREAK_DATA otherwise
 *
 *
 * DEPRECATED
 *
 * Returns 0
*/
int
xslAddBreakPoint(const xmlChar * url ATTRIBUTE_UNUSED,
                 long lineNumber ATTRIBUTE_UNUSED,
                 const xmlChar * templateName ATTRIBUTE_UNUSED,
                 int type ATTRIBUTE_UNUSED)
{
    return 0;
}

/**
 * xslDeleteBreakPoint:
 * @breakPointNumber : 0 < breakPointNumber <= xslBreakPointCount(void)
 *
 * DEPRECATED
 *
 * Returns 0
*/
int
xslDeleteBreakPoint(int breakPointNumber ATTRIBUTE_UNUSED)
{
    return 0;
}

/**
 * xslEnableBreakPoint:
 * @breakPointNumber : 0 < breakPointNumber <= xslBreakPointCount(void)
 * @enable : enable break point if 1, disable if 0, toggle if -1
 *
 * DEPRECATED
 *
 * Returns 0
*/
int
xslEnableBreakPoint(int breakPointNumber ATTRIBUTE_UNUSED,
                    int enable ATTRIBUTE_UNUSED)
{
    return 0;
}

/**
 * xslIsBreakPointEnabled:
 * @breakPointNumber : 0 < breakPointNumber <= xslBreakPointCount(void)
 *
 * DEPRECATED
 *
 * Returns  -1
*/
int
xslIsBreakPointEnabled(int breakPointNumber ATTRIBUTE_UNUSED)
{
    return -1;
}

/**
 * xslBreakPointCount:
 *
 * Return the number of breakpoints present
 */
int
xslBreakPointCount(void)
{
    return 0;
}

/**
 * xslGetBreakPoint:
 * @breakPointNumber : 0 < breakPointNumber <= xslBreakPointCount(void)
 *
 * DEPRECATED
 *
 * Returns NULL
*/
void *
xslGetBreakPoint(int breakPointNumber ATTRIBUTE_UNUSED)
{
    return NULL;
}

/**
 * xslPrintBreakPoint:
 * @file : file != NULL
 * @breakPointNumber : 0 < breakPointNumber <= xslBreakPointCount(void)
 *
 * DEPRECATED
 *
 * Returns 0
 */
int
xslPrintBreakPoint(FILE * file ATTRIBUTE_UNUSED,
                   int breakPointNumber ATTRIBUTE_UNUSED)
{
    return 0;
}

/**
 * xslIsBreakPoint:
 * @url : url non-null, non-empty file name that has been loaded by debugger
 * @lineNumber : number >= 0 and is available in url specified
 *
 * DEPRECATED
 *
 * Returns 0
*/
int
xslIsBreakPoint(const xmlChar * url ATTRIBUTE_UNUSED,
                long lineNumber ATTRIBUTE_UNUSED)
{
    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslIsBreakPoint' not overloaded\n");

    return 0;
}

/**
 * xslIsBreakPointNode:
 * @node : node != NULL
 *
 * DEPRECATED
 *
 * Returns: 0
 */
int
xslIsBreakPointNode(xmlNodePtr node ATTRIBUTE_UNUSED)
{
    return 0;
}

/**
 * xslAddCallInfo:
 * @templateName : template name to add
 * @url : url for templateName
 *
 * DEPRECATED
 *
 * Returns NULL
 */
void *
xslAddCallInfo(const xmlChar * templateName ATTRIBUTE_UNUSED,
               const xmlChar * url ATTRIBUTE_UNUSED)
{
    return NULL;
}

/**
 * xslAddCall:
 * @templ : current template being applied
 * @source : the source node being processed
 *
 * DEPRECATED
 *
 * Returns 0
 */
int
xslAddCall(xsltTemplatePtr templ ATTRIBUTE_UNUSED,
           xmlNodePtr source ATTRIBUTE_UNUSED)
{
    return 0;
}

/**
 * xslDropCall :
 *
 * DEPRECATED
 */
void
xslDropCall(void)
{
}

/** 
 * xslStepupToDepth :
 * @depth :the frame depth to step up to  
 *
 * DEPRECATED
 *
 * Returns 0
 */
int
xslStepupToDepth(int depth ATTRIBUTE_UNUSED)
{
    return 0;
}

/** 
 * xslStepdownToDepth :
 * @depth : the frame depth to step down to 
 *
 * DEPRECATED
 *
 * Returns 0
 */
int
xslStepdownToDepth(int depth ATTRIBUTE_UNUSED)
{
    return 0;
}


/**
 * xslGetCall :
 * @depth : 0 < depth <= xslCallDepth(void)  
 *
 * DEPRECATED
 *
 * Return  NULL
 */
void *
xslGetCall(int depth ATTRIBUTE_UNUSED)
{
    return NULL;
}


/** 
 * xslGetCallStackTop :
 *
 * DEPRECATED
 *
 * Returns NULL
 */
void *
xslGetCallStackTop(void)
{
    return NULL;
}


/** 
 * xslCallDepth :
 *
 * DEPRECATED
 *
 * Returns 0
 */
int
xslCallDepth(void)
{
    return 0;
}


/**
 * xslDebugInit :
 *
 * DEPRECATED
 */
void
xslDebugInit(void)
{
}


/**
 * xslDebugFree :
 *
 * Free up any memory taken by debugging
 */
void
xslDebugFree(void)
{
}

/**
 * xslDebugBreak:
 * @templ : The source node being executed
 * @node : The data node being processed
 * @root : The template being applide to "node"
 * @ctxt : stylesheet being processed
 *
 * DEPRECATED
 */
void
xslDebugBreak(xmlNodePtr templ ATTRIBUTE_UNUSED,
              xmlNodePtr node ATTRIBUTE_UNUSED,
              xsltTemplatePtr root ATTRIBUTE_UNUSED,
              xsltTransformContextPtr ctxt ATTRIBUTE_UNUSED)
{
}

/** 
 * xslDebugGotControl :
 * @reached : true if debugger has received control
 *
 * DEPRECATED
 * Returns 0
 */
int
xslDebugGotControl(int reached ATTRIBUTE_UNUSED)
{
    return 0;
}

/**
 * xslFindTemplateNode: 
 * @style : valid stylesheet collection context to look into
 * @name : template name to look for
 *
 * DEPRECATED
 *
 * Returns NULL
 */
xmlNodePtr
xslFindTemplateNode(xsltStylesheetPtr style ATTRIBUTE_UNUSED,
                    const xmlChar * name ATTRIBUTE_UNUSED)
{

    return NULL;
}

/**
 * xslFindBreakPointByLineNo:
 * @ctxt : valid ctxt to look into
 * @url : url non-null, non-empty file name that has been loaded by
 *                    debugger
 * @lineNumber : number >= 0 and is available in url specified
 *
 * DEPRECATED
 *
 * Returns NULL
*/
xmlNodePtr
xslFindNodeByLineNo(xsltTransformContextPtr ctxt ATTRIBUTE_UNUSED,
                    const xmlChar * url ATTRIBUTE_UNUSED,
                    long lineNumber ATTRIBUTE_UNUSED)
{
    return NULL;
}

/**
 * xslFindBreakPointById:
 * @id : The break point id to look for
 *
 * DEPRECATED
 *
 * Returns 0
 */
int
xslFindBreakPointById(int id ATTRIBUTE_UNUSED)
{
    return 0;
}

/**
 * xslFindBreakPointByLineNo:
 * @url :  a valid url that has been loaded by debugger
 * @lineNumber : lineNumber >= 0 and is available in url specified
 *
 * DEPRECATED
 *
 * Returns 0
*/
int
xslFindBreakPointByLineNo(const xmlChar * url ATTRIBUTE_UNUSED,
                          long lineNumber ATTRIBUTE_UNUSED)
{
    return 0;
}

/**
 * xslFindBreakPointByName:
 * @templateName : template name to look for
 *
 * DEPRECATED
 *
 * Returns 0
*/
int
xslFindBreakPointByName(const xmlChar * templateName ATTRIBUTE_UNUSED)
{
    return 0;
}
