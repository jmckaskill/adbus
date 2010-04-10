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

#include "internal.h"
#include "message-queue.h"
#include <dmem/vector.h>

#ifndef _WIN32
#   include <sys/time.h>
#endif

typedef struct MTI_LoopRegistration MTI_LoopRegistration;
typedef struct MTI_LoopIdle MTI_LoopIdle;

struct MTI_LoopRegistration
{
    MT_Callback  cb;
    void*        user;
};

struct MTI_LoopIdle
{
    MT_Callback cb;
    void*       user;
};

DVECTOR_INIT(Handle, MT_Handle)
DVECTOR_INIT(LoopIdle, MTI_LoopIdle)
DVECTOR_INIT(LoopRegistration, MTI_LoopRegistration)

struct MT_MainLoop
{
    int                         exit;
    int                         exitcode;
    d_Vector(Handle)            handles;
    d_Vector(LoopRegistration)  regs;
    d_Vector(LoopIdle)          idle;
    MTI_MessageQueue            queue;

#ifdef _WIN32
    HANDLE                      timer;
#else
    MTI_LoopRegistration        tickreg;
    struct timeval              tick;
    struct timeval              nexttick;
#endif
};

void MTI_CallIdle(MT_MainLoop* e);

