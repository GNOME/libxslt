/*
 * variables.c: Implementation of the variable storage and lookup
 *
 * Reference:
 *   http://www.w3.org/TR/1999/REC-xslt-19991116
 *
 * See Copyright for the status of this software.
 *
 * Daniel.Veillard@imag.fr
 */

#include "xsltconfig.h"

#include <string.h>

#include <libxml/xmlmemory.h>
#include <libxml/tree.h>
#include <libxml/valid.h>
#include <libxml/hash.h>
#include <libxml/xmlerror.h>
#include <libxml/xpathInternals.h>
#include <libxml/parserInternals.h>
#include "xslt.h"
#include "xsltInternals.h"
#include "xsltutils.h"
#include "variables.h"

#define DEBUG_VARIABLES

/*
 * Types are private:
 */


/************************************************************************
 *									*
 *			Module interfaces				*
 *									*
 ************************************************************************/

/**
 * xsltRegisterVariable:
 * @style:  the XSLT stylesheet
 * @name:  the variable name
 * @ns_uri:  the variable namespace URI
 * @value:  the variable value or NULL
 *
 * Register a new variable value. If @value is NULL it unregisters
 * the variable
 *
 * Returns 0 in case of success, -1 in case of error
 */
int
xsltRegisterVariable(xsltStylesheetPtr style, const xmlChar *name,
		     const xmlChar *ns_uri, xmlXPathObjectPtr value) {
    if (style == NULL)
	return(-1);
    if (name == NULL)
	return(-1);

    if (style->variablesHash == NULL)
	style->variablesHash = xmlHashCreate(0);
    if (style->variablesHash == NULL)
	return(-1);
    return(xmlHashUpdateEntry2((xmlHashTablePtr) style->variablesHash,
		               name, ns_uri,
			       (void *) value,
			       (xmlHashDeallocator) xmlXPathFreeObject));
}

/**
 * xsltVariableLookup:
 * @style:  the XSLT stylesheet
 * @name:  the variable name
 * @ns_uri:  the variable namespace URI
 *
 * Search in the Variable array of the context for the given
 * variable value.
 *
 * Returns the value or NULL if not found
 */
xmlXPathObjectPtr
xsltVariableLookup(xsltStylesheetPtr style, const xmlChar *name,
		   const xmlChar *ns_uri) {
    if (style == NULL)
	return(NULL);

    if (style->variablesHash == NULL)
	return(NULL);
    if (name == NULL)
	return(NULL);

    return((xmlXPathObjectPtr)
	   xmlHashLookup2((xmlHashTablePtr) style->variablesHash,
	                  name, ns_uri));
}


/**
 * xsltFreeVariableHashes:
 * @style: an XSLT stylesheet
 *
 * Free up the memory used by xsltAddVariable/xsltGetVariable mechanism
 */
void
xsltFreeVariableHashes(xsltStylesheetPtr style) {
    if (style->variablesHash != NULL)
	xmlHashFree((xmlHashTablePtr) style->variablesHash,
		    (xmlHashDeallocator) xmlXPathFreeObject);
}

