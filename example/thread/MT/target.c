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

#include "target.h"
#include <stddef.h>

/* ------------------------------------------------------------------------- */

void MT_Target_Init(MT_Target* t)
{ 
    memset(t, 0, sizeof(MT_Target));
	t->loop = MT_Current();
}

/* ------------------------------------------------------------------------- */

void MT_Target_InitToLoop(MT_Target* t, MT_MainLoop* loop)
{ 
    memset(t, 0, sizeof(MT_Target));
	t->loop = loop; 
}

/* ------------------------------------------------------------------------- */

void MT_Target_Destroy(MT_Target* t)
{
    while (1) {
        MT_Message* m;
        MT_QueueItem* q = MT_Queue_Consume(&t->queue);
        if (q == NULL)
            break;

        /* Disable the calling of this message */
        m = (MT_Message*) ((char*) q - offsetof(MT_Message, titem));
        m->target = NULL;
        m->call = NULL;
    }
}

/* ------------------------------------------------------------------------- */

void MT_Target_Post(MT_Target* t, MT_Message* m)
{
    MT_Queue_Produce(&t->queue, &m->titem);
    MT_Loop_Post(t->loop, m);
}

/* ------------------------------------------------------------------------- */

void MTI_Target_FinishMessage(MT_Message* m)
{
    MT_Target* t = m->target;
    m->target = NULL;

    /* Remove any completed messages from the head of the queue. Note the
     * finished message may not be the head of the queue due to a race between
     * the target produce and the message queue produce.
     */
    while (1) {
        MT_QueueItem* q = MT_Queue_Consume(&t->queue);
        if (q == NULL)
            break;

        m = (MT_Message*) ((char*) q - offsetof(MT_Message, titem));
        if (m->target)
            break;

        if (m->free) {
            m->free(m->user);
        }
    }
}



