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

#pragma once

#include "misc.h"
#include "dmem/hash.h"
#include "dmem/vector.h"
#include "dmem/string.h"
#include "dmem/list.h"
#include <setjmp.h>
#include <stdint.h>

// ----------------------------------------------------------------------------

DILIST_INIT(Bind, adbus_ConnBind);

struct adbus_ConnBind
{
    d_IList(Bind)           fl;
    struct ObjectPath*      path;
    adbus_Interface*        interface;
    void*                   cuser2;
    adbus_ProxyMsgCallback  proxy;
    void*                   puser;
    adbus_Callback          release[2];
    void*                   ruser[2];
    adbus_ProxyCallback     relproxy;
    void*                   relpuser;
};


DVECTOR_INIT(ObjectPath, struct ObjectPath*);
DHASH_MAP_INIT_STRSZ(ObjectPath, struct ObjectPath*);
DHASH_MAP_INIT_STRSZ(Bind, adbus_ConnBind*);

struct ObjectPath
{
    adbus_Connection*       connection;
    dh_strsz_t              path;
    d_Hash(Bind)            interfaces;
    d_Vector(ObjectPath)    children;
    struct ObjectPath*      parent;
};

ADBUSI_FUNC void adbusI_freeBind(adbus_ConnBind* bind);
// This does not free the binds themselves, but rather resets the path pointer
// of all the binds
ADBUSI_FUNC void adbusI_freeObjectPath(struct ObjectPath* p);

// ----------------------------------------------------------------------------

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

struct ServiceLookup
{
    adbus_Connection*       connection;
    dh_strsz_t              service;
    dh_strsz_t              unique;
};

ADBUSI_FUNC void adbusI_freeServiceLookup(struct ServiceLookup* service);

/* The match lookup is simply a linked list of matches which we run through
 * checking the message against each match. Every message runs through this
 * list.
 */

DILIST_INIT(Match, adbus_ConnMatch);

struct adbus_ConnMatch
{
    d_IList(Match)          hl;
    adbus_Match             m;
    adbus_State*            state;
    adbus_Proxy*            proxy;
    struct ServiceLookup*   service;
};

ADBUSI_FUNC void adbusI_freeMatch(adbus_ConnMatch* m);

/* For replies we have an optimised match lookup. The connection holds a hash
 * table of sender -> Remote. The Remote then holds a hash table of reply
 * serial -> adbus_ConnReply. For service names we pre-resolve the service
 * name and register for the unique name (see ServiceLookup).
 */

DILIST_INIT(Reply, adbus_ConnReply);

struct adbus_ConnReply
{
    d_IList(Reply)              fl;

    struct Remote*              remote;
    uint32_t                    serial;

    adbus_MsgCallback           callback;
    void*                       cuser;

    adbus_MsgCallback           error;
    void*                       euser;

    adbus_ProxyMsgCallback      proxy;
    void*                       puser;

    adbus_Callback              release[2];
    void*                       ruser[2];

    adbus_ProxyCallback         relproxy;
    void*                       relpuser;
};

ADBUSI_FUNC void adbusI_freeReply(adbus_ConnReply* reply);

DHASH_MAP_INIT_UINT32(Reply, adbus_ConnReply*);

struct Remote
{
    adbus_Connection*           connection;
    dh_strsz_t                  name;
    d_Hash(Reply)               replies;
};

ADBUSI_FUNC struct Remote* adbusI_getRemote(adbus_Connection* c, const char* name);
// This does not free the replies themselves but rather resets the remote pointer
ADBUSI_FUNC void adbusI_freeRemote(struct Remote* remote);

// ----------------------------------------------------------------------------

DHASH_MAP_INIT_STRSZ(Remote, struct Remote*);
DHASH_MAP_INIT_STRSZ(ServiceLookup, struct ServiceLookup*);
DVECTOR_INIT(char, char);

struct adbus_Connection
{
    /** \privatesection */
    volatile long               ref;

    d_Hash(ObjectPath)          paths;
    d_Hash(Remote)              remotes;

    // We keep free lists for all registrable services so that they can be
    // released in adbus_conn_free.

    d_IList(Match)              matches;
    d_IList(Reply)              replies;
    d_IList(Bind)               binds;

    d_Hash(ServiceLookup)       services;

    uint32_t                    nextSerial;
    adbus_Bool                  connected;
    char*                       uniqueService;

    adbus_Callback              connectCallback;
    void*                       connectData;

    adbus_ConnectionCallbacks   callbacks;
    void*                       user;

    adbus_State*                state;
    adbus_Proxy*                bus;

    adbus_Interface*            introspectable;
    adbus_Interface*            properties;

    d_Vector(char)              parseBuffer;
    adbus_MsgFactory*           returnMessage;
};


ADBUSI_FUNC int adbusI_dispatchBind(adbus_CbData* d);
ADBUSI_FUNC int adbusI_dispatchReply(adbus_CbData* d);
ADBUSI_FUNC int adbusI_dispatchMatch(adbus_CbData* d);

ADBUSI_FUNC struct ServiceLookup* adbusI_lookupService(
        adbus_Connection*   c,
        const char*         service,
        int                 size);


