/*
 * date.c: Implementation of the EXSLT -- Dates and Times module
 *
 * References:
 *   http://www.exslt.org/date/date.html
 *
 * See Copyright for the status of this software.
 *
 * Authors:
 *   Charlie Bozeman <cbozeman@HiWAAY.net>
 *   Thomas Broyer <tbroyer@ltgt.net>
 *
 * TODO:
 * handle duration
 * implement "other" date/time extension functions
 */

#define IN_LIBEXSLT
#include "libexslt/libexslt.h"

#include <stdlib.h>
#include <string.h>

#if defined(WIN32) && !defined (__CYGWIN__)
#include <win32config.h>
#else
#include "config.h"
#endif

#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <libxslt/xsltconfig.h>
#include <libxslt/xsltutils.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/extensions.h>

#include "exslt.h"

#ifdef HAVE_TIME_H
#include <time.h>
#endif

#if 0
#define DEBUG_EXSLT_DATE
#endif

/* types of date and/or time (from schema datatypes) */
typedef enum {
    XS_DATETIME = 1,
    XS_DATE,
    XS_TIME,
    XS_GYEARMONTH,
    XS_GYEAR,
    XS_GMONTHDAY,
    XS_GMONTH,
    XS_GDAY,
    XS_DURATION
} exsltDateType;

/* date object */
typedef struct _exsltDate exsltDate;
typedef exsltDate *exsltDatePtr;
struct _exsltDate {
    exsltDateType	type;
    long		year;
    unsigned int	mon	:4;	/* 1 <=  mon    <= 12   */
    unsigned int	day	:5;	/* 1 <=  day    <= 31   */
    unsigned int	hour	:5;	/* 0 <=  hour   <= 23   */
    unsigned int	min	:6;	/* 0 <=  min    <= 59	*/
    double		sec;
    int			tz_flag	:1;	/* is tzo explicitely set? */
    int			tzo	:11;	/* -1440 <= tzo <= 1440 */
};

/****************************************************************
 *								*
 *			Compat./Port. macros			*
 *								*
 ****************************************************************/

#if defined(HAVE_TIME_H) && defined(HAVE_LOCALTIME)		\
    && defined(HAVE_TIME) && defined(HAVE_GMTIME)		\
    && defined(HAVE_MKTIME)
#define WITH_TIME
#endif

/****************************************************************
 *								*
 *		Convenience macros and functions		*
 *								*
 ****************************************************************/

#define IS_TZO_CHAR(c)						\
	((c == 0) || (c == 'Z') || (c == '+') || (c == '-'))

#define VALID_YEAR(yr)          (yr != 0)
#define VALID_MONTH(mon)        ((mon >= 1) && (mon <= 12))
/* VALID_DAY should only be used when month is unknown */
#define VALID_DAY(day)          ((day >= 1) && (day <= 31))
#define VALID_HOUR(hr)          ((hr >= 0) && (hr <= 23))
#define VALID_MIN(min)          ((min >= 0) && (min <= 59))
#define VALID_SEC(sec)          ((sec >= 0) && (sec < 60))
#define VALID_TZO(tzo)          ((tzo > -1440) && (tzo < 1440))
#define IS_LEAP(y)						\
	(((y % 4 == 0) && (y % 100 != 0)) || (y % 400 == 0))

static const int daysInMonth[12] =
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static const int daysInMonthLeap[12] =
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

#define VALID_MDAY(dt)						\
	(IS_LEAP(dt->year) ?					\
	           (dt->day <= daysInMonthLeap[dt->mon - 1]) :	\
	           (dt->day <= daysInMonth[dt->mon - 1]))

#define VALID_DATE(dt)						\
	(VALID_YEAR(dt->year) && VALID_MONTH(dt->mon) && VALID_MDAY(dt))

#define VALID_TIME(dt)						\
	(VALID_HOUR(dt->hour) && VALID_MIN(dt->min) &&		\
	 VALID_SEC(dt->sec) && VALID_TZO(dt->tzo))

#define VALID_DATETIME(dt)					\
	(VALID_DATE(dt) && VALID_TIME(dt))


static const int dayInYearByMonth[12] =
	{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
static const int dayInLeapYearByMonth[12] =
	{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 };

#define DAY_IN_YEAR(day, month, year)				\
        ((IS_LEAP(year) ?					\
                dayInLeapYearByMonth[month - 1] :		\
                dayInYearByMonth[month - 1]) + day)

/**
 * _exsltDateParseGYear:
 * @dt:  pointer to a date structure
 * @str: pointer to the string to analyze
 *
 * Parses a xs:gYear without time zone and fills in the appropriate
 * field of the @dt structure. @str is updated to point just after the
 * xs:gYear. It is supposed that @dt->year is big enough to contain
 * the year.
 *
 * Returns 0 or the error code
 */
static int
_exsltDateParseGYear (exsltDatePtr dt, const xmlChar **str) {
    const xmlChar *cur = *str, *firstChar;
    int isneg = 0, digcnt = 0;

    if (((*cur < '0') || (*cur > '9')) &&
	(*cur != '-') && (*cur != '+'))
	return -1;

    if (*cur == '-') {
	isneg = 1;
	cur++;
    }

    firstChar = cur;

    while ((*cur >= '0') && (*cur <= '9')) {
	dt->year = dt->year * 10 + (*cur - '0');
	cur++;
	digcnt++;
    }

    /* year must be at least 4 digits (CCYY); over 4
     * digits cannot have a leading zero. */
    if ((digcnt < 4) || ((digcnt > 4) && (*firstChar == '0')))
	return 1;

    if (isneg)
	dt->year = - dt->year;

    if (!VALID_YEAR(dt->year))
	return 2;

    *str = cur;

#ifdef DEBUG_EXSLT_DATE
    xsltGenericDebug(xsltGenericDebugContext,
		     "Parsed year %04i\n", dt->year);
#endif

    return 0;
}

/**
 * FORMAT_GYEAR:
 * @dt:  the #exsltDate to format
 * @cur: a pointer to an allocated buffer
 *
 * Formats @dt in xsl:gYear format. Result is appended to @cur and
 * @cur is updated to point after the xsl:gYear.
 */
#define FORMAT_GYEAR(dt, cur)					\
	if (dt->year < 0) {					\
	    *cur = '-';						\
	    cur++;						\
	}							\
	{							\
	    int year = (dt->year < 0) ? - dt->year : dt->year;	\
	    xmlChar tmp_buf[100], *tmp = tmp_buf;		\
	    /* virtually adds leading zeros */			\
	    while (year < 1000)					\
		year *= 10;					\
	    /* result is in reverse-order */			\
	    while (year > 0) {					\
		*tmp = '0' + (year % 10);			\
		year /= 10;					\
		tmp++;						\
	    }							\
	    /* restore the correct order */			\
	    while (tmp > tmp_buf) {				\
		tmp--;						\
		*cur = *tmp;					\
		cur++;						\
	    }							\
	}

/**
 * PARSE_2_DIGITS:
 * @num:  the integer to fill in
 * @cur:  an #xmlChar *
 * @invalid: an integer
 *
 * Parses a 2-digits integer and updates @num with the value. @cur is
 * updated to point just after the integer.
 * In case of error, @invalid is set to %TRUE, values of @num and
 * @cur are undefined.
 */
#define PARSE_2_DIGITS(num, cur, invalid)			\
	if ((cur[0] < '0') || (cur[0] > '9') ||			\
	    (cur[1] < '0') || (cur[1] > '9'))			\
	    invalid = 1;					\
	else							\
	    num = (cur[0] - '0') * 10 + (cur[1] - '0');		\
	cur += 2;

/**
 * FORMAT_2_DIGITS:
 * @num:  the integer to format
 * @cur: a pointer to an allocated buffer
 *
 * Formats a 2-digits integer. Result is appended to @cur and
 * @cur is updated to point after the integer.
 */
#define FORMAT_2_DIGITS(num, cur)				\
	*cur = '0' + ((num / 10) % 10);				\
	cur++;							\
	*cur = '0' + (num % 10);				\
	cur++;

/**
 * PARSE_FLOAT:
 * @num:  the double to fill in
 * @cur:  an #xmlChar *
 * @invalid: an integer
 *
 * Parses a float and updates @num with the value. @cur is
 * updated to point just after the float. The float must have a
 * 2-digits integer part and may or may not have a decimal part.
 * In case of error, @invalid is set to %TRUE, values of @num and
 * @cur are undefined.
 */
#define PARSE_FLOAT(num, cur, invalid)				\
	PARSE_2_DIGITS(num, cur, invalid);			\
	if (!invalid && (*cur == '.')) {			\
	    double mult = 1;				        \
	    cur++;						\
	    if ((*cur < '0') || (*cur > '9'))			\
		invalid = 1;					\
	    while ((*cur >= '0') && (*cur <= '9')) {		\
		mult /= 10;					\
		num += (*cur - '0') * mult;			\
		cur++;						\
	    }							\
	}

/**
 * FORMAT_FLOAT:
 * @num:  the double to format
 * @cur: a pointer to an allocated buffer
 *
 * Formats a float. Result is appended to @cur and @cur is updated to
 * point after the integer. The float representation has a 2-digits
 * integer part and may or may not have a decimal part.
 */
#define FORMAT_FLOAT(num, cur)					\
	{							\
            xmlChar *sav, *str;                                 \
            if (num < 10.0)                                     \
                *cur++ = '0';                                   \
            str = xmlXPathCastNumberToString(num);              \
            sav = str;                                          \
            while (*str != 0)                                   \
                *cur++ = *str++;                                \
            xmlFree(sav);                                       \
	}

/**
 * _exsltDateParseGMonth:
 * @dt:  pointer to a date structure
 * @str: pointer to the string to analyze
 *
 * Parses a xs:gMonth without time zone and fills in the appropriate
 * field of the @dt structure. @str is updated to point just after the
 * xs:gMonth.
 *
 * Returns 0 or the error code
 */
static int
_exsltDateParseGMonth (exsltDatePtr dt, const xmlChar **str) {
    const xmlChar *cur = *str;
    int ret = 0;

    PARSE_2_DIGITS(dt->mon, cur, ret);
    if (ret != 0)
	return ret;

    if (!VALID_MONTH(dt->mon))
	return 2;

    *str = cur;

#ifdef DEBUG_EXSLT_DATE
    xsltGenericDebug(xsltGenericDebugContext,
		     "Parsed month %02i\n", dt->mon);
#endif

    return 0;
}

/**
 * FORMAT_GMONTH:
 * @dt:  the #exsltDate to format
 * @cur: a pointer to an allocated buffer
 *
 * Formats @dt in xsl:gMonth format. Result is appended to @cur and
 * @cur is updated to point after the xsl:gMonth.
 */
#define FORMAT_GMONTH(dt, cur)					\
	FORMAT_2_DIGITS(dt->mon, cur)

/**
 * _exsltDateParseGDay:
 * @dt:  pointer to a date structure
 * @str: pointer to the string to analyze
 *
 * Parses a xs:gDay without time zone and fills in the appropriate
 * field of the @dt structure. @str is updated to point just after the
 * xs:gDay.
 *
 * Returns 0 or the error code
 */
static int
_exsltDateParseGDay (exsltDatePtr dt, const xmlChar **str) {
    const xmlChar *cur = *str;
    int ret = 0;

    PARSE_2_DIGITS(dt->day, cur, ret);
    if (ret != 0)
	return ret;

    if (!VALID_DAY(dt->day))
	return 2;

    *str = cur;

#ifdef DEBUG_EXSLT_DATE
    xsltGenericDebug(xsltGenericDebugContext,
		     "Parsed day %02i\n", dt->day);
#endif

    return 0;
}

/**
 * FORMAT_GDAY:
 * @dt:  the #exsltDate to format
 * @cur: a pointer to an allocated buffer
 *
 * Formats @dt in xsl:gDay format. Result is appended to @cur and
 * @cur is updated to point after the xsl:gDay.
 */
#define FORMAT_GDAY(dt, cur)					\
	FORMAT_2_DIGITS(dt->day, cur)

/**
 * FORMAT_DATE:
 * @dt:  the #exsltDate to format
 * @cur: a pointer to an allocated buffer
 *
 * Formats @dt in xsl:date format. Result is appended to @cur and
 * @cur is updated to point after the xsl:date.
 */
#define FORMAT_DATE(dt, cur)					\
	FORMAT_GYEAR(dt, cur);					\
	*cur = '-';						\
	cur++;							\
	FORMAT_GMONTH(dt, cur);					\
	*cur = '-';						\
	cur++;							\
	FORMAT_GDAY(dt, cur);

/**
 * _exsltDateParseTime:
 * @dt:  pointer to a date structure
 * @str: pointer to the string to analyze
 *
 * Parses a xs:time without time zone and fills in the appropriate
 * fields of the @dt structure. @str is updated to point just after the
 * xs:time.
 * In case of error, values of @dt fields are undefined.
 *
 * Returns 0 or the error code
 */
static int
_exsltDateParseTime (exsltDatePtr dt, const xmlChar **str) {
    const xmlChar *cur = *str;
    int ret = 0;

    PARSE_2_DIGITS(dt->hour, cur, ret);
    if (ret != 0)
	return ret;

    if (*cur != ':')
	return 1;
    cur++;

    PARSE_2_DIGITS(dt->min, cur, ret);
    if (ret != 0)
	return ret;

    if (*cur != ':')
	return 1;
    cur++;

    PARSE_FLOAT(dt->sec, cur, ret);
    if (ret != 0)
	return ret;

    if (!VALID_TIME(dt))
	return 2;

    *str = cur;

#ifdef DEBUG_EXSLT_DATE
    xsltGenericDebug(xsltGenericDebugContext,
		     "Parsed time %02i:%02i:%02.f\n",
		     dt->hour, dt->min, dt->sec);
#endif

    return 0;
}

/**
 * FORMAT_TIME:
 * @dt:  the #exsltDate to format
 * @cur: a pointer to an allocated buffer
 *
 * Formats @dt in xsl:time format. Result is appended to @cur and
 * @cur is updated to point after the xsl:time.
 */
#define FORMAT_TIME(dt, cur)					\
	FORMAT_2_DIGITS(dt->hour, cur);				\
	*cur = ':';						\
	cur++;							\
	FORMAT_2_DIGITS(dt->min, cur);				\
	*cur = ':';						\
	cur++;							\
	FORMAT_FLOAT(dt->sec, cur);

/**
 * _exsltDateParseTimeZone:
 * @dt:  pointer to a date structure
 * @str: pointer to the string to analyze
 *
 * Parses a time zone without time zone and fills in the appropriate
 * field of the @dt structure. @str is updated to point just after the
 * time zone.
 *
 * Returns 0 or the error code
 */
static int
_exsltDateParseTimeZone (exsltDatePtr dt, const xmlChar **str) {
    const xmlChar *cur = *str;
    int ret = 0;

    if (str == NULL)
	return -1;

    switch (*cur) {
    case 0:
	dt->tz_flag = 0;
	dt->tzo = 0;

	break;

    case 'Z':
	dt->tz_flag = 1;
	dt->tzo = 0;

	cur++;
	break;

    case '+':
    case '-': {
	int isneg = 0, tmp = 0;
	isneg = (*cur == '-');

	cur++;

	PARSE_2_DIGITS(tmp, cur, ret);
	if (ret != 0)
	    return ret;
	if (!VALID_HOUR(tmp))
	    return 2;

	if (*cur != ':')
	    return 1;
	cur++;

	dt->tzo = tmp * 60;

	PARSE_2_DIGITS(tmp, cur, ret);
	if (ret != 0)
	    return ret;
	if (!VALID_MIN(tmp))
	    return 2;

	dt->tzo += tmp;
	if (isneg)
	    dt->tzo = - dt->tzo;

	if (!VALID_TZO(dt->tzo))
	    return 2;

	break;
      }
    default:
	return 1;
    }

    *str = cur;

#ifdef DEBUG_EXSLT_DATE
    xsltGenericDebug(xsltGenericDebugContext,
		     "Parsed time zone offset (%s) %i\n",
		     dt->tz_flag ? "explicit" : "implicit", dt->tzo);
#endif

    return 0;
}

/**
 * FORMAT_TZ:
 * @dt:  the #exsltDate to format
 * @cur: a pointer to an allocated buffer
 *
 * Formats @dt timezone. Result is appended to @cur and
 * @cur is updated to point after the timezone.
 */
#define FORMAT_TZ(dt, cur)					\
	if (dt->tzo == 0) {					\
	    *cur = 'Z';						\
	    cur++;						\
	} else {						\
	    int aTzo = (dt->tzo < 0) ? - dt->tzo : dt->tzo;	\
	    int tzHh = aTzo / 60, tzMm = aTzo % 60;		\
	    *cur = (dt->tzo < 0) ? '-' : '+' ;			\
	    cur++;						\
	    FORMAT_2_DIGITS(tzHh, cur);				\
	    *cur = ':';						\
	    cur++;						\
	    FORMAT_2_DIGITS(tzMm, cur);				\
	}

/****************************************************************
 *								*
 *	XML Schema Dates/Times Datatypes Handling		*
 *								*
 ****************************************************************/

/**
 * exsltDateCreateDate:
 *
 * Creates a new #exsltDate, uninitialized.
 *
 * Returns the #exsltDate
 */
static exsltDatePtr
exsltDateCreateDate (void) {
    exsltDatePtr ret;

    ret = (exsltDatePtr) xmlMalloc(sizeof(exsltDate));
    if (ret == NULL) {
	xsltGenericError(xsltGenericErrorContext,
			 "exsltDateCreateDate: out of memory\n");
	return (NULL);
    }
    memset (ret, 0, sizeof(exsltDate));

    return ret;
}

/**
 * exsltDateFreeDate:
 * @date: an #exsltDatePtr
 *
 * Frees up the @date
 */
static void
exsltDateFreeDate (exsltDatePtr date) {
    if (date == NULL)
	return;

    xmlFree(date);
}

#ifdef WITH_TIME
/**
 * exsltDateCurrent:
 *
 * Returns the current date and time.
 */
static exsltDatePtr
exsltDateCurrent (void) {
    struct tm *localTm, *gmTm;
    time_t secs;
    exsltDatePtr ret;

    ret = exsltDateCreateDate();
    if (ret == NULL)
        return NULL;

    /* get current time */
    secs    = time(NULL);
    localTm = localtime(&secs);

    ret->type = XS_DATETIME;

    /* get real year, not years since 1900 */
    ret->year = localTm->tm_year + 1900;

    ret->mon  = localTm->tm_mon + 1;
    ret->day  = localTm->tm_mday;
    ret->hour = localTm->tm_hour;
    ret->min  = localTm->tm_min;

    /* floating point seconds */
    ret->sec  = (double) localTm->tm_sec;

    /* determine the time zone offset from local to gm time */
    gmTm         = gmtime(&secs);
    ret->tz_flag = 0;
    ret->tzo     = (((ret->day * 1440) + (ret->hour * 60) + ret->min) -
                    ((gmTm->tm_mday * 1440) + (gmTm->tm_hour * 60) +
                      gmTm->tm_min));

    return ret;
}
#endif

/**
 * exsltDateParse:
 * @dateTime:  string to analyse
 *
 * Parses a date/time string
 *
 * Returns a newly built #exsltDatePtr of NULL in case of error
 */
static exsltDatePtr
exsltDateParse (const xmlChar *dateTime) {
    exsltDatePtr dt;
    int ret;
    const xmlChar *cur = dateTime;

#define RETURN_TYPE_IF_VALID(t)					\
    if (IS_TZO_CHAR(*cur)) {					\
	ret = _exsltDateParseTimeZone(dt, &cur);		\
	if (ret == 0) {						\
	    if (*cur != 0)					\
		goto error;					\
	    dt->type = t;					\
	    return dt;						\
	}							\
    }

    if (dateTime == NULL)
	return NULL;

    if ((*cur != '-') && (*cur < '0') && (*cur > '9'))
	return NULL;

    dt = exsltDateCreateDate();
    if (dt == NULL)
	return NULL;

    if ((cur[0] == '-') && (cur[1] == '-')) {
	/*
	 * It's an incomplete date (xs:gMonthDay, xs:gMonth or
	 * xs:gDay)
	 */
	cur += 2;

	/* is it an xs:gDay? */
	if (*cur == '-') {
	  ++cur;
	    ret = _exsltDateParseGDay(dt, &cur);
	    if (ret != 0)
		goto error;

	    RETURN_TYPE_IF_VALID(XS_GDAY);

	    goto error;
	}

	/*
	 * it should be an xs:gMonthDay or xs:gMonth
	 */
	ret = _exsltDateParseGMonth(dt, &cur);
	if (ret != 0)
	    goto error;

	if (*cur != '-')
	    goto error;
	cur++;

	/* is it an xs:gMonth? */
	if (*cur == '-') {
	    cur++;
	    RETURN_TYPE_IF_VALID(XS_GMONTH);
	    goto error;
	}

	/* it should be an xs:gMonthDay */
	ret = _exsltDateParseGDay(dt, &cur);
	if (ret != 0)
	    goto error;

	RETURN_TYPE_IF_VALID(XS_GMONTHDAY);

	goto error;
    }

    /*
     * It's a right-truncated date or an xs:time.
     * Try to parse an xs:time then fallback on right-truncated dates.
     */
    if ((*cur >= '0') && (*cur <= '9')) {
	ret = _exsltDateParseTime(dt, &cur);
	if (ret == 0) {
	    /* it's an xs:time */
	    RETURN_TYPE_IF_VALID(XS_TIME);
	}
    }

    /* fallback on date parsing */
    cur = dateTime;

    ret = _exsltDateParseGYear(dt, &cur);
    if (ret != 0)
	goto error;

    /* is it an xs:gYear? */
    RETURN_TYPE_IF_VALID(XS_GYEAR);

    if (*cur != '-')
	goto error;
    cur++;

    ret = _exsltDateParseGMonth(dt, &cur);
    if (ret != 0)
	goto error;

    /* is it an xs:gYearMonth? */
    RETURN_TYPE_IF_VALID(XS_GYEARMONTH);

    if (*cur != '-')
	goto error;
    cur++;

    ret = _exsltDateParseGDay(dt, &cur);
    if ((ret != 0) || !VALID_DATE(dt))
	goto error;

    /* is it an xs:date? */
    RETURN_TYPE_IF_VALID(XS_DATE);

    if (*cur != 'T')
	goto error;
    cur++;

    /* it should be an xs:dateTime */
    ret = _exsltDateParseTime(dt, &cur);
    if (ret != 0)
	goto error;

    ret = _exsltDateParseTimeZone(dt, &cur);
    if ((ret != 0) || (*cur != 0) || !VALID_DATETIME(dt))
	goto error;

    dt->type = XS_DATETIME;

    return dt;

error:
    if (dt != NULL)
	exsltDateFreeDate(dt);
    return NULL;
}

/**
 * exsltDateFormatDateTime:
 * @dt: an #exsltDate
 *
 * Formats @dt in xs:dateTime format.
 *
 * Returns a newly allocated string, or NULL in case of error
 */
static xmlChar *
exsltDateFormatDateTime (const exsltDatePtr dt) {
    xmlChar buf[100], *cur = buf;

    if ((dt == NULL) ||	!VALID_DATETIME(dt))
	return NULL;

    FORMAT_DATE(dt, cur);
    *cur = 'T';
    cur++;
    FORMAT_TIME(dt, cur);
    FORMAT_TZ(dt, cur);
    *cur = 0;

    return xmlStrdup(buf);
}

/**
 * exsltDateFormatDate:
 * @dt: an #exsltDate
 *
 * Formats @dt in xs:date format.
 *
 * Returns a newly allocated string, or NULL in case of error
 */
static xmlChar *
exsltDateFormatDate (const exsltDatePtr dt) {
    xmlChar buf[100], *cur = buf;

    if ((dt == NULL) || !VALID_DATETIME(dt))
	return NULL;

    FORMAT_DATE(dt, cur);
    if (dt->tz_flag || (dt->tzo != 0)) {
	FORMAT_TZ(dt, cur);
    }
    *cur = 0;

    return xmlStrdup(buf);
}

/**
 * exsltDateFormatTime:
 * @dt: an #exsltDate
 *
 * Formats @dt in xs:time format.
 *
 * Returns a newly allocated string, or NULL in case of error
 */
static xmlChar *
exsltDateFormatTime (const exsltDatePtr dt) {
    xmlChar buf[100], *cur = buf;

    if ((dt == NULL) || !VALID_TIME(dt))
	return NULL;

    FORMAT_TIME(dt, cur);
    if (dt->tz_flag || (dt->tzo != 0)) {
	FORMAT_TZ(dt, cur);
    }
    *cur = 0;

    return xmlStrdup(buf);
}

/****************************************************************
 *								*
 *		EXSLT - Dates and Times functions		*
 *								*
 ****************************************************************/

/**
 * exsltDateDateTime:
 *
 * Implements the EXSLT - Dates and Times date-time() function:
 *     string date:date-time()
 * 
 * Returns the current date and time as a date/time string.
 */
static xmlChar *
exsltDateDateTime (void) {
    xmlChar *ret = NULL;
#ifdef WITH_TIME
    exsltDatePtr cur;

    cur = exsltDateCurrent();
    if (cur != NULL) {
	ret = exsltDateFormatDateTime(cur);
	exsltDateFreeDate(cur);
    }
#endif

    return ret;
}

/**
 * exsltDateDate:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times date() function:
 *     string date:date (string?)
 * 
 * Returns the date specified in the date/time string given as the
 * argument.  If no argument is given, then the current local
 * date/time, as returned by date:date-time is used as a default
 * argument.
 * The date/time string specified as an argument must be a string in
 * the format defined as the lexical representation of either
 * xs:dateTime or xs:date.  If the argument is not in either of these
 * formats, returns NULL.
 */
static xmlChar *
exsltDateDate (const xmlChar *dateTime) {
    exsltDatePtr dt = NULL;
    xmlChar *ret = NULL;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return NULL;
    } else {
	dt = exsltDateParse(dateTime);
	if (dt == NULL)
	    return NULL;
	if ((dt->type != XS_DATETIME) && (dt->type != XS_DATE)) {
	    exsltDateFreeDate(dt);
	    return NULL;
	}
    }

    ret = exsltDateFormatDate(dt);
    exsltDateFreeDate(dt);

    return ret;
}

/**
 * exsltDateTime:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times time() function:
 *     string date:time (string?)
 * 
 * Returns the time specified in the date/time string given as the
 * argument.  If no argument is given, then the current local
 * date/time, as returned by date:date-time is used as a default
 * argument.
 * The date/time string specified as an argument must be a string in
 * the format defined as the lexical representation of either
 * xs:dateTime or xs:time.  If the argument is not in either of these
 * formats, returns NULL.
 */
static xmlChar *
exsltDateTime (const xmlChar *dateTime) {
    exsltDatePtr dt = NULL;
    xmlChar *ret = NULL;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return NULL;
    } else {
	dt = exsltDateParse(dateTime);
	if (dt == NULL)
	    return NULL;
	if ((dt->type != XS_DATETIME) && (dt->type != XS_TIME)) {
	    exsltDateFreeDate(dt);
	    return NULL;
	}
    }

    ret = exsltDateFormatTime(dt);
    exsltDateFreeDate(dt);

    return ret;
}

/**
 * exsltDateYear:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times year() function
 *    number date:year (string?)
 * Returns the year of a date as a number.  If no argument is given,
 * then the current local date/time, as returned by date:date-time is
 * used as a default argument.
 * The date/time string specified as the first argument must be a
 * right-truncated string in the format defined as the lexical
 * representation of xs:dateTime in one of the formats defined in [XML
 * Schema Part 2: Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date (CCYY-MM-DD)
 *  - xs:gYearMonth (CCYY-MM)
 *  - xs:gYear (CCYY)
 * If the date/time string is not in one of these formats, then NaN is
 * returned.
 */
static double
exsltDateYear (const xmlChar *dateTime) {
    exsltDatePtr dt;
    double ret;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return xmlXPathNAN;
    } else {
	dt = exsltDateParse(dateTime);
	if (dt == NULL)
	    return xmlXPathNAN;
	if ((dt->type != XS_DATETIME) && (dt->type != XS_DATE) &&
	    (dt->type != XS_GYEARMONTH) && (dt->type != XS_GYEAR)) {
	    exsltDateFreeDate(dt);
	    return xmlXPathNAN;
	}
    }

    ret = (double) dt->year;
    exsltDateFreeDate(dt);

    return ret;
}

/**
 * exsltDateLeapYear:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times leap-year() function:
 *    boolean date:leap-yea (string?)
 * Returns true if the year given in a date is a leap year.  If no
 * argument is given, then the current local date/time, as returned by
 * date:date-time is used as a default argument.
 * The date/time string specified as the first argument must be a
 * right-truncated string in the format defined as the lexical
 * representation of xs:dateTime in one of the formats defined in [XML
 * Schema Part 2: Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date (CCYY-MM-DD)
 *  - xs:gYearMonth (CCYY-MM)
 *  - xs:gYear (CCYY)
 * If the date/time string is not in one of these formats, then NaN is
 * returned.
 */
static xmlXPathObjectPtr
exsltDateLeapYear (const xmlChar *dateTime) {
    double year;
    int yr;

    year =  exsltDateYear(dateTime);
    if (xmlXPathIsNaN(year))
	return xmlXPathNewFloat(xmlXPathNAN);

    yr = (int) year;
    if (IS_LEAP(yr))
	return xmlXPathNewBoolean(1);

    return xmlXPathNewBoolean(0);
}

/**
 * exsltDateMonthInYear:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times month-in-year() function:
 *    number date:month-in-year (string?)
 * Returns the month of a date as a number.  If no argument is given,
 * then the current local date/time, as returned by date:date-time is
 * used the default argument.
 * The date/time string specified as the argument is a left or
 * right-truncated string in the format defined as the lexical
 * representation of xs:dateTime in one of the formats defined in [XML
 * Schema Part 2: Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date (CCYY-MM-DD)
 *  - xs:gYearMonth (CCYY-MM)
 *  - xs:gMonth (--MM--)
 *  - xs:gMonthDay (--MM-DD)
 * If the date/time string is not in one of these formats, then NaN is
 * returned.
 */
static double
exsltDateMonthInYear (const xmlChar *dateTime) {
    exsltDatePtr dt;
    double ret;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return xmlXPathNAN;
    } else {
	dt = exsltDateParse(dateTime);
	if (dt == NULL)
	    return xmlXPathNAN;
	if ((dt->type != XS_DATETIME) && (dt->type != XS_DATE) &&
	    (dt->type != XS_GYEARMONTH) && (dt->type != XS_GMONTH) &&
	    (dt->type != XS_GMONTHDAY)) {
	    exsltDateFreeDate(dt);
	    return xmlXPathNAN;
	}
    }

    ret = (double) dt->mon;
    exsltDateFreeDate(dt);

    return ret;
}

/**
 * exsltDateMonthName:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Time month-name() function
 *    string date:month-name (string?)
 * Returns the full name of the month of a date.  If no argument is
 * given, then the current local date/time, as returned by
 * date:date-time is used the default argument.
 * The date/time string specified as the argument is a left or
 * right-truncated string in the format defined as the lexical
 * representation of xs:dateTime in one of the formats defined in [XML
 * Schema Part 2: Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date (CCYY-MM-DD)
 *  - xs:gYearMonth (CCYY-MM)
 *  - xs:gMonth (--MM--)
 * If the date/time string is not in one of these formats, then an
 * empty string ('') is returned.
 * The result is an English month name: one of 'January', 'February',
 * 'March', 'April', 'May', 'June', 'July', 'August', 'September',
 * 'October', 'November' or 'December'.
 */
static const xmlChar *
exsltDateMonthName (const xmlChar *dateTime) {
    static const xmlChar monthNames[13][10] = {
        { 0 },
	{ 'J', 'a', 'n', 'u', 'a', 'r', 'y', 0 },
	{ 'F', 'e', 'b', 'r', 'u', 'a', 'r', 'y', 0 },
	{ 'M', 'a', 'r', 'c', 'h', 0 },
	{ 'A', 'p', 'r', 'i', 'l', 0 },
	{ 'M', 'a', 'y', 0 },
	{ 'J', 'u', 'n', 'e', 0 },
	{ 'J', 'u', 'l', 'y', 0 },
	{ 'A', 'u', 'g', 'u', 's', 't', 0 },
	{ 'S', 'e', 'p', 't', 'e', 'm', 'b', 'e', 'r', 0 },
	{ 'O', 'c', 't', 'o', 'b', 'e', 'r', 0 },
	{ 'N', 'o', 'v', 'e', 'm', 'b', 'e', 'r', 0 },
	{ 'D', 'e', 'c', 'e', 'm', 'b', 'e', 'r', 0 }
    };
    int month;
    month = exsltDateMonthInYear(dateTime);
    if (!VALID_MONTH(month))
      month = 0;
    return monthNames[month];
}

/**
 * exsltDateMonthAbbreviation:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Time month-abbreviation() function
 *    string date:month-abbreviation (string?)
 * Returns the abbreviation of the month of a date.  If no argument is
 * given, then the current local date/time, as returned by
 * date:date-time is used the default argument.
 * The date/time string specified as the argument is a left or
 * right-truncated string in the format defined as the lexical
 * representation of xs:dateTime in one of the formats defined in [XML
 * Schema Part 2: Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date (CCYY-MM-DD)
 *  - xs:gYearMonth (CCYY-MM)
 *  - xs:gMonth (--MM--)
 * If the date/time string is not in one of these formats, then an
 * empty string ('') is returned.
 * The result is an English month abbreviation: one of 'Jan', 'Feb',
 * 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov' or
 * 'Dec'.
 */
static const xmlChar *
exsltDateMonthAbbreviation (const xmlChar *dateTime) {
    static const xmlChar monthAbbreviations[13][4] = {
        { 0 },
	{ 'J', 'a', 'n', 0 },
	{ 'F', 'e', 'b', 0 },
	{ 'M', 'a', 'r', 0 },
	{ 'A', 'p', 'r', 0 },
	{ 'M', 'a', 'y', 0 },
	{ 'J', 'u', 'n', 0 },
	{ 'J', 'u', 'l', 0 },
	{ 'A', 'u', 'g', 0 },
	{ 'S', 'e', 'p', 0 },
	{ 'O', 'c', 't', 0 },
	{ 'N', 'o', 'v', 0 },
	{ 'D', 'e', 'c', 0 }
    };
    int month;
    month = exsltDateMonthInYear(dateTime);
    if(!VALID_MONTH(month))
      month = 0;
    return monthAbbreviations[month];
}

/**
 * _exsltDayInWeek:
 * @yday: year day (1-366)
 * @yr: year
 *
 * Determine the day-in-week from @yday and @yr. 0001-01-01 was
 * a Monday so all other days are calculated from there. Take the 
 * number of years since (or before) add the number of leap years and
 * the day-in-year and mod by 7. This is a function  because negative
 * years must be handled a little differently and there is no zero year.
 *
 * Returns day in week (Sunday = 0)
 */
static int
_exsltDateDayInWeek(int yday, long yr)
{
    int ret;

    if (yr < 0) {
        ret = ((yr + (((yr+1)/4)-((yr+1)/100)+((yr+1)/400)) + yday) % 7);
        if (ret < 0) 
            ret += 7;
    } else
        ret = (((yr-1) + (((yr-1)/4)-((yr-1)/100)+((yr-1)/400)) + yday) % 7);

    return ret;
}

/**
 * exsltDateWeekInYear:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times week-in-year() function
 *    number date:week-in-year (string?)
 * Returns the week of the year as a number.  If no argument is given,
 * then the current local date/time, as returned by date:date-time is
 * used as the default argument.  For the purposes of numbering,
 * counting follows ISO 8601: week 1 in a year is the week containing
 * the first Thursday of the year, with new weeks beginning on a
 * Monday.
 * The date/time string specified as the argument is a right-truncated
 * string in the format defined as the lexical representation of
 * xs:dateTime in one of the formats defined in [XML Schema Part 2:
 * Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date (CCYY-MM-DD)
 * If the date/time string is not in one of these formats, then NaN is
 * returned.
 */
static double
exsltDateWeekInYear (const xmlChar *dateTime) {
    exsltDatePtr dt;
    int fdiy, fdiw, ret;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return xmlXPathNAN;
    } else {
	dt = exsltDateParse(dateTime);
	if (dt == NULL)
	    return xmlXPathNAN;
	if ((dt->type != XS_DATETIME) && (dt->type != XS_DATE)) {
	    exsltDateFreeDate(dt);
	    return xmlXPathNAN;
	}
    }

    fdiy = DAY_IN_YEAR(1, 1, dt->year);
    
    /*
     * Determine day-in-week (0=Sun, 1=Mon, etc.) then adjust so Monday
     * is the first day-in-week
     */
    fdiw = (_exsltDateDayInWeek(fdiy, dt->year) + 6) % 7;

    ret = DAY_IN_YEAR(dt->day, dt->mon, dt->year) / 7;

    /* ISO 8601 adjustment, 3 is Thu */
    if (fdiw <= 3)
	ret += 1;

    exsltDateFreeDate(dt);

    return (double) ret;
}

/**
 * exsltDateWeekInMonth:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times week-in-month() function
 *    number date:week-in-month (string?)
 * The date:week-in-month function returns the week in a month of a
 * date as a number. If no argument is given, then the current local
 * date/time, as returned by date:date-time is used the default
 * argument. For the purposes of numbering, the first day of the month
 * is in week 1 and new weeks begin on a Monday (so the first and last
 * weeks in a month will often have less than 7 days in them).
 * The date/time string specified as the argument is a right-truncated
 * string in the format defined as the lexical representation of
 * xs:dateTime in one of the formats defined in [XML Schema Part 2:
 * Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date (CCYY-MM-DD)
 * If the date/time string is not in one of these formats, then NaN is
 * returned.
 */
static double
exsltDateWeekInMonth (const xmlChar *dateTime) {
    exsltDatePtr dt;
    int fdiy, fdiw, ret;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return xmlXPathNAN;
    } else {
	dt = exsltDateParse(dateTime);
	if (dt == NULL)
	    return xmlXPathNAN;
	if ((dt->type != XS_DATETIME) && (dt->type != XS_DATE)) {
	    exsltDateFreeDate(dt);
	    return xmlXPathNAN;
	}
    }

    fdiy = DAY_IN_YEAR(1, dt->mon, dt->year);
    /*
     * Determine day-in-week (0=Sun, 1=Mon, etc.) then adjust so Monday
     * is the first day-in-week
     */
    fdiw = (_exsltDateDayInWeek(fdiy, dt->year) + 6) % 7;

    ret = ((dt->day + fdiw) / 7) + 1;

    exsltDateFreeDate(dt);

    return (double) ret;
}

/**
 * exsltDateDayInYear:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times day-in-year() function
 *    number date:day-in-year (string?)
 * Returns the day of a date in a year as a number.  If no argument is
 * given, then the current local date/time, as returned by
 * date:date-time is used the default argument.
 * The date/time string specified as the argument is a right-truncated
 * string in the format defined as the lexical representation of
 * xs:dateTime in one of the formats defined in [XML Schema Part 2:
 * Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date (CCYY-MM-DD)
 * If the date/time string is not in one of these formats, then NaN is
 * returned.
 */
static double
exsltDateDayInYear (const xmlChar *dateTime) {
    exsltDatePtr dt;
    int ret;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return xmlXPathNAN;
    } else {
	dt = exsltDateParse(dateTime);
	if (dt == NULL)
	    return xmlXPathNAN;
	if ((dt->type != XS_DATETIME) && (dt->type != XS_DATE)) {
	    exsltDateFreeDate(dt);
	    return xmlXPathNAN;
	}
    }

    ret = DAY_IN_YEAR(dt->day, dt->mon, dt->year);

    exsltDateFreeDate(dt);

    return (double) ret;
}

/**
 * exsltDateDayInMonth:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times day-in-month() function:
 *    number date:day-in-month (string?)
 * Returns the day of a date as a number.  If no argument is given,
 * then the current local date/time, as returned by date:date-time is
 * used the default argument.
 * The date/time string specified as the argument is a left or
 * right-truncated string in the format defined as the lexical
 * representation of xs:dateTime in one of the formats defined in [XML
 * Schema Part 2: Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date (CCYY-MM-DD)
 *  - xs:gMonthDay (--MM-DD)
 *  - xs:gDay (---DD)
 * If the date/time string is not in one of these formats, then NaN is
 * returned.
 */
static double
exsltDateDayInMonth (const xmlChar *dateTime) {
    exsltDatePtr dt;
    double ret;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return xmlXPathNAN;
    } else {
	dt = exsltDateParse(dateTime);
	if (dt == NULL)
	    return xmlXPathNAN;
	if ((dt->type != XS_DATETIME) && (dt->type != XS_DATE) &&
	    (dt->type != XS_GMONTHDAY) && (dt->type != XS_GDAY)) {
	    exsltDateFreeDate(dt);
	    return xmlXPathNAN;
	}
    }

    ret = (double) dt->day;
    exsltDateFreeDate(dt);

    return ret;
}

/**
 * exsltDateDayOfWeekInMonth:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times day-of-week-in-month()
 * function
 *    number date:day-of-week-in-month (string?)
 * Returns the day-of-the-week in a month of a date as a number
 * (e.g. 3 for the 3rd Tuesday in May).  If no argument is
 * given, then the current local date/time, as returned by
 * date:date-time is used the default argument.
 * The date/time string specified as the argument is a right-truncated
 * string in the format defined as the lexical representation of
 * xs:dateTime in one of the formats defined in [XML Schema Part 2:
 * Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date (CCYY-MM-DD)
 * If the date/time string is not in one of these formats, then NaN is
 * returned.
 */
static double
exsltDateDayOfWeekInMonth (const xmlChar *dateTime) {
    exsltDatePtr dt;
    int ret;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return xmlXPathNAN;
    } else {
	dt = exsltDateParse(dateTime);
	if (dt == NULL)
	    return xmlXPathNAN;
	if ((dt->type != XS_DATETIME) && (dt->type != XS_DATE)) {
	    exsltDateFreeDate(dt);
	    return xmlXPathNAN;
	}
    }

    ret = (dt->day / 7) + 1;

    exsltDateFreeDate(dt);

    return (double) ret;
}

/**
 * exsltDateDayInWeek:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times day-in-week() function:
 *    number date:day-in-week (string?)
 * Returns the day of the week given in a date as a number.  If no
 * argument is given, then the current local date/time, as returned by
 * date:date-time is used the default argument.
 * The date/time string specified as the argument is a left or
 * right-truncated string in the format defined as the lexical
 * representation of xs:dateTime in one of the formats defined in [XML
 * Schema Part 2: Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date (CCYY-MM-DD)
 * If the date/time string is not in one of these formats, then NaN is
 * returned.
 * The numbering of days of the week starts at 1 for Sunday, 2 for
 * Monday and so on up to 7 for Saturday.
 */
static double
exsltDateDayInWeek (const xmlChar *dateTime) {
    exsltDatePtr dt;
    int diy;
    double ret;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return xmlXPathNAN;
    } else {
	dt = exsltDateParse(dateTime);
	if (dt == NULL)
	    return xmlXPathNAN;
	if ((dt->type != XS_DATETIME) && (dt->type != XS_DATE)) {
	    exsltDateFreeDate(dt);
	    return xmlXPathNAN;
	}
    }

    diy = DAY_IN_YEAR(dt->day, dt->mon, dt->year);

    ret = (double) _exsltDateDayInWeek(diy, dt->year) + 1;

    exsltDateFreeDate(dt);

    return ret;
}

/**
 * exsltDateDayName:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Time day-name() function
 *    string date:day-name (string?)
 * Returns the full name of the day of the week of a date.  If no
 * argument is given, then the current local date/time, as returned by
 * date:date-time is used the default argument.
 * The date/time string specified as the argument is a left or
 * right-truncated string in the format defined as the lexical
 * representation of xs:dateTime in one of the formats defined in [XML
 * Schema Part 2: Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date (CCYY-MM-DD)
 * If the date/time string is not in one of these formats, then an
 * empty string ('') is returned.
 * The result is an English day name: one of 'Sunday', 'Monday',
 * 'Tuesday', 'Wednesday', 'Thursday' or 'Friday'.
 */
static const xmlChar *
exsltDateDayName (const xmlChar *dateTime) {
    static const xmlChar dayNames[8][10] = {
        { 0 },
	{ 'S', 'u', 'n', 'd', 'a', 'y', 0 },
	{ 'M', 'o', 'n', 'd', 'a', 'y', 0 },
	{ 'T', 'u', 'e', 's', 'd', 'a', 'y', 0 },
	{ 'W', 'e', 'd', 'n', 'e', 's', 'd', 'a', 'y', 0 },
	{ 'T', 'h', 'u', 'r', 's', 'd', 'a', 'y', 0 },
	{ 'F', 'r', 'i', 'd', 'a', 'y', 0 },
	{ 'S', 'a', 't', 'u', 'r', 'd', 'a', 'y', 0 }
    };
    int day;
    day = exsltDateDayInWeek(dateTime);
    if((day < 1) || (day > 7))
      day = 0;
    return dayNames[day];
}

/**
 * exsltDateDayAbbreviation:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Time day-abbreviation() function
 *    string date:day-abbreviation (string?)
 * Returns the abbreviation of the day of the week of a date.  If no
 * argument is given, then the current local date/time, as returned by
 * date:date-time is used the default argument.
 * The date/time string specified as the argument is a left or
 * right-truncated string in the format defined as the lexical
 * representation of xs:dateTime in one of the formats defined in [XML
 * Schema Part 2: Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:date (CCYY-MM-DD)
 * If the date/time string is not in one of these formats, then an
 * empty string ('') is returned.
 * The result is a three-letter English day abbreviation: one of
 * 'Sun', 'Mon', 'Tue', 'Wed', 'Thu' or 'Fri'.
 */
static const xmlChar *
exsltDateDayAbbreviation (const xmlChar *dateTime) {
    static const xmlChar dayAbbreviations[8][4] = {
        { 0 },
	{ 'S', 'u', 'n', 0 },
	{ 'M', 'o', 'n', 0 },
	{ 'T', 'u', 'e', 0 },
	{ 'W', 'e', 'd', 0 },
	{ 'T', 'h', 'u', 0 },
	{ 'F', 'r', 'i', 0 },
	{ 'S', 'a', 't', 0 }
    };
    int day;
    day = exsltDateDayInWeek(dateTime);
    if((day < 1) || (day > 7))
      day = 0;
    return dayAbbreviations[day];
}

/**
 * exsltDateHourInDay:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times day-in-month() function:
 *    number date:day-in-month (string?)
 * Returns the hour of the day as a number.  If no argument is given,
 * then the current local date/time, as returned by date:date-time is
 * used the default argument.
 * The date/time string specified as the argument is a left or
 * right-truncated string in the format defined as the lexical
 * representation of xs:dateTime in one of the formats defined in [XML
 * Schema Part 2: Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:time (hh:mm:ss)
 * If the date/time string is not in one of these formats, then NaN is
 * returned.
 */
static double
exsltDateHourInDay (const xmlChar *dateTime) {
    exsltDatePtr dt;
    double ret;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return xmlXPathNAN;
    } else {
	dt = exsltDateParse(dateTime);
	if (dt == NULL)
	    return xmlXPathNAN;
	if ((dt->type != XS_DATETIME) && (dt->type != XS_TIME)) {
	    exsltDateFreeDate(dt);
	    return xmlXPathNAN;
	}
    }

    ret = (double) dt->hour;
    exsltDateFreeDate(dt);

    return ret;
}

/**
 * exsltDateMinuteInHour:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times day-in-month() function:
 *    number date:day-in-month (string?)
 * Returns the minute of the hour as a number.  If no argument is
 * given, then the current local date/time, as returned by
 * date:date-time is used the default argument.
 * The date/time string specified as the argument is a left or
 * right-truncated string in the format defined as the lexical
 * representation of xs:dateTime in one of the formats defined in [XML
 * Schema Part 2: Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:time (hh:mm:ss)
 * If the date/time string is not in one of these formats, then NaN is
 * returned.
 */
static double
exsltDateMinuteInHour (const xmlChar *dateTime) {
    exsltDatePtr dt;
    double ret;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return xmlXPathNAN;
    } else {
	dt = exsltDateParse(dateTime);
	if (dt == NULL)
	    return xmlXPathNAN;
	if ((dt->type != XS_DATETIME) && (dt->type != XS_TIME)) {
	    exsltDateFreeDate(dt);
	    return xmlXPathNAN;
	}
    }

    ret = (double) dt->min;
    exsltDateFreeDate(dt);

    return ret;
}

/**
 * exsltDateSecondInMinute:
 * @dateTime: a date/time string
 *
 * Implements the EXSLT - Dates and Times second-in-minute() function:
 *    number date:day-in-month (string?)
 * Returns the second of the minute as a number.  If no argument is
 * given, then the current local date/time, as returned by
 * date:date-time is used the default argument.
 * The date/time string specified as the argument is a left or
 * right-truncated string in the format defined as the lexical
 * representation of xs:dateTime in one of the formats defined in [XML
 * Schema Part 2: Datatypes].  The permitted formats are as follows:
 *  - xs:dateTime (CCYY-MM-DDThh:mm:ss)
 *  - xs:time (hh:mm:ss)
 * If the date/time string is not in one of these formats, then NaN is
 * returned.
 */
static double
exsltDateSecondInMinute (const xmlChar *dateTime) {
    exsltDatePtr dt;
    double ret;

    if (dateTime == NULL) {
#ifdef WITH_TIME
	dt = exsltDateCurrent();
	if (dt == NULL)
#endif
	    return xmlXPathNAN;
    } else {
	dt = exsltDateParse(dateTime);
	if (dt == NULL)
	    return xmlXPathNAN;
	if ((dt->type != XS_DATETIME) && (dt->type != XS_TIME)) {
	    exsltDateFreeDate(dt);
	    return xmlXPathNAN;
	}
    }

    ret = dt->sec;
    exsltDateFreeDate(dt);

    return ret;
}


/****************************************************************
 *								*
 *		Wrappers for use by the XPath engine		*
 *								*
 ****************************************************************/

#ifdef WITH_TIME
/**
 * exsltDateDateTimeFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps #exsltDateDateTime for use by the XPath engine
 */
static void
exsltDateDateTimeFunction (xmlXPathParserContextPtr ctxt, int nargs) {
    xmlChar *ret;

    if (nargs != 0) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    ret = exsltDateDateTime();
    xmlXPathReturnString(ctxt, ret);
}
#endif

/**
 * exsltDateDateFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps #exsltDateDate for use by the XPath engine
 */
static void
exsltDateDateFunction (xmlXPathParserContextPtr ctxt, int nargs) {
    xmlChar *ret, *dt = NULL;

    if ((nargs < 0) || (nargs > 1)) {
	xmlXPathSetArityError(ctxt);
	return;
    }
    if (nargs == 1) {
	dt = xmlXPathPopString(ctxt);
	if (xmlXPathCheckError(ctxt)) {
	    xmlXPathSetTypeError(ctxt);
	    return;
	}
    }

    ret = exsltDateDate(dt);

    if (ret == NULL) {
	xsltGenericDebug(xsltGenericDebugContext,
			 "{http://exslt.org/dates-and-times}date: "
			 "invalid date or format %s\n", dt);
	xmlXPathReturnEmptyString(ctxt);
    } else {
	xmlXPathReturnString(ctxt, ret);
    }

    if (dt != NULL)
	xmlFree(dt);
}

/**
 * exsltDateTimeFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps #exsltDateTime for use by the XPath engine
 */
static void
exsltDateTimeFunction (xmlXPathParserContextPtr ctxt, int nargs) {
    xmlChar *ret, *dt = NULL;

    if ((nargs < 0) || (nargs > 1)) {
	xmlXPathSetArityError(ctxt);
	return;
    }
    if (nargs == 1) {
	dt = xmlXPathPopString(ctxt);
	if (xmlXPathCheckError(ctxt)) {
	    xmlXPathSetTypeError(ctxt);
	    return;
	}
    }

    ret = exsltDateTime(dt);

    if (ret == NULL) {
	xsltGenericDebug(xsltGenericDebugContext,
			 "{http://exslt.org/dates-and-times}time: "
			 "invalid date or format %s\n", dt);
	xmlXPathReturnEmptyString(ctxt);
    } else {
	xmlXPathReturnString(ctxt, ret);
    }

    if (dt != NULL)
	xmlFree(dt);
}

/**
 * exsltDateYearFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps #exsltDateYear for use by the XPath engine
 */
static void
exsltDateYearFunction (xmlXPathParserContextPtr ctxt, int nargs) {
    xmlChar *dt = NULL;
    double ret;

    if ((nargs < 0) || (nargs > 1)) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    if (nargs == 1) {
	dt = xmlXPathPopString(ctxt);
	if (xmlXPathCheckError(ctxt)) {
	    xmlXPathSetTypeError(ctxt);
	    return;
	}
    }

    ret = exsltDateYear(dt);

    if (dt != NULL)
	xmlFree(dt);

    xmlXPathReturnNumber(ctxt, ret);
}

/**
 * exsltDateLeapYearFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps #exsltDateLeapYear for use by the XPath engine
 */
static void
exsltDateLeapYearFunction (xmlXPathParserContextPtr ctxt,
			   int nargs) {
    xmlChar *dt = NULL;
    xmlXPathObjectPtr ret;

    if ((nargs < 0) || (nargs > 1)) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    if (nargs == 1) {
	dt = xmlXPathPopString(ctxt);
	if (xmlXPathCheckError(ctxt)) {
	    xmlXPathSetTypeError(ctxt);
	    return;
	}
    }

    ret = exsltDateLeapYear(dt);

    if (dt != NULL)
	xmlFree(dt);

    valuePush(ctxt, ret);
}

#define X_IN_Y(x, y)						\
static void							\
exsltDate##x##In##y##Function (xmlXPathParserContextPtr ctxt,	\
			      int nargs) {			\
    xmlChar *dt = NULL;						\
    double ret;							\
								\
    if ((nargs < 0) || (nargs > 1)) {				\
	xmlXPathSetArityError(ctxt);				\
	return;							\
    }								\
								\
    if (nargs == 1) {						\
	dt = xmlXPathPopString(ctxt);				\
	if (xmlXPathCheckError(ctxt)) {				\
	    xmlXPathSetTypeError(ctxt);				\
	    return;						\
	}							\
    }								\
								\
    ret = exsltDate##x##In##y(dt);				\
								\
    if (dt != NULL)						\
	xmlFree(dt);						\
								\
    xmlXPathReturnNumber(ctxt, ret);				\
}

/**
 * exsltDateMonthInYearFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps #exsltDateMonthInYear for use by the XPath engine
 */
X_IN_Y(Month,Year)

/**
 * exsltDateMonthNameFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps #exsltDateMonthName for use by the XPath engine
 */
static void
exsltDateMonthNameFunction (xmlXPathParserContextPtr ctxt,
			    int nargs) {
    xmlChar *dt = NULL;
    const xmlChar *ret;

    if ((nargs < 0) || (nargs > 1)) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    if (nargs == 1) {
	dt = xmlXPathPopString(ctxt);
	if (xmlXPathCheckError(ctxt)) {
	    xmlXPathSetTypeError(ctxt);
	    return;
	}
    }

    ret = exsltDateMonthName(dt);

    if (dt != NULL)
	xmlFree(dt);

    if (ret == NULL)
	xmlXPathReturnEmptyString(ctxt);
    else
	xmlXPathReturnString(ctxt, xmlStrdup(ret));
}

/**
 * exsltDateMonthAbbreviationFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps #exsltDateMonthAbbreviation for use by the XPath engine
 */
static void
exsltDateMonthAbbreviationFunction (xmlXPathParserContextPtr ctxt,
			    int nargs) {
    xmlChar *dt = NULL;
    const xmlChar *ret;

    if ((nargs < 0) || (nargs > 1)) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    if (nargs == 1) {
	dt = xmlXPathPopString(ctxt);
	if (xmlXPathCheckError(ctxt)) {
	    xmlXPathSetTypeError(ctxt);
	    return;
	}
    }

    ret = exsltDateMonthAbbreviation(dt);

    if (dt != NULL)
	xmlFree(dt);

    if (ret == NULL)
	xmlXPathReturnEmptyString(ctxt);
    else
	xmlXPathReturnString(ctxt, xmlStrdup(ret));
}

/**
 * exsltDateWeekInYearFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps #exsltDateWeekInYear for use by the XPath engine
 */
X_IN_Y(Week,Year)

/**
 * exsltDateWeekInMonthFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps #exsltDateWeekInMonthYear for use by the XPath engine
 */
X_IN_Y(Week,Month)

/**
 * exsltDateDayInYearFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps #exsltDateDayInYear for use by the XPath engine
 */
X_IN_Y(Day,Year)

/**
 * exsltDateDayInMonthFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps #exsltDateDayInMonth for use by the XPath engine
 */
X_IN_Y(Day,Month)

/**
 * exsltDateDayOfWeekInMonthFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps #exsltDayOfWeekInMonth for use by the XPath engine
 */
X_IN_Y(DayOfWeek,Month)

/**
 * exsltDateDayInWeekFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps #exsltDateDayInWeek for use by the XPath engine
 */
X_IN_Y(Day,Week)

/**
 * exsltDateDayNameFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps #exsltDateDayName for use by the XPath engine
 */
static void
exsltDateDayNameFunction (xmlXPathParserContextPtr ctxt,
			    int nargs) {
    xmlChar *dt = NULL;
    const xmlChar *ret;

    if ((nargs < 0) || (nargs > 1)) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    if (nargs == 1) {
	dt = xmlXPathPopString(ctxt);
	if (xmlXPathCheckError(ctxt)) {
	    xmlXPathSetTypeError(ctxt);
	    return;
	}
    }

    ret = exsltDateDayName(dt);

    if (dt != NULL)
	xmlFree(dt);

    if (ret == NULL)
	xmlXPathReturnEmptyString(ctxt);
    else
	xmlXPathReturnString(ctxt, xmlStrdup(ret));
}

/**
 * exsltDateMonthDayFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps #exsltDateDayAbbreviation for use by the XPath engine
 */
static void
exsltDateDayAbbreviationFunction (xmlXPathParserContextPtr ctxt,
			    int nargs) {
    xmlChar *dt = NULL;
    const xmlChar *ret;

    if ((nargs < 0) || (nargs > 1)) {
	xmlXPathSetArityError(ctxt);
	return;
    }

    if (nargs == 1) {
	dt = xmlXPathPopString(ctxt);
	if (xmlXPathCheckError(ctxt)) {
	    xmlXPathSetTypeError(ctxt);
	    return;
	}
    }

    ret = exsltDateDayAbbreviation(dt);

    if (dt != NULL)
	xmlFree(dt);

    if (ret == NULL)
	xmlXPathReturnEmptyString(ctxt);
    else
	xmlXPathReturnString(ctxt, xmlStrdup(ret));
}


/**
 * exsltDateHourInDayFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps #exsltDateHourInDay for use by the XPath engine
 */
X_IN_Y(Hour,Day)

/**
 * exsltDateMinuteInHourFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps #exsltDateMinuteInHour for use by the XPath engine
 */
X_IN_Y(Minute,Hour)

/**
 * exsltDateSecondInMinuteFunction:
 * @ctxt: an XPath parser context
 * @nargs : the number of arguments
 *
 * Wraps #exsltDateSecondInMinute for use by the XPath engine
 */
X_IN_Y(Second,Minute)

/**
 * exsltDateRegister:
 *
 * Registers the EXSLT - Dates and Times module
 */
void
exsltDateRegister(void)
{
#ifdef WITH_TIME
    xsltRegisterExtModuleFunction((const xmlChar *) "date-time",
			  (const xmlChar *) EXSLT_DATE_NAMESPACE,
			  exsltDateDateTimeFunction);
#endif
    xsltRegisterExtModuleFunction((const xmlChar *) "date",
			  (const xmlChar *) EXSLT_DATE_NAMESPACE,
			  exsltDateDateFunction);
    xsltRegisterExtModuleFunction((const xmlChar *) "time",
			  (const xmlChar *) EXSLT_DATE_NAMESPACE,
			  exsltDateTimeFunction);
    xsltRegisterExtModuleFunction((const xmlChar *) "year",
			  (const xmlChar *) EXSLT_DATE_NAMESPACE,
			  exsltDateYearFunction);
    xsltRegisterExtModuleFunction((const xmlChar *) "leap-year",
			  (const xmlChar *) EXSLT_DATE_NAMESPACE,
			  exsltDateLeapYearFunction);
    xsltRegisterExtModuleFunction((const xmlChar *) "month-in-year",
			  (const xmlChar *) EXSLT_DATE_NAMESPACE,
			  exsltDateMonthInYearFunction);
    xsltRegisterExtModuleFunction((const xmlChar *) "month-name",
			  (const xmlChar *) EXSLT_DATE_NAMESPACE,
			  exsltDateMonthNameFunction);
    xsltRegisterExtModuleFunction((const xmlChar *) "month-abbreviation",
			  (const xmlChar *) EXSLT_DATE_NAMESPACE,
			  exsltDateMonthAbbreviationFunction);
    xsltRegisterExtModuleFunction((const xmlChar *) "week-in-year",
			  (const xmlChar *) EXSLT_DATE_NAMESPACE,
			  exsltDateWeekInYearFunction);
    xsltRegisterExtModuleFunction((const xmlChar *) "week-in-month",
			  (const xmlChar *) EXSLT_DATE_NAMESPACE,
			  exsltDateWeekInMonthFunction);
    xsltRegisterExtModuleFunction((const xmlChar *) "day-in-year",
			  (const xmlChar *) EXSLT_DATE_NAMESPACE,
			  exsltDateDayInYearFunction);
    xsltRegisterExtModuleFunction((const xmlChar *) "day-in-month",
                            (const xmlChar *) EXSLT_DATE_NAMESPACE,
                            exsltDateDayInMonthFunction);
    xsltRegisterExtModuleFunction((const xmlChar *) "day-of-week-in-month",
                            (const xmlChar *) EXSLT_DATE_NAMESPACE,
                            exsltDateDayOfWeekInMonthFunction);
    xsltRegisterExtModuleFunction((const xmlChar *) "day-in-week",
                            (const xmlChar *) EXSLT_DATE_NAMESPACE,
                            exsltDateDayInWeekFunction);
    xsltRegisterExtModuleFunction((const xmlChar *) "day-name",
                            (const xmlChar *) EXSLT_DATE_NAMESPACE,
                            exsltDateDayNameFunction);
    xsltRegisterExtModuleFunction((const xmlChar *) "day-abbreviation",
                            (const xmlChar *) EXSLT_DATE_NAMESPACE,
                            exsltDateDayAbbreviationFunction);
    xsltRegisterExtModuleFunction((const xmlChar *) "hour-in-day",
                            (const xmlChar *) EXSLT_DATE_NAMESPACE,
                            exsltDateHourInDayFunction);
    xsltRegisterExtModuleFunction((const xmlChar *) "minute-in-hour",
                            (const xmlChar *) EXSLT_DATE_NAMESPACE,
                            exsltDateMinuteInHourFunction);
    xsltRegisterExtModuleFunction((const xmlChar *) "second-in-minute",
                            (const xmlChar *) EXSLT_DATE_NAMESPACE,
                            exsltDateSecondInMinuteFunction);
}
