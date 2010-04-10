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

/* To get timeradd, timersub, and timercmp */
#define _BSD_SOURCE

#include "mainloop.h"

#ifndef _WIN32

#include <sys/time.h>
#include <errno.h>

/* ------------------------------------------------------------------------- */

void MT_Loop_SetTick(MT_MainLoop* e, MT_Time period, MT_Callback cb, void* user)
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

/* ------------------------------------------------------------------------- */

int MT_Current_Step(void)
{
	MT_MainLoop* e = MT_Current();
    struct timeval time, timeout;
    fd_set fds;
    int maxfd = 0;
    size_t i;
    int ready = 0;

    if (e->exit)
        return e->exitcode;

    MTI_CallIdle(e);

    FD_ZERO(&fds);
    for (i = 0; i < dv_size(&e->handles); i++)
    {
        int fd = dv_a(&e->handles, i);
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
            MTI_LoopRegistration* r = &dv_a(&e->regs, i);
            int fd = dv_a(&e->handles, i);
            if (FD_ISSET(fd, &fds)) {
                r->cb(r->user);
            }
        }

    } else if (errno != EINTR) {
        return -1;

    }

    return e->exitcode;
}

#endif

