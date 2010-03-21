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

#include "connection.h"
#include "misc.h"

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

