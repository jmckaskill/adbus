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
#   include <winsock2.h>
#endif

#include "mainloop-win.h"

#ifdef _WIN32

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
    size_t i;

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
    dv_free(HANDLE, &s->handles);

    free(s); 
}
/* ------------------------------------------------------------------------- */

static MT_LoopRegistration* AddSocket(
        MT_MainLoop*    s,
        MT_Socket       sock,
        MT_Callback     read,
        MT_Callback     write,
        MT_Callback     close,
        MT_Callback     accept,
        void*           user)
{
    MT_LoopRegistration *r, **pr;
    HANDLE *ph;

    assert(read || write || close || accept);

    r           = NEW(MT_LoopRegistration);
    r->socket   = sock;
    r->read     = read;
    r->write    = write;
    r->close    = close;
    r->accept   = accept;
    r->user     = user;
    r->handle   = WSACreateEvent();
    r->isSocket = 1;

    if (read) {
        r->mask |= FD_READ;
    }

    if (write) {
        r->mask |= FD_WRITE;
    }

    if (close) {
        r->mask |= FD_CLOSE;
    }

    if (accept) {
        r->mask |= FD_ACCEPT;
    }

    WSAEventSelect(r->socket, r->handle, r->mask);

    pr      = dv_push(LoopRegistration, &s->regs, 1);
    *pr     = r;

    ph      = dv_push(HANDLE, &s->handles, 1);
    *ph     = r->handle;

    return r;
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
    return AddSocket(s, sock, read, write, close, NULL, user);
}

/* ------------------------------------------------------------------------- */

MT_LoopRegistration* MT_Loop_AddServerSocket(
        MT_MainLoop*    s,
        MT_Socket       fd,
        MT_Callback     accept,
        void*           user)
{
    return AddSocket(s, fd, NULL, NULL, NULL, accept, user);
}


/* ------------------------------------------------------------------------- */

MT_LoopRegistration* MT_Loop_AddHandle(
        MT_MainLoop*    s,
        MT_Handle       h,
        MT_Callback     cb,
        void*           user)
{
    MT_LoopRegistration *r, **pr;
    HANDLE *ph;

    assert(cb);

    r           = NEW(MT_LoopRegistration);
    r->cb       = cb;
    r->user     = user;
    r->handle   = h;

    pr      = dv_push(LoopRegistration, &s->regs, 1);
    *pr     = r;

    ph      = dv_push(HANDLE, &s->handles, 1);
    *ph     = r->handle;

    return r;
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
    (void) s;
    if (r->isSocket) {
        if (flags & MT_LOOP_READ) {
            assert(r->read);
            r->mask |= FD_READ;
        }

        if (flags & MT_LOOP_WRITE) {
            assert(r->write);
            r->mask |= FD_WRITE;
        }

        if (flags & MT_LOOP_CLOSE) {
            assert(r->close);
            r->mask |= FD_CLOSE;
        }

        if (flags & MT_LOOP_ACCEPT) {
            assert(r->accept);
            r->mask |= FD_ACCEPT;
        }
    }
}

/* ------------------------------------------------------------------------- */

void MT_Loop_Disable(
        MT_MainLoop*            s,
        MT_LoopRegistration*    r,
        int                     flags)
{
    (void) s;
    if (r->isSocket) {
        if (flags & MT_LOOP_READ) {
            r->mask &= ~FD_READ;
        }

        if (flags & MT_LOOP_WRITE) {
            r->mask &= ~FD_WRITE;
        }

        if (flags & MT_LOOP_CLOSE) {
            r->mask &= ~FD_CLOSE;
        }

        if (flags & MT_LOOP_ACCEPT) {
            r->mask &= ~FD_ACCEPT;
        }
    }
}

/* ------------------------------------------------------------------------- */

void MT_Loop_Remove(MT_MainLoop* s, MT_LoopRegistration* r)
{
    int i;
    if (r->idle) {
        for (i = 0; i < (int) dv_size(&s->idle); i++) {
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
        for (i = 0; i < (int) dv_size(&s->regs); i++) {
            if (dv_a(&s->regs, i) == r) {
                dv_erase(LoopRegistration, &s->regs, i, 1);
                dv_erase(HANDLE, &s->handles, i, 1);
                break;
            }
        }
    }
}

/* ------------------------------------------------------------------------- */

static void CallIdle(MT_MainLoop* s)
{
    while (s->currentIdle < (int) dv_size(&s->idle)) {
        MT_LoopRegistration* r = dv_a(&s->idle, s->currentIdle);
        assert(s->currentIdle >= 0);
        r->idle(r->user);
        s->currentIdle++;
    }
}

/* ------------------------------------------------------------------------- */

static void Process(MT_MainLoop* s, DWORD index)
{
    MT_LoopRegistration* r = dv_a(&s->regs, index);
    if (r->isSocket) {

        long flags;
        WSANETWORKEVENTS events;
        if (WSAEnumNetworkEvents(r->socket, r->handle, &events))
            return;

        flags = events.lNetworkEvents & r->mask;

        if (flags & FD_READ) {
            r->read(r->user);
        }

        if (flags & FD_CLOSE) {
            r->close(r->user);
            return;
        }

        if (flags & FD_ACCEPT) {
            r->accept(r->user);
        }
        
        if (flags & FD_WRITE) {
            r->write(r->user);
        }

    } else {
        r->cb(r->user);
    }
}

int MT_Current_Run(void)
{
	MT_MainLoop* s = MT_Current();
    while (!s->exit) {

        DWORD ret = WaitForMultipleObjects(
                dv_size(&s->handles),
                dv_data(&s->handles),
                FALSE,
                0);

        if (ret < WAIT_OBJECT_0 + dv_size(&s->handles)) {
            Process(s, ret);

        } else if (ret == WAIT_TIMEOUT) {

            s->currentIdle = 0;
            CallIdle(s);

            ret = WaitForMultipleObjects(
                    dv_size(&s->handles),
                    dv_data(&s->handles),
                    FALSE,
                    INFINITE);

            if (ret < WAIT_OBJECT_0 + dv_size(&s->handles)) {
                Process(s, ret);
            } else {
                return -1;
            }

        } else {
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
