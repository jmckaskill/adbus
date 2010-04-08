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

#define HW_LIBRARY
#include "EventLoop.h"
#include <vector>
#include <sys/time.h>

typedef struct Registration Registration;

struct Registration
{
	int          fd;
	HW_Callback  cb;
	void*        user;
};

DVECTOR_INIT(Registration, struct Registration)

struct HW_EventLoop
{
	int							exit;
	int							exitcode;
	struct timeval				tick;
	Registration				tickreg;
	d_Vector(Registration)		regs;
};

HW_EventLoop* HW_Loop_New()
{ 
	HW_EventLoop* e = new HW_EventLoop; 
	e->tickreg.cb = NULL;
	e->exit = 0;
	return e;
}

void HW_Loop_Free(HW_EventLoop* e)
{ delete e; }

void HW_Loop_SetTick(HW_EventLoop* e, HW_Time period, HW_Callback cb, void* user)
{
	if (period != HW_TIME_INVALID && period > 0)
	{
		e->tick.tv_sec = period / 1000000;
		e->tick.tv_usec = period % 1000000;
		e->tickreg.cb = cb;
		e->tickreg.user = user;
	}
	else
	{
		e->tickreg.cb = NULL;
	}
}

void HW_Loop_Register(HW_EventLoop* e, HW_Handle h, HW_Callback cb, void* user)
{
	Registration* r = dv_push(Registration, &e->regs, 1);
	r->fd = h;
	r->cb = cb;
	r->user = user;
}

void HW_Loop_Unregister(HW_EventLoop* e, HW_Handle h)
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

int HW_Loop_Step(HW_EventLoop* e)
{
	struct timeval nexttick, time, timeout;
	fd_set fds = {};
	int maxfd = 0;
	size_t i;
	int ready = 0;

	if (e->exit)
		return e->exitcode;

	for (i = 0; i < dv_size(&e->regs); i++)
	{
		int fd = e->regs[i].fd;
		if (fd > maxfd)
			maxfd = fd;
		FD_SET(fd, &fds);
	}

	if (e->tickreg.cb) {
		if (timercmp(&time, &nexttick, <)) {
			timersub(&nexttick, &time, &timeout);
			ready = select(maxfd + 1, &fds, NULL, NULL, &timeout);
		}
	} else {
		ready = select(maxfd + 1, &fds, NULL, NULL, NULL);
	}


	if (ready == 0) {
		gettimeofday(&time, NULL);
		timeradd(&time, &e->tick, &nexttick);
		e->tickreg.cb(e->tickreg.user);

	} else if (ready > 0) {
		for (size_t i = 0; i < e->regs.size(); i++) {
			if (FD_ISSET(e->regs[i].fd, &fds)) {
				e->regs[i].cb(e->regs[i].user);
			}
		}

	} else if (errno != EINTR) {
		return -1;

	}

	return e->exitcode;
}

int HW_Loop_Run(HW_EventLoop* e)
{
	int ret = 0;
	while (!ret && !e->exit) {
		ret = HW_Loop_Step(e);
	}
	return ret;
}


void HW_Loop_Exit(HW_EventLoop* e, int code)
{
	e->exit = 1;
	e->exitcode = code;
}

#endif
