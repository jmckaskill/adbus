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

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------

struct ADBusCallDetails;
struct ADBusMessage;
struct ADBusConnection;
struct ADBusObject;
struct ADBusMember;
struct ADBusMatch;

// ----------------------------------------------------------------------------

ADBUS_API void ADBusSetupError(
        struct ADBusCallDetails*    details,
        const char*                 errorName,
        int                         errorNameSize,
        const char*                 errorMessage,
        int                         errorMessageSize);

ADBUS_API void ADBusSetupErrorExpanded(
        struct ADBusMessage*        message,
        struct ADBusConnection*     connection,
        uint32_t                    replySerial,
        const char*                 destination,
        int                         destinationSize,
        const char*                 errorName,
        int                         errorNameSize,
        const char*                 errorMessage,
        int                         errorMessageSize);

ADBUS_API void ADBusSetupSignal(
        struct ADBusMessage*        message,
        struct ADBusConnection*     connection,
        struct ADBusObject*         object,
        struct ADBusMember*         signal);

ADBUS_API void ADBusSetupReturn(
        struct ADBusMessage*        message,
        struct ADBusConnection*     connection,
        struct ADBusMessage*        originalMessage);

ADBUS_API void ADBusSetupReturnExpanded(
        struct ADBusMessage*        message,
        struct ADBusConnection*     connection,
        uint32_t                    replySerial,
        const char*                 destination,
        int                         destinationSize);

ADBUS_API void ADBusSetupMethodCall(
        struct ADBusMessage*        message,
        struct ADBusConnection*     connection,
        uint32_t                    serial);

ADBUS_API void ADBusSetupAddBusMatch(
        struct ADBusMessage*        message,
        struct ADBusConnection*     connection,
        const struct ADBusMatch*    match);

ADBUS_API void ADBusSetupRemoveBusMatch(
        struct ADBusMessage*        message,
        struct ADBusConnection*     connection,
        const struct ADBusMatch*    match);

ADBUS_API void ADBusSetupRequestServiceName(
        struct ADBusMessage*        message,
        struct ADBusConnection*     connection,
        const char*                 service,
        int                         size,
        uint32_t                    flags);

ADBUS_API void ADBusSetupReleaseServiceName(
        struct ADBusMessage*        message,
        struct ADBusConnection*     connection,
        const char*                 service,
        int                         size);

ADBUS_API void ADBusSetupHello(
        struct ADBusMessage*        message,
        struct ADBusConnection*     connection);

// ----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif


