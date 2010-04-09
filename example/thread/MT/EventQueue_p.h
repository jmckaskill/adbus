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
#include "EventLoop.h"
#include "Lock.h"

struct MTI_EventQueue
{
	MT_EventLoop*			loop;

    MT_AtomicPtr            last;

    char                    pad[16 - sizeof(MT_Spinlock) - sizeof(MT_Message*)];
    MT_Message*             first;
    MT_Message              dummy;

#ifdef _WIN32
    MT_Handle               handle;
#else
    int                     pipe[2];
#endif
};

void MTI_Queue_Init(MTI_EventQueue* q, MT_EventLoop* loop);
void MTI_Queue_Destroy(MTI_EventQueue* q);
void MTI_Queue_Dispatch(void* u);

MTI_EventQueue* MTI_Loop_Queue(MT_EventLoop* loop);
void MTI_Queue_Cancel(MTI_EventQueue* q, MT_Message* m);

/* Thread safe as long as the queue is not freed */
void MTI_Queue_Post(MTI_EventQueue* q, MT_Target* t, MT_Message* m);

