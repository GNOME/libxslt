
#ifndef __EXSLT_H__
#define __EXSLT_H__

#include <libxml/tree.h>
#include "exsltconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBEXSLT_PUBLIC extern const char *exsltLibraryVersion;
LIBEXSLT_PUBLIC extern const int exsltLibexsltVersion;
LIBEXSLT_PUBLIC extern const int exsltLibxsltVersion;
LIBEXSLT_PUBLIC extern const int exsltLibxmlVersion;

#define EXSLT_COMMON_NAMESPACE ((const xmlChar *) "http://exslt.org/common")
#define EXSLT_MATH_NAMESPACE ((const xmlChar *) "http://exslt.org/math")
#define EXSLT_SETS_NAMESPACE ((const xmlChar *) "http://exslt.org/sets")
#define EXSLT_FUNCTIONS_NAMESPACE ((const xmlChar *) "http://exslt.org/functions")
#define EXSLT_STRINGS_NAMESPACE ((const xmlChar *) "http://exslt.org/strings")
#define EXSLT_DATE_NAMESPACE ((const xmlChar *) "http://exslt.org/dates-and-times")
#define SAXON_NAMESPACE ((const xmlChar *) "http://icl.com/saxon")
#define EXSLT_DYNAMIC_NAMESPACE ((const xmlChar *) "http://exslt.org/dynamic")

void LIBEXSLT_PUBLIC exsltCommonRegister (void);
void LIBEXSLT_PUBLIC exsltMathRegister (void);
void LIBEXSLT_PUBLIC exsltSetsRegister (void);
void LIBEXSLT_PUBLIC exsltFuncRegister (void);
void LIBEXSLT_PUBLIC exsltStrRegister (void);
void LIBEXSLT_PUBLIC exsltDateRegister (void);
void LIBEXSLT_PUBLIC exsltSaxonRegister (void);
void LIBEXSLT_PUBLIC exsltDynRegister(void);

void LIBEXSLT_PUBLIC exsltRegisterAll (void);

#ifdef __cplusplus
}
#endif
#endif /* __EXSLT_H__ */

