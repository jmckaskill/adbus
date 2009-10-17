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

#include <string.h>

#include "Common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ADBusUser;
struct ADBusMessage;
struct ADBusIterator;
struct ADBusMarshaller;

struct ADBusCallDetails
{
    struct ADBusConnection* connection;

    // Incoming message
    // Valid only if callback is originally in response to a method call
    struct ADBusMessage*    message;
    // Valid for method call callbacks
    struct ADBusIterator*   arguments;

    // Response
    // manualReply indicates to the callee whether there is a return message
    uint                    manualReply;
    // Messages to use for replying - may be NULL if the original caller
    // requested no reply
    // To send an error set the replyType to ADBusErrorMessageReply
    // and use ADBusSetupError
    struct ADBusMessage*    returnMessage;

    // Properties
    // The iterator can be used to get the new property value and on a get
    // callback, the marshaller should be filled with the property value.
    // Based off of the message (if non NULL) or other conditions the
    // callback code may want to send back an error (if non NULL).
    struct ADBusIterator*   propertyIterator;
    struct ADBusMarshaller* propertyMarshaller;

    // User data
    // For Interface callbacks:
    // User1 is from ADBusSetMethodCallCallback etc
    // User2 is from ADBusBindInterface
    // For match callbacks both are from ADBusAddMatch
    const struct ADBusUser* user1;
    const struct ADBusUser* user2;
};
#define ADBusInitCallDetails(P) memset(P, 0, sizeof(struct ADBusCallDetails))

typedef void (*ADBusMessageCallback)(struct ADBusCallDetails*);

typedef void (*ADBusUserFreeFunction)(struct ADBusUser*);

struct ADBusUser
{
    ADBusUserFreeFunction  free;
};

ADBUS_API void ADBusUserFree(struct ADBusUser* data);

#ifdef __cplusplus
}
#endif
