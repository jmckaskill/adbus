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

#ifdef _WIN32

#define MT_LIBRARY
#include "Time.h"
#include <windows.h>

/* -------------------------------------------------------------------------- */

MT_Time MT_TIME_FROM_FILETIME(FILETIME* ft)
{
    uint64_t res = 0;
    res |= ft->dwHighDateTime;
    res <<= 32;
    res |= ft->dwLowDateTime;
    res /= 10; /*convert into microseconds*/

    /*converting file Time to unix epoch*/
    return (MT_Time) (res - INT64_C(11644473600000000));
}

void MT_TIME_TO_FILETIME(MT_Time t, FILETIME* ft)
{
    /*converting unix epoch to filetime epoch*/
    uint64_t res = t + UINT64_C(11644473600000000);
    res *= 10; /* convert to 100ns increments */

    ft->dwHighDateTime = (uint32_t) (res >> 32);
    ft->dwLowDateTime  = (uint32_t) res;
}

MT_Time MT_CurrentTime()
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    return MT_TIME_FROM_FILETIME(&ft);
}

/* -------------------------------------------------------------------------- */

// Use of localtime is discouraged since it may not be thread safe.  We can't
// reliably use localtime_s or mktime on windows since they are not available
// under wince.

static int IsLeapYear(int year)
{
    if (year % 400 == 0) {
        return 1;
    } else if (year % 100 == 0) {
        return 0;
    } else if (year % 4 == 0) {
        return 1;
    } else {
        return 0;
    }
}

static const int kDays[12]     = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
static const int kLeapDays[12] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335};

static int DayOfYear(int year, int month, int day)
{
    if (IsLeapYear(year)) {
        return kLeapDays[month] + day - 1;
    } else {
        return kDays[month] + day - 1;
    }
}

int MT_TIME_TO_TM(MT_Time t, struct tm* tm)
{
    FILETIME ft, lft;
    SYSTEMTIME st;

    MT_TIME_TO_FILETIME(t, &ft);
    if (!FileTimeToLocalFileTime(&ft, &lft))
        return -1;
    if (!FileTimeToSystemTime(&lft, &st))
        return -1;

    tm->tm_yday = DayOfYear(st.wYear, st.wMonth - 1, st.wDay);
    tm->tm_wday = st.wDayOfWeek;
    tm->tm_year = st.wYear - 1900;
    tm->tm_mon  = st.wMonth - 1;
    tm->tm_mday = st.wDay;
    tm->tm_hour = st.wHour;
    tm->tm_min  = st.wMinute;
    tm->tm_sec  = st.wSecond;
    tm->tm_isdst = -1;

    return 0;
}

MT_Time MT_TIME_FROM_TM(struct tm* tm)
{
    FILETIME ft, lft;
    SYSTEMTIME st;
    st.wYear      = (WORD) (tm->tm_year + 1900);
    st.wMonth     = (WORD) (tm->tm_mon + 1);
    st.wDay       = (WORD) tm->tm_mday;
    st.wHour      = (WORD) tm->tm_hour;
    st.wMinute    = (WORD) tm->tm_min;
    st.wSecond    = (WORD) tm->tm_sec;
    st.wMilliseconds = 0;

    if (!SystemTimeToFileTime(&st, &lft))
        return MT_TIME_INVALID;
    if (!LocalFileTimeToFileTime(&lft, &ft))
        return MT_TIME_INVALID;

    return MT_TIME_FROM_FILETIME(&ft);
}

#endif
