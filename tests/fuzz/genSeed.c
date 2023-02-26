/*
 * genSeed.c: Generate the seed corpora for fuzzing.
 *
 * See Copyright for the status of this software.
 */

#include <stdio.h>
#include <string.h>
#include <glob.h>
#include <libgen.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#include <libxml/parserInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/xsltutils.h>
#include "fuzz.h"

#define PATH_SIZE 500
#define SEED_BUF_SIZE 16384

typedef int
(*fileFunc)(const char *base, FILE *out);

typedef int
(*mainFunc)(const char *testsDir);

static struct {
    FILE *out;
    xmlHashTablePtr entities; /* Maps URLs to xmlFuzzEntityInfos */
    xmlExternalEntityLoader oldLoader;
    fileFunc processFile;
    const char *fuzzer;
    const char *docDir;
    char cwd[PATH_SIZE];
} globalData;

/*
 * A custom entity loader that writes all external DTDs or entities to a
 * single file in the format expected by xmlFuzzEntityLoader.
 */
static xmlParserInputPtr
fuzzEntityRecorder(const char *URL, const char *ID, xmlParserCtxtPtr ctxt) {
    xmlParserInputPtr in;
    xmlChar *data;
    static const int chunkSize = 16384;
    int len;

    in = xmlNoNetExternalEntityLoader(URL, ID, ctxt);
    if (in == NULL)
        return(NULL);

    if (globalData.entities == NULL) {
        globalData.entities = xmlHashCreate(4);
    } else if (xmlHashLookup(globalData.entities,
                             (const xmlChar *) URL) != NULL) {
        return(in);
    }

    do {
        len = xmlParserInputBufferGrow(in->buf, chunkSize);
        if (len < 0) {
            fprintf(stderr, "Error reading %s\n", URL);
            xmlFreeInputStream(in);
            return(NULL);
        }
    } while (len > 0);

    data = xmlStrdup(xmlBufContent(in->buf->buffer));
    if (data == NULL) {
        fprintf(stderr, "Error allocating entity data\n");
        xmlFreeInputStream(in);
        return(NULL);
    }

    xmlFreeInputStream(in);

    xmlHashAddEntry(globalData.entities, (const xmlChar *) URL, data);

    return(xmlNoNetExternalEntityLoader(URL, ID, ctxt));
}

static void
fuzzRecorderInit(FILE *out) {
    globalData.out = out;
    globalData.entities = xmlHashCreate(8);
    globalData.oldLoader = xmlGetExternalEntityLoader();
    xmlSetExternalEntityLoader(fuzzEntityRecorder);
}

static void
fuzzRecorderWriteAndFree(void *entry, const xmlChar *file) {
    char *data = entry;
    xmlFuzzWriteString(globalData.out, (const char *) file);
    xmlFuzzWriteString(globalData.out, data);
    xmlFree(data);
}

static void
fuzzRecorderWrite(const char *file) {
    xmlHashRemoveEntry(globalData.entities, (const xmlChar *) file,
                       fuzzRecorderWriteAndFree);
}

static void
fuzzRecorderCleanup() {
    xmlSetExternalEntityLoader(globalData.oldLoader);
    /* Write remaining entities (in random order). */
    xmlHashFree(globalData.entities, fuzzRecorderWriteAndFree);
    globalData.out = NULL;
    globalData.entities = NULL;
    globalData.oldLoader = NULL;
}

static int
processXslt(const char *sheetFile, FILE *out) {
    struct stat statbuf;
    xsltStylesheetPtr sheet;
    xmlDocPtr doc;
    char docFile[PATH_SIZE];
    char base[PATH_SIZE] = "";
    size_t len, size;

    len = strlen(sheetFile);
    if ((len < 5) || (len >= PATH_SIZE) ||
        (strcmp(sheetFile + len - 4, ".xsl") != 0)) {
        fprintf(stderr, "invalid stylesheet file: %s\n", sheetFile);
        return(-1);
    }
    strncat(base, sheetFile, len - 4);

    if (globalData.docDir == NULL) {
        size = snprintf(docFile, sizeof(docFile), "%s.xml", base);
    } else {
        size = snprintf(docFile, sizeof(docFile), "%s/%s.xml",
                        globalData.docDir, base);
    }
    if (size >= sizeof(docFile)) {
        fprintf(stderr, "creating pattern failed\n");
        return(-1);
    }

    /* Document might not exist, for example with imported stylesheets. */
    if (stat(docFile, &statbuf) != 0)
        return(-1);

    /* Malloc limit. */
    xmlFuzzWriteInt(out, 0, 4);

    fuzzRecorderInit(out);

    sheet = xsltParseStylesheetFile(BAD_CAST sheetFile);
    doc = xmlReadFile(docFile, NULL, XSLT_PARSE_OPTIONS);
    xmlFreeDoc(xsltApplyStylesheet(sheet, doc, NULL));
    xmlFreeDoc(doc);
    xsltFreeStylesheet(sheet);

    fuzzRecorderWrite(sheetFile);
    fuzzRecorderWrite(docFile);
    fuzzRecorderCleanup();

    return(0);
}

static int
processPattern(const char *pattern) {
    glob_t globbuf;
    int ret = 0;
    int res;
    size_t i;

    res = glob(pattern, 0, NULL, &globbuf);
    if (res == GLOB_NOMATCH)
        return(0);
    if (res != 0) {
        fprintf(stderr, "couldn't match pattern %s\n", pattern);
        return(-1);
    }

    for (i = 0; i < globbuf.gl_pathc; i++) {
        struct stat statbuf;
        char outPath[PATH_SIZE];
        char *dirBuf = NULL;
        char *baseBuf = NULL;
        const char *path, *dir, *base;
        FILE *out = NULL;
        int dirChanged = 0;
        size_t size;

        res = -1;
        path = globbuf.gl_pathv[i];

        if ((stat(path, &statbuf) != 0) || (!S_ISREG(statbuf.st_mode)))
            continue;

        dirBuf = (char *) xmlCharStrdup(path);
        baseBuf = (char *) xmlCharStrdup(path);
        if ((dirBuf == NULL) || (baseBuf == NULL)) {
            fprintf(stderr, "memory allocation failed\n");
            ret = -1;
            goto error;
        }
        dir = dirname(dirBuf);
        base = basename(baseBuf);

        size = snprintf(outPath, sizeof(outPath), "seed/%s/%s",
                        globalData.fuzzer, base);
        if (size >= sizeof(outPath)) {
            fprintf(stderr, "creating path failed\n");
            ret = -1;
            goto error;
        }
        out = fopen(outPath, "wb");
        if (out == NULL) {
            fprintf(stderr, "couldn't open %s for writing\n", outPath);
            ret = -1;
            goto error;
        }
        if (chdir(dir) != 0) {
            fprintf(stderr, "couldn't chdir to %s\n", dir);
            ret = -1;
            goto error;
        }
        dirChanged = 1;
        res = globalData.processFile(base, out);

error:
        if ((dirChanged) && (chdir(globalData.cwd) != 0)) {
            fprintf(stderr, "couldn't chdir to %s\n", globalData.cwd);
            ret = -1;
            break;
        }
        if (out != NULL) {
            fclose(out);
            if (res != 0) {
                unlink(outPath);
                ret = -1;
            }
        }
        xmlFree(dirBuf);
        xmlFree(baseBuf);
    }

    globfree(&globbuf);
    return(ret);
}

static int
processTestDir(const char *testsDir, const char *subDir, const char *docDir) {
    char pattern[PATH_SIZE];
    size_t size;

    size = snprintf(pattern, sizeof(pattern), "%s/%s/*.xsl",
                    testsDir, subDir);
    if (size >= sizeof(pattern)) {
        fprintf(stderr, "creating pattern failed\n");
        return -1;
    }

    globalData.docDir = docDir;
    return processPattern(pattern);
}

static int
processTests(const char *testsDir) {
    processTestDir(testsDir, "REC", NULL);
    processTestDir(testsDir, "general", "../docs");
    processTestDir(testsDir, "exslt/*", NULL);

    return 0;
}

static int
processXPath(const char *testsDir ATTRIBUTE_UNUSED) {
#define UTF8_Auml   "\xC3\x84"
#define UTF8_szlig  "\xC3\x9F"
#define UTF8_ALPHA  "\xCE\xB1"
#define UTF8_DEJA   "d\xC3\xA9j\xC3\xA0"
    static const char *xml =
        "<?pi content?>\n"
        "<a xmlns:a=\"a\">\n"
        "    <b xmlns:b=\"b\" a=\"1\" id=\"b\">\n"
        "        <c b=\"2\">" UTF8_Auml "rger</c>\n"
        "        <b:d b=\"3\">text</b:d>\n"
        "        <!-- comment -->\n"
        "        <a:b b=\"4\">" UTF8_szlig "&#x1f600;</a:b>\n"
        "        <b:c a=\"4\"><![CDATA[text]]></b:c>\n"
        "    </b>\n"
        "    <?pi content?>\n"
        "    <a:e xmlns:c=\"c\" a=\"" UTF8_ALPHA "\">\n"
        "        <c:d b=\"2\"/>\n"
        "        <a:c>99</a:c>\n"
        "        <e a=\"2\">content</e>\n"
        "    </a:e>\n"
        "    <b/>\n"
        "    <a:a/>\n"
        "    <!-- comment -->\n"
        "</a>\n";
    static const char *exprs[] = {
        "crypto:md4('a')",
        "crypto:md5('a')",
        "crypto:rc4_decrypt(crypto:rc4_encrypt('key','msg'))",
        "crypto:sha1('a')",
        "date:add('2016-01-01T12:00:00','-P1Y2M3DT10H30M45S')",
        "date:add-duration('-P1Y2M3DT10H30M45S','-P1Y2M3DT10H30M45S')",
        "date:date('2016-01-01T12:00:00')",
        "date:date-time()",
        "date:day-abbreviation('2016-01-01T12:00:00')",
        "date:day-in-month('2016-01-01T12:00:00')",
        "date:day-in-week('2016-01-01T12:00:00')",
        "date:day-in-year('2016-01-01T12:00:00')",
        "date:day-name('2016-01-01T12:00:00')",
        "date:day-of-week-in-month('2016-01-01T12:00:00')",
        "date:difference('1999-06-10T20:03:48','2016-01-01T12:00:00')",
        "date:duration('1234567890')",
        "date:format-date('2016-01-01T12:00:00','GyyyyMMwwWWDDddFFEaHHkkKKhhMMssSSSzZ')",
        "date:hour-in-day('2016-01-01T12:00:00')",
        "date:leap-year('2016-01-01T12:00:00')",
        "date:minute-in-hour('2016-01-01T12:00:00')",
        "date:month-abbreviation('2016-01-01T12:00:00')",
        "date:month-in-year('2016-01-01T12:00:00')",
        "date:month-name('2016-01-01T12:00:00')",
        "date:parse-date('20160101120000','yyyyMMddkkmmss')",
        "date:second-in-minute('2016-01-01T12:00:00')",
        "date:seconds('2016-01-01T12:00:00')",
        "date:sum(str:split('-P1Y2M3DT10H30M45S,-P1Y2M3DT10H30M45S,P999999999S',','))",
        "date:time('2016-01-01T12:00:00')",
        "date:week-in-month('2016-01-01T12:00:00')",
        "date:week-in-year('2016-01-01T12:00:00')",
        "date:year('2016-01-01T12:00:00')",
        "dyn:evaluate('1+1')",
        "dyn:map(//*,'.')",
        "(1.1+-24.5)*0.8-(25div3.5)mod0.2",
        "/a/b/c/text()|//e/c:d/@b",
        "(//*[@*][1])[1]",
        "exsl:node-set($n)",
        "exsl:node-set('s')",
        "exsl:object-type(1)",
        "boolean(.)",
        "ceiling(.)",
        "concat(.,'a')",
        "contains(.,'e')",
        "count(.)",
        "false()",
        "floor(.)",
        "id(.)",
        "lang(.)",
        "last()",
        "local-name(.)",
        "name(.)",
        "namespace-uri(.)",
        "normalize-space(.)",
        "not(.)",
        "number(.)",
        "number('1.0')",
        "position()",
        "round(.)",
        "starts-with(.,'t')",
        "string-length(.)",
        "string(.)",
        "string(1.0)",
        "substring(.,2,3)",
        "substring-after(.,'e')",
        "substring-before(.,'e')",
        "sum(*)",
        "translate(.,'e','a')",
        "true()",
        "math:abs(-1.5)",
        "math:acos(-0.5)",
        "math:asin(-0.5)",
        "math:atan(-0.5)",
        "math:atan2(-1.5,-1.5)",
        "math:constant('E',20)",
        "math:cos(-1.5)",
        "math:exp(-1.5)",
        "math:highest(str:split('1.2,-0.5,-2.2e8,-0.1e-5',','))",
        "math:log(2.0)",
        "math:lowest(str:split('1.2,-0.5,-2.2e8,-0.1e-5',','))",
        "math:max(str:split('1.2,-0.5,-2.2e8,-0.1e-5',','))",
        "math:min(str:split('1.2,-0.5,-2.2e8,-0.1e-5',','))",
        "math:power(2.0,0.5)",
        "math:random()",
        "math:sin(-1.5)",
        "math:sqrt(2.0)",
        "math:tan(-1.5)",
        "saxon:eval(saxon:expression('1+1'))",
        "saxon:evaluate('1+1')",
        "saxon:line-number()",
        "saxon:line-number(*)",
        "saxon:systemId()",
        "set:difference(//*,//a:*)",
        "set:distinct(//*)",
        "set:has-same-node(//*,//a:*)",
        "set:intersection(//*,//a:*)",
        "set:leading(//*,/*/*[3])",
        "set:trailing(//*,/*/*[2])",
        "str:align('" UTF8_DEJA "','--------','center')",
        "str:align('" UTF8_DEJA "','--------','left')",
        "str:align('" UTF8_DEJA "','--------','right')",
        "str:concat(str:split('ab,cd,ef',','))",
        "str:decode-uri('%41%00%2d')",
        "str:encode-uri(';/?:@&=+$,[]',true())",
        "str:encode-uri('|<>',false())",
        "str:padding(81,' ')",
        "str:replace('abcdefgh',str:split('a,c,e,g',','),str:split('w,x,y,z',','))",
        "str:split('a, sim, lis',', ')",
        "str:tokenize('2016-01-01T12:00:00','-T:')",
        "current()",
        "document('')",
        "element-available('exsl:document')",
        "format-number(1.0,'##,##,00.00##')",
        "format-number(1.0,'#.#;-0.0%')",
        "function-available('exsl:node-set')",
        "generate-id(.)",
        "system-property('xsl:version')",
        "unparsed-entity-uri('a')"
    };
    size_t numExprs = sizeof(exprs) / sizeof(*exprs);
    size_t i, size;
    int ret = 0;

    for (i = 0; i < numExprs; i++) {
        char outPath[PATH_SIZE];
        FILE *out;

        size = snprintf(outPath, sizeof(outPath), "seed/xpath/%03d", (int) i);
        if (size >= PATH_SIZE) {
            ret = -1;
            continue;
        }
        out = fopen(outPath, "wb");
        if (out == NULL) {
            ret = -1;
            continue;
        }
        /* Memory limit. */
        xmlFuzzWriteInt(out, 0, 4);
        xmlFuzzWriteString(out, exprs[i]);
        xmlFuzzWriteString(out, xml);

        fclose(out);
    }

    return(ret);
}

int
main(int argc, const char **argv) {
    mainFunc process = processTests;
    const char *fuzzer;
    int ret = 0;

    if (argc < 3) {
        fprintf(stderr, "usage: genSeed [FUZZER] [PATTERN...]\n");
        return(1);
    }

    xmlSetGenericErrorFunc(NULL, xmlFuzzErrorFunc);
    xsltSetGenericErrorFunc(NULL, xmlFuzzErrorFunc);

    fuzzer = argv[1];
    if (strcmp(fuzzer, "xslt") == 0) {
        globalData.processFile = processXslt;
    } else if (strcmp(fuzzer, "xpath") == 0) {
        process = processXPath;
    } else {
        fprintf(stderr, "unknown fuzzer %s\n", fuzzer);
        return(1);
    }
    globalData.fuzzer = fuzzer;

    if (getcwd(globalData.cwd, PATH_SIZE) == NULL) {
        fprintf(stderr, "couldn't get current directory\n");
        return(1);
    }

    process(argv[2]);

    return(ret);
}

