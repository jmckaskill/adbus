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

#define HW_LIBRARY
#include "EventLoop.h"
#include "Lock.h"
#include "Thread.h"
#include "EventQueue_p.h"
#include "dmem/vector.h"
#include <windows.h>

struct Registration
{
	HW_Callback  cb;
	void*        user;
};

DVECTOR_INIT(HANDLE, HANDLE)
DVECTOR_INIT(Registration, struct Registration)

struct HW_EventLoop
{
	int							exit;
	int							exitcode;
	HANDLE						timer;
	d_Vector(HANDLE)			handles;
	d_Vector(Registration)		regs;
	HWI_EventQueue*				queue;
};

static HW_ThreadStorage sEventLoops;

HW_EventLoop* HW_Loop_New()
{ 
	HW_EventLoop* e = NEW(HW_EventLoop);
	e->timer = INVALID_HANDLE_VALUE;

	HW_ThreadStorage_Ref(&sEventLoops);
	HW_ThreadStorage_Set(&sEventLoops, e);

	e->queue = HWI_Queue_New(e);

	return e;
}

void HW_Loop_Free(HW_EventLoop* e)
{ 
	HWI_Queue_Free(e->queue);

	HW_ThreadStorage_Set(&sEventLoops, NULL);
	HW_ThreadStorage_Deref(&sEventLoops);

	if (e->timer != INVALID_HANDLE_VALUE) {
		CloseHandle(e->timer);
	}
	free(e); 
}

HW_EventLoop* HW_Loop_Current()
{ return (HW_EventLoop*) HW_ThreadStorage_Get(&sEventLoops); }

void HW_Loop_SetTick(HW_EventLoop* e, HW_Time period, HW_Callback cb, void* user)
{
	if (period != HW_TIME_INVALID && period > 0) {

		if (e->timer == INVALID_HANDLE_VALUE) {
			e->timer = CreateEvent(NULL, FALSE, FALSE, NULL);
			timeSetEvent((UINT) (period / 1000), 0, (LPTIMECALLBACK) e->timer, (DWORD_PTR) NULL, TIME_PERIODIC | TIME_CALLBACK_EVENT_SET);
		} else {
			HW_Loop_Unregister(e, e->timer);
		}

		HW_Loop_Register(e, e->timer, cb, user);

	} else {

		if (e->timer != INVALID_HANDLE_VALUE) {
			HW_Loop_Unregister(e, e->timer);
			CloseHandle(e->timer);
			e->timer = INVALID_HANDLE_VALUE;
		}

	}
}

void HW_Loop_Register(HW_EventLoop* e, HW_Handle h, HW_Callback cb, void* user)
{
	HANDLE* ph = dv_push(HANDLE, &e->handles, 1);
	struct Registration* r = dv_push(Registration, &e->regs, 1);

	*ph = h;
	r->cb = cb;
	r->user = user;
}

void HW_Loop_Unregister(HW_EventLoop* e, HW_Handle h)
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

int HW_Loop_Step(HW_EventLoop* e)
{
	struct Registration* r;
	DWORD ret;

	if (e->exit)
		return e->exitcode;

	ret = WaitForMultipleObjects((DWORD) dv_size(&e->handles), &dv_a(&e->handles, 0), FALSE, INFINITE);
	if (ret >= WAIT_OBJECT_0 + dv_size(&e->handles))
		return -1;

	r = &dv_a(&e->regs, ret);
	r->cb(r->user);

	return 0;
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

void HW_Loop_Post(HW_EventLoop* e, HW_Message* m)
{ HWI_Queue_Post(e->queue, m); }

#endif
