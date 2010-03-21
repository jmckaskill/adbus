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

#include <adbus.h>

#include "connection.h"
#include "misc.h"

// ----------------------------------------------------------------------------

static int adbusI_ConnectCallback(adbus_CbData* d)
{
    size_t uniqueSize;
    const char* unique = adbus_check_string(d, &uniqueSize);

    adbusI_log("connected %*s", uniqueSize, unique);

    adbus_Connection* c = d->connection;
    assert(!c->uniqueService);
    c->uniqueService = adbusI_strdup(unique);
    c->connected = 1;

    if (c->connectCallback) {
        c->connectCallback(c->connectData);
    }
    return 0;
}

void adbus_conn_connect(
        adbus_Connection*       c,
        adbus_Callback          callback,
        void*                   user)
{
    adbusI_log("connecting");

    assert(!c->connected && !c->connectCallback);

    c->connectCallback  = callback;
    c->connectData      = user;

    adbus_Call f;
    adbus_call_method(c->bus, &f, "Hello", -1);
    f.callback  = &adbusI_ConnectCallback;

    adbus_call_send(c->bus, &f);
}

// ----------------------------------------------------------------------------

adbus_Bool adbus_conn_isconnected(const adbus_Connection* c)
{
    return c->connected;
}

// ----------------------------------------------------------------------------

const char*  adbus_conn_uniquename(
        const adbus_Connection* c,
        size_t* size)
{
    if (size)
        *size = c->connected ? strlen(c->uniqueService) : 0;
    return c->connected ? c->uniqueService : NULL;
}





