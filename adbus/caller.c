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

#include <adbus/adbus.h>

#include <string.h>

// ----------------------------------------------------------------------------

void adbus_call_init(
        adbus_Caller*    f,
        adbus_Connection* c,
        adbus_Message*    message)
{
    adbus_msg_reset(message);
    memset(f, 0, sizeof(adbus_Caller));
    f->connection       = c;
    f->msg              = message;
    f->destinationSize  = -1;
    f->pathSize         = -1;
    f->interfaceSize    = -1;
    f->memberSize       = -1;
    f->type             = ADBUS_MSG_METHOD;
}

// ----------------------------------------------------------------------------

uint32_t adbus_call_send(adbus_Caller* f)
{
    adbus_Connection* c = f->connection;

    uint32_t serial = (f->serial) ? f->serial : adbus_conn_serial(c);
    f->matchId = 0;
    f->errorMatchId = 0;

    // Add matches for method_return and error
    {
        adbus_Match m;
        adbus_match_init(&m);
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
            m.replySerial   = serial;
        }

        if (f->callback) {
            m.type = ADBUS_MSG_RETURN;
            m.callback = f->callback;
            m.user1 = f->user1;
            m.user2 = f->user2;
            f->matchId = adbus_conn_addmatch(c, &m);
        }

        if (f->errorCallback) {
            m.type = ADBUS_MSG_ERROR;
            m.callback = f->errorCallback;
            m.user1 = f->errorUser1;
            m.user2 = f->errorUser2;
            f->errorMatchId = adbus_conn_addmatch(c, &m);
        }
    }

    // Send message
    {
        adbus_Message* m = f->msg;
        adbus_msg_settype(m, f->type);
        adbus_msg_setserial(m, serial);
        if (f->destination)
            adbus_msg_setdestination(m, f->destination, f->destinationSize);
        if (f->path)
            adbus_msg_setpath(m, f->path, f->pathSize);
        if (f->member)
            adbus_msg_setmember(m, f->member, f->memberSize);
        if (f->interface)
            adbus_msg_setinterface(m, f->interface, f->interfaceSize);

        adbus_conn_send(c, m);
    }

    return f->matchId;
}



