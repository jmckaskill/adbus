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

#include "internal.h"

/* Messages can only be sent to a single target, so for subscriptions with
 * more than one subscriber, we wrap the message with a broadcast message that
 * has multiple message headers (one for each target).
 */

typedef struct MTI_BroadcastMessage MTI_BroadcastMessage;

struct MTI_BroadcastMessage
{
    MT_AtomicInt    ref;
    MT_Message*     wrappedMessage;
    MT_Message      headers[1];
};

struct MT_Subscription
{
    MT_Spinlock         lock;

    MT_Target*          target;
    MT_Subscription*    tnext;
    MT_Subscription*    tprev;

    MT_Signal*          signal;
    MT_Subscription*    snext;
    MT_Subscription*    sprev;
};

void MTI_Signal_UnsubscribeAll(MT_Signal* s);
void MTI_Target_UnsubscribeAll(MT_Target* t);


