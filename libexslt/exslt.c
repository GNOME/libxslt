#include <libxml/xmlversion.h>

#include <libxslt/xsltconfig.h>
#include <libxslt/extensions.h>

#include "exsltconfig.h"
#include "exslt.h"

const char *exsltLibraryVersion = LIBEXSLT_VERSION_STRING;
const int exsltLibexsltVersion = LIBEXSLT_VERSION;
const int exsltLibxsltVersion = LIBXSLT_VERSION;
const int exsltLibxmlVersion = LIBXML_VERSION;

/**
 * exsltRegisterAll:
 *
 * Registers all available EXSLT extensions
 */
void
exsltRegisterAll (void) {
    exsltCommonRegister();
    exsltMathRegister();
    exsltSetsRegister();
    exsltFuncRegister();
}

