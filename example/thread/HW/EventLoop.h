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

struct HW_Message
{
	HW_AtomicInt			ref;
	HW_AtomicPtr			next;
	HW_Callback				call;
	HW_Callback				free;
	void*					user;
};

HW_API void HW_Message_Ref(HW_Message* m);
HW_API void HW_Message_Deref(HW_Message* m);

HW_API HW_EventLoop* HW_Loop_New();
HW_API void HW_Loop_Free(HW_EventLoop* e);

HW_API void HW_Loop_SetTick(HW_EventLoop* e, HW_Time period, HW_Callback cb, void* user);
HW_API void HW_Loop_Register(HW_EventLoop* e, HW_Handle h, HW_Callback cb, void* user);
HW_API void HW_Loop_Unregister(HW_EventLoop* e, HW_Handle h);

HW_API int  HW_Loop_Step(HW_EventLoop* e);
HW_API int  HW_Loop_Run(HW_EventLoop* e);
HW_API void HW_Loop_Exit(HW_EventLoop* e, int code);

/* Thread safe as long as the loop is not freed */
HW_API void HW_Loop_Post(HW_EventLoop* q, HW_Message* m);
HW_API void HW_Loop_Broadcast(HW_Message* m); 

HW_API HW_EventLoop* HW_Loop_Current();


#ifdef __cplusplus
namespace HW
{
	class Message
	{
	public:
		virtual void Call() = 0;
		virtual ~Event() {}

		void Broadcast()				{Setup(); HW_BroadcastMessage(&m_Header);}
		void Post(HW_EventLoop* loop)	{Setup(); HW_PostMessage(loop, &m_Header);}

		void Ref()						{HW_Message_Ref(&m_Header);}
		void Deref()					{HW_Message_Deref(&m_Header);}

	private:
		static void Callback(void* u)	{((Message*) u)->Call();}
		static void Free(void* u)		{delete (Message*) u;}

		void Setup()
		{
			m_Header.callback	= &Callback;
			m_Header.free		= &Free;
			m_Header.user		= this;
		}

		HW_Message m_Header;
	};

	class EventLoop
	{
		HW_NOT_COPYABLE(EventLoop);
	public:
		EventLoop() : m(HW_Loop_New()) {}
		~EventLoop() {HW_Loop_Free(m);}

		void SetTick(HW_Time period, HW_Callback cb, void* user)
		{ HW_Loop_SetTick(m, period, cb, user); }

		void Register(HW_Handle h, HW_Callback cb, void* user)
		{ HW_Loop_Register(m, h, cb, user); }

		void Unregister(HW_Handle h)
		{ HW_Loop_Unregister(m, h); }

		int Run()
		{ return HW_Loop_Run(m); }

		void Exit(int code)
		{ HW_Loop_Exit(m, code); }

		operator HW_EventLoop*() {return m;}

	private:
		HW_EventLoop* m;
	};
}
#endif

