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

#include "Signal.h"
#include "Factory.h"
#include "Interface_p.h"
#include "Message.h"
#include "ObjectPath.h"

// ----------------------------------------------------------------------------

struct ADBusSignal
{
    struct ADBusMessage*    message;
    struct ADBusObjectPath* path;
    struct ADBusMember*     signal;
};

// ----------------------------------------------------------------------------

struct ADBusSignal* ADBusNewSignal(
        struct ADBusObjectPath* path, 
        struct ADBusMember*     member)
{
    struct ADBusSignal* s = NEW(struct ADBusSignal);
    s->message  = ADBusCreateMessage();
    s->path     = path;
    s->signal   = member;
    return s;
}

// ----------------------------------------------------------------------------

void ADBusFreeSignal(struct ADBusSignal* s)
{
    if (s) {
        ADBusFreeMessage(s->message);
        free(s);
    }
}

// ----------------------------------------------------------------------------

void ADBusSignalFactory(
        struct ADBusSignal*     s,
        struct ADBusFactory*    f)
{
    ADBusInitFactory(f, s->path->connection, s->message);
    f->type         = ADBusSignalMessage;
    f->flags        = ADBusNoReplyExpectedFlag;
    f->path         = s->path->path;
    f->interface    = s->signal->interface->name;
    f->member       = s->signal->name;
}



