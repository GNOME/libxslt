
#ifndef __EXSLT_H__
#define __EXSLT_H__

#include <libxml/tree.h>
#include "exsltconfig.h"

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

void exsltCommonRegister (void);
void exsltMathRegister (void);
void exsltSetsRegister (void);
void exsltFuncRegister (void);
void exsltStrRegister (void);
void exsltDateRegister (void);
void exsltSaxonRegister (void);

void exsltRegisterAll (void);

#endif /* __EXSLT_H__ */

