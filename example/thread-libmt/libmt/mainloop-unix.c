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

#ifndef _WIN32

#include <sys/time.h>
#include <errno.h>

/* ------------------------------------------------------------------------- */

void MT_Loop_Register(MT_MainLoop* s, MT_Handle h, MT_Callback cb, void* user)
{
    MTI_LoopHandle* ph;
    MTI_LoopRegistration* pr;

    ph          = dv_push(LoopHandle, &s->handles, 1);
    ph->fd      = h;
    ph->events  = POLLIN;

    pr          = dv_push(LoopRegistration, &s->regs, 1);
    pr->cb      = cb;
    pr->user    = user;
    pr->handle  = h;
}

/* ------------------------------------------------------------------------- */

void MT_Loop_SetTick(MT_MainLoop* e, MT_Time period, MT_Callback cb, void* user)
{
    if (period != MT_TIME_INVALID && period > 0) {
        e->tick = period;
        e->nexttick = MT_CurrentTime() + period;
        e->tickreg.cb = cb;
        e->tickreg.user = user;
    } else {
        e->tickreg.cb = NULL;
    }
}

/* ------------------------------------------------------------------------- */

int MT_Current_Step(void)
{
	MT_MainLoop* e = MT_Current();
    MT_Time time;
    int ready = 0;

    if (e->exit)
        return e->exitcode;

    MTI_CallIdle(e);

    if (e->tickreg.cb) {
        time = MT_CurrentTime();

        /* Go on to emit tick straight away */
        if (time > e->nexttick) {
            MT_Time timeout = e->nexttick - time;
            ready = poll(
                    dv_data(&e->handles),
                    dv_size(&e->handles),
                    timeout / 1000);
        }
    } else {
        ready = poll(
                dv_data(&e->handles),
                dv_size(&e->handles),
                -1);
    }


    if (ready == 0) {
        assert(e->tickreg.cb);
        e->nexttick = MT_CurrentTime() + e->tick;
        e->tickreg.cb(e->tickreg.user);

    } else if (ready > 0) {
        size_t i;
        for (i = 0; i < dv_size(&e->handles); i++) {
            struct pollfd* pfd = &dv_a(&e->handles, i);
            if (pfd->revents) {
                MTI_LoopRegistration* r = &dv_a(&e->regs, i);
                r->cb(r->user);
            }
        }

    } else if (errno != EINTR) {
        return -1;

    }

    return e->exitcode;
}

#endif

