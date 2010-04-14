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

/* Needed for pipe2 */
#if defined __linux__ && !defined _GNU_SOURCE
#   define _GNU_SOURCE
#endif

#include "message-queue.h"
#include "target.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#ifndef _WIN32
#	include <unistd.h>
#	include <fcntl.h>
#endif

/* ------------------------------------------------------------------------- */

/* Platform specific stuff */
#ifdef _WIN32
static void CreateHandle(MTI_MessageQueue* q)
{ q->handles[0] = CreateEvent(NULL, FALSE, FALSE, NULL); }

static void FreeHandle(MTI_MessageQueue* q)
{ CloseHandle(q->handles[0]); }

MT_Handle GetHandle(MTI_MessageQueue* q)
{ return q->handles[0]; }

static void ResetHandle(MTI_MessageQueue* q)
{ (void) q; }

static void WakeUp(MTI_MessageQueue* q)
{ SetEvent(q->handles[0]); }


/* ------------------------------------------------------------------------- */

#else
#define READ 0
#define WRITE 1
static void CreateHandle(MTI_MessageQueue* q)
{ 
#ifdef __linux__
    pipe2(q->handles, O_CLOEXEC); 
#else
    pipe(q->handles);
    fcntl(q->handles[0], F_SETFD, FD_CLOEXEC);
    fcntl(q->handles[1], F_SETFD, FD_CLOEXEC);
#endif
}

static void FreeHandle(MTI_MessageQueue* q)
{
    close(q->handles[0]);
    close(q->handles[1]);
}

MT_Handle GetHandle(MTI_MessageQueue* q)
{ return q->handles[READ]; }

static void ResetHandle(MTI_MessageQueue* q)
{
    char buf[256];
    read(q->handles[READ], buf, 256);
}

static void WakeUp(MTI_MessageQueue* q)
{
    char ch = 0;
    write(q->handles[WRITE], &ch, 1);
}

#endif

/* ------------------------------------------------------------------------- */

/* Non platform specific stuff */

MT_Handle MTI_Queue_Init(MTI_MessageQueue* q)
{
    memset(q, 0, sizeof(MTI_MessageQueue));
    CreateHandle(q);
    return GetHandle(q);
}

/* ------------------------------------------------------------------------- */

void MTI_Queue_Destroy(MTI_MessageQueue* q)
{
    MT_QueueItem* item;
    while ((item = MT_Queue_Consume(&q->queue)) != NULL) {
        MT_Message* m = (MT_Message*) ((char*) item - offsetof(MT_Message, qitem));

        if (m->target) {
            MTI_Target_FinishMessage(m);
        } else if (m->free) {
            m->free(m);
        }
    }
    assert(!q->queue.first && !q->queue.last);
    FreeHandle(q);
}

/* ------------------------------------------------------------------------- */

void MTI_Queue_Dispatch(void* u)
{
    MTI_MessageQueue* q = (MTI_MessageQueue*) u;
    MT_QueueItem* item;

    ResetHandle(q);
    MT_AtomicInt_Set(&q->woken, 0);

    while ((item = MT_Queue_Consume(&q->queue)) != NULL) {

        MT_Message* m = (MT_Message*) ((char*) item - offsetof(MT_Message, qitem));

        LOG("Calling %p", m);
        if (m->call) {
            m->call(m);
        }

        if (m->target) {
            MTI_Target_FinishMessage(m);
        } else if (m->free) {
            m->free(m);
        }
    }
}

/* ------------------------------------------------------------------------- */

void MTI_Queue_Post(MTI_MessageQueue* q, MT_Message* m)
{
    MT_Queue_Produce(&q->queue, &m->qitem);

    if (MT_AtomicInt_SetFrom(&q->woken, 0, 1) == 0) {
        WakeUp(q);
    }
}


