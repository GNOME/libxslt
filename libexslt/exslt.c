
#include <libxslt/extensions.h>

#include "exslt.h"

/**
 * exslRegisterAll:
 *
 * Registers all available EXSLT extensions
 */
void
exslRegisterAll (void) {
    exslCommonRegister();
    exslMathRegister();
    exslSetsRegister();
    exslFuncRegister();
}

