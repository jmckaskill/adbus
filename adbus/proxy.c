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
#include "misc.h"

#include "memory/kpool.h"

// ----------------------------------------------------------------------------

struct adbus_Proxy
{
    adbus_Connection* connection;
    adbus_Message*    message;
    char*                   service;
    char*                   path;
    char*                   interface;
};

// ----------------------------------------------------------------------------

adbus_Proxy* adbus_proxy_new(
        adbus_Connection* connection,
        const char*             service,
        int                     ssize,
        const char*             path,
        int                     psize,
        const char*             interface,
        int                     isize)
{
    adbus_Proxy* p = NEW(adbus_Proxy);
    p->connection = connection;
    p->message = adbus_msg_new();

    p->service   = (ssize >= 0)
                 ? adbusI_strndup(service, ssize)
                 : adbusI_strdup(service);
    p->path      = (psize >= 0)
                 ? adbusI_strndup(path, psize)
                 : adbusI_strdup(path);
    p->interface = (isize >= 0)
                 ? adbusI_strndup(interface, isize)
                 : adbusI_strdup(interface);

    return p;
}

// ----------------------------------------------------------------------------

void adbus_proxy_free(adbus_Proxy* p)
{
    if (p) {
        adbus_msg_free(p->message);
        free(p->service);
        free(p->path);
        free(p->interface);
        free(p);
    }
}

// ----------------------------------------------------------------------------

void adbus_call_proxy(
        adbus_Caller*       f,
        adbus_Proxy*        p,
        const char*         member,
        int                 size)
{
    adbus_call_init(f, p->connection, p->message);
    f->destination = p->service;
    f->interface = p->interface;
    f->path = p->path;
    f->member = member;
    f->memberSize = size;
}







