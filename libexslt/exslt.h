
#ifndef __EXSLT_H__
#define __EXSLT_H__

#include <libxml/tree.h>

#define EXSLT_COMMON_NAMESPACE ((const xmlChar *) "http://exslt.org/common")
#define EXSLT_MATH_NAMESPACE ((const xmlChar *) "http://exslt.org/math")
#define EXSLT_SETS_NAMESPACE ((const xmlChar *) "http://exslt.org/sets")
#define EXSLT_FUNCTIONS_NAMESPACE ((const xmlChar *) "http://exslt.org/functions")

void exslCommonRegister (void);
void exslMathRegister (void);
void exslSetsRegister (void);
void exslFuncRegister (void);

void exslRegisterAll (void);

#endif /* __EXSLT_H__ */

