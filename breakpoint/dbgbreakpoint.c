
/***************************************************************************
                          breakpoint.c  -  description
                             -------------------
    begin                : Fri Nov 2 2001
    copyright            : (C) 2001 by Keith Isdale
    email                : k_isdale@tpg.com.au
 ***************************************************************************/

#include "config.h"

#ifdef WITH_DEBUGGER

/*
-----------------------------------------------------------
       Breakpoint debugger functions
-----------------------------------------------------------
*/


#include "xsltutils.h"
#include "breakpoint.h"


/**
 * xslActiveBreakPoint();
 * Return the break point number that we stoped at
 */
int
xslActiveBreakPoint()
{
    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslDebugInit' not overloaded\n");

    return 0;
}

/**
 * xslSetActiveBreakPoint:
 * @breakPointNumber : 0 < breakPointNumber <= xslBreakPointCount()
 *
 * Return 1 on success,
 *        0 otherwise 
 */
int
xslSetActiveBreakPoint(int breakPointNumber ATTRIBUTE_UNUSED)
{
    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslSetActiveBreakPoint' not overloaded\n");
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
 * Add break point at file and line number specifiec
 * Returns  break point number if successfull,
 *	    0 otherwise 
*/
int
xslAddBreakPoint(const xmlChar * url ATTRIBUTE_UNUSED, long lineNumber ATTRIBUTE_UNUSED,
                 const xmlChar * templateName ATTRIBUTE_UNUSED, int type ATTRIBUTE_UNUSED)
{

    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslAddBreakPoint' not overloaded\n");
    return 0;
}


/**
 * xslDeleteBreakPoint:
 * @breakPointNumber : 0 < breakPointNumber <= xslBreakPointCount()
 *
 * Delete the break point specified
 * Returns 1 if successfull,
 *	    0 otherwise
*/
int
xslDeleteBreakPoint(int breakPointNumber ATTRIBUTE_UNUSED)
{

    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslDeleteBreakPoint' not overloaded\n");

    return 0;
}




/**
 * xslEnableBreakPoint:
 * @breakPointNumber : 0 < breakPointNumber <= xslBreakPointCount()
 * @enable : enable break point if 1, disable if 0, toggle if -1
 *
 * Enable or disable a break point
 * Returns 1 if successfull,
 *	    0 otherwise
*/
int
xslEnableBreakPoint(int breakPointNumber ATTRIBUTE_UNUSED, int enable ATTRIBUTE_UNUSED)
{
    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslEnableBreakPoint' not overloaded\n");

    return 0;
}


/**
 * xslIsBreakPointEnabled:
 * @breakPointNumber : 0 < breakPointNumber <= xslBreakPointCount()
 *
 * Is the breakpoint at breakPointNumber specified enabled
 * Returns  -1 if breakPointNumber is invalid
 *           0 if break point is disabled 
 *           1 if break point is enabled      
*/
int
xslIsBreakPointEnabled(int breakPointNumber ATTRIBUTE_UNUSED)
{
    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslIsBreakPointEnabled' not overloaded\n");
    return -1;
}


/**
 * xslBreakPointCount:
 *
 * Return the number of breakpoints present
 */
int
xslBreakPointCount()
{
    return 0;
}


/**
 * xslGetBreakPoint:
 * @breakPointNumber : 0 < breakPointNumber <= xslBreakPointCount()
 *
 * Lookup the value of break point at breakPointNumber specified
 * Returns break point if breakPointNumber is valid,  
 *          NULL otherwise
*/
xslBreakPointPtr
xslGetBreakPoint(int breakPointNumber ATTRIBUTE_UNUSED)
{
    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslGetBreakPoint' not overloaded\n");
    return NULL;
}


/**
 * xslPrintBreakPoint:
 * @file : file != NULL
 * @breakPointNumber : 0 < breakPointNumber <= xslBreakPointCount()
 *
 * Print the details of break point to file specified
 *
 * Returns 1 if successfull, 
 *          0 otherwise
 */
int
xslPrintBreakPoint(FILE * file ATTRIBUTE_UNUSED, int breakPointNumber ATTRIBUTE_UNUSED)
{
    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslPrintBreakPoint' not overloaded\n");

    return 0;
}


/**
 * xslIsBreakPoint:
 * @url : url non-null, non-empty file name that has been loaded by
 *                    debugger
 * @lineNumber : number >= 0 and is available in url specified
 *
 * Determine if there is a break point at file and line number specified
 * Returns 1  if successfull,
 *         0 otherwise
*/
int
xslIsBreakPoint(const xmlChar * url ATTRIBUTE_UNUSED, long lineNumber ATTRIBUTE_UNUSED)
{
    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslIsBreakPoint' not overloaded\n");

    return 0;
}


/**
 * xslIsBreakPointNode:
 * @node : node != NULL
 *
 * Determine if a node is a break point
 * Returns : 1 on sucess, 
 *           0 otherwise
 */
int
xslIsBreakPointNode(xmlNodePtr node ATTRIBUTE_UNUSED)
{
    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslIsBreakPointNode' not overloaded\n");


    return 0;
}

#endif
