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


#include "Proxy.h"
#include "Factory.h"
#include "Misc_p.h"
#include "memory/kpool.h"

// ----------------------------------------------------------------------------

struct ADBusProxy
{
    struct ADBusConnection* connection;
    struct ADBusMessage*    message;
    char*                   service;
    char*                   path;
    char*                   interface;
};

// ----------------------------------------------------------------------------

struct ADBusProxy* ADBusCreateProxy(
        struct ADBusConnection* connection,
        const char*             service,
        int                     ssize,
        const char*             path,
        int                     psize,
        const char*             interface,
        int                     isize)
{
    struct ADBusProxy* p = NEW(struct ADBusProxy);
    p->connection = connection;
    p->message = ADBusCreateMessage();

    p->service   = (ssize >= 0)
                 ? strndup_(service, ssize)
                 : strdup_(service);
    p->path      = (psize >= 0)
                 ? strndup_(path, psize)
                 : strdup_(path);
    p->interface = (isize >= 0)
                 ? strndup_(interface, isize)
                 : strdup_(interface);

    return p;
}

// ----------------------------------------------------------------------------

void ADBusFreeProxy(struct ADBusProxy* p)
{
    if (p) {
        ADBusFreeMessage(p->message);
        free(p->service);
        free(p->path);
        free(p->interface);
        free(p);
    }
}

// ----------------------------------------------------------------------------

void ADBusProxyFactory(
        struct ADBusProxy*      p,
        struct ADBusFactory*    f)
{
    ADBusInitFactory(f, p->connection, p->message);
    f->destination = p->service;
    f->interface = p->interface;
    f->path = p->path;
}







