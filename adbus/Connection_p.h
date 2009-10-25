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
#include "khash.h"
#include "vector.h"
#include "str.h"
#include <setjmp.h>

// ----------------------------------------------------------------------------
// Struct definitions
// ----------------------------------------------------------------------------

VECTOR_INSTANTIATE(struct ADBusMatch, match_)
VECTOR_INSTANTIATE(uint8_t, u8)
VECTOR_INSTANTIATE(struct ADBusObject*, pobject_)

// ----------------------------------------------------------------------------

struct ServiceNameMatch
{
    uint32_t                match;
    char*                   serviceName;
    char*                   uniqueName;
    int                     refCount;
};
KHASH_MAP_INIT_STR(ServiceNameMatch, struct ServiceNameMatch)

// ----------------------------------------------------------------------------

struct BoundInterface
{
    struct ADBusInterface*  interface;
    struct ADBusUser*       user2;
};
KHASH_MAP_INIT_STR(BoundInterface, struct BoundInterface)

// ----------------------------------------------------------------------------

struct ADBusObject
{
    struct ADBusConnection*     connection;
    char*                       name;
    khash_t(BoundInterface)*    interfaces;
    pobject_vector_t            children;
    struct ADBusObject*         parent;
};
typedef struct ADBusObject* ADBusObjectPtr;
KHASH_MAP_INIT_STR(ADBusObjectPtr, struct ADBusObject*)

// ----------------------------------------------------------------------------

struct ADBusConnection
{
    match_vector_t              registrations;
    khash_t(ADBusObjectPtr)*    objects;
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

    khash_t(ServiceNameMatch)   serviceNames;

    jmp_buf                     errorJmpBuf;
};

// ----------------------------------------------------------------------------

struct ADBusStreamBuffer
{
    u8vector_t                  buf;
};


