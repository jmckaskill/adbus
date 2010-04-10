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

#include "mainloop.h"

static MT_ThreadStorage sEventLoops;

/* ------------------------------------------------------------------------- */

MT_MainLoop* MT_Loop_New(void)
{ 
    MT_MainLoop* s = NEW(MT_MainLoop);
    MT_Handle queueHandle;

#ifdef _WIN32
    s->time = INVALID_HANDLE_VALUE;
#endif

    MT_ThreadStorage_Ref(&sEventLoops);

    queueHandle = MTI_Queue_Init(&s->queue);
    MT_Loop_Register(s, queueHandle, MTI_Queue_Dispatch, &s->queue);

    return s;
}

/* ------------------------------------------------------------------------- */

void MT_Loop_Free(MT_MainLoop* s)
{ 
    MTI_Queue_Destroy(&s->queue);
    MT_ThreadStorage_Deref(&sEventLoops);

#ifdef _WIN32
    if (s->timer != INVALID_HANDLE_VALUE) {
        CloseHandle(s->timer);
    }
#endif

    dv_free(Handle, &s->handles);
    dv_free(LoopRegistration, &s->regs);
    dv_free(LoopIdle, &s->idle);

    free(s); 
}

/* ------------------------------------------------------------------------- */

void MT_SetCurrent(MT_MainLoop* s)
{ MT_ThreadStorage_Set(&sEventLoops, s); }

/* ------------------------------------------------------------------------- */

MT_MainLoop* MT_Current(void)
{ return (MT_MainLoop*) MT_ThreadStorage_Get(&sEventLoops); }

/* ------------------------------------------------------------------------- */

void MT_Loop_Register(MT_MainLoop* s, MT_Handle h, MT_Callback cb, void* user)
{
    MT_Handle* ph = dv_push(Handle, &s->handles, 1);
    MTI_LoopRegistration* r = dv_push(LoopRegistration, &s->regs, 1);

    *ph = h;
    r->cb = cb;
    r->user = user;
}

/* ------------------------------------------------------------------------- */

void MT_Loop_Unregister(MT_MainLoop* s, MT_Handle h)
{
    size_t i;
    for (i = 0; i < dv_size(&s->handles); i++) {
        if (dv_a(&s->handles, i) == h) {
            dv_erase(LoopRegistration, &s->regs, i, 1);
            dv_erase(Handle, &s->handles, i, 1);
            break;
        }
    }
}

/* ------------------------------------------------------------------------- */

void MT_Loop_AddIdle(MT_MainLoop* s, MT_Callback cb, void* user)
{
    MTI_LoopIdle* idle = dv_push(LoopIdle, &s->idle, 1);
    idle->cb = cb;
    idle->user = user;
}

/* ------------------------------------------------------------------------- */

void MT_Loop_RemoveIdle(MT_MainLoop* s, MT_Callback cb, void* user)
{
    dv_remove(LoopIdle, &s->idle, ENTRY->cb == cb && ENTRY->user == user);
}

/* ------------------------------------------------------------------------- */

void MTI_CallIdle(MT_MainLoop* s)
{
    size_t i;
    for (i = 0; i < dv_size(&s->idle); i++) {
        MTI_LoopIdle* idle = &dv_a(&s->idle, i);
        idle->cb(idle->user);
    }
}

/* ------------------------------------------------------------------------- */

int MT_Current_Run(void)
{
	MT_MainLoop* s = MT_Current();
    while (!s->exit) {
        if (MT_Current_Step()) {
            return -1;
        }
    }
    return s->exitcode;
}

/* ------------------------------------------------------------------------- */

void MT_Current_Exit(int code)
{
	MT_MainLoop* s = MT_Current();
    s->exit = 1;
    s->exitcode = code;
}

/* ------------------------------------------------------------------------- */

void MT_Loop_Post(MT_MainLoop* s, MT_Message* m)
{ MTI_Queue_Post(&s->queue, m); }


