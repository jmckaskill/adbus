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
#include "Thread.h"
#include <process.h>
#include <windows.h>

struct ThreadData
{
    MT_ThreadFunction   func;
    void*               arg;
};

DWORD __stdcall start_thread(void* arg)
{
    struct ThreadData* d = (struct ThreadData*) arg;
    MT_ThreadFunction func = d->func;
    void* funcarg = d->arg;
    free(d);

    func(funcarg);
    return 0;
}

MT_Thread MT_Thread_Start(MT_ThreadFunction func, void* arg)
{ 
    MT_Thread handle = HW_HANDLE_INVALID;

    struct ThreadData* d = NEW(struct ThreadData);
    d->func = func;
    d->arg = arg;

    _beginthreadex(NULL, 0, &start_thread, d, 0, &handle);

    return handle;
}


void MT_Thread_Join(MT_Thread thread)
{ WaitForSingleObject(thread, INFINITE); }

void MT_ThreadStorage_Ref(MT_ThreadStorage* s)
{
    MT_Spinlock_Enter(&s->lock);
    if (s->ref++ == 0) {
        s->tls = TlsAlloc();
    }
    MT_Spinlock_Exit(&s->lock);
}

void MT_ThreadStorage_Deref(MT_ThreadStorage* s)
{
    MT_Spinlock_Enter(&s->lock);
    if (--s->ref == 0) {
        TlsFree(s->tls);
    }
    MT_Spinlock_Exit(&s->lock);
}

#endif
