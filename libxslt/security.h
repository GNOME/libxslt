/*
 * security.h: interface for the XSLT security framework
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#ifndef __XML_XSLT_SECURITY_H__
#define __XML_XSLT_SECURITY_H__

#include <libxml/tree.h>
#include "xsltInternals.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * xsltSecurityPref:
 *
 * structure to indicate the preferences for security in the XSLT
 * transformation.
 */
typedef struct _xsltSecurityPrefs xsltSecurityPrefs;
typedef xsltSecurityPrefs *xsltSecurityPrefsPtr;

/**
 * xsltSecurityOption:
 *
 * the set of option that can be configured
 */
typedef enum {
    XSLT_SECPREF_READ_FILE = 1,
    XSLT_SECPREF_WRITE_FILE,
    XSLT_SECPREF_CREATE_DIRECTORY,
    XSLT_SECPREF_READ_NETWORK,
    XSLT_SECPREF_WRITE_NETWORK
} xsltSecurityOption;

/**
 * xsltSecurityCheck:
 *
 * User provided function to check the value of a string like a file
 * path or an URL ...
 */
typedef int (*xsltSecurityCheck)	(xsltSecurityPrefsPtr sec,
					 xsltTransformContextPtr ctxt,
					 const char *value);

/*
 * Module interfaces
 */
xsltSecurityPrefsPtr	xsltNewSecurityPrefs	(void);
void			xsltFreeSecurityPrefs	(xsltSecurityPrefsPtr sec);
int			xsltSetSecurityPrefs	(xsltSecurityPrefsPtr sec,
						 xsltSecurityOption option,
						 xsltSecurityCheck func);
xsltSecurityCheck	xsltGetSecurityPrefs	(xsltSecurityPrefsPtr sec,
						 xsltSecurityOption option);

void			xsltSetDefaultSecurityPrefs(xsltSecurityPrefsPtr sec);
xsltSecurityPrefsPtr	xsltGetDefaultSecurityPrefs(void);

int			xsltSetCtxtSecurityPrefs(xsltSecurityPrefsPtr sec,
						 xsltTransformContextPtr ctxt);

int			xsltSecurityAllow	(xsltSecurityPrefsPtr sec,
						 xsltTransformContextPtr ctxt,
						 const char *value);
int			xsltSecurityForbid	(xsltSecurityPrefsPtr sec,
						 xsltTransformContextPtr ctxt,
						 const char *value);
/*
 * internal interfaces
 */
int			xsltCheckWrite		(xsltSecurityPrefsPtr sec,
						 xsltTransformContextPtr ctxt,
						 const xmlChar *URL);
int			xsltCheckRead		(xsltSecurityPrefsPtr sec,
						 xsltTransformContextPtr ctxt,
						 const xmlChar *URL);

#ifdef __cplusplus
}
#endif

#endif /* __XML_XSLT_SECURITY_H__ */

