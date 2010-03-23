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


#define ADBUS_LIBRARY
#include "connection.h"

/* -------------------------------------------------------------------------- */

// Shifts any replies tied to s->service to s->unique
static void MoveReplies(struct ServiceLookup* s)
{
    adbus_Connection* c = s->connection;

    // Add unique in first since it may resize the hash table
    int addedunique = 0;
    dh_Iter ui = dh_put(Remote, &c->remotes, s->unique, &addedunique);
    struct Remote* unique = dh_val(&c->remotes, ui);

    // Get service
    dh_Iter si = dh_get(Remote, &c->remotes, s->service);
    if (si == dh_end(&c->remotes)) {
        // If service didn't exist clean up
        if (addedunique)
          dh_del(Remote, &c->remotes, ui);
        return;
    }

    struct Remote* service = dh_val(&c->remotes, si);

    // Setup unique
    if (addedunique) {
        unique = NEW(struct Remote);
        unique->name.str = adbusI_strndup(s->unique.str, s->unique.sz);
        unique->name.sz  = s->unique.sz;
        unique->connection = c;

        dh_key(&c->remotes, ui) = unique->name;
        dh_val(&c->remotes, ui) = unique;
    }

    // Move over the replies
    for (dh_Iter i = dh_begin(&service->replies); i != dh_end(&service->replies); i++) {
        if (dh_exist(&service->replies, i)) {
            adbus_ConnReply* reply = dh_val(&service->replies, i);
            int added;
            dh_Iter j = dh_put(Reply, &unique->replies, reply->serial, &added);
            assert(added);
            reply->remote = unique;
            dh_val(&unique->replies, j) = reply;
            dh_key(&unique->replies, j) = reply->serial;
        }
    }
    dh_clear(Reply, &service->replies);

    // Remove service remote
    dh_del(Remote, &c->remotes, si);
    service->connection = NULL;
    adbusI_freeRemote(service);
}

static int GetNameOwner(adbus_CbData* d)
{
    struct ServiceLookup* s = (struct ServiceLookup*) d->user1;

    size_t uniquesz;
    const char* unique = adbus_check_string(d, &uniquesz);
    adbus_check_end(d);

    if (ADBUS_TRACE_MATCH) {
        adbusI_log("got service %s [%s]", s->service.str, unique);
    }

    assert(!s->unique.str);
    s->unique.str = adbusI_strndup(unique, uniquesz);
    s->unique.sz  = uniquesz;

    MoveReplies(s);

    return 0;
}

static int NameOwnerChanged(adbus_CbData* d)
{
    struct ServiceLookup* s = (struct ServiceLookup*) d->user1;

    size_t tosz;
    adbus_check_string(d, NULL);
    adbus_check_string(d, NULL);
    const char* to = adbus_check_string(d, &tosz);
    adbus_check_end(d);

    if (ADBUS_TRACE_MATCH) {
        adbusI_log("service changed %s [%s -> %s]", s->service.str, s->unique.str, to);
    }

    free((char*) s->unique.str);
    s->unique.str = adbusI_strndup(to, tosz);
    s->unique.sz  = tosz;

    MoveReplies(s);

    return 0;
}

/* -------------------------------------------------------------------------- */

struct ServiceLookup* adbusI_lookupService(
        adbus_Connection*       c,
        const char*             service,
        int                     size)
{
    if (!service)
        return NULL;

    dh_strsz_t name = {
        service,
        size >= 0 ? (size_t) size : strlen(service),
    };

    if (name.sz == 0 ||  *name.str == ':')
        return NULL;

    if (    name.sz == sizeof("org.freedesktop.DBus") - 1
        &&  memcmp("org.freedesktop.DBus", name.str, name.sz) == 0)
    {
        return NULL;
    }

    if (ADBUS_TRACE_MATCH) {
        adbusI_log("add service %*s", name.sz, name.str);
    }

    int added = 0;
    dh_Iter si = dh_put(ServiceLookup, &c->services, name, &added);
    if (!added) {
        return dh_val(&c->services, si);
    }

    struct ServiceLookup* s = NEW(struct ServiceLookup);
    s->service.str          = adbusI_strndup(name.str, name.sz);
    s->service.sz           = name.sz;
    s->connection           = c;

    dh_key(&c->services, si) = s->service;
    dh_val(&c->services, si) = s;

    adbus_Proxy* p = adbus_proxy_new(c->state);
    adbus_proxy_init(p, c, "org.freedesktop.DBus", -1, "/org/freedesktop/DBus", -1);

    {
        // Add the NameOwnerChanged match
        adbus_Argument arg0;
        adbus_arg_init(&arg0, 1);
        arg0.value = s->service.str;
        arg0.size  = s->service.sz;

        adbus_Match m;
        adbus_match_init(&m);
        m.arguments     = &arg0;
        m.argumentsSize = 1;
        m.callback      = &NameOwnerChanged;
        m.cuser         = s;

        adbus_proxy_signal(p, &m, "NameOwnerChanged", -1);
    }

    {
        // Call GetNameOwner - do this after adding the NameOwnerChanged match to
        // avoid a race condition. Setup the match for the reply first
        adbus_Call f;
        adbus_call_method(p, &f, "GetNameOwner", -1);
        f.callback = &GetNameOwner;
        f.cuser    = s;

        adbus_msg_setsig(f.msg, "s", 1);
        adbus_msg_string(f.msg, s->service.str, s->service.sz);

        adbus_call_send(p, &f);
    }

    adbus_proxy_free(p);

    return s;
}

/* -------------------------------------------------------------------------- */

void adbusI_freeServiceLookup(struct ServiceLookup* s)
{
    if (s) {
        free((char*) s->service.str);
        free((char*) s->unique.str);
        free(s);
    }
}
