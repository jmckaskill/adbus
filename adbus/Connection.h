// vim: ts=4 sw=4 sts=4 et
//
// Copyright (c) 2009 James R. McKaskill
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// ----------------------------------------------------------------------------

#pragma once

#include "Common.h"
#include "Marshaller.h"
#include "Message.h"
#include "Parser.h"
#include "User.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ADBusConnection;
struct ADBusInterface;
struct ADBusMember;
struct ADBusObject;


// Connection management

ADBUS_API struct ADBusConnection* ADBusCreateConnection();

ADBUS_API void ADBusFreeConnection(
        struct ADBusConnection*     connection);

typedef void (*ADBusConnectionSendCallback)(
        struct ADBusConnection*     connection,
        struct ADBusUser*           user,
        const uint8_t*              data,
        size_t                      size);

ADBUS_API void ADBusSetConnectionSendCallback(
        struct ADBusConnection*     connection,
        ADBusConnectionSendCallback callback,
        struct ADBusUser*           data);

ADBUS_API int ADBusConnectionParse(
        struct ADBusConnection*     connection,
        const uint8_t*              data,
        size_t                      size);

// Bus management

typedef void (*ADBusConnectToBusCallback)(
        struct ADBusConnection*     /*connection*/,
        struct ADBusUser*           /*user*/);

ADBUS_API void ADBusConnectToBus(
        struct ADBusConnection*     connection,
        ADBusConnectToBusCallback   callback,
        struct ADBusUser*           user);

ADBUS_API uint ADBusIsConnectedToBus(
        struct ADBusConnection* connection);

//void ADBusAddService(struct ADBusConnection* c, const char* name, int size);
//void ADBusRemoveService(struct ADBusConnection* c, const char* name, int size);

ADBUS_API const char* ADBusGetUniqueServiceName(
        struct ADBusConnection*     connection,
        int*                        size);


// Callback Registrations

typedef int (*ADBusMatchCallback)(
        struct ADBusConnection*     connection,
        int                         id,
        struct ADBusUser*           user,
        struct ADBusMessage*        message);

struct ADBusMatch
{
    enum ADBusMessageType type;

    uint            addMatchToBusDaemon;
    uint            removeOnFirstMatch;

    uint32_t        replySerial;

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

    ADBusMatchCallback callback;

    struct ADBusUser user;
};

ADBUS_API uint32_t ADBusNextSerial(
        struct ADBusConnection*     connection);

ADBUS_API int ADBusAddMatch(
        struct ADBusConnection*     connection,
        const struct ADBusMatch*    match);

ADBUS_API void ADBusRemoveMatch(
        struct ADBusConnection*     connection,
        int                         id);


// Message management

ADBUS_API void ADBusSendError(
        struct ADBusConnection*     connection,
        struct ADBusMessage*        message,
        const char*                 errorName,
        int                         errorNameSize,
        const char*                 errorMessage,
        int                         errorMessageSize);

ADBUS_API void ADBusSendErrorExpanded(
        struct ADBusConnection*     connection,
        int                         replySerial,
        const char*                 destination,
        int                         destinationSize,
        const char*                 errorName,
        int                         errorNameSize,
        const char*                 errorMessage,
        int                         errorMessageSize);

ADBUS_API void ADBusSetupSignalMarshaller(
        struct ADBusObject*         object,
        struct ADBusMember*         signal,
        struct ADBusMarshaller*     marshaller);

ADBUS_API void ADBusSetupReturnMarshaller(
        struct ADBusConnection*     connection,
        struct ADBusMessage*        message,
        struct ADBusMarshaller*     marshaller);

ADBUS_API void ADBusSetupReturnMarshallerExpanded(
        struct ADBusConnection*     connection,
        int                         replySerial,
        const char*                 destination,
        int                         destinationSize,
        struct ADBusMarshaller*     marshaller);

ADBUS_API void ADBusConnectionSendMessage(
        struct ADBusConnection*     connection,
        struct ADBusMarshaller*     marshaller);


// Interface management

ADBUS_API struct ADBusInterface* ADBusCreateInterface(
        const char*                 name,
        int                         size);

ADBUS_API void ADBusFreeInterface(
        struct ADBusInterface*      interface);

typedef int (*ADBusMemberCallback)(
        struct ADBusConnection*     /*connection*/,
        const struct ADBusUser*     /*bindData*/,
        const struct ADBusMember*   /*member*/,
        struct ADBusMessage*        /*message*/);

enum ADBusMemberType
{
    ADBusMethodMember,
    ADBusSignalMember,
    ADBusPropertyMember,
};

ADBUS_API struct ADBusMember* ADBusAddMember(
        struct ADBusInterface*      interface,
        enum ADBusMemberType        type,
        const char*                 name,
        int                         size);

ADBUS_API struct ADBusMember* ADBusGetInterfaceMember(
        struct ADBusInterface*      interface,
        enum ADBusMemberType        type,
        const char*                 name,
        int                         size);

enum ADBusArgumentDirection
{
    ADBusInArgument,
    ADBusOutArgument,
};

ADBUS_API void ADBusAddArgument(
        struct ADBusMember*         member,
        enum ADBusArgumentDirection direction,
        const char*                 name,
        int                         nameSize,
        const char*                 type,
        int                         typeSize);

ADBUS_API const char* ADBusFullMemberSignature(
        const struct ADBusMember*   member,
        enum ADBusArgumentDirection direction,
        int*                        size);

ADBUS_API void ADBusAddAnnotation(
        struct ADBusMember*         member,
        const char*                 name,
        int                         nameSize,
        const char*                 value,
        int                         valueSize);

ADBUS_API void ADBusSetMemberUserData(
        struct ADBusMember*         member,
        struct ADBusUser*           user);

ADBUS_API const struct ADBusUser* ADBusMemberUserData(
        const struct ADBusMember*   member);

ADBUS_API void ADBusSetMethodCallback(
        struct ADBusMember*         member,
        ADBusMemberCallback         callback);

ADBUS_API void ADBusSetPropertyType(
        struct ADBusMember*         member,
        const char*                 type,
        int                         typeSize);

ADBUS_API void ADBusSetPropertyGetCallback(
        struct ADBusMember*         member,
        ADBusMemberCallback         callback);

ADBUS_API void ADBusSetPropertySetCallback(
        struct ADBusMember*         member,
        ADBusMemberCallback         callback);


// Object management

ADBUS_API struct ADBusObject* ADBusAddObject(
        struct ADBusConnection*     connection,
        const char*                 name,
        int                         size);

ADBUS_API void ADBusRemoveObject(
        struct ADBusConnection*     connection,
        struct ADBusObject*         object);

ADBUS_API void ADBusBindInterface(
        struct ADBusObject*         object,
        struct ADBusInterface*      interface,
        struct ADBusUser*           bindData);


#ifdef __cplusplus
}
#endif
