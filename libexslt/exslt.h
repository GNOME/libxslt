
#ifndef __EXSLT_H__
#define __EXSLT_H__

#include <libxml/tree.h>

extern const char *exsltLibraryVersion;
extern const int exsltLibexsltVersion;
extern const int exsltLibxsltVersion;
extern const int exsltLibxmlVersion;

#define EXSLT_COMMON_NAMESPACE ((const xmlChar *) "http://exslt.org/common")
#define EXSLT_MATH_NAMESPACE ((const xmlChar *) "http://exslt.org/math")
#define EXSLT_SETS_NAMESPACE ((const xmlChar *) "http://exslt.org/sets")
#define EXSLT_FUNCTIONS_NAMESPACE ((const xmlChar *) "http://exslt.org/functions")

void exsltCommonRegister (void);
void exsltMathRegister (void);
void exsltSetsRegister (void);
void exsltFuncRegister (void);

void exsltRegisterAll (void);

#endif /* __EXSLT_H__ */

