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
        adbusI_TrackedRemote* t = dh_val(&c->tracker.lookup, ti);
        t->ref++;
        return t;

    } else {
        adbusI_TrackedRemote* t = NEW(adbusI_TrackedRemote);
        t->ref                  = 1;
        t->tracker              = &c->tracker;

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
            adbus_proxy_method(c->bus, &f, "GetNameOwner", -1);
            f.callback = &GetNameOwner;
            f.cuser    = t;

            adbus_msg_setsig(f.msg, "s", 1);
            adbus_msg_string(f.msg, t->service.str, t->service.sz);

            adbus_call_send(&f);
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
    if (t && --t->ref == 0) {

        if (t->tracker) {
            dh_Iter ii = dh_get(Tracked, &t->tracker->lookup, t->unique);
            if (ii != dh_end(&t->tracker->lookup)) {
                dh_del(Tracked, &t->tracker->lookup, ii);
            }
            t->tracker = NULL;
        }

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

