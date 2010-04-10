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

#ifdef _WIN32

#ifdef _MSC_VER
#	pragma warning(disable:4055) /* type cast HANDLE to LPTIMECALLBACK */
#endif

/* ------------------------------------------------------------------------- */

void MT_Loop_SetTick(MT_MainLoop* e, MT_Time period, MT_Callback cb, void* user)
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

/* ------------------------------------------------------------------------- */

int MT_Current_Step(void)
{
	MT_MainLoop* e = MT_Current();
    struct Registration* r;
    DWORD ret;

	LOG("Step");
    if (e->exit)
        return e->exitcode;

    MTI_CallIdle(e);
    ret = WaitForMultipleObjects((DWORD) dv_size(&e->handles), &dv_a(&e->handles, 0), FALSE, INFINITE);
    if (ret >= WAIT_OBJECT_0 + dv_size(&e->handles))
        return -1;

    r = &dv_a(&e->regs, ret);
    r->cb(r->user);

    return 0;
}

#endif

