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

#include "Factory.h"

#include "Connection.h"
#include "Match.h"
#include "Message.h"

#include <string.h>

// ----------------------------------------------------------------------------

void ADBusInitFactory(
        struct ADBusFactory*    f,
        struct ADBusConnection* c,
        struct ADBusMessage*    message)
{
    ADBusResetMessage(message);
    memset(f, 0, sizeof(struct ADBusFactory));
    f->connection       = c;
    f->message          = message;
    f->destinationSize  = -1;
    f->pathSize         = -1;
    f->interfaceSize    = -1;
    f->memberSize       = -1;
    f->type             = ADBusMethodCallMessage;
    f->args             = ADBusArgumentMarshaller(message);
}

// ----------------------------------------------------------------------------

uint32_t ADBusCallFactory(struct ADBusFactory* f)
{
    struct ADBusConnection* c = f->connection;

    if (f->serial == 0) {
        f->serial = ADBusNextSerial(c);
    }

    f->matchId = 0;
    f->errorMatchId = 0;

    // Add matches for method_return and error
    {
        struct ADBusMatch m;
        ADBusInitMatch(&m);
        m.removeOnFirstMatch = 1;
        m.destination       = f->destination;
        m.destinationSize   = f->destinationSize;
        m.path              = f->path;
        m.pathSize          = f->pathSize;
        m.interface         = f->interface;
        m.interfaceSize     = f->interfaceSize;
        m.member            = f->member;
        m.memberSize        = f->memberSize;

        if (f->serial) {
            m.replySerial   = f->serial;
        }

        if (f->callback) {
            m.type = ADBusMethodReturnMessage;
            m.callback = f->callback;
            m.user1 = f->user1;
            m.user2 = f->user2;
            f->matchId = ADBusAddMatch(c, &m);
        }

        if (f->errorCallback) {
            m.type = ADBusErrorMessage;
            m.callback = f->errorCallback;
            m.user1 = f->errorUser1;
            m.user2 = f->errorUser2;
            f->errorMatchId = ADBusAddMatch(c, &m);
        }
    }

    // Send message
    {
        struct ADBusMessage* m = f->message;
        ADBusSetMessageType(m, f->type);
        ADBusSetSerial(m, f->serial);
        if (f->destination)
            ADBusSetDestination(m, f->destination, f->destinationSize);
        if (f->path)
            ADBusSetPath(m, f->path, f->pathSize);
        if (f->member)
            ADBusSetMember(m, f->member, f->memberSize);
        if (f->interface)
            ADBusSetInterface(m, f->interface, f->interfaceSize);

        ADBusSendMessage(c, m);
    }

    return f->matchId;
}



