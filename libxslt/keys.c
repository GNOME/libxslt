/*
 * keys.c: Implemetation of the keys support
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
#include <libxml/parserInternals.h>
#include <libxml/xpathInternals.h>
#include "xslt.h"
#include "xsltInternals.h"
#include "xsltutils.h"
#include "imports.h"
#include "templates.h"

#define DEBUG_KEYS

typedef struct _xsltKeyDef xsltKeyDef;
typedef xsltKeyDef *xsltKeyDefPtr;
struct _xsltKeyDef {
    struct _xsltKeyDef *next;
    xmlChar *name;
    xmlChar *nameURI;
    xmlChar *match;
    xmlChar *use;
};

typedef struct _xsltKeyTable xsltKeyTable;
typedef xsltKeyTable *xsltKeyTablePtr;
struct _xsltKeyTable {
    struct _xsltKeyTable *next;
    xmlChar *name;
    xmlChar *nameURI;
    xmlHashTablePtr keys;
};


/************************************************************************
 * 									*
 * 			Type functions 					*
 * 									*
 ************************************************************************/

/**
 * xsltNewKeyDef:
 * @name:  the key name or NULL
 * @nameURI:  the name URI or NULL
 *
 * Create a new XSLT KeyDef
 *
 * Returns the newly allocated xsltKeyDefPtr or NULL in case of error
 */
xsltKeyDefPtr
xsltNewKeyDef(const xmlChar *name, const xmlChar *nameURI) {
    xsltKeyDefPtr cur;

    cur = (xsltKeyDefPtr) xmlMalloc(sizeof(xsltKeyDef));
    if (cur == NULL) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltNewKeyDef : malloc failed\n");
	return(NULL);
    }
    memset(cur, 0, sizeof(xsltKeyDef));
    if (name != NULL)
	cur->name = xmlStrdup(name);
    if (nameURI != NULL)
	cur->nameURI = xmlStrdup(nameURI);
    return(cur);
}

/**
 * xsltFreeKeyDef:
 * @keyd:  an XSLT key definition
 *
 * Free up the memory allocated by @keyd
 */
void
xsltFreeKeyDef(xsltKeyDefPtr keyd) {
    if (keyd == NULL)
	return;
    if (keyd->name != NULL)
	xmlFree(keyd->name);
    if (keyd->nameURI != NULL)
	xmlFree(keyd->nameURI);
    if (keyd->match != NULL)
	xmlFree(keyd->match);
    if (keyd->use != NULL)
	xmlFree(keyd->use);
    memset(keyd, -1, sizeof(xsltKeyDef));
    xmlFree(keyd);
}

/**
 * xsltFreeKeyDefList:
 * @keyd:  an XSLT key definition list
 *
 * Free up the memory allocated by all the elements of @keyd
 */
void
xsltFreeKeyDefList(xsltKeyDefPtr keyd) {
    xsltKeyDefPtr cur;

    while (keyd != NULL) {
	cur = keyd;
	keyd = keyd->next;
	xsltFreeKeyDef(cur);
    }
}

/**
 * xsltNewKeyTable:
 * @name:  the key name or NULL
 * @nameURI:  the name URI or NULL
 *
 * Create a new XSLT KeyTable
 *
 * Returns the newly allocated xsltKeyTablePtr or NULL in case of error
 */
xsltKeyTablePtr
xsltNewKeyTable(const xmlChar *name, const xmlChar *nameURI) {
    xsltKeyTablePtr cur;

    cur = (xsltKeyTablePtr) xmlMalloc(sizeof(xsltKeyTable));
    if (cur == NULL) {
        xsltGenericError(xsltGenericErrorContext,
		"xsltNewKeyTable : malloc failed\n");
	return(NULL);
    }
    memset(cur, 0, sizeof(xsltKeyTable));
    if (name != NULL)
	cur->name = xmlStrdup(name);
    if (nameURI != NULL)
	cur->nameURI = xmlStrdup(nameURI);
    cur->keys = xmlHashCreate(0);
    return(cur);
}

/**
 * xsltFreeKeyTable:
 * @keyt:  an XSLT key table
 *
 * Free up the memory allocated by @keyt
 */
void
xsltFreeKeyTable(xsltKeyTablePtr keyt) {
    if (keyt == NULL)
	return;
    if (keyt->name != NULL)
	xmlFree(keyt->name);
    if (keyt->nameURI != NULL)
	xmlFree(keyt->nameURI);
    if (keyt->keys != NULL)
	xmlHashFree(keyt->keys, 
		    (xmlHashDeallocator) xmlXPathFreeNodeSet);
    memset(keyt, -1, sizeof(xsltKeyTable));
    xmlFree(keyt);
}

/**
 * xsltFreeKeyTableList:
 * @keyt:  an XSLT key table list
 *
 * Free up the memory allocated by all the elements of @keyt
 */
void
xsltFreeKeyTableList(xsltKeyTablePtr keyt) {
    xsltKeyTablePtr cur;

    while (keyt != NULL) {
	cur = keyt;
	keyt = keyt->next;
	xsltFreeKeyTable(cur);
    }
}

/************************************************************************
 * 									*
 * 		The interpreter for the precompiled patterns		*
 * 									*
 ************************************************************************/


/**
 * xsltFreeKeys:
 * @style: an XSLT stylesheet
 *
 * Free up the memory used by XSLT keys in a stylesheet
 */
void
xsltFreeKeys(xsltStylesheetPtr style) {
    if (style->keys)
	xsltFreeKeyDefList((xsltKeyDefPtr) style->keys);
}

/**
 * xsltAddKey:
 * @style: an XSLT stylesheet
 * @name:  the key name or NULL
 * @nameURI:  the name URI or NULL
 * @match:  the match value
 * @use:  the use value
 *
 * add a key definition to a stylesheet
 *
 * Returns 0 in case of success, and -1 in case of failure.
 */
int	
xsltAddKey(xsltStylesheetPtr style, const xmlChar *name,
	   const xmlChar *nameURI, const xmlChar *match, const xmlChar *use) {
    xsltKeyDefPtr key;

    if ((style == NULL) || (name == NULL) || (match == NULL) || (use == NULL))
	return(-1);

#ifdef DEBUG_KEYS
    xsltGenericDebug(xsltGenericDebugContext,
	"Add key %s, match %s, use %s\n", name, match, use);
#endif

    key = xsltNewKeyDef(name, nameURI);
    key->match = xmlStrdup(match);
    key->use = xmlStrdup(use);
    key->next = style->keys;
    style->keys = key;
    return(0);
}

/**
 * xsltGetKey:
 * @ctxt: an XSLT transformation context
 * @name:  the key name or NULL
 * @nameURI:  the name URI or NULL
 * @value:  the key value to look for
 *
 * Lookup a key
 *
 * Returns the nodeset resulting from the query or NULL
 */
xmlNodeSetPtr
xsltGetKey(xsltTransformContextPtr ctxt, const xmlChar *name,
	   const xmlChar *nameURI, const xmlChar *value) {
    xmlNodeSetPtr ret;
    xsltKeyTablePtr table;

    if ((ctxt == NULL) || (name == NULL) || (value == NULL))
	return(NULL);

#ifdef DEBUG_KEYS
    xsltGenericDebug(xsltGenericDebugContext,
	"Get key %s, value %s\n", name, value);
#endif

    table = (xsltKeyTablePtr) ctxt->document->keys;
    while (table != NULL) {
	if (xmlStrEqual(table->name, name) &&
	    (((nameURI == NULL) && (table->nameURI == NULL)) ||
	     ((nameURI != NULL) && (table->nameURI != NULL) &&
	      (xmlStrEqual(table->nameURI, nameURI))))) {
	    ret = (xmlNodeSetPtr)xmlHashLookup(table->keys, value);
	    return(ret);
	}
	table = table->next;
    }
    return(NULL);
}

/**
 * xsltInitCtxtKey:
 * @ctxt: an XSLT transformation context
 * @doc:  an XSLT document
 * @keyd: the key definition
 *
 * Computes the key tables this key and for the current input document.
 */
void
xsltInitCtxtKey(xsltTransformContextPtr ctxt, xsltDocumentPtr doc,
	        xsltKeyDefPtr keyd) {
    int i;
    xmlChar *pattern = NULL;
    xmlNodeSetPtr nodelist = NULL, keylist;
    xmlXPathObjectPtr res = NULL;
    xmlChar *str;
    xmlXPathCompExprPtr comp;
    xsltKeyTablePtr table;


    /*
     * Prepare the search
     */
    if (keyd->match[0] != '/') {
	pattern = xmlStrdup((xmlChar *)"//");
	pattern = xmlStrcat(pattern, keyd->match);
    } else {
	pattern = xmlStrdup(keyd->match);
    }

    /*
     * Evaluate the nodelist
     */

    comp = xmlXPathCompile(pattern);
    if (comp == NULL)
	goto error;
    ctxt->document = doc;
    ctxt->node = (xmlNodePtr) doc->doc;
    ctxt->xpathCtxt->node = (xmlNodePtr) doc->doc;
    res = xmlXPathCompiledEval(comp, ctxt->xpathCtxt);
    xmlXPathFreeCompExpr(comp);
    if (res != NULL) {
	if (res->type == XPATH_NODESET) {
	    nodelist = res->nodesetval;
#ifdef DEBUG_KEYS
	    xsltGenericDebug(xsltGenericDebugContext,
		 "xsltInitCtxtKey: %s evaluates to %d nodes\n",
			     pattern, nodelist->nodeNr);
#endif
	} else {
#ifdef DEBUG_KEYS
	    xsltGenericDebug(xsltGenericDebugContext,
		 "xsltInitCtxtKey: %s is not a node set\n", pattern);
#endif
	    goto error;
	}
    } else {
#ifdef DEBUG_KEYS
	xsltGenericDebug(xsltGenericDebugContext,
	     "xsltInitCtxtKey: %s evaluation failed\n", pattern);
#endif
	goto error;
    }

    /*
     * for each node in the list evaluate the key and insert the node
     */
    if ((nodelist == NULL) || (nodelist->nodeNr <= 0))
	goto error;

    table = xsltNewKeyTable(keyd->name, keyd->nameURI);
    if (table == NULL)
	goto error;

    comp = xmlXPathCompile(keyd->use);
    if (comp == NULL)
	goto error;
    for (i = 0;i < nodelist->nodeNr;i++) {
	ctxt->node = nodelist->nodeTab[i];
	str = xsltEvalXPathString(ctxt, comp);
	if (str != NULL) {
#ifdef DEBUG_KEYS
	    xsltGenericDebug(xsltGenericDebugContext,
		 "xsl:key : node associated to(%s,%s)\n",
		             keyd->name, str);
#endif
	    keylist = xmlHashLookup(table->keys, str);
	    if (keylist == NULL) {
		keylist = xmlXPathNodeSetCreate(nodelist->nodeTab[i]);
		xmlHashAddEntry(table->keys, str, keylist);
	    } else {
		xmlXPathNodeSetAdd(keylist, nodelist->nodeTab[i]);
	    }
	    nodelist->nodeTab[i]->_private = keyd;
	    xmlFree(str);
#ifdef DEBUG_KEYS
	} else {
	    xsltGenericDebug(xsltGenericDebugContext,
		 "xsl:key : use %s failed to return a string\n",
		             keyd->use);
#endif
	}
    }
    xmlXPathFreeCompExpr(comp);

    table->next = doc->keys;
    doc->keys = table;

error:
    if (res != NULL)
	xmlXPathFreeObject(res);
    if (pattern != NULL)
	xmlFree(pattern);
}

/**
 * xsltInitCtxtKeys:
 * @ctxt:  an XSLT transformation context
 * @doc:  an XSLT document
 *
 * Computes all the keys tables for the current input document.
 * Should be done before global varibales are initialized.
 */
void
xsltInitCtxtKeys(xsltTransformContextPtr ctxt, xsltDocumentPtr doc) {
    xsltStylesheetPtr style;
    xsltKeyDefPtr keyd;

    if ((ctxt == NULL) || (doc == NULL))
	return;
#ifdef DEBUG_KEYS
    xsltGenericDebug(xsltGenericDebugContext, "Initializing keys on %s\n",
		     doc->doc->URL);
#endif
    style = ctxt->style;
    while (style != NULL) {
	keyd = (xsltKeyDefPtr) style->keys;
	while (keyd != NULL) {
	    xsltInitCtxtKey(ctxt, doc, keyd);

	    keyd = keyd->next;
	}

	style = xsltNextImport(style);
    }
}

/*
 * xsltIsKey:
 * @ctxt: an XSLT transformation context
 * @node: a node
 *
 * Returns 1 if the node is a Key, 0 otherwise
 */
int
xsltIsKey(xsltTransformContextPtr ctxt, xmlNodePtr node) {
    if (node == NULL)
	return(0);

    /*
     * TODO:
     * Something a bit more complex may be needed if we want
     * to be able to do similar things with IDs
     */
    if (node->_private != NULL)
	return(1);
    return(0);
}

/*
 * xsltFreeDocumentKeys:
 * @doc: a XSLT document
 *
 * Free the keys associated to a document
 */
void	
xsltFreeDocumentKeys(xsltDocumentPtr doc) {
    if (doc != NULL)
        xsltFreeKeyTableList(doc->keys);
}

