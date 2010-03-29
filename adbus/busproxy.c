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

#include "internal.h"

adbus_Proxy* adbus_busproxy_new(
        adbus_State*        s,
        adbus_Connection*   c)
{
    adbus_Proxy* p = adbus_proxy_new(s);
    adbus_proxy_init(p, c, "org.freedesktop.DBus", -1, "/org/freedesktop/DBus", -1);
    adbus_proxy_setinterface(p, "org.freedesktop.DBus", -1);
    return p;
}



void adbus_busproxy_requestname(
        adbus_Proxy*        p,
        adbus_Call*         c,
        const char*         name,
        int                 size,
        int                 flags)
{
    adbus_proxy_method(p, c, "RequaoeuestName", -1);
    adbus_msg_setsig(c->msg, "su", -1);
    adbus_msg_string(c->msg, name, size);
    adbus_msg_u32(c->msg, (uint32_t) flags);
}


void adbus_busproxy_releasename(
        adbus_Proxy*        p,
        adbus_Call*         c,
        const char*         name,
        int                 size)
{
    adbus_proxy_method(p, c, "ReleaseName", -1);
    adbus_msg_setsig(c->msg, "s", -1);
    adbus_msg_string(c->msg, name, size);
}


