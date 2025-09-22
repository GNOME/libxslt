/*
 * xpath.h: Header for fuzz targets
 *
 * See Copyright for the status of this software.
 */

#ifndef __XML_XSLT_TESTS_FUZZ_H__
#define __XML_XSLT_TESTS_FUZZ_H__

#include <stddef.h>
#include <libxml/xmlstring.h>
#include <libxml/xpath.h>

#ifdef __GNUC__
#define ATTRIBUTE_UNUSED __attribute__((unused))
#else
#define ATTRIBUTE_UNUSED
#endif

int
LLVMFuzzerInitialize(int *argc, char ***argv);

int
LLVMFuzzerTestOneInput(const char *data, size_t size);

int
xsltFuzzXPathInit(void);

xmlXPathObjectPtr
xsltFuzzXPath(const char *data, size_t size);

void
xsltFuzzXPathFreeObject(xmlXPathObjectPtr obj);

void
xsltFuzzXPathCleanup(void);

int
xsltFuzzXsltInit(void);

xmlChar *
xsltFuzzXslt(const char *data, size_t size);

void
xsltFuzzXsltCleanup(void);

/* Utility functions */

void
xmlFuzzErrorFunc(void *ctx ATTRIBUTE_UNUSED, const char *msg ATTRIBUTE_UNUSED,
                 ...);

void
xmlFuzzMemSetup(void);

void
xmlFuzzMemSetLimit(size_t limit);

void
xmlFuzzDataInit(const char *data, size_t size);

void
xmlFuzzDataCleanup(void);

void
xmlFuzzWriteInt(FILE *out, size_t v, int size);

size_t
xmlFuzzReadInt(int size);

const char *
xmlFuzzReadRemaining(size_t *size);

void
xmlFuzzWriteString(FILE *out, const char *str);

const char *
xmlFuzzReadString(size_t *size);

void
xmlFuzzReadEntities(void);

const char *
xmlFuzzMainUrl(void);

const char *
xmlFuzzMainEntity(size_t *size);

const char *
xmlFuzzSecondaryUrl(void);

const char *
xmlFuzzSecondaryEntity(size_t *size);

xmlParserInputPtr
xmlFuzzEntityLoader(const char *URL, const char *ID, xmlParserCtxtPtr ctxt);

#endif
