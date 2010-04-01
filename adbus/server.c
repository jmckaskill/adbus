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
    adbusI_serv_initBus(s, bus);

    return s;
}

/** Frees the server
 *  \relates adbus_Server
 */
void adbus_serv_free(adbus_Server* s)
{
    if (s) {
        adbusI_releaseService(s, s->bus.remote, "org.freedesktop.DBus");
        adbusI_freeServiceQueue(s);
        adbusI_serv_freeBus(s);
        assert(dl_isempty(&s->remotes.async));
        assert(dl_isempty(&s->remotes.sync));
        free(s);
    }
}

/* -------------------------------------------------------------------------- */

adbus_Remote* adbus_serv_caller(adbus_Server* s)
{ return s->caller; }

/* -------------------------------------------------------------------------- */

int adbusI_serv_dispatch(adbus_Server* s, adbus_Remote* from, adbus_Message* m)
{
    adbus_Remote* r;
    adbus_Remote* direct = NULL;

    ADBUSI_LOG_MSG_2(m, "dispatch (remote %p)", (void*) from);

    if (m->destination) {
        direct = adbusI_lookupRemote(s, m->destination);
    } else if (m->type == ADBUS_MSG_METHOD) {
        direct = s->bus.remote;
    }

    /* Call async remotes first since sync remotes can call back into this
     * function and otherwise the async remotes get a weird message ordering.
     */

    DL_FOREACH(Remote, r, &s->remotes.async, hl) {
        s->caller = from;
        if (    (r == direct || adbusI_serv_matches(&r->matches, m))
             && r->send(r->user, m) != (int) m->size) 
        {
            return -1;
        }
    }

    DL_FOREACH(Remote, r, &s->remotes.sync, hl) {
        s->caller = from;
        if (    (r == direct || adbusI_serv_matches(&r->matches, m))
             && r->send(r->user, m) != (int) m->size) 
        {
            return -1;
        }
    }

    if (m->destination && m->type == ADBUS_MSG_METHOD && !direct) {
        adbusI_serv_invalidDestination(s, m);
    }

    return 0;
}






