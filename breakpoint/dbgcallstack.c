
/***************************************************************************
                          dbgcallstack.c  -  description
                             -------------------
    begin                : Fri Nov 2 2001
    copyright            : (C) 2001 by Keith Isdale
    email                : k_isdale@tpg.com.au
 ***************************************************************************/

#include "config.h"

#ifdef WITH_DEBUGGER

/*
------------------------------------------------------
                  Xsl call stack related
-----------------------------------------------------
*/

#include "xsltutils.h"
#include "breakpoint.h"


/**
 * xslAddCallInfo:
 * @templateName : template name to add
 * @url : url for templateName
 *
 * Returns a reference to the added info if sucessfull, otherwise NULL  
 */
xslCallPointInfoPtr
xslAddCallInfo(const xmlChar * templateName, const xmlChar * url)
{
    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslAddCallInfo' not overloaded\n");

    return NULL;
}


/**
 * xslAddCall:
 * @templ : current template being applied
 * @source : the source node being processed
 *
 * Add template "call" to call stack
 * Returns : 1 on sucess, 0 otherwise
 */
int
xslAddCall(xsltTemplatePtr templ, xmlNodePtr source)
{
    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslAddCall' not overloaded\n");

    return 0;
}


/**
 * xslDropCall :
 *
 * Drop the topmost item off the call stack
 */
void
xslDropCall()
{

    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslDropCall' not overloaded\n");

}


/** 
 * xslStepupToDepth :
 * @depth :the frame depth to step up to  
 *
 * Set the frame depth to step up to
 * Returns 1 on sucess , 0 otherwise
 */
int
xslStepupToDepth(int depth)
{
    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslStepupToDepth' not overloaded\n");
    return 0;
}


/** 
 * xslStepdownToDepth :
 * @depth : the frame depth to step down to 
 *
 * Set the frame depth to step down to
 * Returns 1 on sucess , 0 otherwise
 */
int
xslStepdownToDepth(int depth)
{
    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslStepdownToDepth' not overloaded\n");
    return 0;
}


/**
 * xslGetCall :
 * @depth : 0 < depth <= xslCallDepth()  
 *
 * Retrieve the call point at specified call depth 

 * Return  non-null a if depth is valid
 *         NULL otherwise 
 */
xslCallPointPtr
xslGetCall(int depth)
{
    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslGetCall' not overloaded\n");

    return 0;
}


/** 
 * xslGetCallStackTop :
 *
 * Returns the top of the call stack
 */
xslCallPointPtr
xslGetCallStackTop()
{
    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslGetCallStackTop' not overloaded\n");

    return NULL;
}


/** 
 * xslCallDepth :
 *
 * Return the depth of call stack
 */
int
xslCallDepth()
{
    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslCallDepth' not overloaded\n");
    return 0;
}

#endif
