#ifndef __LIBXSLT_WIN32_CONFIG__
#define __LIBXSLT_WIN32_CONFIG__

#define HAVE_CTYPE_H
#define HAVE_STDLIB_H
#define HAVE_MALLOC_H
#define HAVE_TIME_H
#define HAVE_FCNTL_H

#include <io.h>

#ifndef LIBXSLT_DLL_IMPORT
#define LIBXSLT_DLL_IMPORT
#endif

#define HAVE_ISINF
#define HAVE_ISNAN

#include <math.h>
static int isinf (double d) {
    int expon = 0;
    double val = frexp (d, &expon);
    if (expon == 1025) {
        if (val == 0.5) {
            return 1;
        } else if (val == -0.5) {
            return -1;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
static int isnan (double d) {
    int expon = 0;
    double val = frexp (d, &expon);
    if (expon == 1025) {
        if (val == 0.5) {
            return 0;
        } else if (val == -0.5) {
            return 0;
        } else {
            return 1;
        }
    } else {
        return 0;
    }
}

#include <direct.h>

#define HAVE_SYS_STAT_H
#define HAVE__STAT

/* Microsoft's C runtime names all non-ANSI functions with a leading
   underscore. Since functionality is still the same, they can be used. */
#ifdef _MSC_VER
#include <libxml/xmlversion.h>
#ifndef WITH_TRIO
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#endif /* WITH_TRIO */
#endif /* _MSC_VER */


#ifndef ATTRIBUTE_UNUSED
#define ATTRIBUTE_UNUSED
#endif

#endif /* __LIBXSLT_WIN32_CONFIG__ */

