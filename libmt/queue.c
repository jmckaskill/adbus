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

#include "internal.h"
#include <assert.h>

/* ------------------------------------------------------------------------- */

MT_QueueItem* MT_Queue_Consume(MT_Queue* s)
{
    MT_QueueItem *first, *next;

    first = s->first;
    if (!first) {
        return NULL;
    }

    next = first->next;

    if (next) {
        /* More in the list */
        s->first = next;
        return first;

    } else {
        s->first = NULL;
        if (MT_AtomicPtr_SetFrom(&s->last, first, NULL) == first) {
            /* We had: next == NULL and s->first == s->last (ie no tail). Note
             * checking for s->first == s->last guarantees that next is still NULL
             * as the producers first update s->last and then the next pointer. We
             * just changed s->last to NULL. The next producer will then set
             * s->last and s->first to the next message.
             */
            return first;

        } else {
            /* We had: next == NULL, but s->first != s->last. IE we have a tail,
             * but it had not been released to us when we read next. Wait for the
             * tail to be released to us before we continue.
             */
            s->first = first;
            return NULL;
        }
    }
}

/* ------------------------------------------------------------------------- */

void MT_Queue_Produce(MT_Queue* s, MT_QueueItem* newval)
{
    MT_QueueItem* prevlast;
    newval->next = NULL;

    /* Append a new item to the list */
    prevlast = MT_AtomicPtr_Set(&s->last, newval);

    /* Release the item to the consumer */
    if (prevlast) {
        (void) MT_AtomicPtr_Set(&prevlast->next, newval);
    } else {
        (void) MT_AtomicPtr_Set(&s->first, newval);
    }
}

