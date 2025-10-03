/*
 * runtest.c: libxslt test suite
 *
 * See Copyright for the status of this software.
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <io.h>
  #include <direct.h>
#else
  #include <unistd.h>
  #include <glob.h>
#endif

#include <libxml/parser.h>
#include <libxslt/extensions.h>
#include <libxslt/transform.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/xsltlocale.h>
#include <libxslt/xsltutils.h>
#include <libexslt/exslt.h>

/*
 * O_BINARY is just for Windows compatibility - if it isn't defined
 * on this system, avoid any compilation error
 */
#ifdef	O_BINARY
#define RD_FLAGS	O_RDONLY | O_BINARY
#define WR_FLAGS	O_WRONLY | O_CREAT | O_TRUNC | O_BINARY
#else
#define RD_FLAGS	O_RDONLY
#define WR_FLAGS	O_WRONLY | O_CREAT | O_TRUNC
#endif

typedef int (*functest) (const char *filename, int options);

typedef struct testDesc testDesc;
typedef testDesc *testDescPtr;
struct testDesc {
    const char *desc; /* description of the test */
    functest    func; /* function implementing the test */
    const char *dir;  /* directory to change to */
    const char *in;   /* glob to path for input files */
    int     options;  /* parser options for the test */
};

static int update_results = 0;
static char* temp_directory = NULL;
static int checkTestFile(const char *filename);

#if defined(_WIN32)

typedef struct
{
      size_t gl_pathc;    /* Count of paths matched so far  */
      char **gl_pathv;    /* List of matched pathnames.  */
      size_t gl_offs;     /* Slots to reserve in 'gl_pathv'.  */
} glob_t;

#define GLOB_DOOFFS 0
static int glob(const char *pattern, ATTRIBUTE_UNUSED int flags,
                ATTRIBUTE_UNUSED int errfunc(const char *epath, int eerrno),
                glob_t *pglob) {
    glob_t *ret;
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind;
    unsigned int nb_paths = 0;
    char directory[500];
    int len;

    if ((pattern == NULL) || (pglob == NULL)) return(-1);

    strncpy(directory, pattern, 499);
    for (len = strlen(directory);len >= 0;len--) {
        if (directory[len] == '/') {
	    len++;
	    directory[len] = 0;
	    break;
	}
    }
    if (len <= 0)
        len = 0;


    ret = pglob;
    memset(ret, 0, sizeof(glob_t));

    hFind = FindFirstFileA(pattern, &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE)
        return(0);
    nb_paths = 20;
    ret->gl_pathv = (char **) malloc(nb_paths * sizeof(char *));
    if (ret->gl_pathv == NULL) {
	FindClose(hFind);
        return(-1);
    }
    strncpy(directory + len, FindFileData.cFileName, 499 - len);
    ret->gl_pathv[ret->gl_pathc] = strdup(directory);
    if (ret->gl_pathv[ret->gl_pathc] == NULL)
        goto done;
    ret->gl_pathc++;
    while(FindNextFileA(hFind, &FindFileData)) {
        if (FindFileData.cFileName[0] == '.')
	    continue;
        if (ret->gl_pathc + 2 > nb_paths) {
            char **tmp = realloc(ret->gl_pathv, nb_paths * 2 * sizeof(char *));
            if (tmp == NULL)
                break;
            ret->gl_pathv = tmp;
            nb_paths *= 2;
	}
	strncpy(directory + len, FindFileData.cFileName, 499 - len);
	ret->gl_pathv[ret->gl_pathc] = strdup(directory);
        if (ret->gl_pathv[ret->gl_pathc] == NULL)
            break;
        ret->gl_pathc++;
    }
    ret->gl_pathv[ret->gl_pathc] = NULL;

done:
    FindClose(hFind);
    return(0);
}



static void globfree(glob_t *pglob) {
    unsigned int i;
    if (pglob == NULL)
        return;

    for (i = 0;i < pglob->gl_pathc;i++) {
         if (pglob->gl_pathv[i] != NULL)
             free(pglob->gl_pathv[i]);
    }
}

#endif

/************************************************************************
 *									*
 *		Libxml2 specific routines				*
 *									*
 ************************************************************************/

static int nb_tests = 0;
static int nb_errors = 0;

static int
fatalError(void) {
    fprintf(stderr, "Exiting tests on fatal error\n");
    exit(1);
}

/*
 * Trapping the error messages at the generic level to grab the equivalent of
 * stderr messages on CLI tools.
 */
static char testErrors[32769];
static int testErrorsSize = 0;

static void XMLCDECL
testErrorHandler(void *ctx  ATTRIBUTE_UNUSED, const char *msg, ...) {
    va_list args;
    int res;

    if (testErrorsSize >= 32768)
        return;
    va_start(args, msg);
    res = vsnprintf(&testErrors[testErrorsSize],
                    32768 - testErrorsSize,
		    msg, args);
    va_end(args);
    if (testErrorsSize + res >= 32768) {
        /* buffer is full */
	testErrorsSize = 32768;
	testErrors[testErrorsSize] = 0;
    } else {
        testErrorsSize += res;
    }
    testErrors[testErrorsSize] = 0;
}

#if LIBXML_VERSION < 21300

/**
 * xmlParserPrintFileContext:
 * @input:  an xmlParserInputPtr input
 *
 * Displays current context within the input content for error tracking
 */

static void
xmlParserPrintFileContextInternal(xmlParserInputPtr input ,
		xmlGenericErrorFunc chanl, void *data ) {
    const xmlChar *cur, *base;
    unsigned int n, col;	/* GCC warns if signed, because compared with sizeof() */
    xmlChar  content[81]; /* space for 80 chars + line terminator */
    xmlChar *ctnt;

    if (input == NULL) return;
    cur = input->cur;
    base = input->base;
    /* skip backwards over any end-of-lines */
    while ((cur > base) && ((*(cur) == '\n') || (*(cur) == '\r'))) {
	cur--;
    }
    n = 0;
    /* search backwards for beginning-of-line (to max buff size) */
    while ((n++ < (sizeof(content)-1)) && (cur > base) &&
   (*(cur) != '\n') && (*(cur) != '\r'))
        cur--;
    if ((*(cur) == '\n') || (*(cur) == '\r')) cur++;
    /* calculate the error position in terms of the current position */
    col = input->cur - cur;
    /* search forward for end-of-line (to max buff size) */
    n = 0;
    ctnt = content;
    /* copy selected text to our buffer */
    while ((*cur != 0) && (*(cur) != '\n') &&
   (*(cur) != '\r') && (n < sizeof(content)-1)) {
		*ctnt++ = *cur++;
	n++;
    }
    *ctnt = 0;
    /* print out the selected text */
    chanl(data ,"%s\n", content);
    /* create blank line with problem pointer */
    n = 0;
    ctnt = content;
    /* (leave buffer space for pointer + line terminator) */
    while ((n<col) && (n++ < sizeof(content)-2) && (*ctnt != 0)) {
	if (*(ctnt) != '\t')
	    *(ctnt) = ' ';
	ctnt++;
    }
    *ctnt++ = '^';
    *ctnt = 0;
    chanl(data ,"%s\n", content);
}

static void
testStructuredErrorHandler(void *ctx ATTRIBUTE_UNUSED, const xmlError *err) {
    char *file = NULL;
    int line = 0;
    int code = -1;
    int domain;
    void *data = NULL;
    const char *str;
    const xmlChar *name = NULL;
    xmlNodePtr node;
    xmlErrorLevel level;
    xmlParserInputPtr input = NULL;
    xmlParserInputPtr cur = NULL;
    xmlParserCtxtPtr ctxt = NULL;

    if (err == NULL)
        return;

    file = err->file;
    line = err->line;
    code = err->code;
    domain = err->domain;
    level = err->level;
    node = err->node;
    if ((domain == XML_FROM_PARSER) || (domain == XML_FROM_HTML) ||
        (domain == XML_FROM_DTD) || (domain == XML_FROM_NAMESPACE) ||
	(domain == XML_FROM_IO) || (domain == XML_FROM_VALID)) {
	ctxt = err->ctxt;
    }
    str = err->message;

    if (code == XML_ERR_OK)
        return;

    if ((node != NULL) && (node->type == XML_ELEMENT_NODE))
        name = node->name;

    /*
     * Maintain the compatibility with the legacy error handling
     */
    if (ctxt != NULL) {
        input = ctxt->input;
        if ((input != NULL) && (input->filename == NULL) &&
            (ctxt->inputNr > 1)) {
            cur = input;
            input = ctxt->inputTab[ctxt->inputNr - 2];
        }
        if (input != NULL) {
            if (input->filename)
                testErrorHandler(data, "%s:%d: ", input->filename, input->line);
            else if ((line != 0) && (domain == XML_FROM_PARSER))
                testErrorHandler(data, "Entity: line %d: ", input->line);
        }
    } else {
        if (file != NULL)
            testErrorHandler(data, "%s:%d: ", file, line);
        else if ((line != 0) && (domain == XML_FROM_PARSER))
            testErrorHandler(data, "Entity: line %d: ", line);
    }
    if (name != NULL) {
        testErrorHandler(data, "element %s: ", name);
    }
    if (code == XML_ERR_OK)
        return;
    switch (domain) {
        case XML_FROM_PARSER:
            testErrorHandler(data, "parser ");
            break;
        case XML_FROM_NAMESPACE:
            testErrorHandler(data, "namespace ");
            break;
        case XML_FROM_DTD:
        case XML_FROM_VALID:
            testErrorHandler(data, "validity ");
            break;
        case XML_FROM_HTML:
            testErrorHandler(data, "HTML parser ");
            break;
        case XML_FROM_MEMORY:
            testErrorHandler(data, "memory ");
            break;
        case XML_FROM_OUTPUT:
            testErrorHandler(data, "output ");
            break;
        case XML_FROM_IO:
            testErrorHandler(data, "I/O ");
            break;
        case XML_FROM_XINCLUDE:
            testErrorHandler(data, "XInclude ");
            break;
        case XML_FROM_XPATH:
            testErrorHandler(data, "XPath ");
            break;
        case XML_FROM_XPOINTER:
            testErrorHandler(data, "parser ");
            break;
        case XML_FROM_REGEXP:
            testErrorHandler(data, "regexp ");
            break;
        case XML_FROM_MODULE:
            testErrorHandler(data, "module ");
            break;
        case XML_FROM_SCHEMASV:
            testErrorHandler(data, "Schemas validity ");
            break;
        case XML_FROM_SCHEMASP:
            testErrorHandler(data, "Schemas parser ");
            break;
        case XML_FROM_RELAXNGP:
            testErrorHandler(data, "Relax-NG parser ");
            break;
        case XML_FROM_RELAXNGV:
            testErrorHandler(data, "Relax-NG validity ");
            break;
        case XML_FROM_CATALOG:
            testErrorHandler(data, "Catalog ");
            break;
        case XML_FROM_C14N:
            testErrorHandler(data, "C14N ");
            break;
        case XML_FROM_XSLT:
            testErrorHandler(data, "XSLT ");
            break;
        default:
            break;
    }
    if (code == XML_ERR_OK)
        return;
    switch (level) {
        case XML_ERR_NONE:
            testErrorHandler(data, ": ");
            break;
        case XML_ERR_WARNING:
            testErrorHandler(data, "warning : ");
            break;
        case XML_ERR_ERROR:
            testErrorHandler(data, "error : ");
            break;
        case XML_ERR_FATAL:
            testErrorHandler(data, "error : ");
            break;
    }
    if (code == XML_ERR_OK)
        return;
    if (str != NULL) {
        int len;
	len = xmlStrlen((const xmlChar *)str);
	if ((len > 0) && (str[len - 1] != '\n'))
	    testErrorHandler(data, "%s\n", str);
	else
	    testErrorHandler(data, "%s", str);
    } else {
        testErrorHandler(data, "%s\n", "out of memory error");
    }
    if (code == XML_ERR_OK)
        return;

    if (ctxt != NULL) {
        xmlParserPrintFileContextInternal(input, testErrorHandler, data);
        if (cur != NULL) {
            if (cur->filename)
                testErrorHandler(data, "%s:%d: \n", cur->filename, cur->line);
            else if ((line != 0) && (domain == XML_FROM_PARSER))
                testErrorHandler(data, "Entity: line %d: \n", cur->line);
            xmlParserPrintFileContextInternal(cur, testErrorHandler, data);
        }
    }
    if ((domain == XML_FROM_XPATH) && (err->str1 != NULL) &&
        (err->int1 < 100) &&
	(err->int1 < xmlStrlen((const xmlChar *)err->str1))) {
	xmlChar buf[150];
	int i;

	testErrorHandler(data, "%s\n", err->str1);
	for (i=0;i < err->int1;i++)
	     buf[i] = ' ';
	buf[i++] = '^';
	buf[i] = 0;
	testErrorHandler(data, "%s\n", buf);
    }
}

#else /* LIBXML_VERSION */

static void
testStructuredErrorHandler(void *ctx ATTRIBUTE_UNUSED, const xmlError *err) {
    xmlFormatError(err, testErrorHandler, NULL);
}

#endif /* LIBXML_VERSION */

static void
initializeLibxml2(void) {
    xmlInitParser();
    /* TODO: Update this code for the thread safe versions based on
     * the xmlCtxtSetResourceLoader function
     */
    xmlSetExternalEntityLoader(xmlGetExternalEntityLoader());
    xmlSetGenericErrorFunc(NULL, testErrorHandler);
    xsltSetGenericErrorFunc(NULL, testErrorHandler);
    xmlSetStructuredErrorFunc(NULL,
            (xmlStructuredErrorFunc) testStructuredErrorHandler);
    exsltRegisterAll();
    xsltRegisterTestModule();
    xsltMaxDepth = 200;
}


/************************************************************************
 *									*
 *		File name and path utilities				*
 *									*
 ************************************************************************/

static char *
changeSuffix(const char *filename, const char *suffix) {
    const char *dot;
    char *ret;
    char res[500];
    int baseLen;

    dot = strrchr(filename, '.');
    baseLen = dot ? dot - filename : (int) strlen(filename);
    snprintf(res, sizeof(res), "%.*s%s", baseLen, filename, suffix);

    ret = strdup(res);
    if (ret == NULL) {
        fprintf(stderr, "strdup failed\n");
        fatalError();
    }

    return(ret);
}

static int
checkTestFile(const char *filename) {
    struct stat buf;

    if (stat(filename, &buf) == -1)
        return(0);

#if defined(_WIN32)
    if (!(buf.st_mode & _S_IFREG))
        return(0);
#else
    if (!S_ISREG(buf.st_mode))
        return(0);
#endif

    return(1);
}

static int compareFileMem(const char *filename, const char *mem, int size) {
    int res;
    int fd;
    char bytes[4096];
    int idx = 0;
    struct stat info;

    if (update_results) {
        if (size == 0) {
            unlink(filename);
            return(0);
        }
        fd = open(filename, WR_FLAGS, 0644);
        if (fd < 0) {
	    fprintf(stderr, "failed to open %s for writing", filename);
            return(-1);
	}
        res = write(fd, mem, size);
        close(fd);
        return(res != size);
    }

    if (stat(filename, &info) < 0) {
        if (size == 0)
            return(0);
        fprintf(stderr, "failed to stat %s\n", filename);
	return(-1);
    }
    if (info.st_size != size) {
        fprintf(stderr, "file %s is %ld bytes, result is %d bytes\n",
	        filename, (long) info.st_size, size);
        return(-1);
    }
    fd = open(filename, RD_FLAGS);
    if (fd < 0) {
	fprintf(stderr, "failed to open %s for reading", filename);
        return(-1);
    }
    while (idx < size) {
        res = read(fd, bytes, 4096);
	if (res <= 0)
	    break;
	if (res + idx > size)
	    break;
	if (memcmp(bytes, &mem[idx], res) != 0) {
	    int ix;
	    for (ix=0; ix<res; ix++)
		if (bytes[ix] != mem[idx+ix])
			break;
	    fprintf(stderr,"Compare error at position %d\n", idx+ix);
	    close(fd);
	    return(1);
	}
	idx += res;
    }
    close(fd);
    if (idx != size) {
	fprintf(stderr,"Compare error index %d, size %d\n", idx, size);
    }
    return(idx != size);
}

/************************************************************************
 *									*
 *		Tests implementations					*
 *									*
 ************************************************************************/

/************************************************************************
 *									*
 *		XSLT tests						*
 *									*
 ************************************************************************/

static int
xsltTest(const char *filename, int options) {
    xsltStylesheetPtr style;
    xmlDocPtr styleDoc, doc = NULL, outDoc;
    xmlChar *out = NULL;
    const char *outSuffix, *errSuffix;
    char *docFilename, *outFilename, *errFilename;
    int outSize = 0;
    int res;
    int ret = 0;

    if (strcmp(filename, "./test-10-3.xsl") == 0) {
        void *locale = xsltNewLocale(BAD_CAST "de", 0);
        xmlChar *str1, *str2;

        /* Skip test requiring "de" locale */
        if (locale == NULL)
            return(0);

        /*
         * Some C libraries like musl or older macOS don't support
         * collation with locales.
         */
        str1 = xsltStrxfrm(locale, BAD_CAST "\xC3\xA4");
        str2 = xsltStrxfrm(locale, BAD_CAST "b");
        res = xmlStrcmp(str1, str2);
        xmlFree(str1);
        xmlFree(str2);
        xsltFreeLocale(locale);

        if (res >= 0) {
            fprintf(stderr, "Warning: Your C library doesn't seem to support "
                    "collation with locales\n");
            return(0);
        }
    }

#if LIBXML_VERSION < 21300
    if (strcmp(filename, "./test_bad.xsl") == 0)
        return(0);
#endif

    styleDoc = xmlReadFile(filename, NULL, XSLT_PARSE_OPTIONS | options);
    style = xsltLoadStylesheetPI(styleDoc);
    if (style != NULL) {
        /* Standalone stylesheet */
        doc = styleDoc;
        docFilename = strdup(filename);

        outSuffix = ".stand.out";
        errSuffix = ".stand.err";
    } else {
        docFilename = changeSuffix(filename, ".xml");
        if (!checkTestFile(docFilename)) {
            xmlFreeDoc(styleDoc);
            goto out;
        }
        style = xsltParseStylesheetDoc(styleDoc);
        if (style == NULL) {
            xmlFreeDoc(styleDoc);
        } else {
            doc = xmlReadFile(docFilename, NULL, XSLT_PARSE_OPTIONS | options);
        }

        outSuffix = ".out";
        errSuffix = ".err";
    }

    if (style != NULL) {
        const char *params[] = {
            "test", "'passed_value'",
            "test2", "'passed_value2'",
            NULL
        };

        outDoc = xsltApplyStylesheet(style, doc, params);
        if (outDoc == NULL) {
            /* xsltproc compat */
	    testErrorHandler(NULL, "no result for %s\n", docFilename);
        } else {
            xsltSaveResultToString(&out, &outSize, outDoc, style);
            xmlFreeDoc(outDoc);
        }
        xsltFreeStylesheet(style);
    }
    xmlFreeDoc(doc);

    outFilename = changeSuffix(filename, outSuffix);
    res = compareFileMem(outFilename, (char *) out, outSize);
    if (res != 0) {
        fprintf(stderr, "Result for %s failed\n", filename);
        /* printf("####\n%s####\n", out); */
        ret = -1;
    }
    free(outFilename);
    xmlFree(out);

    errFilename = changeSuffix(filename, errSuffix);
    res = compareFileMem(errFilename, testErrors, testErrorsSize);
    if (res != 0) {
        fprintf(stderr, "Error for %s failed\n", filename);
        /* printf("####\n%s####\n", testErrors); */
        ret = -1;
    }
    free(errFilename);

out:
    free(docFilename);
    return(ret);
}

/************************************************************************
 *									*
 *			Tests Descriptions				*
 *									*
 ************************************************************************/

static
testDesc testDescriptions[] = {
    { "REC2 tests",
      xsltTest, "REC2", "./*.xsl", 0 },
    { "REC tests",
      xsltTest, "REC", "./*.xsl", 0 },
    { "REC tests (standalone)",
      xsltTest, "REC", "./stand*.xml", 0 },
    { "REC tests without dictionaries",
      xsltTest, "REC", "./*.xsl", XML_PARSE_NODICT },
    { "REC tests without dictionaries (standalone)",
      xsltTest, "REC", "./stand*.xml", XML_PARSE_NODICT },
    { "general tests",
      xsltTest, "general", "./*.xsl", 0 },
    { "general tests without dictionaries",
      xsltTest, "general", "./*.xsl", XML_PARSE_NODICT },
#if defined(LIBXML_ICONV_ENABLED) || defined(LIBXML_ICU_ENABLED)
    { "encoding tests",
      xsltTest, "encoding", "./*.xsl", 0 },
#endif
    { "documents tests",
      xsltTest, "documents", "./*.xsl", 0 },
    { "numbers tests",
      xsltTest, "numbers", "./*.xsl", 0 },
    { "keys tests",
      xsltTest, "keys", "./*.xsl", 0 },
    { "namespaces tests",
      xsltTest, "namespaces", "./*.xsl", 0 },
    { "extensions tests",
      xsltTest, "extensions", "./*.xsl", 0 },
    { "reports tests",
      xsltTest, "reports", "./*.xsl", 0 },
    { "exslt common tests",
      xsltTest, "exslt/common", "./*.xsl", 0 },
#if defined(EXSLT_CRYPTO_ENABLED) && !defined(_WIN32)
    { "exslt crypto tests",
      xsltTest, "exslt/crypto", "./*.xsl", 0 },
#endif
    { "exslt date tests",
      xsltTest, "exslt/date", "./*.xsl", 0 },
    { "exslt dynamic tests",
      xsltTest, "exslt/dynamic", "./*.xsl", 0 },
    { "exslt functions tests",
      xsltTest, "exslt/functions", "./*.xsl", 0 },
    { "exslt math tests",
      xsltTest, "exslt/math", "./*.xsl", 0 },
    { "exslt saxon tests",
      xsltTest, "exslt/saxon", "./*.xsl", 0 },
    { "exslt sets tests",
      xsltTest, "exslt/sets", "./*.xsl", 0 },
    { "exslt strings tests",
      xsltTest, "exslt/strings", "./*.xsl", 0 },
#ifdef LIBXSLT_DEFAULT_PLUGINS_PATH
    { "plugin tests",
      xsltTest, "plugins", "./*.xsl", 0 },
#endif
    {NULL, NULL, NULL, NULL, 0}
};

/************************************************************************
 *									*
 *		The main code driving the tests				*
 *									*
 ************************************************************************/

static int
launchTests(testDescPtr tst) {
    int res = 0, err = 0;
    size_t i;
    char oldDir[500] = {0};

    if (tst->dir) {
        if (getcwd(oldDir, sizeof(oldDir)) == NULL) {
            fprintf(stderr, "Can't can't get current directory\n");
	    nb_errors++;
            return(1);
        }
        if (chdir(tst->dir) < 0) {
            fprintf(stderr, "Can't change directory to %s\n", tst->dir);
	    nb_errors++;
            return(1);
        }
    }

    if (tst->in != NULL) {
	glob_t globbuf;

	globbuf.gl_offs = 0;
	glob(tst->in, GLOB_DOOFFS, NULL, &globbuf);
	for (i = 0;i < globbuf.gl_pathc;i++) {
            testErrorsSize = 0;
            testErrors[0] = 0;
            nb_tests++;
            res = tst->func(globbuf.gl_pathv[i], tst->options);
            xmlResetLastError();
            if (res != 0) {
                fprintf(stderr, "File %s generated an error\n",
                        globbuf.gl_pathv[i]);
                nb_errors++;
                err++;
            }
            testErrorsSize = 0;
	}
	globfree(&globbuf);
    } else {
        testErrorsSize = 0;
	testErrors[0] = 0;
        nb_tests++;
        res = tst->func(NULL, tst->options);
	if (res != 0) {
	    nb_errors++;
	    err++;
	}
    }

    if (oldDir[0] && chdir(oldDir) < 0) {
        fprintf(stderr, "Can't change directory to %s\n", oldDir);
	nb_errors++;
        err++;
    }

    return(err);
}

static int verbose = 0;
static int tests_quiet = 0;

static int
runtest(int i) {
    int ret = 0, res;
    int old_errors, old_tests;

    old_errors = nb_errors;
    old_tests = nb_tests;
    if ((tests_quiet == 0) && (testDescriptions[i].desc != NULL))
	printf("## Running %s\n", testDescriptions[i].desc);
    res = launchTests(&testDescriptions[i]);
    if (res != 0)
	ret++;
    if (verbose) {
	if (nb_errors == old_errors)
	    printf("Ran %d tests, no errors\n", nb_tests - old_tests);
	else
	    printf("Ran %d tests, %d errors\n",
		   nb_tests - old_tests,
		   nb_errors - old_errors);
    }
    return(ret);
}

int
main(int argc ATTRIBUTE_UNUSED, char **argv ATTRIBUTE_UNUSED) {
    int i, a, ret = 0;
    int subset = 0;

#if defined(_WIN32)
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1400 && _MSC_VER < 1900
    _set_output_format(_TWO_DIGIT_EXPONENT);
#endif

    initializeLibxml2();

    for (a = 1; a < argc;a++) {
        if (!strcmp(argv[a], "-v"))
	    verbose = 1;
        else if (!strcmp(argv[a], "-u"))
	    update_results = 1;
        else if (!strcmp(argv[a], "-quiet"))
	    tests_quiet = 1;
        else if (!strcmp(argv[a], "--out"))
	    temp_directory = argv[++a];
	else {
	    for (i = 0; testDescriptions[i].func != NULL; i++) {
	        if (strstr(testDescriptions[i].desc, argv[a])) {
		    ret += runtest(i);
		    subset++;
		}
	    }
	}
    }
    if (subset == 0) {
	for (i = 0; testDescriptions[i].func != NULL; i++) {
	    ret += runtest(i);
	}
    }
    if (nb_errors == 0) {
        ret = 0;
	printf("Total %d tests, no errors\n",
	       nb_tests);
    } else {
        ret = 1;
	printf("Total %d tests, %d errors\n",
	       nb_tests, nb_errors);
    }
    xmlCleanupParser();

    return(ret);
}
