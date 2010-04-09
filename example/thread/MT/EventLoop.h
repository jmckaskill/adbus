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
	MT_Target*				target;
    MT_AtomicPtr            targetNext;
    MT_AtomicPtr            queueNext;
    MT_Callback             call;
    MT_Callback             free;
    void*                   user;
};

MT_API void MT_Message_Post(MT_Message* m, MT_Target* t);

MT_API MT_EventLoop* MT_Loop_New(void);
MT_API void MT_Loop_Free(MT_EventLoop* e);

MT_API void MT_SetCurrent(MT_EventLoop* e);
MT_API MT_EventLoop* MT_Current(void);

MT_API void MT_Loop_SetTick(MT_EventLoop* e, MT_Time period, MT_Callback cb, void* user);
MT_API void MT_Loop_Register(MT_EventLoop* e, MT_Handle h, MT_Callback cb, void* user);
MT_API void MT_Loop_Unregister(MT_EventLoop* e, MT_Handle h);
MT_API void MT_Loop_AddIdle(MT_EventLoop* e, MT_Callback cb, void* user);
MT_API void MT_Loop_RemoveIdle(MT_EventLoop* e, MT_Callback cb, void* user);
MT_API void MT_Loop_Post(MT_EventLoop* e, MT_Message* m);

MT_INLINE void MT_Current_SetTick(MT_Time period, MT_Callback cb, void* user)
{ MT_Loop_SetTick(MT_Current(), period, cb, user); }

MT_INLINE void MT_Current_Register(MT_Handle h, MT_Callback cb, void* user)
{ MT_Loop_Register(MT_Current(), h, cb, user); }

MT_INLINE void MT_Current_Unregister(MT_Handle h)
{ MT_Loop_Unregister(MT_Current(), h); }

MT_INLINE void MT_Current_AddIdle(MT_Callback cb, void* user)
{ MT_Loop_AddIdle(MT_Current(), cb, user); }

MT_INLINE void MT_Current_RemoveIdle(MT_Callback cb, void* user)
{ MT_Loop_RemoveIdle(MT_Current(), cb, user); }

MT_API int  MT_Current_Step(void);
MT_API int  MT_Current_Run(void);
MT_API void MT_Current_Exit(int code);

#if defined _WIN32
#	define MT_PRI_LOOP "%u"
MT_API unsigned int MT_Loop_Printable(MT_EventLoop* e);

#elif defined __linux__
#	define MT_PRI_LOOP "%p"
MT_API void* MT_Loop_Printable(MT_EventLoop* e);

#else
#	define MT_PRI_LOOP "%p"
MT_API void* MT_Loop_Printable(MT_EventLoop* e);

#endif


#ifdef __cplusplus
namespace MT
{
    class Message
    {
    public:
        virtual void Call() = 0;
        virtual ~Event() {}

        void Post(MT_EventLoop* loop)   {Setup(); MT_Message_Post(&m_Header, loop);}

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

