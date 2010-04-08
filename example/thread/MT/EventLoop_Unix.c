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

#ifndef _WIN32

#define MT_LIBRARY
#include "EventLoop.h"
#include "Thread.h"
#include "EventQueue_p.h"
#include "dmem/vector.h"
#include <sys/time.h>
#include <errno.h>

typedef struct Registration Registration;

struct Registration
{
    int         fd;
    MT_Callback cb;
    void*       user;
};

struct Idle
{
    MT_Callback cb;
    void*       user;
};

DVECTOR_INIT(Registration, struct Registration)
DVECTOR_INIT(Idle, struct Idle)

struct MT_EventLoop
{
    int                         exit;
    int                         exitcode;
    struct timeval              tick;
    struct timeval              nexttick;
    Registration                tickreg;
    d_Vector(Registration)      regs;
    d_Vector(Idle)              idle;
    MTI_EventQueue*             queue;
};

static MT_ThreadStorage sEventLoops;

MT_EventLoop* MT_Loop_New(void)
{ 
    MT_EventLoop* e = NEW(MT_EventLoop);
    e->tickreg.cb   = NULL;
    e->exit         = 0;

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

    free(e); 
}

MT_EventLoop* MT_Loop_Current(void)
{ return (MT_EventLoop*) MT_ThreadStorage_Get(&sEventLoops); }

void MT_Loop_SetTick(MT_EventLoop* e, MT_Time period, MT_Callback cb, void* user)
{
    if (period != MT_TIME_INVALID && period > 0) {
        e->tick.tv_sec = period / 1000000;
        e->tick.tv_usec = period % 1000000;
        e->tickreg.cb = cb;
        e->tickreg.user = user;
    } else {
        e->tickreg.cb = NULL;
    }
}

void MT_Loop_Register(MT_EventLoop* e, MT_Handle h, MT_Callback cb, void* user)
{
    Registration* r = dv_push(Registration, &e->regs, 1);
    r->fd = h;
    r->cb = cb;
    r->user = user;
}

void MT_Loop_Unregister(MT_EventLoop* e, MT_Handle h)
{
    size_t i;
    for (i = 0; i < dv_size(&e->regs); i++)
    {
        if (dv_a(&e->regs, i).fd == h) {
            dv_erase(Registration, &e->regs, i, 1);
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
    struct timeval time, timeout;
    fd_set fds;
    int maxfd = 0;
    size_t i;
    int ready = 0;

    if (e->exit)
        return e->exitcode;

    RunIdle(e);

    FD_ZERO(&fds);
    for (i = 0; i < dv_size(&e->regs); i++)
    {
        int fd = dv_a(&e->regs, i).fd;
        if (fd > maxfd)
            maxfd = fd;
        FD_SET(fd, &fds);
    }

    if (e->tickreg.cb) {
        gettimeofday(&time, NULL);
        /* Go on to emit tick straight away */
        if (timercmp(&time, &e->nexttick, <)) {
            timersub(&e->nexttick, &time, &timeout);
            ready = select(maxfd + 1, &fds, NULL, NULL, &timeout);
        }
    } else {
        ready = select(maxfd + 1, &fds, NULL, NULL, NULL);
    }


    if (ready == 0) {
        gettimeofday(&time, NULL);
        timeradd(&time, &e->tick, &e->nexttick);
        e->tickreg.cb(e->tickreg.user);

    } else if (ready > 0) {
        for (i = 0; i < dv_size(&e->regs); i++) {
            struct Registration* r = &dv_a(&e->regs, i);
            if (FD_ISSET(r->fd, &fds)) {
                r->cb(r->user);
            }
        }

    } else if (errno != EINTR) {
        return -1;

    }

    return e->exitcode;
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

