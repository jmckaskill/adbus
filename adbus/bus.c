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

#include "connection.h"
#include "misc.h"

// ----------------------------------------------------------------------------

static int ConnectCallback(adbus_CbData* d)
{
    size_t uniqueSize;
    const char* unique = adbus_check_string(d, &uniqueSize);

    adbus_Connection* c = d->connection;
    free(c->uniqueService);
    c->uniqueService = adbusI_strdup(unique);
    c->connected = 1;

    adbus_ConnectCallback callback
            = (adbus_ConnectCallback) adbusI_puser_get(d->user1);

    if (callback) {
        callback(unique, uniqueSize, d->user2);
    }
    return 0;
}

void adbus_conn_connect(
        adbus_Connection*     c,
        adbus_ConnectCallback     callback,
        adbus_User*           callbackData)
{
    assert(!c->connected);

    adbus_Caller f;
    adbus_call_proxy(&f, c->bus, "Hello", -1);
    f.callback  = &ConnectCallback;
    f.user1     = adbusI_puser_new(callback);
    f.user2     = callbackData;

    adbus_call_send(&f);
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

// ----------------------------------------------------------------------------

static int ServiceCallback(adbus_CbData* d)
{
    adbus_NameCallback callback
            = (adbus_NameCallback) adbusI_puser_get(d->user1);

    uint32_t retcode = adbus_check_uint32(d);
    callback(d->user2, retcode);
    return 0;
}

// ----------------------------------------------------------------------------

uint32_t adbus_conn_requestname(
        adbus_Connection*             c,
        const char*                         service,
        int                                 size,
        uint32_t                            flags,
        adbus_NameCallback                callback,
        adbus_User*                   user)
{
    adbus_Caller f;
    adbus_call_proxy(&f, c->bus, "RequestName", -1);

    adbus_msg_append(f.msg, "su", -1);
    adbus_msg_string(f.msg, service, size);
    adbus_msg_uint32(f.msg, flags);

    if (callback) {
        f.callback  = &ServiceCallback;
        f.user1     = adbusI_puser_new(callback);
        f.user2     = user;
    }

    return adbus_call_send(&f);
}

// ----------------------------------------------------------------------------

uint32_t adbus_conn_releasename(
        adbus_Connection*             c,
        const char*                         service,
        int                                 size,
        adbus_NameCallback                callback,
        adbus_User*                   user)
{

    adbus_Caller f;
    adbus_call_proxy(&f, c->bus, "ReleaseName", -1);

    adbus_msg_append(f.msg, "s", -1);
    adbus_msg_string(f.msg, service, size);

    if (callback) {
        f.callback  = &ServiceCallback;
        f.user1     = adbusI_puser_new(callback);
        f.user2     = user;
    }

    return adbus_call_send(&f);
}





