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

#include "client-bus.h"
#include "connection.h"

/* -------------------------------------------------------------------------- */

static int ConnectCallback(adbus_CbData* d)
{
    adbus_Connection* c = d->connection;

    size_t uniqueSize;
    const char* unique = adbus_check_string(d, &uniqueSize);

    ADBUSI_LOG_1("connected: \"%s\" (connection %p)", unique, (void*) c);

    assert(!c->connect.unique);
    adbusI_InterlockedExchangePointer((void* volatile*) &c->connect.unique, adbusI_strdup(unique));

    if (c->connect.cb) {
        c->connect.cb(c->connect.user);
    }

    return 0;
}

void adbus_conn_connect(
        adbus_Connection*       c,
        adbus_Callback          callback,
        void*                   user)
{
    ADBUSI_LOG_1("connecting (connection %p)", (void*) c);

    assert(!c->connect.unique && !c->connect.cb);

    c->connect.cb   = callback;
    c->connect.user = user;

    {
        adbus_Call f;
        adbus_proxy_method(c->bus, &f, "Hello", -1);
        f.callback  = &ConnectCallback;

        adbus_call_send(&f);
    }
}

/* -------------------------------------------------------------------------- */

adbus_Bool adbus_conn_isconnected(const adbus_Connection* c)
{ return c->connect.unique != NULL; }

/* -------------------------------------------------------------------------- */

const char*  adbus_conn_uniquename(
        const adbus_Connection* c,
        size_t* size)
{
    const char* unique = c->connect.unique;
    if (size)
        *size = unique ? strlen(unique) : 0;
    return unique;
}

/* -------------------------------------------------------------------------- */

void adbusI_freeConnBusData(adbusI_ConnBusData* d)
{ free(d->unique); }




