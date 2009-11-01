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
#include "Connection.h"
#include "Message.h"


struct ADBusFactory
{
    struct ADBusConnection*     connection;
    struct ADBusMessage*        message;
    struct ADBusMarshaller*     args;

    enum ADBusMessageType       type;
    uint32_t                    serial;
    uint8_t                     flags;

    const char*                 destination;
    int                         destinationSize;
    const char*                 path;
    int                         pathSize;
    const char*                 interface;
    int                         interfaceSize;
    const char*                 member;
    int                         memberSize;

    ADBusMessageCallback        callback;
    struct ADBusUser*           user1;
    struct ADBusUser*           user2;

    ADBusMessageCallback        errorCallback;
    struct ADBusUser*           errorUser1;
    struct ADBusUser*           errorUser2;

    uint32_t                    matchId;
    uint32_t                    errorMatchId;

};

ADBUS_API void ADBusInitFactory(
        struct ADBusFactory*    factory,
        struct ADBusConnection* connection,
        struct ADBusMessage*    message);

ADBUS_API uint32_t ADBusCallFactory(
        struct ADBusFactory*    factory);


