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

#include "subscriber.h"
#include <assert.h>

/* ------------------------------------------------------------------------- */

void MT_Signal_Init(MT_Signal* s)
{
    memset(s, 0, sizeof(MT_Signal));
}

/* ------------------------------------------------------------------------- */

void MT_Signal_Destroy(MT_Signal* s)
{
    MTI_Signal_UnsubscribeAll(s);
}

/* ------------------------------------------------------------------------- */

#define READY   0
#define UPDATE  1
#define DESTROY 2

static int TryLockForUpdate(MT_AtomicInt* a)
{
    while (1) {
        int val = MT_AtomicInt_SetFrom(a, READY, UPDATE);

        if (val == READY) {
            /* Grabbed the update lock */
            return 1;
        } else if (val == DESTROY) {
            /* Can't grab the lock */
            return 0;
        }

        /* Otherwise keep on spinning */
        assert(val == UPDATE);
    }
}

static void UpdateRelease(MT_AtomicInt* a)
{ MT_AtomicInt_Set(a, READY); }

static void LockForDestroy(MT_AtomicInt* a)
{
    while (1) {
        int prev = MT_AtomicInt_SetFrom(a, READY, DESTROY);
        if (prev == READY) {
            return;
        }
    }
}

/* ------------------------------------------------------------------------- */

static void BroadcastCall(MT_Message* m)
{
    MTI_BroadcastMessage* bm = (MTI_BroadcastMessage*) m->user;
    MT_Message* origmsg = bm->wrappedMessage;
    
    m->user = origmsg->user;
    if (origmsg->call) {
        origmsg->call(m);
    }
    m->user = bm;
}

static void BroadcastFree(MT_Message* m)
{
    MTI_BroadcastMessage* bm = (MTI_BroadcastMessage*) m->user;
    MT_Message* origmsg = bm->wrappedMessage;

    if (MT_AtomicInt_Decrement(&bm->ref) == 0) {

        m->user = origmsg->user;
        if (origmsg->free) {
            origmsg->free(m);
        }

        free(bm);
    }
}

void MT_Signal_Emit(MT_Signal* s, MT_Message* m)
{
    MT_Subscription* sub;

    int ret = TryLockForUpdate(&s->lock);

    /* You're not allowed to emit the signal whilst its being destroyed */
    (void) ret;
    assert(ret);

    sub = s->subscriptions;
    if (sub == NULL) {
        /* No subscriptions */
        assert(s->count == 0);
        UpdateRelease(&s->lock);

        if (m->free) {
            m->free(m);
        }

    } else if (sub->snext == NULL) {
        /* Single subscription */
        assert(s->count == 1);
        MT_Target_Post(sub->target, m);
        UpdateRelease(&s->lock);

    } else {
        /* Broadcast subscription */
        int i;
        size_t alloc;
        MTI_BroadcastMessage* bm;

        assert(s->count > 1);

        alloc = sizeof(MTI_BroadcastMessage) + ((s->count - 1) * sizeof(MT_Message));

        bm = (MTI_BroadcastMessage*) calloc(1, alloc);
        bm->ref = s->count;
        bm->wrappedMessage = m;

        for (i = 0; i < s->count; i++) {
            assert(sub);

            bm->headers[i].call = &BroadcastCall;
            bm->headers[i].free = &BroadcastFree;
            bm->headers[i].user = bm;

            MT_Target_Post(sub->target, &bm->headers[i]);

            sub = sub->snext;
        }

        UpdateRelease(&s->lock);
    }

}


/* ------------------------------------------------------------------------- */

void MT_Connect(MT_Signal* s, MT_Target* t)
{
    int ret;
    MT_Subscription* sub = NEW(MT_Subscription);


    ret = TryLockForUpdate(&t->lock);

    /* You're not allowed to connect to a target that is being destroyed */
    assert(ret);

    sub->tnext = t->subscriptions;
    if (sub->tnext) {
        sub->tnext->tprev = sub;
    }

    UpdateRelease(&t->lock);


    ret = TryLockForUpdate(&s->lock);

    /* You're not allowed to connect to a signal that is being destroyed */
    assert(ret);

    sub->snext = s->subscriptions;
    if (sub->snext) {
        sub->snext->sprev = sub;
    }
    s->count++;

    UpdateRelease(&s->lock);
}

/* ------------------------------------------------------------------------- */

void MTI_Signal_UnsubscribeAll(MT_Signal* s)
{
    MT_Subscription *sub, *next;

    LockForDestroy(&s->lock);

    for (sub = s->subscriptions; sub != NULL; sub = next) {
        next = sub->snext;

        /* If sub->target is non null, the target has not gotten to this node
         * yet. If the target begins to destroy itself it will then either be
         * forced to wait here (if it grabbed the update lock before we could)
         * or at LockForDestroy (if we grabbed its update lock first).  Thus
         * sub->target is guarenteed to be safe to use.
         */

        MT_Spinlock_Enter(&sub->lock);

        if (!sub->target) {
            /* The target has already removed itself from this subscription, we
             * can go ahead and free it.
             */

            free(sub);

        } else if (TryLockForUpdate(&sub->target->lock)) {
            /* The target has not removed itself yet and we managed to grab its
             * update lock. We can now remove the subscription from the target.
             */

            if (sub->tnext) {
                sub->tnext->tprev = sub->tprev;
            }

            if (sub->tprev) {
                sub->tprev->tnext = sub->tnext;
            }

            if (sub->target->subscriptions == sub) {
                sub->target->subscriptions = sub->tnext;
            }

            UpdateRelease(&sub->target->lock);

            free(sub);

        } else {
            /* The target has not removed itself yet, but we failed to grab the
             * update lock. This means that the target is currently being
             * destroyed. We just reset s->signal and then the target will free
             * the subscription once it gets around to it.
             */

            sub->signal = NULL;
            MT_Spinlock_Exit(&sub->lock);
        }
    }
}

/* ------------------------------------------------------------------------- */

/* Copy of MTI_Signal_UnsubscribeAll changed for the target's side */
void MTI_Target_UnsubscribeAll(MT_Target* t)
{
    MT_Subscription *sub, *next;

    LockForDestroy(&t->lock);

    for (sub = t->subscriptions; sub != NULL; sub = next) {
        next = sub->snext;

        /* If sub->sinal is non null, the signal has not gotten to this node
         * yet. If the signal begins to destroy itself it will then either be
         * forced to wait here (if it grabbed the update lock before we could)
         * or at LockForDestroy (if we grabbed its update lock first).  Thus
         * sub->signal is guarenteed to be safe to use.
         */

        MT_Spinlock_Enter(&sub->lock);

        if (!sub->signal) {
            /* The signal has already removed itself from this subscription, we
             * can go ahead and free it.
             */

            free(sub);

        } else if (TryLockForUpdate(&sub->signal->lock)) {
            /* The signal has not removed itself yet and we managed to grab its
             * update lock. We can now remove the subscription from the signal.
             */

            if (sub->snext) {
                sub->snext->sprev = sub->sprev;
            }

            if (sub->sprev) {
                sub->sprev->snext = sub->snext;
            }

            if (sub->signal->subscriptions == sub) {
                sub->signal->subscriptions = sub->tnext;
            }

            UpdateRelease(&sub->signal->lock);

            free(sub);

        } else {
            /* The signal has not removed itself yet, but we failed to grab the
             * update lock. This means that the signal is currently being
             * destroyed. We just reset s->target and then the signal will free
             * the subscription once it gets around to it.
             */

            sub->target = NULL;
            MT_Spinlock_Exit(&sub->lock);
        }
    }
}

