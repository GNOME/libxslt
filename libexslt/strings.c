#define IN_LIBEXSLT
#include "libexslt/libexslt.h"

#if defined(WIN32) && !defined (__CYGWIN__)
#include <win32config.h>
#else
#include "config.h"
#endif

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/parser.h>
#include <libxml/encoding.h>

#include <libxslt/xsltconfig.h>
#include <libxslt/xsltutils.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/extensions.h>

#include "exslt.h"

/**
 * exsltStrTokenizeFunction:
 * @ctxt: an XPath parser context
 * @nargs: the number of arguments
 *
 * Splits up a string and returns a node set of token elements, each
 * containing one token from the string. 
 */
static void
exsltStrTokenizeFunction (xmlXPathParserContextPtr ctxt, int nargs) {
    xmlChar *str, *delimiters, *cur;
    const xmlChar *token, *delimiter;
    xmlNodePtr node;
    xmlDocPtr doc;
    xmlXPathObjectPtr ret;

    if ((nargs < 1) || (nargs > 2)) {
	xmlXPathSetArityError (ctxt);
	return;
    }

    if (nargs == 2) {
	delimiters = xmlXPathPopString (ctxt);
	if (xmlXPathCheckError(ctxt))
	    return;
    } else {
	delimiters = xmlStrdup((const xmlChar *) "\t\r\n ");
    }
    if (delimiters == NULL)
	return;

    str = xmlXPathPopString (ctxt);
    if (xmlXPathCheckError(ctxt) || (str == NULL)) {
	xmlFree (delimiters);
	return;
    }

    doc = xsltXPathGetTransformContext(ctxt)->document->doc;
    ret = xmlXPathNewNodeSet (NULL);
    if (ret != NULL) {
	ret->boolval = 1;
	/*
	 * This is a hack: token elements are added as children of a
	 * fake element node. This is necessary to free them up
	 * correctly when freeing the node-set.
	 */
	ret->user = (void *) xmlNewDocNode (doc, NULL,
				(const xmlChar *) "fake", NULL);
	if (ret->user == NULL)
	    goto error;
    }

    for (cur = str, token = str; *cur != 0; cur++) {
	for (delimiter = delimiters; *delimiter != 0; delimiter++) {
	    if (*cur == *delimiter) {
		if (cur == token) {
		    /* discard empty tokens */
		    break;
		}
		*cur = 0;
		node = xmlNewDocNode (doc, NULL,
				      (const xmlChar *) "token", token);
		*cur = *delimiter;
		token = cur + 1;

		xmlAddChild ((xmlNodePtr) ret->user, node);
		xmlXPathNodeSetAdd (ret->nodesetval, node);
		break;
	    }
	}
    }
    node = xmlNewDocNode (doc, NULL, (const xmlChar *) "token", token);
    xmlAddChild ((xmlNodePtr) ret->user, node);
    xmlXPathNodeSetAdd (ret->nodesetval, node);

    valuePush (ctxt, ret);
    ret = NULL;		/* hack to prevent freeing ret later */

error:
    if (ret != NULL)
	xmlXPathFreeObject (ret);
    if (str != NULL)
	xmlFree(str);
    if (delimiters != NULL)
	xmlFree(delimiters);
}

/**
 * exsltStrPaddingFunction:
 * @ctxt: an XPath parser context
 * @nargs: the number of arguments
 *
 * Creates a padding string of a certain length.
 */
static void
exsltStrPaddingFunction (xmlXPathParserContextPtr ctxt, int nargs) {
    int number, str_len = 0;
    xmlChar *str = NULL, *ret = NULL, *tmp;

    if ((nargs < 1) && (nargs > 2)) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    if (nargs == 2) {
	str = xmlXPathPopString(ctxt);
	str_len = xmlUTF8Strlen(str);
    }
    if (str_len == 0) {
	if (str != NULL) xmlFree(str);
	str = xmlStrdup((const xmlChar *) " ");
	str_len = 1;
    }

    number = (int) xmlXPathPopNumber(ctxt);

    if (number <= 0) {
	xmlXPathReturnEmptyString(ctxt);
	xmlFree(str);
	return;
    }

    while (number >= str_len) {
	ret = xmlStrncat(ret, str, str_len);
	number -= str_len;
    }
    tmp = xmlUTF8Strndup (str, number);
    ret = xmlStrcat(ret, tmp);
    if (tmp != NULL)
	xmlFree (tmp);

    xmlXPathReturnString(ctxt, ret);

    if (str != NULL)
	xmlFree(str);
}

/**
 * exsltStrAlignFunction:
 * @ctxt: an XPath parser context
 * @nargs: the number of arguments
 *
 * Aligns a string within another string.
 */
static void
exsltStrAlignFunction (xmlXPathParserContextPtr ctxt, int nargs) {
    xmlChar *str, *padding, *alignment, *ret;
    int str_l, padding_l;

    if ((nargs < 2) || (nargs > 3)) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    if (nargs == 3)
	alignment = xmlXPathPopString(ctxt);
    else
	alignment = NULL;

    padding = xmlXPathPopString(ctxt);
    str = xmlXPathPopString(ctxt);

    str_l = xmlUTF8Strlen (str);
    padding_l = xmlUTF8Strlen (padding);

    if (str_l == padding_l) {
	xmlXPathReturnString (ctxt, str);
	xmlFree(padding);
	xmlFree(alignment);
	return;
    }

    if (str_l > padding_l) {
	ret = xmlUTF8Strndup (str, padding_l);
    } else {
	if (xmlStrEqual(alignment, (const xmlChar *) "right")) {
	    ret = xmlUTF8Strndup (padding, padding_l - str_l);
	    ret = xmlStrcat (ret, str);
	} else if (xmlStrEqual(alignment, (const xmlChar *) "center")) {
	    int left = (padding_l - str_l) / 2;
	    int right_start;

	    ret = xmlUTF8Strndup (padding, left);
	    ret = xmlStrcat (ret, str);

	    right_start = xmlUTF8Strsize (padding, left + str_l);
	    ret = xmlStrcat (ret, padding + right_start);
	} else {
	    int str_s;

	    str_s = xmlStrlen (str);
	    ret = xmlStrdup (str);
	    ret = xmlStrcat (ret, padding + str_s);
	}
    }

    xmlXPathReturnString (ctxt, ret);

    xmlFree(str);
    xmlFree(padding);
    xmlFree(alignment);
}

/**
 * exsltStrConcatFunction:
 * @ctxt: an XPath parser context
 * @nargs: the number of arguments
 *
 * Takes a node set and returns the concatenation of the string values
 * of the nodes in that node set.  If the node set is empty, it
 * returns an empty string.
 */
static void
exsltStrConcatFunction (xmlXPathParserContextPtr ctxt, int nargs) {
    xmlXPathObjectPtr obj;
    xmlChar *ret = NULL;
    int i;

    if (nargs  != 1) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    if (!xmlXPathStackIsNodeSet(ctxt)) {
	xmlXPathSetTypeError(ctxt);
	return;
    }

    obj = valuePop (ctxt);

    if (xmlXPathNodeSetIsEmpty(obj->nodesetval)) {
	xmlXPathReturnEmptyString(ctxt);
	return;
    }

    for (i = 0; i < obj->nodesetval->nodeNr; i++) {
	xmlChar *tmp;
	tmp = xmlXPathCastNodeToString(obj->nodesetval->nodeTab[i]);

	ret = xmlStrcat (ret, tmp);

	xmlFree(tmp);
    }

    xmlXPathFreeObject (obj);

    xmlXPathReturnString(ctxt, ret);
}

/**
 * exsltStrRegister:
 *
 * Registers the EXSLT - Strings module
 */

void
exsltStrRegister (void) {
    xsltRegisterExtModuleFunction ((const xmlChar *) "tokenize",
				   EXSLT_STRINGS_NAMESPACE,
				   exsltStrTokenizeFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "padding",
				   EXSLT_STRINGS_NAMESPACE,
				   exsltStrPaddingFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "align",
				   EXSLT_STRINGS_NAMESPACE,
				   exsltStrAlignFunction);
    xsltRegisterExtModuleFunction ((const xmlChar *) "concat",
				   EXSLT_STRINGS_NAMESPACE,
				   exsltStrConcatFunction);
}
