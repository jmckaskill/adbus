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

#include "CommonMessages.h"

#include "Connection.h"
#include "Connection_p.h"
#include "Interface_p.h"
#include "Message.h"
#include "User.h"
#include "str.h"

// ----------------------------------------------------------------------------

void ADBusSetupError(
        struct ADBusCallDetails*    details,
        const char*                 errorName,
        int                         errorNameSize,
        const char*                 errorMessage,
        int                         errorMessageSize)
{
    if (!details->returnMessage)
        return;

    details->manualReply = 0;

    size_t destSize;
    const char* destination = ADBusGetSender(details->message, &destSize);
    int serial = ADBusGetSerial(details->message);

    ADBusSetupErrorExpanded(
            details->returnMessage,
            details->connection,
            serial,
            destination,
            destSize,
            errorName,
            errorNameSize,
            errorMessage,
            errorMessageSize);
}

// ----------------------------------------------------------------------------

void ADBusSetupErrorExpanded(
        struct ADBusMessage*        message,
        struct ADBusConnection*     connection,
        uint32_t                    replySerial,
        const char*                 destination,
        int                         destinationSize,
        const char*                 errorName,
        int                         errorNameSize,
        const char*                 errorMessage,
        int                         errorMessageSize)
{
    ADBusResetMessage(message);
    ADBusSetMessageType(message, ADBusErrorMessage);
    ADBusSetFlags(message, ADBusNoReplyExpectedFlag);
    ADBusSetSerial(message, ADBusNextSerial(connection));

    ADBusSetReplySerial(message, replySerial);
    ADBusSetErrorName(message, errorName, errorNameSize);
    if (destination)
        ADBusSetDestination(message, destination, destinationSize);

    if (errorMessage)
    {
        struct ADBusMarshaller* marshaller = ADBusArgumentMarshaller(message);
        ADBusBeginArgument(marshaller, "s", -1);
        ADBusAppendString(marshaller, errorMessage, errorMessageSize);
        ADBusEndArgument(marshaller);
    }
}

// ----------------------------------------------------------------------------

void ADBusSetupSignal(
        struct ADBusMessage*        message,
        struct ADBusConnection*     connection,
        struct ADBusObject*         object,
        struct ADBusMember*         signal)
{
    ADBusResetMessage(message);
    ADBusSetMessageType(message, ADBusSignalMessage);
    ADBusSetFlags(message, ADBusNoReplyExpectedFlag);
    ADBusSetSerial(message, ADBusNextSerial(connection));

    ADBusSetPath(message, object->name, -1);
    ADBusSetInterface(message, signal->interface->name, -1);
    ADBusSetMember(message, signal->name, -1);
}

// ----------------------------------------------------------------------------

void ADBusSetupReturn(
        struct ADBusMessage*        message,
        struct ADBusConnection*     connection,
        struct ADBusMessage*        originalMessage)
{
    size_t destinationSize;
    const char* destination = ADBusGetSender(originalMessage, &destinationSize);
    int replySerial = ADBusGetSerial(originalMessage);

    ADBusSetupReturnExpanded(
            message,
            connection,
            replySerial,
            destination,
            destinationSize);
}

// ----------------------------------------------------------------------------

void ADBusSetupReturnExpanded(
        struct ADBusMessage*        message,
        struct ADBusConnection*     connection,
        uint32_t                    replySerial,
        const char*                 destination,
        int                         destinationSize)
{
    ADBusResetMessage(message);
    ADBusSetMessageType(message, ADBusMethodReturnMessage);
    ADBusSetFlags(message, ADBusNoReplyExpectedFlag);
    ADBusSetSerial(message, ADBusNextSerial(connection));

    ADBusSetReplySerial(message, replySerial);
    if (destination)
        ADBusSetDestination(message, destination, destinationSize);
}

// ----------------------------------------------------------------------------

void ADBusSetupMethodCall(
        struct ADBusMessage*        message,
        struct ADBusConnection*     connection)
{
    ADBusResetMessage(message);
    ADBusSetMessageType(message, ADBusMethodCallMessage);
    ADBusSetSerial(message, ADBusNextSerial(connection));
}

// ----------------------------------------------------------------------------

static void Append(
        str_t* match,
        const char* fieldName,
        const char* field,
        int size)
{
    if (!field)
        return;

    if (size < 0)
        size = strlen(field);

    str_append(match, fieldName);
    str_append(match, "='");
    str_append_n(match, field, size);
    str_append(match, "',");
}

static void GetMatchString(struct ADBusMatch* match, str_t* matchString)
{
    if (match->type == ADBusMethodCallMessage)
        str_append(matchString, "type='method_call',");
    else if (match->type == ADBusMethodReturnMessage)
        str_append(matchString, "type='method_return',");
    else if (match->type == ADBusErrorMessage)
        str_append(matchString, "type='error',");
    else if (match->type == ADBusSignalMessage)
        str_append(matchString, "type='signal',");

    Append(matchString, "sender", match->sender, match->senderSize);
    Append(matchString, "interface", match->interface, match->interfaceSize);
    Append(matchString, "member", match->member, match->memberSize);
    Append(matchString, "path", match->path, match->pathSize);
    Append(matchString, "destination", match->destination, match->destinationSize);

    // Remove the trailing ','
    if (str_size(matchString) > 0)
        str_remove_end(matchString, 1);
}

// ----------------------------------------------------------------------------

static void SetupBusCall(
        struct ADBusMessage* message,
        struct ADBusConnection* connection)
{
    ADBusSetupMethodCall(message, connection);
    ADBusSetDestination(message, "org.freedesktop.DBus", -1);
    ADBusSetPath(message, "/", -1);
    ADBusSetInterface(message, "org.freedesktop.DBus", -1);
}

// ----------------------------------------------------------------------------

void ADBusSetupAddBusMatch(
        struct ADBusMessage*        message,
        struct ADBusConnection*     connection,
        struct ADBusMatch*          match)
{
    SetupBusCall(message, connection);
    ADBusSetMember(message, "AddMatch", -1);

    str_t mstr = NULL;
    GetMatchString(match, &mstr);

    struct ADBusMarshaller* marshaller = ADBusArgumentMarshaller(message);
    ADBusBeginArgument(marshaller, "s", -1);
    ADBusAppendString(marshaller, mstr, str_size(&mstr));
    ADBusEndArgument(marshaller);

    str_free(&mstr);
}

// ----------------------------------------------------------------------------

void ADBusSetupRemoveBusMatch(
        struct ADBusMessage*        message,
        struct ADBusConnection*     connection,
        struct ADBusMatch*          match)
{
    SetupBusCall(message, connection);
    ADBusSetMember(message, "RemoveMatch", -1);

    str_t mstr = NULL;
    GetMatchString(match, &mstr);

    struct ADBusMarshaller* marshaller = ADBusArgumentMarshaller(message);
    ADBusBeginArgument(marshaller, "s", -1);
    ADBusAppendString(marshaller, mstr, str_size(&mstr));
    ADBusEndArgument(marshaller);

    str_free(&mstr);
}

// ----------------------------------------------------------------------------

void ADBusSetupRequestServiceName(
        struct ADBusMessage*        message,
        struct ADBusConnection*     connection,
        const char*                 service,
        int                         size,
        uint32_t                    flags)
{
    SetupBusCall(message, connection);
    ADBusSetMember(message, "RequestName", -1);

    struct ADBusMarshaller* marshaller = ADBusArgumentMarshaller(message);
    ADBusBeginArgument(marshaller, "s", -1);
    ADBusAppendString(marshaller, service, size);
    ADBusEndArgument(marshaller);

    ADBusBeginArgument(marshaller, "u", -1);
    ADBusAppendUInt32(marshaller, flags);
    ADBusEndArgument(marshaller);
}

// ----------------------------------------------------------------------------

void ADBusSetupReleaseServiceName(
        struct ADBusMessage*        message,
        struct ADBusConnection*     connection,
        const char*                 service,
        int                         size)
{
    SetupBusCall(message, connection);
    ADBusSetMember(message, "ReleaseName", -1);

    struct ADBusMarshaller* marshaller = ADBusArgumentMarshaller(message);
    ADBusBeginArgument(marshaller, "s", -1);
    ADBusAppendString(marshaller, service, size);
    ADBusEndArgument(marshaller);
}

// ----------------------------------------------------------------------------

void ADBusSetupHello(
        struct ADBusMessage*        message,
        struct ADBusConnection*     connection)
{
    SetupBusCall(message, connection);
    ADBusSetMember(message, "Hello", -1);
}









