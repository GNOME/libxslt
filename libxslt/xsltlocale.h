/*
 * Summary: Locale handling
 * Description: Interfaces for locale handling. Needed for language dependent
 *              sorting.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Nick Wellnhofer
 */

#ifndef __XML_XSLTLOCALE_H__
#define __XML_XSLTLOCALE_H__

#include <libxml/xmlstring.h>

#ifdef HAVE_XLOCALE_H

#define XSLT_LOCALE_XLOCALE

#include <locale.h>
#include <xlocale.h>

#if defined(__GLIBC__) && __GLIBC__ == 2 && __GLIBC_MINOR__ <= 2
typedef __locale_t xsltLocale;
#else
typedef locale_t xsltLocale;
#endif
typedef xmlChar xsltLocaleChar;

#else
#if defined(_MSC_VER) || defined (__MINGW32__) && defined(__MSVCRT__)

#define XSLT_LOCALE_MSVCRT

#include <locale.h>

typedef _locale_t xsltLocale;
typedef wchar_t xsltLocaleChar;

#else

#define XSLT_LOCALE_NONE

typedef void *xsltLocale;
typedef xmlChar xsltLocaleChar;

#endif
#endif

xsltLocale xsltNewLocale(const xmlChar *langName);
void xsltFreeLocale(xsltLocale locale);
xsltLocaleChar *xsltStrxfrm(xsltLocale locale, const xmlChar *string);
int xsltLocaleStrcmp(const xsltLocaleChar *str1, const xsltLocaleChar *str2);

#endif /* __XML_XSLTLOCALE_H__ */
