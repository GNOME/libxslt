
/***************************************************************************
                          dbgsearch.c  -  description
                             -------------------
    begin                : Fri Nov 2 2001
    copyright            : (C) 2001 by Keith Isdale
    email                : k_isdale@tpg.com.au
 ***************************************************************************/


#include "config.h"

#ifdef WITH_DEBUGGER

/*
-----------------------------------------------------------
       Search/Find debugger functions
-----------------------------------------------------------
*/

#include "xsltutils.h"
#include "breakpoint.h"

/**
 * xslFindTemplateNode: 
 * @style : valid stylesheet collection context to look into
 * @name : template name to look for
 *
 * Returns : template node found if successfull
 *           NULL otherwise 
 */
xmlNodePtr
xslFindTemplateNode(xsltStylesheetPtr style ATTRIBUTE_UNUSED,
                    const xmlChar * name ATTRIBUTE_UNUSED)
{

    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslFindTemplateNode' not overloaded\n");
    return NULL;
}


/**
 * xslFindBreakPointByLineNo:
 * @ctxt : valid ctxt to look into
 * @url : url non-null, non-empty file name that has been loaded by
 *                    debugger
 * @lineNumber : number >= 0 and is available in url specified
 *
 * Find the closest line number in file specified that can be a point 
 * Returns  line number number if successfull,
 *	    0 otherwise
*/
xmlNodePtr
xslFindNodeByLineNo(xsltTransformContextPtr ctxt ATTRIBUTE_UNUSED,
                    const xmlChar * url ATTRIBUTE_UNUSED,
                    long lineNumber ATTRIBUTE_UNUSED)
{
    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslFindNodeByLineNo' not overloaded\n");

    return NULL;
}


/**
 * xslFindBreakPointById:
 * @id : The break point id to look for
 *
 * Find the break point number for given break point id
 * Returns break point number can be found for given the break point id
 *          0 otherwise 
 */
int
xslFindBreakPointById(int id ATTRIBUTE_UNUSED)
{
    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslFindBreakPointById' not overloaded\n");

    return 0;
}


/**
 * xslFindBreakPointByLineNo:
 * @url :  a valid url that has been loaded by debugger
 * @lineNumber : lineNumber >= 0 and is available in url specified
 *
 * Find the break point number for a given url and line number
 * Returns break point number number if successfull,
 *	    0 otherwise
*/
int
xslFindBreakPointByLineNo(const xmlChar * url ATTRIBUTE_UNUSED,
                          long lineNumber ATTRIBUTE_UNUSED)
{
    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslFindBreakPointByLineNo' not overloaded\n");

    return 0;
}


/**
 * xslFindBreakPointByName:
 * @templateName : template name to look for
 *
 * Find the breakpoint at template with "match" or "name" or equal to 
 *    templateName
 * Returns break point number given the template name is found
 *          0 otherwise
*/
int
xslFindBreakPointByName(const xmlChar * templateName ATTRIBUTE_UNUSED)
{
    xsltGenericError(xsltGenericErrorContext,
                     "Error!: Debugger function 'xslFindBreakPointByName' not overloaded\n");

    return 0;
}

#endif
