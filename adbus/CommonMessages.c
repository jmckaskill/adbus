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
#include "Misc_p.h"
#include "User.h"
#include "memory/kstring.h"

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
        ADBusAppendArguments(marshaller, "s", -1);
        ADBusAppendString(marshaller, errorMessage, errorMessageSize);
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
        struct ADBusConnection*     connection,
        uint32_t                    serial)
{
    ADBusResetMessage(message);
    ADBusSetMessageType(message, ADBusMethodCallMessage);
    if (serial)
        ADBusSetSerial(message, serial);
    else
        ADBusSetSerial(message, ADBusNextSerial(connection));
}

// ----------------------------------------------------------------------------

static void Append(
        kstring_t* match,
        const char* fieldName,
        const char* field,
        int size)
{
    if (!field)
        return;

    if (size < 0)
        size = strlen(field);

    ks_cat(match, fieldName);
    ks_cat(match, "='");
    ks_cat_n(match, field, size);
    ks_cat(match, "',");
}

static void GetMatchString(const struct ADBusMatch* match, kstring_t* matchString)
{
    if (match->type == ADBusMethodCallMessage)
        ks_cat(matchString, "type='method_call',");
    else if (match->type == ADBusMethodReturnMessage)
        ks_cat(matchString, "type='method_return',");
    else if (match->type == ADBusErrorMessage)
        ks_cat(matchString, "type='error',");
    else if (match->type == ADBusSignalMessage)
        ks_cat(matchString, "type='signal',");

    // We only want to add the sender field if we are not going to have to do
    // a lookup conversion
    if (match->sender
     && !ADBusRequiresServiceLookup_(match->sender, match->senderSize))
    {
        Append(matchString, "sender", match->sender, match->senderSize);
    }
    Append(matchString, "interface", match->interface, match->interfaceSize);
    Append(matchString, "member", match->member, match->memberSize);
    Append(matchString, "path", match->path, match->pathSize);
    Append(matchString, "destination", match->destination, match->destinationSize);

    for (size_t i = 0; i < match->argumentsSize; ++i) {
        struct ADBusMatchArgument* arg = &match->arguments[i];
        ks_printf(matchString, "arg%d='", arg->number);
        if (arg->valueSize < 0) {
            ks_cat(matchString, arg->value);
        } else {
            ks_cat_n(matchString, arg->value, arg->valueSize);
        }
        ks_cat(matchString, "',");
    }

    // Remove the trailing ','
    if (ks_size(matchString) > 0)
        ks_remove_end(matchString, 1);
}

// ----------------------------------------------------------------------------

static void SetupBusCall(
        struct ADBusMessage* message,
        struct ADBusConnection* connection)
{
    ADBusSetupMethodCall(message, connection, 0);
    ADBusSetDestination(message, "org.freedesktop.DBus", -1);
    ADBusSetPath(message, "/", -1);
    ADBusSetInterface(message, "org.freedesktop.DBus", -1);
}

// ----------------------------------------------------------------------------

void ADBusSetupAddBusMatch(
        struct ADBusMessage*        message,
        struct ADBusConnection*     connection,
        const struct ADBusMatch*    match)
{
    SetupBusCall(message, connection);
    ADBusSetMember(message, "AddMatch", -1);

    kstring_t* mstr = ks_new();
    GetMatchString(match, mstr);

    struct ADBusMarshaller* marshaller = ADBusArgumentMarshaller(message);
    ADBusAppendArguments(marshaller, "s", -1);
    ADBusAppendString(marshaller, ks_cstr(mstr), ks_size(mstr));

    ks_free(mstr);
}

// ----------------------------------------------------------------------------

void ADBusSetupRemoveBusMatch(
        struct ADBusMessage*        message,
        struct ADBusConnection*     connection,
        const struct ADBusMatch*    match)
{
    SetupBusCall(message, connection);
    ADBusSetMember(message, "RemoveMatch", -1);

    kstring_t* mstr = ks_new();
    GetMatchString(match, mstr);

    struct ADBusMarshaller* marshaller = ADBusArgumentMarshaller(message);
    ADBusAppendArguments(marshaller, "s", -1);
    ADBusAppendString(marshaller, ks_cstr(mstr), ks_size(mstr));

    ks_free(mstr);
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
    ADBusAppendArguments(marshaller, "su", -1);
    ADBusAppendString(marshaller, service, size);
    ADBusAppendUInt32(marshaller, flags);
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
    ADBusAppendArguments(marshaller, "s", -1);
    ADBusAppendString(marshaller, service, size);
}

// ----------------------------------------------------------------------------

void ADBusSetupHello(
        struct ADBusMessage*        message,
        struct ADBusConnection*     connection)
{
    SetupBusCall(message, connection);
    ADBusSetMember(message, "Hello", -1);
}









