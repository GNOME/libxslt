/*
 * xsltlocale.c: locale handling
 *
 * Reference:
 * RFC 3066: Tags for the Identification of Languages
 * http://www.ietf.org/rfc/rfc3066.txt
 * ISO 639-1, ISO 3166-1
 *
 * Author: Nick Wellnhofer
 */

#define IN_LIBXSLT
#include "libxslt.h"

#include <string.h>
#include <libxml/xmlmemory.h>

#include "xsltlocale.h"
#include "xsltutils.h"

#if defined(__GLIBC__) && __GLIBC__ == 2 && __GLIBC_MINOR__ <= 2
#define newlocale __newlocale
#define freelocale __freelocale
#define strxfrm_l __strxfrm_l
#define LC_COLLATE_MASK (1 << LC_COLLATE)
#endif

#define ISALPHA(c) ((c & 0xc0) == 0x40 && (unsigned)((c & 0x1f) - 1) < 26)
#define TOUPPER(c) (c & ~0x20)
#define TOLOWER(c) (c | 0x20)

/**
 * xsltNewLocale:
 * @languageTag: RFC 3066 language tag
 *
 * Creates a new locale of an opaque system dependent type based on the
 * language tag. Three-letter language codes (ISO 639-2 Alpha-3) are not
 * supported.
 *
 * Returns the locale or NULL on error or if no matching locale was found
 */
xsltLocale
xsltNewLocale(const xmlChar *languageTag) {
#ifdef XSLT_LOCALE_XLOCALE
    xsltLocale locale;
    char localeName[23]; /* 8*lang + "-" + 8*region + ".utf8\0" */
    const xmlChar *p = languageTag;
    const char *region = NULL;
    char *q = localeName;
    int c, i, llen;
    
    /* Convert something like "pt-br" to "pt_BR.utf8" */
    
    if (languageTag == NULL)
    	return(NULL);
    
    for (i=0; i<8 && ISALPHA(*p); ++i)
	*q++ = TOLOWER(*p++);
    
    if (i == 0)
    	return(NULL);
    
    llen = i;
    *q++ = '_';
    
    if (*p) {
    	if (*p++ != '-')
    	    return(NULL);
	
	for (i=0; i<8 && ISALPHA(*p); ++i)
	    *q++ = TOUPPER(*p++);
    
    	if (i == 0 || *p)
    	    return(NULL);
    	
        memcpy(q, ".utf8", 6);
        locale = newlocale(LC_COLLATE_MASK, localeName, NULL);
        if (locale != NULL)
            return(locale);
        
        q = localeName + llen + 1;
    }
    
    /* Try to find most common country for language */
    
    if (llen != 2)
        return(NULL);

    c = localeName[1];
    
    /* This is based on the locales from glibc 2.3.3 */
    
    switch (localeName[0]) {
        case 'a':
            if (c == 'a' || c == 'm') region = "ET";
            else if (c == 'f') region = "ZA";
            else if (c == 'n') region = "ES";
            else if (c == 'r') region = "AE";
            else if (c == 'z') region = "AZ";
            break;
        case 'b':
            if (c == 'e') region = "BY";
            else if (c == 'g') region = "BG";
            else if (c == 'n') region = "BD";
            else if (c == 'r') region = "FR";
            else if (c == 's') region = "BA";
            break;
        case 'c':
            if (c == 'a') region = "ES";
            else if (c == 's') region = "CZ";
            else if (c == 'y') region = "GB";
            break;
        case 'd':
            if (c == 'a') region = "DK";
            else if (c == 'e') region = "DE";
            break;
        case 'e':
            if (c == 'l') region = "GR";
            else if (c == 'n') region = "US";
            else if (c == 's' || c == 'u') region = "ES";
            else if (c == 't') region = "EE";
            break;
        case 'f':
            if (c == 'a') region = "IR";
            else if (c == 'i') region = "FI";
            else if (c == 'o') region = "FO";
            else if (c == 'r') region = "FR";
            break;
        case 'g':
            if (c == 'a') region = "IE";
            else if (c == 'l') region = "ES";
            else if (c == 'v') region = "GB";
            break;
        case 'h':
            if (c == 'e') region = "IL";
            else if (c == 'i') region = "IN";
            else if (c == 'r') region = "HT";
            else if (c == 'u') region = "HU";
            break;
        case 'i':
            if (c == 'd') region = "ID";
            else if (c == 's') region = "IS";
            else if (c == 't') region = "IT";
            else if (c == 'w') region = "IL";
            break;
        case 'j':
            if (c == 'a') region = "JP";
            break;
        case 'k':
            if (c == 'l') region = "GL";
            else if (c == 'o') region = "KR";
            else if (c == 'w') region = "GB";
            break;
        case 'l':
            if (c == 't') region = "LT";
            else if (c == 'v') region = "LV";
            break;
        case 'm':
            if (c == 'k') region = "MK";
            else if (c == 'l' || c == 'r') region = "IN";
            else if (c == 'n') region = "MN";
            else if (c == 's') region = "MY";
            else if (c == 't') region = "MT";
            break;
        case 'n':
            if (c == 'b' || c == 'n' || c == 'o') region = "NO";
            else if (c == 'e') region = "NP";
            else if (c == 'l') region = "NL";
            break;
        case 'o':
            if (c == 'm') region = "ET";
            break;
        case 'p':
            if (c == 'a') region = "IN";
            else if (c == 'l') region = "PL";
            else if (c == 't') region = "PT";
            break;
        case 'r':
            if (c == 'o') region = "RO";
            else if (c == 'u') region = "RU";
            break;
        case 's':
            switch (c) {
                case 'e': region = "NO"; break;
                case 'h': region = "YU"; break;
                case 'k': region = "SK"; break;
                case 'l': region = "SI"; break;
                case 'o': region = "ET"; break;
                case 'q': region = "AL"; break;
                case 't': region = "ZA"; break;
                case 'v': region = "SE"; break;
            }
            break;
        case 't':
            if (c == 'a' || c == 'e') region = "IN";
            else if (c == 'h') region = "TH";
            else if (c == 'i') region = "ER";
            else if (c == 'r') region = "TR";
            else if (c == 't') region = "RU";
            break;
        case 'u':
            if (c == 'k') region = "UA";
            else if (c == 'r') region = "PK";
            break;
        case 'v':
            if (c == 'i') region = "VN";
            break;
        case 'w':
            if (c == 'a') region = "BE";
            break;
        case 'x':
            if (c == 'h') region = "ZA";
            break;
        case 'z':
            if (c == 'h') region = "CN";
            else if (c == 'u') region = "ZA";
            break;
    }
    
    if (region == NULL)
        return(NULL);
    
    *q++ = region[0];
    *q++ = region[1];
    memcpy(q, ".utf8", 6);
    locale = newlocale(LC_COLLATE_MASK, localeName, NULL);
    
    return(locale);
#endif

#ifdef XSLT_LOCALE_MSVCRT
    const char *localeName = NULL;
    int c;
    
    /* We only look at the language and ignore the region. I think Windows
       doesn't care about the region for LC_COLLATE anyway. */
    
    if (languageTag == NULL ||
        !languageTag[0] ||
        !languageTag[1] ||
        languageTag[2] && languageTag[2] != '-')
    	return(NULL);
    
    c = TOLOWER(languageTag[1]);
    
    switch (TOLOWER(languageTag[0])) {
        case 'c':
            if (c == 's') localeName = "csy"; /* Czech */
            break;
        case 'd':
            if (c == 'a') localeName = "dan"; /* Danish */
            else if (c == 'e') localeName = "deu"; /* German */
            break;
        case 'e':
            if (c == 'l') localeName = "ell"; /* Greek */
            else if (c == 'n') localeName = "english";
            else if (c == 's') localeName = "esp"; /* Spanish */
            break;
        case 'f':
            if (c == 'i') localeName = "fin"; /* Finnish */
            else if (c == 'r') localeName = "fra"; /* French */
            break;
        case 'h':
            if (c == 'u') localeName = "hun"; /* Hungarian */
            break;
        case 'i':
            if (c == 's') localeName = "isl"; /* Icelandic */
            else if (c == 't') localeName = "ita"; /* Italian */
            break;
        case 'j':
            if (c == 'a') localeName = "jpn"; /* Japanese */
            break;
        case 'k':
            if (c == 'o') localeName = "kor"; /* Korean */
            break;
        case 'n':
            if (c == 'l') localeName = "nld"; /* Dutch */
            else if (c == 'o') localeName = "norwegian";
            break;
        case 'p':
            if (c == 'l') localeName = "plk"; /* Polish */
            else if (c == 't') localeName = "ptg"; /* Portuguese */
            break;
        case 'r':
            if (c == 'u') localeName = "rus"; /* Russian */
            break;
        case 's':
            if (c == 'k') localeName = "sky"; /* Slovak */
            else if (c == 'v') localeName = "sve"; /* Swedish */
            break;
        case 't':
            if (c == 'r') localeName = "trk"; /* Turkish */
            break;
        case 'z':
            if (c == 'h') localeName = "chinese";
            break;
    }
    
    if (localeName == NULL)
        return(NULL);

    return(_create_locale(LC_COLLATE, localeName));
#endif

#ifdef XSLT_LOCALE_NONE
    return(NULL);
#endif
}

/**
 * xsltFreeLocale:
 *
 * Frees a locale created with xsltNewLocale
 */
void
xsltFreeLocale(xsltLocale locale) {
#ifdef XSLT_LOCALE_XLOCALE
    freelocale(locale);
#endif

#ifdef XSLT_LOCALE_MSVCRT
    _free_locale(locale);
#endif
}

/**
 * xsltStrxfrm:
 * @locale: locale created with xsltNewLocale
 * @string: UTF-8 string to transform
 *
 * Transforms a string according to locale. The transformed string must then be
 * compared with xsltLocaleStrcmp and freed with xmlFree.
 *
 * Returns the transformed string or NULL on error
 */
xsltLocaleChar *
xsltStrxfrm(xsltLocale locale, const xmlChar *string)
{
#ifdef XSLT_LOCALE_NONE
    return(NULL);
#else
    size_t xstrlen, r;
    xsltLocaleChar *xstr;
    
#ifdef XSLT_LOCALE_XLOCALE
    xstrlen = strxfrm_l(NULL, (const char *)string, 0, locale) + 1;
    xstr = (xsltLocaleChar *) xmlMalloc(xstrlen);
    if (xstr == NULL) {
	xsltTransformError(NULL, NULL, NULL,
	    "xsltStrxfrm : out of memory error\n");
	return(NULL);
    }

    r = strxfrm_l((char *)xstr, (const char *)string, xstrlen, locale);
#endif

#ifdef XSLT_LOCALE_MSVCRT
    wchar_t *wcs;
    wchar_t dummy;
    int wcslen;
    int i, j;
    
    /* convert UTF8 to Windows wide chars (UTF16) */
    
    wcslen = xmlUTF8Strlen(string);
    if (wcslen < 0) {
	xsltTransformError(NULL, NULL, NULL,
	    "xsltStrxfrm : invalid UTF-8 string\n");
        return(NULL);
    }
    wcs = (wchar_t *) xmlMalloc(sizeof(wchar_t) * (wcslen + 1));
    if (wcs == NULL) {
	xsltTransformError(NULL, NULL, NULL,
	    "xsltStrxfrm : out of memory error\n");
	return(NULL);
    }

    for (i=0, j=0; i<wcslen; ++i) {
        int len = 4; /* not really, but string is already checked */
        int c = xmlGetUTF8Char(string, &len);
#if 0        
        if (c < 0) {
	    xsltTransformError(NULL, NULL, NULL,
	        "xsltStrxfrm : invalid UTF-8 string\n");
            xmlFree(wcs);
            return(NULL);
        }
#endif

        if (c == (wchar_t)c) {
            wcs[j] = (wchar_t)c;
            ++j;
        }
        
        string += len;
    }
    
    wcs[j] = 0;
    
    /* _wcsxfrm_l needs a dummy strDest because it always writes at least one
       terminating zero wchar */
    xstrlen = _wcsxfrm_l(&dummy, wcs, 0, locale);
    if (xstrlen == INT_MAX) {
	xsltTransformError(NULL, NULL, NULL, "xsltStrxfrm : strxfrm failed\n");
        xmlFree(wcs);
        return(NULL);
    }
    ++xstrlen;
    xstr = (wchar_t *) xmlMalloc(sizeof(wchar_t) * xstrlen);
    if (xstr == NULL) {
	xsltTransformError(NULL, NULL, NULL,
	    "xsltStrxfrm : out of memory error\n");
        xmlFree(wcs);
	return(NULL);
    }

    r = _wcsxfrm_l(xstr, wcs, xstrlen, locale);
    
    xmlFree(wcs);
#endif /* XSLT_LOCALE_MSVCRT */
    
    if (r >= xstrlen) {
	xsltTransformError(NULL, NULL, NULL, "xsltStrxfrm : strxfrm failed\n");
        xmlFree(xstr);
        return(NULL);
    }

    return(xstr);
#endif /* XSLT_LOCALE_NONE */
}

/**
 * xsltLocaleStrcmp:
 * @str1: a string transformed with xsltStrxfrm
 * @str2: a string transformed with xsltStrxfrm
 *
 * Compares two strings transformed with xsltStrxfrm
 *
 * Returns a value < 0 if str1 sorts before str2,
 *         a value > 0 if str1 sorts after str2,
 *         0 if str1 and str2 are equal wrt sorting
 */
int
xsltLocaleStrcmp(const xsltLocaleChar *str1, const xsltLocaleChar *str2) {
#ifdef XSLT_LOCALE_MSVCRT
    if (str1 == str2) return(0);
    if (str1 == NULL) return(-1);
    if (str2 == NULL) return(1);
    return(wcscmp(str1, str2));
#else
    return(xmlStrcmp(str1, str2));
#endif
}
