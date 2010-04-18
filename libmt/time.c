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

#include "internal.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifdef _MSC_VER
#   define snprintf _snprintf
#   define strdup _strdup
#endif

/* -------------------------------------------------------------------------- */

#define BUFSZ 128

char* MT_NewDateString(MT_Time t)
{
    struct tm tm;
    int ret;
    char* buf = (char*) malloc(BUFSZ + 1);

    buf[BUFSZ] = '\0';

    if (MT_ToBrokenDownTime(t, &tm)) {
        strcat(buf, "invalid date");
        return buf;
    }

    /* The fields are fixed size so can use sprintf */
    ret = sprintf(buf, "%d-%02d-%02d",
        (int) tm.tm_year + 1900,
        (int) tm.tm_mon + 1,
        (int) tm.tm_mday);

    (void) ret;
    assert(0 <= ret && ret <= BUFSZ);

    return buf;
}

/* -------------------------------------------------------------------------- */

char* MT_NewDateTimeString(MT_Time t)
{
    struct tm tm;
    int ret;
    char* buf = (char*) malloc(BUFSZ + 1);

    buf[BUFSZ] = '\0';

    if (MT_ToBrokenDownTime(t, &tm)) {
        strcat(buf, "invalid date");
        return buf;
    }

    /* The fields are fixed size so can use sprintf */
    ret = sprintf(buf, "%d-%02d-%02d %02d:%02d:%02d.%06dZ",
        (int) tm.tm_year + 1900,
        (int) tm.tm_mon + 1,
        (int) tm.tm_mday,
        (int) tm.tm_hour,
        (int) tm.tm_min,
        (int) tm.tm_sec,
        (int) (MT_TIME_TO_US(t) % 1000000));

    (void) ret;
    assert(0 <= ret && ret <= BUFSZ);

    return buf;
}

/* -------------------------------------------------------------------------- */

void MT_FreeDateString(char* str)
{ free(str); }

