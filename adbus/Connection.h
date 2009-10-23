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

#include "Common.h"
#include "Interface.h"
#include "Marshaller.h"
#include "Message.h"
#include "User.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ADBusConnection;
struct ADBusInterface;
struct ADBusMember;
struct ADBusObject;

typedef void (*ADBusSendCallback)(
        struct ADBusMessage*        message,
        const struct ADBusUser*     user);

typedef void (*ADBusConnectionCallback)(
        struct ADBusConnection*     connection,
        const struct ADBusUser*     user);

typedef void (*ADBusServiceCallback)(
        struct ADBusConnection*         connection,
        const struct ADBusUser*         user,
        enum ADBusServiceCode           success);

// ----------------------------------------------------------------------------
// Connection management
// ----------------------------------------------------------------------------

ADBUS_API struct ADBusConnection* ADBusCreateConnection();
ADBUS_API void ADBusFreeConnection(struct ADBusConnection* connection);

// ----------------------------------------------------------------------------
// Message
// ----------------------------------------------------------------------------

ADBUS_API void ADBusSetSendCallback(
        struct ADBusConnection*     connection,
        ADBusSendCallback           callback,
        struct ADBusUser*           data);

ADBUS_API void ADBusSendMessage(
        struct ADBusConnection*     connection,
        struct ADBusMessage*        message);

ADBUS_API uint32_t ADBusNextSerial(struct ADBusConnection* c);

// ----------------------------------------------------------------------------
// Parsing and dispatch
// ----------------------------------------------------------------------------

struct ADBusStreamBuffer;

ADBUS_API struct ADBusStreamBuffer* ADBusCreateStreamBuffer();

ADBUS_API void ADBusFreeStreamBuffer(struct ADBusStreamBuffer* buffer);

ADBUS_API int ADBusParse(
        struct ADBusStreamBuffer*       buffer,
        struct ADBusMessage*            message,
        const uint8_t**                 pdata,
        size_t*                         size);

ADBUS_API void ADBusDispatch(
        struct ADBusConnection*         connection,
        struct ADBusMessage*            message);

ADBUS_API void ADBusRawDispatch(
        struct ADBusCallDetails*        details);

// ----------------------------------------------------------------------------
// Bus management
// ----------------------------------------------------------------------------

ADBUS_API void ADBusConnectToBus(
        struct ADBusConnection*     connection,
        ADBusConnectionCallback     callback,
        struct ADBusUser*           user);

ADBUS_API uint ADBusIsConnectedToBus(const struct ADBusConnection* connection);

ADBUS_API const char* ADBusGetUniqueServiceName(
        const struct ADBusConnection*       connection,
        size_t*                             size);

ADBUS_API void ADBusRequestServiceName(
        struct ADBusConnection*             connection,
        const char*                         service,
        int                                 size,
        uint32_t                            flags,
        ADBusServiceCallback                callback,
        struct ADBusUser*                   user);

ADBUS_API void ADBusReleaseServiceName(
        struct ADBusConnection*             connection,
        const char*                         service,
        int                                 size,
        ADBusServiceCallback                callback,
        struct ADBusUser*                   user);

// ----------------------------------------------------------------------------
// Match registrations
// ----------------------------------------------------------------------------

struct ADBusMatchArgument
{
    int             number;
    const char*     value;
    size_t          valueSize;
};

struct ADBusMatch
{
    // The type is checked if it is not ADBusInvalidMessage (0)
    enum ADBusMessageType type;

    // Matches for signals should be added to the bus, but method returns
    // are automatically routed to us by the daemon
    uint            addMatchToBusDaemon;
    uint            removeOnFirstMatch;

    // The replySerial field is only used if checkReplySerial is true
    // this is because 0 is a valid serial
    uint32_t        replySerial;
    uint            checkReplySerial;

    // The strings will all be copied out
    const char*     sender;
    int             senderSize;
    const char*     destination;
    int             destinationSize;
    const char*     interface;
    int             interfaceSize;
    const char*     path;
    int             pathSize;
    const char*     member;
    int             memberSize;
    const char*     errorName;
    int             errorNameSize;

    struct ADBusMatchArgument*  arguments;
    size_t                      argumentsSize;

    ADBusMessageCallback callback;

    // The user pointers will be freed using their free function
    struct ADBusUser* user1;
    struct ADBusUser* user2;

    // The id is ignored if its not set (0), but can be set with a value
    // returned from ADBusNextMatchId
    uint32_t             id;
};
#define ADBusMatchInit(pmatch) memset(pmatch, 0, sizeof(struct ADBusMatch))

ADBUS_API uint32_t ADBusNextMatchId(
        struct ADBusConnection*     connection);

ADBUS_API uint32_t ADBusAddMatch(
        struct ADBusConnection*     connection,
        const struct ADBusMatch*    match);

ADBUS_API void ADBusRemoveMatch(
        struct ADBusConnection*     connection,
        uint32_t                    id);

// ----------------------------------------------------------------------------
// Object management
// ----------------------------------------------------------------------------

ADBUS_API struct ADBusObject* ADBusAddObject(
        struct ADBusConnection*     connection,
        const char*                 path,
        int                         size);

ADBUS_API struct ADBusObject* ADBusGetObject(
        struct ADBusConnection*     connection,
        const char*                 path,
        int                         size);

ADBUS_API int ADBusBindInterface(
        struct ADBusObject*         object,
        struct ADBusInterface*      interface,
        struct ADBusUser*           user2);

ADBUS_API int ADBusUnbindInterface(
        struct ADBusObject*         object,
        struct ADBusInterface*      interface);

ADBUS_API struct ADBusInterface* ADBusGetBoundInterface(
        struct ADBusObject*         object,
        const char*                 interface,
        int                         interfaceSize,
        const struct ADBusUser**    user2);

ADBUS_API struct ADBusMember* ADBusGetBoundMember(
        struct ADBusObject*         object,
        enum ADBusMemberType        type,
        const char*                 member,
        int                         memberSize,
        const struct ADBusUser**    user2);



#ifdef __cplusplus
}
#endif
