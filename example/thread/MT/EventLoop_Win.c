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
#include "EventLoop.h"
#include "Lock.h"
#include "Thread.h"
#include "EventQueue_p.h"
#include "dmem/vector.h"
#include <windows.h>

struct Registration
{
    MT_Callback  cb;
    void*        user;
};

struct Idle
{
    MT_Callback cb;
    void*       user;
};

DVECTOR_INIT(HANDLE, HANDLE)
DVECTOR_INIT(Idle, struct Idle)
DVECTOR_INIT(Registration, struct Registration)

struct MT_EventLoop
{
    int                         exit;
    int                         exitcode;
    HANDLE                      timer;
    d_Vector(HANDLE)            handles;
    d_Vector(Registration)      regs;
    d_Vector(Idle)              idle;
    MTI_EventQueue*             queue;
};

static MT_ThreadStorage sEventLoops;

MT_EventLoop* MT_Loop_New(void)
{ 
    MT_EventLoop* e = NEW(MT_EventLoop);
    e->timer = INVALID_HANDLE_VALUE;

    MT_ThreadStorage_Ref(&sEventLoops);
    MT_ThreadStorage_Set(&sEventLoops, e);

    e->queue = MTI_Queue_New(e);

    return e;
}

void MT_Loop_Free(MT_EventLoop* e)
{ 
    MTI_Queue_Free(e->queue);

    MT_ThreadStorage_Set(&sEventLoops, NULL);
    MT_ThreadStorage_Deref(&sEventLoops);

    if (e->timer != INVALID_HANDLE_VALUE) {
        CloseHandle(e->timer);
    }
    free(e); 
}

MT_EventLoop* MT_Loop_Current(void)
{ return (MT_EventLoop*) MT_ThreadStorage_Get(&sEventLoops); }

void MT_Loop_SetTick(MT_EventLoop* e, MT_Time period, MT_Callback cb, void* user)
{
    if (period != MT_TIME_INVALID && period > 0) {

        if (e->timer == INVALID_HANDLE_VALUE) {
            e->timer = CreateEvent(NULL, FALSE, FALSE, NULL);
            timeSetEvent((UINT) (period / 1000), 0, (LPTIMECALLBACK) e->timer, (DWORD_PTR) NULL, TIME_PERIODIC | TIME_CALLBACK_EVENT_SET);
        } else {
            MT_Loop_Unregister(e, e->timer);
        }

        MT_Loop_Register(e, e->timer, cb, user);

    } else {

        if (e->timer != INVALID_HANDLE_VALUE) {
            MT_Loop_Unregister(e, e->timer);
            CloseHandle(e->timer);
            e->timer = INVALID_HANDLE_VALUE;
        }

    }
}

void MT_Loop_Register(MT_EventLoop* e, MT_Handle h, MT_Callback cb, void* user)
{
    HANDLE* ph = dv_push(HANDLE, &e->handles, 1);
    struct Registration* r = dv_push(Registration, &e->regs, 1);

    *ph = h;
    r->cb = cb;
    r->user = user;
}

void MT_Loop_Unregister(MT_EventLoop* e, MT_Handle h)
{
    size_t i;
    for (i = 0; i < dv_size(&e->handles); i++) {
        if (dv_a(&e->handles, i) == h) {
            dv_erase(Registration, &e->regs, i, 1);
            dv_erase(HANDLE, &e->handles, i, 1);
            break;
        }
    }
}

void MT_Loop_AddIdle(MT_EventLoop* e, MT_Callback cb, void* user)
{
    struct Idle* idle = dv_push(Idle, &e->idle, 1);
    idle->cb = cb;
    idle->user = user;
}

void MT_Loop_RemoveIdle(MT_EventLoop* e, MT_Callback cb, void* user)
{
    dv_remove(Idle, &e->idle, ENTRY->cb == cb && ENTRY->user == user);
}

static void RunIdle(MT_EventLoop* e)
{
    size_t i;
    for (i = 0; i < dv_size(&e->idle); i++) {
        struct Idle* idle = &dv_a(&e->idle, i);
        idle->cb(idle->user);
    }
}

int MT_Loop_Step(MT_EventLoop* e)
{
    struct Registration* r;
    DWORD ret;

    if (e->exit)
        return e->exitcode;

    RunIdle(e);
    ret = WaitForMultipleObjects((DWORD) dv_size(&e->handles), &dv_a(&e->handles, 0), FALSE, INFINITE);
    if (ret >= WAIT_OBJECT_0 + dv_size(&e->handles))
        return -1;

    r = &dv_a(&e->regs, ret);
    r->cb(r->user);

    return 0;
}

int MT_Loop_Run(MT_EventLoop* e)
{
    int ret = 0;
    while (!ret && !e->exit) {
        ret = MT_Loop_Step(e);
    }
    return ret;
}

void MT_Loop_Exit(MT_EventLoop* e, int code)
{
    e->exit = 1;
    e->exitcode = code;
}

void MT_Loop_Post(MT_EventLoop* e, MT_Message* m)
{ MTI_Queue_Post(e->queue, m); }

#endif

