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

int ADBusError(
        struct ADBusCallDetails*    d,
        const char*                 errorName,
        const char*                 errorMsgFormat,
        ...)
{
    kstring_t* msg = ks_new();
    va_list ap;
    va_start(ap, errorMsgFormat);
    ks_vprintf(msg, errorMsgFormat, ap);
    va_end(ap);

    ADBusSetupError(d, errorName, -1, ks_cstr(msg), ks_size(msg));

    ks_free(msg);

    longjmp(d->jmpbuf, ADBusErrorJmp);
    return ADBusErrorMessage;
}

// ----------------------------------------------------------------------------

void ADBusSetupError(
        struct ADBusCallDetails*    d,
        const char*                 errorName,
        int                         errorNameSize,
        const char*                 errorMessage,
        int                         errorMessageSize)
{
    if (!d->retmessage)
        return;

    d->manualReply = 0;

    size_t destsize;
    const char* dest = ADBusGetSender(d->message, &destsize);
    int serial = ADBusGetSerial(d->message);

    struct ADBusMessage* m = d->retmessage;
    ADBusResetMessage(m);
    ADBusSetMessageType(m, ADBusErrorMessage);
    ADBusSetFlags(m, ADBusNoReplyExpectedFlag);
    ADBusSetSerial(m, ADBusNextSerial(d->connection));

    ADBusSetReplySerial(m, serial);
    ADBusSetErrorName(m, errorName, errorNameSize);

    if (dest) {
        ADBusSetDestination(m, dest, destsize);
    }

    if (errorMessage) {
        struct ADBusMarshaller* mar = ADBusArgumentMarshaller(m);
        ADBusAppendArguments(mar, "s", -1);
        ADBusAppendString(mar, errorMessage, errorMessageSize);
    }
}

// ----------------------------------------------------------------------------

void ADBusSetupSignal(
        struct ADBusMessage*        message,
        struct ADBusObjectPath*     path,
        struct ADBusMember*         signal)
{
    ADBusResetMessage(message);
    ADBusSetMessageType(message, ADBusSignalMessage);
    ADBusSetFlags(message, ADBusNoReplyExpectedFlag);
    ADBusSetSerial(message, ADBusNextSerial(path->connection));

    ADBusSetPath(message, path->path, path->pathSize);
    ADBusSetInterface(message, signal->interface->name, -1);
    ADBusSetMember(message, signal->name, -1);
}









