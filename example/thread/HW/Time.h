/* vim: ts=4 sw=4 sts=4 et
 *
 * Copyright (c) 2009 James R. McKaskill
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * ----------------------------------------------------------------------------
 */

#pragma once

#include "Common.h"
#include <stdint.h>
#include <time.h>

/* HW_Time gives the time in microseconds centered on the unix epoch (midnight Jan 1 1970) */
typedef int64_t                   HW_Time;
#define HW_TIME_INVALID           INT64_MAX
#define HW_TIME_ISVALID(x)        ((x) != HW_TIME_INVALID)

#define HW_TIME_FROM_US(x)        HW_Time(x)
#define HW_TIME_FROM_MS(x)        HW_Time(double(x) * 1000)
#define HW_TIME_FROM_SEC(x)       HW_Time(double(x) * 1000000)
#define HW_TIME_FROM_HOURS(x)     HW_Time(double(x) * 1000000 * 3600)
#define HW_TIME_FROM_DAYS(x)      HW_Time(double(x) * 1000000 * 3600 * 24)
#define HW_TIME_FROM_WEEKS(x)     HW_Time(double(x) * 1000000 * 3600 * 24 * 7)
#define HW_TIME_FROM_HZ(x)        HW_Time((1.0 / double(x)) * 1000000)

#define HW_TIME_TO_US(x)          (x)
#define HW_TIME_TO_MS(x)          (double(x) / 1000)
#define HW_TIME_TO_SEC(x)         (double(x) / 1000000)
#define HW_TIME_TO_HOURS(x)       (double(x) / 1000000 / 3600)
#define HW_TIME_TO_DAYS(x)        (double(x) / 1000000 / 3600 / 24 / 7)
#define HW_TIME_TO_WEEKS(x)       (double(x) / 1000000 / 3600 / 24 / 7)

#define HW_TIME_GPS_EPOCH         HW_TIME_FROM_SEC(315964800)


/* Functions to convert to/from broken down time. We use struct tm here
 * instead of SYSTEMTIME for portability. Broken-down time is stored in the
 * structure tm which is defined in <time.h> as follows:
 */

#if 0
struct tm {
    int tm_sec;         /* seconds */
    int tm_min;         /* minutes */
    int tm_hour;        /* hours */
    int tm_mday;        /* day of the month */
    int tm_mon;         /* month */
    int tm_year;        /* year */
    int tm_wday;        /* day of the week */
    int tm_yday;        /* day in the year */
    int tm_isdst;       /* daylight saving time */
};
#endif

/* The members of the tm structure are:
 *
 * tm_sec
 *   The number of seconds after the minute, normally in the range 0 to 59, but
 *   can be up to 60 to allow for leap seconds. 
 * tm_min
 *   The number of minutes after the hour, in the range 0 to 59. 
 * tm_hour
 *   The number of hours past midnight, in the range 0 to 23. 
 * tm_mday
 *   The day of the month, in the range 1 to 31. 
 * tm_mon
 *   The number of months since January, in the range 0 to 11. 
 * tm_year
 *   The number of years since 1900. 
 * tm_wday
 *   The number of days since Sunday, in the range 0 to 6. 
 * tm_yday
 *   The number of days since January 1, in the range 0 to 365. 
 * tm_isdst
 *   A flag that indicates whether daylight saving time is in effect at the time
 *   described. The value is positive if daylight saving time is in effect, zero
 *   if it is not, and negative if the information is not available.
 */
HW_API HW_Time HW_TIME_FROM_TM(struct tm* tm); /* Returns HW_TIME_INVALID on error */
HW_API int     HW_TIME_TO_TM(HW_Time t, struct tm* tm); /* Returns non zero on error */

HW_API HW_Time HW_CurrentTime();

HW_API char* HW_NewDateString(HW_Time t);
HW_API char* HW_NewDateTimeString(HW_Time t);
HW_API void  HW_FreeDateString(char* str);

#ifdef __cplusplus
	struct HW_DateString
	{
	  HW_DateString(char* str) : m_Str(str) {}
	  ~HW_DateString(){HW_FreeDateString(m_Str);}
	  char* m_Str;
	};

	/* These return ISO 8601 strings eg "2010-02-16" and "2010-02-16 22:00:08.067890Z" */
#	define HW_LogDateString(t) HW_DateString(HW_NewDateString(t)).m_Str
#	define HW_LogDateTimeString(t) HW_DateString(HW_NewDateTimeString(t)).m_Str
#endif


