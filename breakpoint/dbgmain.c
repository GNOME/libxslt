
/***************************************************************************
                          dbgmain.c  -  description
                             -------------------
    begin                : Fri Nov 2 2001
    copyright            : (C) 2001 by Keith Isdale
    email                : k_isdale@tpg.com.au
 ***************************************************************************/


#include "config.h"

#ifdef WITH_DEBUGGER

/*
-----------------------------------------------------------
       Main debugger functions
-----------------------------------------------------------
*/

#include "xsltutils.h"
#include "breakpoint.h"

int xslDebugStatus = DEBUG_NONE;

/**
 * xslDebugInit :
 *
 * Initialize debugger
 */
void
xslDebugInit()
{
    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslDebugInit' not overloaded\n");
}


/**
 * xslDebugFree :
 *
 * Free up any memory taken by debugging
 */
void
xslDebugFree()
{
    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslDebugFree' not overloaded\n");

}


extern char *xslShellReadline(char *prompt);


/**
 * @templ : The source node being executed
 * @node : The data node being processed
 * @root : The template being applide to "node"
 * @ctxt : stylesheet being processed
 *
 * A break point has been found so pass control to user
 */
void
xslDebugBreak(xmlNodePtr templ ATTRIBUTE_UNUSED,
              xmlNodePtr node ATTRIBUTE_UNUSED,
              xsltTemplatePtr root ATTRIBUTE_UNUSED,
              xsltTransformContextPtr ctxt ATTRIBUTE_UNUSED)
{
    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslDebugBreak' not overloaded\n");

}

/** 
 * xslDebugGotControl :
 * @reached : true if debugger has received control
 *
 * Set flag that debuger has received control to value of @reached
 * Returns true if any breakpoint was reached previously
 */
int
xslDebugGotControl(int reached)
{
    static int got_control = 0;
    int result = got_control;
    got_control = reached;
    return result;
}

#endif
