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

/* To get localtime_r */
#define _POSIX_SOURCE

#include "internal.h"

#ifndef _WIN32

#include <sys/time.h>
#include <time.h>

/* ------------------------------------------------------------------------- */

MT_Time MT_CurrentTime()
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL))
        return MT_TIME_INVALID;

    return (MT_Time) (tv.tv_sec * 1000000UL + tv.tv_usec);
}

/* ------------------------------------------------------------------------- */

int MT_ToBrokenDownTime(MT_Time t, struct tm* tm)
{
    time_t timet = (time_t) MT_TIME_TO_SEC(t);
    if (localtime_r(&timet, tm) == NULL)
        return -1;

    return 0;
}

/* ------------------------------------------------------------------------- */

MT_Time MT_FromBrokenDownTime(struct tm* tm)
{
    time_t t = mktime(tm);
    if (t == -1)
        return MT_TIME_INVALID;

    return MT_TIME_FROM_SEC(t);
}

#endif

