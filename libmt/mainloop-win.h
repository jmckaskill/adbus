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

#ifdef _WIN32

DVECTOR_INIT(LoopRegistration, MT_LoopRegistration*)
DVECTOR_INIT(HANDLE, HANDLE)

struct MT_LoopRegistration
{
    MT_Socket                       socket;
    MT_Handle                       handle;

    int                             isSocket;
    long                            mask;
    long                            lNetworkEvents;

    MT_Callback                     read;
    MT_Callback                     write;
    MT_Callback                     close;
    MT_Callback                     accept;
    MT_Callback                     idle;
    MT_Callback                     cb;
    void*                           user;

    MT_Time                         period;
    MT_Time                         nextTick;
};

enum MTI_LoopStepState
{
    MTI_LOOP_INIT = 0,
    MTI_LOOP_EVENT,
    MTI_LOOP_IDLE,
};

struct MT_MainLoop
{
    int                             exit;
    int                             exitcode;
    
    d_Vector(LoopRegistration)      regs;
    d_Vector(HANDLE)                handles;
    int                             currentEvent;

    d_Vector(LoopRegistration)      idle;
    int                             currentIdle;

    d_Vector(LoopRegistration)      ticks;

    enum MTI_LoopStepState          state;

    MTI_MessageQueue                queue;
};

#endif

