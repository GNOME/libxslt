/**
 * xsltICUSort.c: module provided by Richard Jinks to provide a
 *                sort function replacement using ICU, it is not
 *                included in standard due to the size of the ICU
 *                library
 *
 * See http://mail.gnome.org/archives/xslt/2002-November/msg00093.html
 *     http://oss.software.ibm.com/icu/index.html
 *
 * Copyright Richard Jinks
 */
#define IN_LIBXSLT
#include "libxslt.h"

#include <libxml/parserInternals.h>

#include "xslt.h"
#include "xsltInternals.h"
#include "xsltutils.h"
#include "transform.h"
#include "templates.h"

#include <unicode/ucnv.h>
#include <unicode/ustring.h>
#include <unicode/utypes.h>
#include <unicode/uloc.h>
#include <unicode/ucol.h>

/**
 * xsltICUSortFunction:
 * @ctxt:  a XSLT process context
 * @sorts:  array of sort nodes
 * @nbsorts:  the number of sorts in the array
 *
 * reorder the current node list accordingly to the set of sorting
 * requirement provided by the arry of nodes.
 * uses the ICU library
 */
void
xsltICUSortFunction(xsltTransformContextPtr ctxt, xmlNodePtr *sorts,
	           int nbsorts) {
    xmlXPathObjectPtr *resultsTab[XSLT_MAX_SORT];
    xmlXPathObjectPtr *results = NULL, *res;
    xmlNodeSetPtr list = NULL;
    int len = 0;
    int i, j, incr;
    int tst;
    int depth;
    xmlNodePtr node;
    xmlXPathObjectPtr tmp;
    const xsltStylePreComp *comp;
    int number[XSLT_MAX_SORT], desc[XSLT_MAX_SORT];

    /* Start ICU change */
    UCollator *coll = 0;
    UConverter *conv;
    UErrorCode status;
    UChar *target,*target2;
    int targetlen, target2len;
    /* End ICU change */

    if ((ctxt == NULL) || (sorts == NULL) || (nbsorts <= 0) ||
	(nbsorts >= XSLT_MAX_SORT))
	return;
    if (sorts[0] == NULL)
	return;
    comp = sorts[0]->_private;
    if (comp == NULL)
	return;

    list = ctxt->nodeList;
    if ((list == NULL) || (list->nodeNr <= 1))
	return; /* nothing to do */

    for (j = 0; j < nbsorts; j++) {
	comp = sorts[j]->_private;
	if ((comp->stype == NULL) && (comp->has_stype != 0)) {
	    xmlChar *stype =
		xsltEvalAttrValueTemplate(ctxt, sorts[j],
					  (const xmlChar *) "data-type",
					  XSLT_NAMESPACE);
	    number[j] = 0;
	    if (stype != NULL) {
		if (xmlStrEqual(stype, (const xmlChar *) "text"))
		    ;
		else if (xmlStrEqual(stype, (const xmlChar *) "number"))
		    number[j] = 1;
		else {
		    xsltTransformError(ctxt, NULL, sorts[j],
			  "xsltDoSortFunction: no support for data-type = %s\n",
			  stype);
		}
                xmlFree(stype);
	    }
        } else {
            number[j] = comp->number;
	}
	if ((comp->order == NULL) && (comp->has_order != 0)) {
	    xmlChar *order = xsltEvalAttrValueTemplate(ctxt, sorts[j],
						       BAD_CAST "order",
						       XSLT_NAMESPACE);
	    desc[j] = 0;
	    if (order != NULL) {
		if (xmlStrEqual(order, (const xmlChar *) "ascending"))
		    ;
		else if (xmlStrEqual(order, (const xmlChar *) "descending"))
		    desc[j] = 1;
		else {
		    xsltTransformError(ctxt, NULL, sorts[j],
			     "xsltDoSortFunction: invalid value %s for order\n",
			     order);
		}
                xmlFree(order);
	    }
        } else {
            desc[j] = comp->descending;
	}
    }

    len = list->nodeNr;

    resultsTab[0] = xsltComputeSortResult(ctxt, sorts[0]);
    for (i = 1;i < XSLT_MAX_SORT;i++)
	resultsTab[i] = NULL;

    results = resultsTab[0];

    comp = sorts[0]->_private;
    if (results == NULL)
	return;

    /* Start ICU change */
    status = U_ZERO_ERROR;
    conv = ucnv_open("UTF8", &status);
    if(U_FAILURE(status)) {
	xsltTransformError(ctxt, NULL, NULL, "xsltICUSortFunction: Error opening converter\n");
    }
    if(comp->has_lang)
	coll = ucol_open((const char *) comp->lang, &status);
    if(U_FAILURE(status) || !comp->has_lang) {
	status = U_ZERO_ERROR;
	coll = ucol_open("en", &status);
    }
    if(U_FAILURE(status)) {
	xsltTransformError(ctxt, NULL, NULL, "xsltICUSortFunction: Error opening collator\n");
    }
    if(comp->lower_first)
	ucol_setAttribute(coll,UCOL_CASE_FIRST,UCOL_LOWER_FIRST,&status);
    else
	ucol_setAttribute(coll,UCOL_CASE_FIRST,UCOL_UPPER_FIRST,&status);
    if(U_FAILURE(status)) {
	xsltTransformError(ctxt, NULL, NULL, "xsltICUSortFunction: Error setting collator attribute\n");
    }
    /* End ICU change */

    /* Shell's sort of node-set */
    for (incr = len / 2; incr > 0; incr /= 2) {
	for (i = incr; i < len; i++) {
	    j = i - incr;
	    if (results[i] == NULL)
		continue;

	    while (j >= 0) {
		if (results[j] == NULL)
		    tst = 1;
		else {
		    if (number[0]) {
			if (results[j]->floatval == results[j + incr]->floatval)
			    tst = 0;
			else if (results[j]->floatval >
				results[j + incr]->floatval)
			    tst = 1;
			else tst = -1;
		    } else {
/*			tst = xmlStrcmp(results[j]->stringval,
				     results[j + incr]->stringval); */
			/* Start ICU change */
			targetlen = xmlStrlen(results[j]->stringval) * 2;
			target2len = xmlStrlen(results[j + incr]->stringval) * 2;
			target = xmlMalloc(targetlen * sizeof(UChar));
			target2 = xmlMalloc(target2len * sizeof(UChar));
			targetlen = ucnv_toUChars(conv, target, targetlen,
                                                  (const char *) results[j]->stringval,
                                                  -1, &status);
			target2len = ucnv_toUChars(conv, target2, target2len,
                                                   (const char *) results[j+incr]->stringval,
                                                   -1, &status);
			tst = ucol_strcoll(coll, target, u_strlen(target), target2, u_strlen(target2));
			/* End ICU change */
		    }
		    if (desc[0])
			tst = -tst;
		}
		if (tst == 0) {
		    /*
		     * Okay we need to use multi level sorts
		     */
		    depth = 1;
		    while (depth < nbsorts) {
			if (sorts[depth] == NULL)
			    break;
			comp = sorts[depth]->_private;
			if (comp == NULL)
			    break;

			/*
			 * Compute the result of the next level for the
			 * full set, this might be optimized ... or not
			 */
			if (resultsTab[depth] == NULL)
			    resultsTab[depth] = xsltComputeSortResult(ctxt,
				                        sorts[depth]);
			res = resultsTab[depth];
			if (res == NULL)
			    break;
			if (res[j] == NULL)
			    tst = 1;
			else {
			    if (number[depth]) {
				if (res[j]->floatval == res[j + incr]->floatval)
				    tst = 0;
				else if (res[j]->floatval >
					res[j + incr]->floatval)
				    tst = 1;
				else tst = -1;
			    } else {
/*				tst = xmlStrcmp(res[j]->stringval,
					     res[j + incr]->stringval); */
				/* Start ICU change */
				targetlen = xmlStrlen(res[j]->stringval) * 2;
				target2len = xmlStrlen(res[j + incr]->stringval) * 2;
				target = xmlMalloc(targetlen * sizeof(UChar));
				target2 = xmlMalloc(target2len * sizeof(UChar));
				targetlen = ucnv_toUChars(conv, target, targetlen,
                                                          (const char *) res[j]->stringval,
                                                          -1, &status);
				target2len = ucnv_toUChars(conv, target2, target2len,
                                                           (const char *) res[j+incr]->stringval,
                                                           -1, &status);
				tst = ucol_strcoll(coll, target, u_strlen(target), target2, u_strlen(target2));
				/* End ICU change */
			    }
			    if (desc[depth])
			      tst = -tst;
			}
			/*
			 * if we still can't differenciate at this level
			 * try one level deeper.
			 */
			if (tst != 0)
			    break;
			depth++;
		    }
		}
		if (tst == 0) {
		    tst = results[j]->index > results[j + incr]->index;
		}
		if (tst > 0) {
		    tmp = results[j];
		    results[j] = results[j + incr];
		    results[j + incr] = tmp;
		    node = list->nodeTab[j];
		    list->nodeTab[j] = list->nodeTab[j + incr];
		    list->nodeTab[j + incr] = node;
		    depth = 1;
		    while (depth < nbsorts) {
			if (sorts[depth] == NULL)
			    break;
			if (resultsTab[depth] == NULL)
			    break;
			res = resultsTab[depth];
			tmp = res[j];
			res[j] = res[j + incr];
			res[j + incr] = tmp;
			depth++;
		    }
		    j -= incr;
		} else
		    break;
	    }
	}
    }

    /* Start ICU change */
    ucol_close(coll);
    ucnv_close(conv);
    /* End ICU change */

    for (j = 0; j < nbsorts; j++) {
	comp = sorts[j]->_private;
	if (resultsTab[j] != NULL) {
	    for (i = 0;i < len;i++)
		xmlXPathFreeObject(resultsTab[j][i]);
	    xmlFree(resultsTab[j]);
	}
    }
}

