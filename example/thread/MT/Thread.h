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

#include "Common.h"
#include "Lock.h"
#include <stdint.h>

#ifdef _WIN32
#   include <windows.h>
    typedef MT_Handle MT_Thread;
#else
#   include <pthread.h>
    typedef pthread_t MT_Thread;
#endif

MT_API void         MT_Thread_Start(MT_Callback func, void* arg);
MT_API MT_Thread    MT_Thread_StartJoinable(MT_Callback func, void* arg);
MT_API void         MT_Thread_Join(MT_Thread thread);

/* Needs to be memset to 0 if not a global */
struct MT_ThreadStorage
{
    MT_Spinlock     lock;
    int             ref;
#ifdef _WIN32
    uint32_t        tls;
#else
    pthread_key_t   tls;
#endif
};

MT_API void MT_ThreadStorage_Ref(MT_ThreadStorage* s);
MT_API void MT_ThreadStorage_Deref(MT_ThreadStorage* s);

#ifdef _WIN32
#   define MT_ThreadStorage_Get(pstorage)       TlsGetValue((pstorage)->tls)
#   define MT_ThreadStorage_Set(pstorage, val)  TlsSetValue((pstorage)->tls, val)
#else
#   define MT_ThreadStorage_Get(pstorage)       pthread_getspecific((pstorage)->tls)
#   define MT_ThreadStorage_Set(pstorage, val)  pthread_setspecific((pstorage)->tls, val)
#endif

