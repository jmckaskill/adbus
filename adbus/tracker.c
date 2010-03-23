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

/* DBus services are a bit weird. When you send a message to a named service,
 * the destination field uses the sender, but the reply coming back will use
 * the unique name. Thus if we want to be able to send messages to a named
 * service and get the reply back correctly or hook up to a signal from a named
 * service, we need to hook up to the NameOwnerChanged singal from the bus.
 * The signature of this is sss (service, old owner's unique name, new owner's
 * unique name). In order to make this as seamless as possible we track the
 * NameOwnerChanged down in the bowels of match dispatch. Thus the user can
 * add a match from a named service as the sender and expect it to work correctly.
 *
 * There is a few caveats of this approach:
 *
 * 1. We only want to hook up to the NameOwnerChanged signal for the service names
 *    we are interested in. Otherwise every time a NameOwnerChanged signal comes
 *    out of the bus, all parties on the bus would have to wake up.
 *
 * 2. There is no real advantage to ref counting the service names to track and
 *    then disconnecting when we no longer need to track a service name. This is
 *    because:
 *
 *    a) Generic code acting on the entire set of remotes should use the unique
 *       name and thus the service name should be only used when its hard coded.
 *    b) Its too easy to hit the worst case by sending a message to a service
 *       (add a match), wait for the reply (remove the match) and repeat.
 *
 *    Thus after getting a match with a named service in the sender, we track
 *    that service name from then on (until the connection is closed).
 *
 * 3. Any return matches should not be tracked across a NameOwnerChanged. This
 *    means anything that supplies a reply serial to match against. This is
 *    because the serials are unique to the particular remote.
 *
 * 4. In reality if you want fully reliable method calls, you need to:
 *    a) Use only unique names.
 *    b) Hook up to the NameOwnerChanged signal and send the message to the new
 *       remote on change.
 *    c) Handle replies coming from one or more remotes in the presence of name
 *       changes.
 *    d) Handle your own timeout.
 *
 *    This is because on receipt of a NameOwnerChanged message, we have no way
 *    of knowing whether the name change occurred before or after the method
 *    call hit the bus. Also the previous owner may get the message before or
 *    after it releases the name and it may or may not reply (some remotes may
 *    release the name, but still process method calls to that service name).
 *
 * We handle the reply matches for service names by:
 * 1. Registering the remote for the service name
 *    a) If the service lookup has already succeeded we skip 2 and 3 and
 *       register it for the unique name.
 * 2. Create a service lookup request
 * 3. Moving all matches for the service name when the service lookup succeeds
 *
 * If after that point the service name changes we don't bother changing the
 * registration. We just leave them sitting still registered to the old unique
 * name. That way a) we can remove them later from an adbus_conn_removematch
 * and b) if a reply does come in later we can still hook it up.
 */


#include "tracker.h"
#include "connection.h"

/* -------------------------------------------------------------------------- */

static int GetNameOwner(adbus_CbData* d)
{
    adbusI_TrackedRemote* t = (adbusI_TrackedRemote*) d->user1;

    size_t uniquesz;
    const char* unique = adbus_check_string(d, &uniquesz);
    adbus_check_end(d);

    ADBUSI_LOG("Got service %s [%s]", t->service.str, unique);

    assert(!t->unique.str);
    t->unique.str = adbusI_strndup(unique, uniquesz);
    t->unique.sz  = uniquesz;

    return 0;
}

/* -------------------------------------------------------------------------- */

static int NameOwnerChanged(adbus_CbData* d)
{
    adbusI_TrackedRemote* t = (adbusI_TrackedRemote*) d->user1;
    size_t tosz;
    const char* to;

    adbus_check_string(d, NULL);
    adbus_check_string(d, NULL);
    to = adbus_check_string(d, &tosz);
    adbus_check_end(d);

    ADBUSI_LOG("Service changed %s [%s -> %s]", t->service.str, t->unique.str, to);

    free((char*) t->unique.str);
    t->unique.str = adbusI_strndup(to, tosz);
    t->unique.sz  = tosz;

    return 0;
}

/* -------------------------------------------------------------------------- */

#define BUS "org.freedesktop.DBus"

adbusI_TrackedRemote* adbusI_getTrackedRemote(
        adbus_Connection*       c,
        const char*             service,
        int                     size)
{
    dh_strsz_t name;
    dh_Iter ti;
    int added;

    name.str = service;
    name.sz  = (size >= 0) ? size : strlen(service);

    ti = dh_put(Tracked, &c->tracker.lookup, name, &added);

    if (!added) {
        return dh_val(&c->tracker.lookup, ti);

    } else {
        adbusI_TrackedRemote* t = NEW(adbusI_TrackedRemote);
        t->ref                  = 1;

        dh_val(&c->tracker.lookup, ti) = t;

        if (*name.str == ':' || (name.sz == sizeof(BUS) - 1 && memcmp(BUS, name.str, name.sz) == 0)) {
            t->unique.str = adbusI_strndup(name.str, name.sz);
            t->unique.sz  = name.sz;
            dh_key(&c->tracker.lookup, ti) = t->unique;
            return t;
        }

        t->service.str                  = adbusI_strndup(name.str, name.sz);
        t->service.sz                   = name.sz;
        dh_key(&c->tracker.lookup, ti)  = t->service;

        /* We have a tracked remote where we need to go to the bus.
         * Artificially increment the refcount so this remote doesn't get
         * removed until the connection gets cleaned up.
         */
        t->ref++;

        {
            /* Add the NameOwnerChanged match */
            adbus_Match m;
            adbus_Argument arg0;

            adbus_arg_init(&arg0, 1);
            arg0.value = t->service.str;
            arg0.size  = t->service.sz;

            adbus_match_init(&m);
            m.arguments     = &arg0;
            m.argumentsSize = 1;
            m.callback      = &NameOwnerChanged;
            m.cuser         = t;

            adbus_proxy_signal(c->bus, &m, "NameOwnerChanged", -1);
        }

        {
            /* Call GetNameOwner - do this after adding the NameOwnerChanged match to
             * avoid a race condition.
             */
            adbus_Call f;
            adbus_call_method(c->bus, &f, "GetNameOwner", -1);
            f.callback = &GetNameOwner;
            f.cuser    = t;

            adbus_msg_setsig(f.msg, "s", 1);
            adbus_msg_string(f.msg, t->service.str, t->service.sz);

            adbus_call_send(c->bus, &f);
        }

        return t;
    }
}

/* -------------------------------------------------------------------------- */

static void FreeTrackedRemote(adbusI_TrackedRemote* t)
{
    free((char*) t->service.str);
    free((char*) t->unique.str);
    free(t);
}

/* -------------------------------------------------------------------------- */

void adbusI_derefTrackedRemote(adbusI_TrackedRemote* t)
{
    if (--t->ref == 0) {
        assert(t->unique.str && !t->service.str);
        FreeTrackedRemote(t);
    }
}

/* -------------------------------------------------------------------------- */

void adbusI_freeRemoteTracker(adbus_Connection* c)
{
    dh_Iter ti;
    d_Hash(Tracked)* h = &c->tracker.lookup;
    for (ti = dh_begin(h); ti != dh_end(h); ti++) {
        if (dh_exist(h, ti)) {
            FreeTrackedRemote(dh_val(h, ti));
        }
    }
    dh_free(Tracked, h);
}

