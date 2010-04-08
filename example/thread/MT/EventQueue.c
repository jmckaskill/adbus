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
#include <unistd.h>
#include <fcntl.h>

typedef struct MTI_Event MTI_Event;

struct MTI_EventQueue
{
    MT_AtomicPtr            prev;
    MT_AtomicPtr            next;

    MT_EventLoop*           loop;

    MT_Spinlock             produceLock;
    MT_Message*             last;

    char                    pad[16 - sizeof(MT_Spinlock) - sizeof(MT_Message*)];
    MT_Message*             first;
    MT_Message              dummy;

#ifdef _WIN32
    MT_Handle               handle;
#else
    int                     pipe[2];
#endif
};

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

void MT_Message_Ref(MT_Message* m)
{ MT_AtomicInt_Increment(&m->ref); }

void MT_Message_Deref(MT_Message* m)
{
    if (MT_AtomicInt_Decrement(&m->ref) == 0 && m->free) {
        m->free(m->user);
    }
}

static MT_Spinlock  sEventQueueLock = MT_SPINLOCK_STATIC_INIT;
static MT_AtomicPtr sEventQueueList = NULL;


MTI_EventQueue* MTI_Queue_New(MT_EventLoop* loop)
{
    MTI_EventQueue* q  = NEW(MTI_EventQueue);
    CreateHandle(q);
    MT_Spinlock_Init(&q->produceLock);

    q->loop       = loop;

    q->last       = &q->dummy;
    q->first      = &q->dummy;

    q->dummy.next = NULL;
    q->dummy.call = NULL;
    q->dummy.free = NULL;
    q->dummy.user = NULL;

    MT_Message_Ref(&q->dummy);

    MT_Spinlock_Enter(&sEventQueueLock);
    {
        MTI_EventQueue* head = (MTI_EventQueue*) sEventQueueList;
        MT_AtomicPtr_Set(&q->next, head);
        if (head) {
            MT_AtomicPtr_Set(&head->prev, q);
        }
        MT_AtomicPtr_Set(&sEventQueueList, q);
    }
    MT_Spinlock_Exit(&sEventQueueLock);

    MT_Loop_Register(q->loop, GetHandle(q), &MTI_Queue_Dispatch, q);

    return q;
}

void MTI_Queue_Free(MTI_EventQueue* q)
{
    MT_Message *m, *next;

    MT_Spinlock_Enter(&q->produceLock);
    MT_Spinlock_Enter(&sEventQueueLock);

    {
        MTI_EventQueue *next = (MTI_EventQueue*) q->next;
        MTI_EventQueue *prev = (MTI_EventQueue*) q->prev;

        /* Remove this queue from the list - we still cant actually free this queue
         * as someone may be currently iterating over it.
         */
        if (next) {
            MT_AtomicPtr_Set(&next->prev, prev);
        }
        if (prev) {
            MT_AtomicPtr_Set(&prev->next, next);
        }

        /* This stops any more messages from being produced */
        q->last = NULL;
    }

    MT_Spinlock_Exit(&sEventQueueLock);
    MT_Spinlock_Exit(&q->produceLock);

    for (m = q->first; m != NULL;)
    {
        next = (MT_Message*) m->next;
        MT_Message_Deref(m);
        m = next;
    }

    if (q->loop)
        MT_Loop_Unregister(q->loop, GetHandle(q));

    FreeHandle(q);
}

void MTI_Queue_Dispatch(void* u)
{
    MTI_EventQueue* q = (MTI_EventQueue*) u;
    ResetHandle(q);

    while (1)
    {
        MT_Message* first = q->first;
        MT_Message* next  = first->next;
        if (next == NULL)
            break;

        q->first = next;
        MT_Message_Deref(first);

        if (next->call)
            next->call(next->user);
    }
}

void MTI_Queue_Post(MTI_EventQueue* q, MT_Message* m)
{
    MT_Spinlock_Enter(&q->produceLock);

    if (q->last) {
        MT_Message_Ref(m);
        q->last->next = m;
        q->last = m;
    }

    MT_Spinlock_Exit(&q->produceLock);

    WakeUp(q);
}

void MT_Loop_Broadcast(MT_Message* m)
{
    MTI_EventQueue* q;
    for (q = (MTI_EventQueue*) sEventQueueList; q != NULL; q = (MTI_EventQueue*) q->next) {
        MTI_Queue_Post(q, m);
    }
}

