
/***************************************************************************
                          breakpoint.h  -  description
                             -------------------
    begin                : Sun Sep 16 2001
    copyright            : (C) 2001 by Keith Isdale
    email                : k_isdale@tpg.com.au
 ***************************************************************************/

#include "config.h"

#ifdef WITH_DEBUGGER
#ifndef BREAKPOINT_H
#define BREAKPOINT_H

#ifdef WITH_XSLT_DEBUG
#define WITH_XSLT_DEBUG_BREAKPOINTS
#endif

#include <libxml/tree.h>

#include <libxslt/xsltInternals.h>
#include <libxml/xpath.h>
#include <libxml/list.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Define the types of status whilst debugging*/
typedef enum {
    DEBUG_NONE,                 /* no debugging allowed */
    DEBUG_INIT,
    DEBUG_STEP,
    DEBUG_STEPUP,
    DEBUG_STEPDOWN,
    DEBUG_NEXT,
    DEBUG_STOP,
    DEBUG_CONT,
    DEBUG_RUN,
    DEBUG_RUN_RESTART,
    DEBUG_QUIT
} DebugStatus;

typedef enum {
    DEBUG_BREAK_SOURCE = 1,
    DEBUG_BREAK_DATA
} BreakPointType;

#define XSL_TOGGLE_BREAKPOINT -1
extern int xslDebugStatus;

typedef struct _xslBreakPoint xslBreakPoint;
typedef xslBreakPoint *xslBreakPointPtr;
struct _xslBreakPoint {
    xmlChar *url;
    long lineNo;
    xmlChar *templateName;      /* only used when printing break point */
    int enabled;
    int type;
    int id;
};


/*
-----------------------------------------------------------
             Break point related functions
----------------------------------------------------------
*/

/**
 * xslFindTemplateNode: 
 * @style : valid stylesheet collection to look into 
 * @name : template name to look for
 *
 * Returns : template node found if successfull,
 *           NULL otherwise 
 */
xmlNodePtr xslFindTemplateNode(const xsltStylesheetPtr style,
                               const xmlChar * name);


/**
 * xslActiveBreakPoint:
 *
 * Return the break point number that we stoped at
*/
int xslActiveBreakPoint(void);


/**
 * xslSetActiveBreakPoint:
 * @breakPointNumber : 0 < breakPointNumber <= xslBreakPointCount()
 *
 * Sets the active break point number
 *   Returns 1 on success,
 *            0 otherwise
 */
int xslSetActiveBreakPoint(int breakPointNumber);


/**
 * xslAddBreakPoint:
 * @url : a valid url that has been loaded by debugger
 * @lineNumber : number >= 0 and is available in url specified and points to 
 *               an xml element
 * @temlateName : the template name of breakpoint or NULL if not adding
 *                 a template break point
 * @type : DEBUG_BREAK_SOURCE if are we stopping at a xsl source line
 *         DEBUG_BREAK_DATA otherwise
 *
 * Add break point at file and line number specified
 * Returns break point number if successfull,
 *	    0 otherwise 
*/
int xslAddBreakPoint(const xmlChar * url, long lineNumber,
                     const xmlChar * templateName, int type);


/**
 * xslDeleteBreakPoint:
 * @breakPointNumber : 0 < breakPointNumber <= xslBreakPointCount()
 *
 * Delete the break point with breakPointNumber specified
 * Returns 1 if successfull,
 *	    0 otherwise
*/
int xslDeleteBreakPoint(int breakPointNumber);


/**
 * xslFindBreakPointById:
 * @id : The break point id to look for
 *
 * Find the break point number for given break point id
 * Returns break point number can be found for given the break point id
 *          0 otherwise 
 */
int xslFindBreakPointById(int id);


/**
 * xslFindBreakPointByLineNo:
 * @url :  a valid url that has been loaded by debugger
 * @lineNumber : lineNumber >= 0 and is available in url specified
 *
 * Find the break point number for a given url and line number
 * Returns breakpoint number number if successfull,
 *	    0 otherwise 
*/
int xslFindBreakPointByLineNo(const xmlChar * url, long lineNumber);


/**
 * xslFindBreakPointByName:
 * @templateName : template name to look for
 *
 * Find the breakpoint at template with "match" or "name" equal 
 *    to templateName
 * Returns the break point number given the template name is found
 *          0 otherwise
*/
int xslFindBreakPointByName(const xmlChar * templateName);


/**
 * xslEnableBreakPoint:
 * @breakPointNumber : 0 < breakPointNumber <= xslBreakPointCount()
 * @enable : enable break point if 1, disable if 0, toggle if -1
 *
 * Enable or disable a break point
 * Returns 1 if successfull,
 *	    0 otherwise
*/
int xslEnableBreakPoint(int breakPointNumber, int enable);


/**
 * xslIsBreakPointEnabled:
 * @breakPointNumber : 0 < breakPointNumber <= xslBreakPointCount()
 *
 * Is the breakpoint at breakPointNumber specified enabled
 * Returns  -1 if breakPointNumber is invalid
 *           0 if break point is disabled 
 *           1 if break point is enabled      
*/
int xslIsBreakPointEnabled(int breakPointNumber);


/**
 * xslBreakPointCount:
 *
 * Returns  the number of break points present
 */
int xslBreakPointCount(void);


/**
 * xslGetBreakPoint:
 * @breakPointNumber : 0 < breakPointNumber <= xslBreakPointCount()
 *
 * Lookup the value of break point at breakPointNumber specified
 * Returns break point if breakPointNumber is valid,
 *	    NULL otherwise
*/
xslBreakPointPtr xslGetBreakPoint(int breakPointNumber);


/**
 * xslPrintBreakPoint:
 * @file : file != NULL
 * @breakPointNumber : 0 < breakPointNumber <= xslBreakPointCount()
 *
 * Print the details of break point to file specified
 *
 * Returns 1 if successfull,
 *	   0 otherwise
 */
int xslPrintBreakPoint(FILE * file, int breakPointNumber);


/**
 * xslIsBreakPoint:
 * @url : url non-null, non-empty file name that has been loaded by
 *                    debugger
 * @lineNumber : number >= 0 and is available in url specified
 *
 * Determine if there is a break point at file and line number specifiec
 * Returns 1  if successfull,  
 *         0 otherwise
*/
int xslIsBreakPoint(const xmlChar * url, long lineNumber);


/**
 * xslIsBreakPointNode:
 * @node : node != NULL
 *
 * Determine if a node is a break point
 * Returns : 1 on sucess, 0 otherwise
 */
int xslIsBreakPointNode(xmlNodePtr node);

/*
-----------------------------------------------------------
       Main debugger functions
-----------------------------------------------------------
*/

/**
 * xslDebugBreak:
 * @templ : The source node being executed
 * @node : The data node being processed
 * @root : The template being applide to "node"
 * @ctxt : stylesheet being processed
 *
 * A break point has been found so pass control to user
 */
void xslDebugBreak(xmlNodePtr templ, xmlNodePtr node, xsltTemplatePtr root,
                   xsltTransformContextPtr ctxt);


/**
 * xslDebugInit :
 *
 * Initialize debugger allocating any memory needed by debugger
 */
void xslDebugInit(void);


/**
 * xslDebugFree :
 *
 * Free up any memory taken by debugger
 */
void xslDebugFree(void);


/** 
 * xslDebugGotControl :
 * @reached : true if debugger has received control
 *
 * Set flag that debuger has received control to value of @reached
 * Returns true if any breakpoint was reached previously
 */
int xslDebugGotControl(int reached);



/*
------------------------------------------------------
                  Xsl call stack related
-----------------------------------------------------
*/

typedef struct _xslCallPointInfo xslCallPointInfo;
typedef xslCallPointInfo *xslCallPointInfoPtr;

struct _xslCallPointInfo {
    const xmlChar *templateName;        /* will be unique */
    const xmlChar *url;
    xslCallPointInfoPtr next;
};


/**
 * xslAddCallInfo:
 * @templateName : template name to add
 * @url : url for the template
 *
 * Returns a reference to the added info if sucessfull, 
 *         NULL otherwise
 */
xslCallPointInfoPtr xslAddCallInfo(const xmlChar * templateName,
                                   const xmlChar * url);

typedef struct _xslCallPoint xslCallPoint;
typedef xslCallPoint *xslCallPointPtr;

struct _xslCallPoint {
    xslCallPointInfoPtr info;
    long lineNo;
    xslCallPointPtr next;
};


/**
 * xslAddCall:
 * @templ : current template being applied
 * @source : the source node being processed
 *
 * Add template "call" to call stack
 * Returns 1 on sucess,
 *         0 otherwise 
 */
int xslAddCall(xsltTemplatePtr templ, xmlNodePtr source);


/**
 * xslDropCall :
 *
 * Drop the topmost item off the call stack
 */
void xslDropCall(void);


/** 
 * xslStepupToDepth :
 * @depth :the frame depth to step up to  
 *
 * Set the frame depth to step up to
 * Returns 1 on sucess,
 *         0 otherwise
 */
int xslStepupToDepth(int depth);


/** 
 * xslStepdownToDepth :
 * @depth : the frame depth to step down to 
 *
 * Set the frame depth to step down to
 * Returns 1 on sucess, 
 *         0 otherwise
 */
int xslStepdownToDepth(int depth);


/**
 * xslGetCall :
 * @depth : 0 < depth <= xslCallDepth()
 *
 * Retrieve the call point at specified call depth 

 * Return non-null a if depth is valid,
 *        NULL otherwise 
 */
xslCallPointPtr xslGetCall(int depth);


/** 
 * xslGetCallStackTop :
 *
 * Returns the top of the call stack
 */
xslCallPointPtr xslGetCallStackTop(void);


/**
 * xslCallDepth :
 *
 * Returns the depth of call stack
 */
int xslCallDepth(void);



#ifdef __cplusplus
}
#endif

#endif

#endif
