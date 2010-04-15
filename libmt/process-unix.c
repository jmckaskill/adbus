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

#ifndef _WIN32
#include <dmem/vector.h>
#include <unistd.h>

int HW_Process_Start(const char* app, const char* dir, const char* args[], size_t argnum)
{
    char** execargs = (char**) alloca(sizeof(char*) * (argnum + 1));
    execargs[0] = app;
    memcpy(&execargs[1], args, argnum * sizeof(char*));
    switch (fork()) {
        case -1:
            /* Error in parent */
            return -1;

        case 0:
            /* Success in parent */
            return 0;

        default:
            /* Success in child */
            if (dir) {
                chdir(dir);
            }
            execv(app, execargs);
            return 0;
    }
}

#endif