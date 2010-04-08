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

#define HW_LIBRARY
#include "EventQueue_p.h"
#include "Lock.h"
#include "EventLoop.h"
#include <assert.h>

typedef struct HWI_Event HWI_Event;

struct HWI_EventQueue
{
	HW_AtomicPtr			prev;
	HW_AtomicPtr			next;

	HW_EventLoop*           loop;

	HW_Spinlock             produceLock;
	HW_Message*             last;

	char                    pad[16 - sizeof(HW_Spinlock) - sizeof(HW_Message*)];
	HW_Message*             first;
	HW_Message              dummy;

#ifdef _WIN32
	HW_Handle               handle;
#else
	int                     pipe[2];
#endif
};

/* Platform specific stuff */
#ifdef _WIN32
static void CreateHandle(HWI_EventQueue* q)
{ q->handle = CreateEvent(NULL, FALSE, FALSE, NULL); }

static void FreeHandle(HWI_EventQueue* q)
{ CloseHandle(q->handle); }

HW_Handle GetHandle(HWI_EventQueue* q)
{ return q->handle; }

static void ResetHandle(HWI_EventQueue* q)
{ (void) q; }

static void WakeUp(HWI_EventQueue* q)
{ SetEvent(q->handle); }



#else
static void CreateHandle(HWI_EventQueue* q)
{ pipe2(q->pipe); }

static void FreeHandle(HWI_EventQueue* q)
{
	close(q->pipe[0]);
	close(q->pipe[1]);
}

HW_Handle GetHandle(HWI_EventQueue* q)
{ return q->pipe[0]; }

static void ResetHandle(HWI_EventQueue* q)
{
	char buf[256];
	read(q->pipe[0], buf, 256);
}

static void WakeUp(HWI_EventQueue* q)
{
	char ch = 0;
	write(q->pipe[1], &ch, 1);
}

#endif



/* Non platform specific stuff */

void HW_Message_Ref(HW_Message* m)
{ HW_AtomicInt_Increment(&m->ref); }

void HW_Message_Deref(HW_Message* m)
{
	if (HW_AtomicInt_Decrement(&m->ref) == 0 && m->free) {
		m->free(m->user);
	}
}

static HW_Spinlock  sEventQueueLock	= HW_SPINLOCK_STATIC_INIT;
static HW_AtomicPtr sEventQueueList	= NULL;


HWI_EventQueue* HWI_Queue_New(HW_EventLoop* loop)
{
	HWI_EventQueue* q  = NEW(HWI_EventQueue);
	CreateHandle(q);
	HW_Spinlock_Init(&q->produceLock);

	q->loop       = loop;

	q->last       = &q->dummy;
	q->first      = &q->dummy;

	q->dummy.next = NULL;
	q->dummy.call = NULL;
	q->dummy.free = NULL;
	q->dummy.user = NULL;

	HW_Message_Ref(&q->dummy);

	HW_Spinlock_Enter(&sEventQueueLock);
	{
		HWI_EventQueue* head = (HWI_EventQueue*) sEventQueueList;
		HW_AtomicPtr_Set(&q->next, head);
		if (head) {
			HW_AtomicPtr_Set(&head->prev, q);
		}
		HW_AtomicPtr_Set(&sEventQueueList, q);
	}
	HW_Spinlock_Exit(&sEventQueueLock);

	HW_Loop_Register(q->loop, GetHandle(q), &HWI_Queue_Dispatch, q);

	return q;
}

void HWI_Queue_Free(HWI_EventQueue* q)
{
	HW_Message *m, *next;

	HW_Spinlock_Enter(&q->produceLock);
	HW_Spinlock_Enter(&sEventQueueLock);

	{
		HWI_EventQueue *next = (HWI_EventQueue*) q->next;
		HWI_EventQueue *prev = (HWI_EventQueue*) q->prev;

		/* Remove this queue from the list - we still cant actually free this queue
		 * as someone may be currently iterating over it.
		 */
		if (next) {
			HW_AtomicPtr_Set(&next->prev, prev);
		}
		if (prev) {
			HW_AtomicPtr_Set(&prev->next, next);
		}

		/* This stops any more messages from being produced */
		q->last = NULL;
	}

	HW_Spinlock_Exit(&sEventQueueLock);
	HW_Spinlock_Exit(&q->produceLock);

	for (m = q->first; m != NULL;)
	{
		next = (HW_Message*) m->next;
		HW_Message_Deref(m);
		m = next;
	}

	if (q->loop)
		HW_Loop_Unregister(q->loop, GetHandle(q));

	FreeHandle(q);
}

void HWI_Queue_Dispatch(void* u)
{
	HWI_EventQueue* q = (HWI_EventQueue*) u;
	ResetHandle(q);

	while (1)
	{
		HW_Message* first = q->first;
		HW_Message* next  = first->next;
		if (next == NULL)
			break;

		q->first = next;
		HW_Message_Deref(first);

		if (next->call)
			next->call(next->user);
	}
}

void HWI_Queue_Post(HWI_EventQueue* q, HW_Message* e)
{
	HW_Spinlock_Enter(&q->produceLock);

	if (q->last) {
		q->last->next = e;
		q->last = e;
	}

	HW_Spinlock_Exit(&q->produceLock);

	WakeUp(q);
}

void HW_Loop_Broadcast(HW_Message* m)
{
	HWI_EventQueue* q;
	for (q = (HWI_EventQueue*) sEventQueueList; q != NULL; q = (HWI_EventQueue*) q->next) {
		HWI_Queue_Post(q, m);
	}
}
