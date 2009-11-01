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

#include "Bus.h"

#include "Connection_p.h"
#include "Factory.h"
#include "Iterator.h"
#include "Misc_p.h"
#include "Proxy.h"

// ----------------------------------------------------------------------------

static int ConnectCallback(struct ADBusCallDetails* d)
{
    size_t uniqueSize;
    const char* unique = ADBusCheckString(d, &uniqueSize);

    struct ADBusConnection* c = d->connection;
    free(c->uniqueService);
    c->uniqueService = strdup_(unique);
    c->connected = 1;

    ADBusConnectionCallback callback
            = (ADBusConnectionCallback) GetUserPointer(d->user1);

    if (callback) {
        callback(unique, uniqueSize, d->user2);
    }
    return 0;
}

void ADBusConnectToBus(
        struct ADBusConnection*     c,
        ADBusConnectionCallback     callback,
        struct ADBusUser*           callbackData)
{
    assert(!c->connected);

    struct ADBusFactory f;
    ADBusProxyFactory(c->bus, &f);
    f.member    = "Hello";
    f.callback  = &ConnectCallback;
    f.user1     = CreateUserPointer(callback);
    f.user2     = callbackData;

    ADBusCallFactory(&f);
}

// ----------------------------------------------------------------------------

uint ADBusIsConnectedToBus(const struct ADBusConnection* c)
{
    return c->connected;
}

// ----------------------------------------------------------------------------

const char*  ADBusGetUniqueServiceName(
        const struct ADBusConnection* c,
        size_t* size)
{
    if (size)
        *size = c->connected ? strlen(c->uniqueService) : 0;
    return c->connected ? c->uniqueService : NULL;
}

// ----------------------------------------------------------------------------

static int ServiceCallback(struct ADBusCallDetails* d)
{
    ADBusServiceCallback callback
            = (ADBusServiceCallback) GetUserPointer(d->user1);

    uint32_t retcode = ADBusCheckUInt32(d);
    callback(d->user2, (enum ADBusServiceCode) retcode);
    return 0;
}

// ----------------------------------------------------------------------------

uint32_t ADBusRequestServiceName(
        struct ADBusConnection*             c,
        const char*                         service,
        int                                 size,
        uint32_t                            flags,
        ADBusServiceCallback                callback,
        struct ADBusUser*                   user)
{
    struct ADBusFactory f;
    ADBusProxyFactory(c->bus, &f);
    f.member = "RequestName";

    ADBusAppendArguments(f.args, "su", -1);
    ADBusAppendString(f.args, service, size);
    ADBusAppendUInt32(f.args, flags);

    if (callback) {
        f.callback  = &ServiceCallback;
        f.user1     = CreateUserPointer(callback);
        f.user2     = user;
    }

    return ADBusCallFactory(&f);
}

// ----------------------------------------------------------------------------

uint32_t ADBusReleaseServiceName(
        struct ADBusConnection*             c,
        const char*                         service,
        int                                 size,
        ADBusServiceCallback                callback,
        struct ADBusUser*                   user)
{

    struct ADBusFactory f;
    ADBusProxyFactory(c->bus, &f);
    f.member = "ReleaseName";

    ADBusAppendArguments(f.args, "s", -1);
    ADBusAppendString(f.args, service, size);

    if (callback) {
        f.callback  = &ServiceCallback;
        f.user1     = CreateUserPointer(callback);
        f.user2     = user;
    }

    return ADBusCallFactory(&f);
}





