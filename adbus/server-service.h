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

struct adbusI_ServiceOwner
{
    adbus_Remote*           remote;
    adbus_Bool              allowReplacement;
};

DVECTOR_INIT(ServiceOwner, adbusI_ServiceOwner)

DHASH_MAP_INIT_STRSZ(ServiceQueue, adbusI_ServiceQueue*)

struct adbusI_ServiceQueue
{
    /* The owner is the head [0] of the queue */
    d_Vector(ServiceOwner)  v;
    dh_strsz_t              name;
};

struct adbusI_ServiceQueueSet
{
    d_Hash(ServiceQueue)    queues;
};

ADBUSI_FUNC void adbusI_freeServiceQueue(adbus_Server* s);

ADBUSI_FUNC int adbusI_requestService(
        adbus_Server* s,
        adbus_Remote* r,
        const char* name,
        size_t size,
        uint32_t flags);

ADBUSI_FUNC int adbusI_releaseService(
        adbus_Server* s,
        adbus_Remote* r,
        const char* name,
        size_t size);

ADBUSI_FUNC adbus_Remote* adbusI_lookupRemote(
        adbus_Server* s,
        const char* name,
        size_t size);


