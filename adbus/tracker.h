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

struct adbusI_TrackedRemote
{
    adbusI_RemoteTracker*   tracker;
    int                     ref;
    dh_strsz_t              service;
    dh_strsz_t              unique;
};

ADBUSI_FUNC void adbusI_refTrackedRemote(adbusI_TrackedRemote* r);
ADBUSI_FUNC void adbusI_derefTrackedRemote(adbusI_TrackedRemote* r);

DHASH_MAP_INIT_STRSZ(Tracked, adbusI_TrackedRemote*);

struct adbusI_RemoteTracker
{
    adbus_Connection*       connection;
    d_Hash(Tracked)         remotes;
};


// Tracked remote comes pre-refed
ADBUSI_FUNC adbusI_TrackedRemote* adbusI_getTrackedRemote(
        adbusI_RemoteTracker* t,
        const char*           service,
        int                   size);

ADBUSI_FUNC void adbusI_initRemoteTracker(adbusI_RemoteTracker* t, adbus_Connection* c);
ADBUSI_FUNC void adbusI_freeRemoteTracker(adbusI_RemoteTracker* t);
