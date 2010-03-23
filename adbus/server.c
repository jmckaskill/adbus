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

#include "server.h"

/** \struct adbus_Server
 *  \brief Bus server
 *
 *  The provided bus server is a minimal single threaded server. The server by
 *  default provides the following members of the "org.freedesktop.DBus"
 *  interface on the "/" and "/org/freedesktop/DBus" paths:
 *  - Hello
 *  - RequestName
 *  - ReleaseName
 *  - ListNames
 *  - NameHasOwner
 *  - GetNameOwner
 *  - AddMatch
 *  - RemoveMatch
 *  - NameOwnerChanged
 *  - NameAcquired
 *  - NameLost
 *
 *  The call to adbus_serv_new() takes a fresh "org.freedesktop.DBus"
 *  adbus_Interface, so that the user API can add application specific or
 *  platform specific members.
 *
 *  The overall workflow is:
 *  -# Remote connects to server.
 *  -# Run the server side of the auth protocol (perhaps using the adbus_Auth
 *  module).
 *  -# Call adbus_serv_connect() to get an adbus_Remote for that remote.
 *  -# Receive incoming data on the remote and call adbus_remote_dispatch() or
 *  adbus_remote_parse() to dispatch.
 *  -# When the remote disconnects call adbus_remote_disconnect() to cleanup.
 *
 */

/** \struct adbus_Remote
 *  \brief Handle used by adbus_Server to refer to a specific remote
 */

/** Creates a new server using the provided fresh "org.freedesktop.DBus"
 *  interface.
 *  \relates adbus_Server
 */
adbus_Server* adbus_serv_new(adbus_Interface* bus)
{
    adbus_Server* s = NEW(adbus_Server);
    s->busInterface = bus;
    adbus_iface_ref(bus);

    adbusI_serv_initbus(s);

    return s;
}

/** Frees the server
 *  \relates adbus_Server
 */
void adbus_serv_free(adbus_Server* s)
{
    if (s == NULL)
        return;

    adbus_Remote* r = s->remotes.next;
    while (r) {
        adbus_Remote* next = r->hl.next;
        adbus_remote_disconnect(r);
        r = next;
    }
    dl_clear(Remote, &s->remotes);

    for (dh_Iter ii = dh_begin(&s->services); ii != dh_end(&s->services); ++ii) {
        if (dh_exist(&s->services, ii)) {
            adbusI_serv_freeservice(dh_val(&s->services, ii));
        }
    }
    dh_clear(Service, &s->services);

    dh_free(Service, &s->services);
    adbusI_serv_freebus(s);
    adbus_iface_deref(s->busInterface);
    free(s);
}




