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

#pragma once

#include "Common.h"
#include "Time.h"
#include "Lock.h"

struct MT_Message
{
    MT_AtomicInt            ref;
    MT_AtomicPtr            next;
    MT_Callback             call;
    MT_Callback             free;
    void*                   user;
};

MT_API void MT_Message_Ref(MT_Message* m);
MT_API void MT_Message_Deref(MT_Message* m);

MT_API MT_EventLoop* MT_Loop_New(void);
MT_API void MT_Loop_Free(MT_EventLoop* e);

MT_API void MT_Loop_SetTick(MT_EventLoop* e, MT_Time period, MT_Callback cb, void* user);
MT_API void MT_Loop_Register(MT_EventLoop* e, MT_Handle h, MT_Callback cb, void* user);
MT_API void MT_Loop_Unregister(MT_EventLoop* e, MT_Handle h);
MT_API void MT_Loop_AddIdle(MT_EventLoop* e, MT_Callback cb, void* user);
MT_API void MT_Loop_RemoveIdle(MT_EventLoop* e, MT_Callback cb, void* user);

MT_API int  MT_Loop_Step(MT_EventLoop* e);
MT_API int  MT_Loop_Run(MT_EventLoop* e);
MT_API void MT_Loop_Exit(MT_EventLoop* e, int code);

/* Thread safe as long as the loop is not freed */
MT_API void MT_Loop_Post(MT_EventLoop* q, MT_Message* m);
MT_API void MT_Loop_Broadcast(MT_Message* m); 

MT_API MT_EventLoop* MT_Loop_Current(void);


#ifdef __cplusplus
namespace MT
{
    class Message
    {
    public:
        virtual void Call() = 0;
        virtual ~Event() {}

        void Broadcast()                {Setup(); MT_BroadcastMessage(&m_Header);}
        void Post(MT_EventLoop* loop)   {Setup(); MT_PostMessage(loop, &m_Header);}

        void Ref()                      {MT_Message_Ref(&m_Header);}
        void Deref()                    {MT_Message_Deref(&m_Header);}

    private:
        static void Callback(void* u)   {((Message*) u)->Call();}
        static void Free(void* u)       {delete (Message*) u;}

        void Setup()
        {
            m_Header.callback   = &Callback;
            m_Header.free       = &Free;
            m_Header.user       = this;
        }

        MT_Message m_Header;
    };

    class EventLoop
    {
        MT_NOT_COPYABLE(EventLoop);
    public:
        EventLoop() : m(MT_Loop_New()) {}
        ~EventLoop() {MT_Loop_Free(m);}

        void SetTick(MT_Time period, MT_Callback cb, void* user)
        { MT_Loop_SetTick(m, period, cb, user); }

        void Register(MT_Handle h, MT_Callback cb, void* user)
        { MT_Loop_Register(m, h, cb, user); }

        void Unregister(MT_Handle h)
        { MT_Loop_Unregister(m, h); }

        int Run()
        { return MT_Loop_Run(m); }

        void Exit(int code)
        { MT_Loop_Exit(m, code); }

        operator MT_EventLoop*() {return m;}

    private:
        MT_EventLoop* m;
    };
}
#endif

