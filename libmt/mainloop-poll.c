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

#include "mainloop-poll.h"

#ifndef _WIN32

#include <errno.h>

static MT_ThreadStorage sEventLoops;

/* ------------------------------------------------------------------------- */

void MT_SetCurrent(MT_MainLoop* s)
{ MT_ThreadStorage_Set(&sEventLoops, s); }

/* ------------------------------------------------------------------------- */

MT_MainLoop* MT_Current(void)
{ return (MT_MainLoop*) MT_ThreadStorage_Get(&sEventLoops); }

/* ------------------------------------------------------------------------- */

MT_MainLoop* MT_Loop_New(void)
{ 
    MT_Handle queueHandle;

    MT_MainLoop* s = NEW(MT_MainLoop);

    MT_ThreadStorage_Ref(&sEventLoops);

    queueHandle = MTI_Queue_Init(&s->queue),
    MT_Loop_AddHandle(s, queueHandle, MTI_Queue_Dispatch, &s->queue);

    return s;
}

/* ------------------------------------------------------------------------- */

void MT_Loop_Free(MT_MainLoop* s)
{ 
    int i;

    MTI_Queue_Destroy(&s->queue);
    MT_ThreadStorage_Deref(&sEventLoops);

    for (i = 0; i < dv_size(&s->regs); i++) {
        free(dv_a(&s->regs, i));
    }

    for (i = 0; i < dv_size(&s->idle); i++) {
        free(dv_a(&s->idle, i));
    }

    dv_free(LoopRegistration, &s->regs);
    dv_free(LoopRegistration, &s->idle);
    dv_free(pollfd, &s->events);

    free(s); 
}
/* ------------------------------------------------------------------------- */

MT_LoopRegistration* MT_Loop_AddClientSocket(
        MT_MainLoop*    s,
        MT_Socket       sock,
        MT_Callback     read,
        MT_Callback     write,
        MT_Callback     close,
        void*           user)
{
    MT_LoopRegistration *r, **pr;
    struct pollfd* pfd;

    assert(read || write || close);

    r               = NEW(MT_LoopRegistration);
    r->fd           = sock;
    r->read         = read;
    r->write        = write;
    r->close        = close;
    r->user         = user;

    pr              = dv_push(LoopRegistration, &s->regs, 1);
    *pr             = r;

    pfd             = dv_push(pollfd, &s->events, 1);
    pfd->fd         = r->fd;
    pfd->events     = 0;
    pfd->revents    = 0;

    if (read) {
        pfd->events |= POLLIN;
    }

    if (write) {
        pfd->events |= POLLOUT;
    }

    if (close) {
        pfd->events |= POLLHUP;
    }

    return r;
}

/* ------------------------------------------------------------------------- */

MT_LoopRegistration* MT_Loop_AddServerSocket(
        MT_MainLoop*    s,
        MT_Handle       h,
        MT_Callback     accept,
        void*           user)
{
    return MT_Loop_AddClientSocket(s, h, accept, NULL, NULL, user);
}

MT_LoopRegistration* MT_Loop_AddHandle(
        MT_MainLoop*    s,
        MT_Handle       h,
        MT_Callback     cb,
        void*           user)
{
    return MT_Loop_AddClientSocket(s, h, cb, NULL, NULL, user);
}

/* ------------------------------------------------------------------------- */

MT_LoopRegistration* MT_Loop_AddIdle(
        MT_MainLoop*    s,
        MT_Callback     idle,
        void*           user)
{
    MT_LoopRegistration *r, **pr;

    assert(idle);

    r       = NEW(MT_LoopRegistration);
    r->idle = idle;
    r->user = user;

    pr  = dv_push(LoopRegistration, &s->idle, 1);
    *pr = r;

    return r;
}

/* ------------------------------------------------------------------------- */

void MT_Loop_Enable(
        MT_MainLoop*            s,
        MT_LoopRegistration*    r,
        int                     flags)
{
    int i;
    for (i = 0; i < dv_size(&s->regs); i++) {
        if (dv_a(&s->regs, i) == r) {
            struct pollfd* pfd = &dv_a(&s->events, i);

            if (flags & (MT_LOOP_READ | MT_LOOP_HANDLE | MT_LOOP_ACCEPT)) {
                assert(r->read);
                pfd->events |= POLLIN;
            }

            if (flags & MT_LOOP_WRITE) {
                assert(r->write);
                pfd->events |= POLLOUT;
            }

            if (flags & MT_LOOP_CLOSE) {
                assert(r->write);
                pfd->events |= POLLHUP;
            }

            break;
        }
    }
}

/* ------------------------------------------------------------------------- */

void MT_Loop_Disable(
        MT_MainLoop*            s,
        MT_LoopRegistration*    r,
        int                     flags)
{
    int i;
    for (i = 0; i < dv_size(&s->regs); i++) {
        if (dv_a(&s->regs, i) == r) {
            struct pollfd* pfd = &dv_a(&s->events, i);

            if (flags & (MT_LOOP_READ | MT_LOOP_HANDLE | MT_LOOP_ACCEPT)) {
                pfd->events &= ~POLLIN;
                pfd->revents &= ~POLLIN;
            }

            if (flags & MT_LOOP_WRITE) {
                pfd->events &= ~POLLOUT;
                pfd->revents &= ~POLLOUT;
            }

            if (flags & MT_LOOP_CLOSE) {
                pfd->events &= ~POLLHUP;
                pfd->revents &= ~POLLHUP;
            }

            break;
        }
    }
}

/* ------------------------------------------------------------------------- */

void MT_Loop_Remove(MT_MainLoop* s, MT_LoopRegistration* r)
{
    int i;
    if (r->idle) {
        for (i = 0; i < dv_size(&s->idle); i++) {
            if (dv_a(&s->idle, i) == r) {
                dv_erase(LoopRegistration, &s->idle, i, 1);

                /* We just removed an idle entry. Need to shift the index down to
                 * match where the current entry is now. Note if we just removed the
                 * current index, we want to redo the current index (which now has the
                 * next indexes data.
                 */
                if (s->currentIdle >= i) {
                    s->currentIdle--;
                }
                break;
            }
        }

    } else {
        for (i = 0; i < dv_size(&s->regs); i++) {
            if (dv_a(&s->regs, i) == r) {
                dv_erase(LoopRegistration, &s->regs, i, 1);
                dv_erase(pollfd, &s->events, i, 1);

                if (s->currentEvent >= i) {
                    s->currentEvent--;
                }
                break;
            }
        }
    }
}

/* ------------------------------------------------------------------------- */

static int ProcessEvent(MT_MainLoop* s)
{
    /* Process the next event from s->events */
    while (s->currentEvent < dv_size(&s->events)) {
        struct pollfd* pfd = &dv_a(&s->events, s->currentEvent);
        MT_LoopRegistration* r = dv_a(&s->regs, s->currentEvent);
        assert(s->currentEvent >= 0);

        /* Mask the events to only those we care about */
        pfd->revents &= POLLIN | POLLOUT | POLLHUP;

        if ((pfd->revents & POLLIN) && r->read) {
            pfd->revents &= ~POLLIN;
            r->read(r->user);
            return 0;
        }

        if ((pfd->revents & POLLHUP) && r->close) {
            pfd->revents &= ~POLLHUP;
            r->close(r->user);
            return 0;
        }

        if ((pfd->revents & POLLOUT) && r->write) {
            pfd->revents &= ~POLLOUT;
            r->write(r->user);
            return 0;
        }

        s->currentEvent++;
    }

    /* We couldn't find any events */
    return -1;
}

static void CallIdle(MT_MainLoop* s)
{
    while (s->currentIdle < dv_size(&s->idle)) {
        MT_LoopRegistration* r = dv_a(&s->idle, s->currentIdle);
        assert(s->currentIdle >= 0);
        r->idle(r->user);
        s->currentIdle++;
    }
}

int MT_Current_Step(void)
{
	MT_MainLoop* s = MT_Current();
    int ready;

    if (s->exit) {
        return s->exitcode;
    }

    /* Try and process an exisiting event */
    if (!ProcessEvent(s)) {
        return 0;
    }

    CallIdle(s);

    do {
        ready = poll(dv_data(&s->events), dv_size(&s->events), -1);
    } while (ready < 0 && errno == EINTR);

    if (ready < 0)
        return -1;

    s->currentIdle = 0;
    s->currentEvent = 0;

    return ProcessEvent(s);
}

/* ------------------------------------------------------------------------- */

int MT_Current_Run(void)
{
	MT_MainLoop* s = MT_Current();

    while (!s->exit) {
        int ready;

        s->currentIdle = 0;
        CallIdle(s);

        ready = poll(dv_data(&s->events), dv_size(&s->events), -1);
        s->currentEvent = 0;

        if (ready > 0) {
            while (s->currentEvent < dv_size(&s->events)) {
                struct pollfd* pfd = &dv_a(&s->events, s->currentEvent);
                MT_LoopRegistration* r = dv_a(&s->regs, s->currentEvent);

                /* Mask the events to only those we care about */
                pfd->revents &= POLLIN | POLLOUT | POLLHUP;

                if ((pfd->revents & POLLIN) && r->read) {
                    r->read(r->user);
                }

                if ((pfd->revents & POLLHUP) && r->close) {
                    r->close(r->user);
                    continue;
                }

                if ((pfd->revents & POLLOUT) && r->write) {
                    r->write(r->user);
                }

                s->currentEvent++;
            }

        } else if (ready < 0 && errno != EINTR) {
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




#endif


