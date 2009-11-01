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

struct ADBusMatchArgument
{
    int             number;
    const char*     value;
    int             valueSize;
};

ADBUS_API void ADBusInitMatchArguments(struct ADBusMatchArgument* args, size_t num);



struct ADBusMatch
{
    // The type is checked if it is not ADBusInvalidMessage (0)
    enum ADBusMessageType type;

    // Matches for signals should be added to the bus, but method returns
    // are automatically routed to us by the daemon
    uint            addMatchToBusDaemon;
    uint            removeOnFirstMatch;

    int64_t         replySerial;

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

ADBUS_API void ADBusInitMatch(struct ADBusMatch* pmatch);



ADBUS_API uint32_t ADBusNextMatchId(
        struct ADBusConnection*     connection);

ADBUS_API uint32_t ADBusAddMatch(
        struct ADBusConnection*     connection,
        const struct ADBusMatch*    match);

ADBUS_API void ADBusRemoveMatch(
        struct ADBusConnection*     connection,
        uint32_t                    id);


