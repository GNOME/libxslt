/**
 * xsltICUSort.c: module to provide a sort function replacement using ICU,
 *                it is not included in standard due to the size of the ICU
 *                library
 *
 * Requires libxslt 1.1.38
 */

#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <unicode/ucnv.h>
#include <unicode/ucol.h>

/**
 * xsltICUNewLocale:
 * @lang:  lang
 *
 * Create a new ICU collator.
 */
void *
xsltICUNewLocale(const xmlChar *lang, int lowerFirst) {
    UCollator *coll;
    UErrorCode status = U_ZERO_ERROR;

    coll = ucol_open((const char *) lang, &status);
    if (U_FAILURE(status)) {
	status = U_ZERO_ERROR;
	coll = ucol_open("en", &status);
        if (U_FAILURE(status)) {
            return NULL;
        }
    }

    if (lowerFirst)
	ucol_setAttribute(coll, UCOL_CASE_FIRST, UCOL_LOWER_FIRST, &status);
    else
	ucol_setAttribute(coll, UCOL_CASE_FIRST, UCOL_UPPER_FIRST, &status);

    return (void *) coll;
}

/**
 * xsltICUNewLocale:
 * @locale:  ICU collator
 *
 * Free the ICU collator.
 */
void
xsltICUFreeLocale(void *coll) {
    ucol_close((UCollator *) coll);
}

/**
 * xsltICUGenSortKey:
 * @locale:  ICU collator
 * @str:  source string
 *
 * Generate a localized sort key.
 */
xmlChar *
xsltICUGenSortKey(void *coll, const xmlChar *str) {
    UConverter *conv;
    UChar *ustr = NULL;
    xmlChar *result = NULL;
    UErrorCode status = U_ZERO_ERROR;
    int32_t ustrLen, resultLen;

    conv = ucnv_open("UTF8", &status);
    if (U_FAILURE(status))
        goto error;

    ustrLen = ucnv_toUChars(conv, NULL, 0, (const char *) str, -1, &status);
    if (U_FAILURE(status) && status != U_BUFFER_OVERFLOW_ERROR)
        goto error;
    status = U_ZERO_ERROR;
    ustr = (UChar *) xmlMalloc((ustrLen + 1) * sizeof(UChar));
    if (ustr == NULL)
        goto error;
    ustrLen = ucnv_toUChars(conv, ustr, ustrLen, (const char *) str, -1,
                            &status);
    if (U_FAILURE(status))
        goto error;

    resultLen = ucol_getSortKey((UCollator *) coll, ustr, ustrLen, NULL, 0);
    if (resultLen == 0)
        goto error;
    result = (xmlChar *) xmlMalloc(resultLen);
    if (result == NULL)
        goto error;
    resultLen = ucol_getSortKey((UCollator *) coll, ustr, ustrLen, result,
                                resultLen);
    if (resultLen == 0) {
        xmlFree(result);
        result = NULL;
        goto error;
    }

error:
    xmlFree(ustr);
    return result;
}

int main(void) {
    xmlDocPtr sourceDoc = xmlReadDoc(
        BAD_CAST
        "<d>\n"
        "  <e>Berta</e>\n"
        "  <e>Ärger</e>\n"
        "</d>\n",
        NULL, NULL, 0
    );
    xmlDocPtr styleDoc = xmlReadDoc(
        BAD_CAST
        "<xsl:stylesheet"
        " version='1.0'"
        " xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>\n"
        "  <xsl:template match='d'>\n"
        "    <xsl:for-each select='*'>\n"
        "      <xsl:sort lang='de'/>\n"
        "      <xsl:copy-of select='.'/>\n"
        "    </xsl:for-each>\n"
        "  </xsl:template>\n"
        "</xsl:stylesheet>\n",
        NULL, NULL, 0
    );
    xsltStylesheetPtr style = xsltParseStylesheetDoc(styleDoc);
    if (style == NULL)
        xmlFreeDoc(styleDoc);

    xsltTransformContextPtr tctxt = xsltNewTransformContext(style, sourceDoc);
    xsltSetCtxtLocaleHandlers(tctxt, xsltICUNewLocale, xsltICUFreeLocale,
                              xsltICUGenSortKey);

    xmlDocPtr resultDoc = xsltApplyStylesheetUser(style, sourceDoc, NULL, NULL,
                                                  NULL, tctxt);

    xsltFreeTransformContext(tctxt);

    xsltSaveResultToFile(stdout, resultDoc, style);

    xmlFreeDoc(resultDoc);
    xsltFreeStylesheet(style);
    xmlFreeDoc(sourceDoc);

    return 0;
}
