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

#define _BSD_SOURCE
#include <stdlib.h>
#include <stdint.h>

#ifdef _WIN32

#include <windows.h>
struct Timer
{
    LARGE_INTEGER start, end, freq;
};

void StartTimer(struct Timer* t)
{
    QueryPerformanceFrequency(&t->freq);
    QueryPerformanceCounter(&t->start);
}

double StopTimer(struct Timer* t, int repeat)
{
    uint64_t ns;
    QueryPerformanceCounter(&t->end);
    ns = t->end.QuadPart - t->start.QuadPart;
    ns *= 1000000000;
    ns /= t->freq.QuadPart;
    ns /= repeat;
    return (double) ns;
}

#else

#include <sys/socket.h>
#include <sys/time.h>

struct Timer
{
    struct timeval start, end, diff;
};

void StartTimer(struct Timer* t)
{
    gettimeofday(&t->start, NULL);
}

double StopTimer(struct Timer* t, int repeat)
{
    uint64_t ns;
    gettimeofday(&t->end, NULL);
    timersub(&t->end, &t->start, &t->diff);
    ns = (t->diff.tv_sec * 1000000 + t->diff.tv_usec) * 1000 / repeat;
    return (double) ns;
}

#endif
