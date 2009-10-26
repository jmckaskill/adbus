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

#include "Connection.h"
#include "User.h"
#include "memory/khash.h"
#include "memory/kvector.h"
#include "memory/kstring.h"
#include <setjmp.h>
#include <stdint.h>

// ----------------------------------------------------------------------------
// Struct definitions
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------

struct Service
{
    uint32_t                signalMatch;
    uint32_t                methodMatch;
    char*                   serviceName;
    char*                   uniqueName;
    int                     refCount;
};

struct Match
{
    struct ADBusMatch       m;
    struct Service*         service;
};

// ----------------------------------------------------------------------------

struct Bind
{
    struct ADBusInterface*  interface;
    struct ADBusUser*       user2;
};

// ----------------------------------------------------------------------------

KVECTOR_INIT(ObjectPtr, struct ADBusObject*)
KHASH_MAP_INIT_STR(ObjectPtr, struct ADBusObject*)
KHASH_MAP_INIT_STR(Bind, struct Bind)

struct ADBusObject
{
    struct ADBusConnection*     connection;
    char*                       name;
    khash_t(Bind)*              interfaces;
    kvector_t(ObjectPtr)*       children;
    struct ADBusObject*         parent;
};

// ----------------------------------------------------------------------------

KVECTOR_INIT(Match, struct Match)
KHASH_MAP_INIT_STR(Service, struct Service*)

struct ADBusConnection
{
    kvector_t(Match)*           registrations;
    khash_t(ObjectPtr)*         objects;
    uint32_t                    nextSerial;
    uint32_t                    nextMatchId;
    uint                        connected;
    char*                       uniqueService;

    ADBusConnectionCallback     connectCallback;
    struct ADBusUser*           connectCallbackData;

    ADBusSendCallback           sendCallback;
    struct ADBusUser*           sendCallbackData;

    ADBusMessageCallback        receiveMessageCallback;
    struct ADBusUser*           receiveMessageData;

    ADBusMessageCallback        sendMessageCallback;
    struct ADBusUser*           sendMessageData;

    struct ADBusMessage*        returnMessage;
    struct ADBusMessage*        outgoingMessage;

    struct ADBusIterator*       dispatchIterator;

    struct ADBusInterface*      introspectable;
    struct ADBusInterface*      properties;

    khash_t(Service)*           services;

    jmp_buf                     errorJmpBuf;
};

// ----------------------------------------------------------------------------

KVECTOR_INIT(uint8_t, uint8_t)

struct ADBusStreamBuffer
{
    kvector_t(uint8_t)*         buf;
};


