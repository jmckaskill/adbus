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

#ifdef __linux__
#   define _GNU_SOURCE
#endif

#define MT_LIBRARY
#include "EventQueue_p.h"
#include "Lock.h"
#include "EventLoop.h"
#include <assert.h>

#ifndef _WIN32
#	include <unistd.h>
#	include <fcntl.h>
#endif

#ifdef _MSC_VER
#	pragma warning(disable:4127) /* conditional expression is constant */
#endif

static void MTI_Message_Free(MT_Message* m);

/* Platform specific stuff */
#ifdef _WIN32
static void CreateHandle(MTI_EventQueue* q)
{ q->handle = CreateEvent(NULL, FALSE, FALSE, NULL); }

static void FreeHandle(MTI_EventQueue* q)
{ CloseHandle(q->handle); }

MT_Handle GetHandle(MTI_EventQueue* q)
{ return q->handle; }

static void ResetHandle(MTI_EventQueue* q)
{ (void) q; }

static void WakeUp(MTI_EventQueue* q)
{ SetEvent(q->handle); }



#else
static void CreateHandle(MTI_EventQueue* q)
{ 
#ifdef __linux__
    pipe2(q->pipe, O_CLOEXEC); 
#else
    pipe(q->pipe);
    fcntl(q->pipe[0], F_SETFD, FD_CLOEXEC);
    fcntl(q->pipe[1], F_SETFD, FD_CLOEXEC);
#endif
}

static void FreeHandle(MTI_EventQueue* q)
{
    close(q->pipe[0]);
    close(q->pipe[1]);
}

MT_Handle GetHandle(MTI_EventQueue* q)
{ return q->pipe[0]; }

static void ResetHandle(MTI_EventQueue* q)
{
    char buf[256];
    read(q->pipe[0], buf, 256);
}

static void WakeUp(MTI_EventQueue* q)
{
    char ch = 0;
    write(q->pipe[1], &ch, 1);
}

#endif



/* Non platform specific stuff */

MTI_EventQueue* MTI_Queue_New(MT_EventLoop* loop)
{
    MTI_EventQueue* q  = NEW(MTI_EventQueue);
    CreateHandle(q);
    MT_Spinlock_Init(&q->produceLock);

	q->loop			= loop;

    q->last			= &q->dummy;
    q->first		= &q->dummy;

    memset(&q->dummy, 0, sizeof(q->dummy));

    MT_Loop_Register(q->loop, GetHandle(q), &MTI_Queue_Dispatch, q);

    return q;
}

void MTI_Queue_Free(MTI_EventQueue* q)
{
    MT_Message *m, *next;

    for (m = q->first; m != NULL;) {
        next = (MT_Message*) m->next;
        MTI_Message_Free(m);
        m = next;
    }

    MT_Loop_Unregister(q->loop, GetHandle(q));
    FreeHandle(q);
	free(q);
}

void MTI_Queue_Dispatch(void* u)
{
    MTI_EventQueue* q = (MTI_EventQueue*) u;
    ResetHandle(q);

    while (1) {
        MT_Message* first = q->first;
        MT_Message* next  = first->next;
        if (next == NULL)
            break;

        q->first = next;
        MTI_Message_Free(first);

        if (next->target) {
            next->target->first = (MT_Message*) next->targetNext;
        }

		LOG("Call message %p", next);
        if (next->call)
            next->call(next->user);
    }
}

void MTI_Queue_Post(MTI_EventQueue* q, MT_Message* m)
{
    /* Append to list */
    MT_Message* prevlast = MT_AtomicPtr_Set(&q->last, t);

    /* Release to target thread */
    MT_AtomicPtr_Set(&prevlast->queueNext, m);

    WakeUp(q);
}

void MTI_Queue_RemoveTarget(MTI_EventQueue* q, MT_Target* t)
{
	MT_Message *tm, *qm, *qnext;

	tm = t->first;

	for (qm = q->first; tm != NULL && qm != NULL; qm = qnext) {
        qnext = (MT_Message*) qm->queueNext;
        while (qnext != NULL && tm != NULL && tm == qnext) {
            qnext = (MT_Message*) qnext->queueNext;
            tm = (MT_Message*) tm->targetNext;
            qm->queueNext = qnext;
            MTI_Message_Free(tm);
        }
	}
}

static void MTI_Message_Free(MT_Message* m)
{
    if (next->target) {
        assert(next->target->loop == MT_Current());
        assert(next->target->first == next);
        next->target->first = next->targetNext;
    }
	if (m->free) {
        m->free(m->user);
    }
}

