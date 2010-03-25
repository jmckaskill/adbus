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

#include "client-reply.h"
#include "connection.h"

/** \struct adbus_Reply
 *  \brief Data structure to register for return and error messages from a
 *  method call.
 *
 *  \note An adbus_Reply structure should always be initialised using
 *  adbus_reply_init().
 *
 *  \note The proxy and relproxy fields should almost always be initialised
 *  using adbus_conn_getproxy().
 *
 *  The normal procedure to call method including registering for its reply
 *  is:
 *
 *  -# Get a serial for the method call.
 *
 *  -# Register for the reply by setting up an adbus_Reply and calling
 *  adbus_conn_addreply() or adbus_state_addreply() to register.
 *
 *  -# Send the method call using adbus_conn_send() or adbus_msg_send().
 *
 *  For example:
 *  \code
 *  static int Reply(adbus_CbData* d)
 *  {
 *      Object* o = (Object*) d->user1;
 *      o->OnReply();
 *      return 0;
 *  }
 *
 *  static int Error(adbus_CbData* d)
 *  {
 *      Object* o = (Object*) d->user1;
 *      o->OnError();
 *      return 0;
 *  }
 *
 *  void CallMethod(adbus_Connection* c, Object* o)
 *  {
 *      uint32_t serial = adbus_conn_serial(c);
 *
 *      // Register for the reply
 *      adbus_Reply reply;
 *      adbus_reply_init(&reply);
 *      reply.serial = serial;
 *      reply.remote = "com.example.Service";
 *      reply.user   = o;
 *      adbus_state_addreply(o->state(), c, &reply);
 *
 *      // Setup the method call
 *      adbus_MsgFactory* m = adbus_msg_new();
 *      adbus_msg_settype(m, ADBUS_MSG_METHOD);
 *      adbus_msg_setserial(m, serial);
 *      adbus_msg_setdestination(m, "com.example.Service", -1); 
 *      adbus_msg_setpath(m, "/", -1);
 *      adbus_msg_setmember(m, "ExampleMethod");
 *
 *      // Send the method call
 *      adbus_msg_send(m, c);
 *      adbus_msg_free(m);
 *  }
 *  \endcode
 *
 *  \note If writing C code, the adbus_State and adbus_Proxy modules \b vastly
 *  simplify the unregistering and thread jumping issues.
 *
 *  \warning If using adbus_conn_addreply() directly you should use a release
 *  callback to figure out if its safe to call adbus_conn_removereply, since
 *  replies are automatically removed upon receiving the first error or reply
 *  message.
 *
 *  \sa adbus_Connection, adbus_conn_addreply(), adbus_state_addreply(),
 *  adbus_Proxy
 */

/** \var adbus_Reply::serial
 *  The serial that replies and errors will be sent in response to.
 */

/** \var adbus_Reply::remote
 *  The remote the original method call was sent to.
 *
 *  Strictly speaking this is not required as a reply to a specific serial
 *  should only come from the destination (or the bus server). This is
 *  required to ensure that noone else on the bus tries to trick us.
 */

/** \var adbus_Reply::remoteSize
 *  Length of the adbus_Reply::remote field or -1 if null terminated
 *  (default).
 */

/** \var adbus_Reply::callback
 *  Function to call on a reply message.
 */

/** \var adbus_Reply::cuser
 *  User data for the adbus_Reply::callback field.
 */

/** \var adbus_Reply::error
 *  Function to call on an error reply.
 */

/** \var adbus_Reply::euser
 *  User data for the adbus_Reply::error field.
 */

/** \var adbus_Reply::proxy
 *  Proxy function to use to call the callback and error fields.
 *
 *  Normally this should be set using adbus_conn_getproxy().
 */

/** \var adbus_Reply::puser
 *  User data for the adbus_Reply::proxy field.
 *
 *  Normally this should be set using adbus_conn_getproxy().
 */

/** \var adbus_Reply::release
 *  Function to call when the reply is removed.
 */

/** \var adbus_Reply::ruser
 *  User data for the adbus_Reply::release field.
 */

/** \var adbus_Reply::relproxy
 *  Proxy funciton to use to call the release fields.
 *
 *  Normally this should be set using adbus_conn_getproxy().
 */

/** \var adbus_Reply::relpuser
 *  User data for the adbus_Reply::relproxy field.
 *
 *  Normally this should be set using adbus_conn_getproxy().
 */


/* -------------------------------------------------------------------------- */

/** Initialise an adbus_Reply structure.
 *  \relates adbus_Reply
 */
void adbus_reply_init(adbus_Reply* r)
{
    memset(r, 0, sizeof(adbus_Reply));
    r->serial = -1;
    r->remoteSize = -1;
}

/* -------------------------------------------------------------------------- */

/** Registers a reply with the connection.
 *  \relates adbus_Connection
 *
 *  \warning This should only be called on the connection thread. If not on
 *  the connection thread consider using adbus_state_addreply().
 */
adbus_ConnReply* adbus_conn_addreply(
        adbus_Connection*       c,
        const adbus_Reply*      reg)
{
    int added;
    dh_Iter ii;
    adbusI_TrackedRemote* remote;
    adbus_ConnReply* reply;
    uint32_t serial = (uint32_t) reg->serial;

    ADBUSI_LOG_REPLY("Add reply", reg);

    assert(reg->serial >= 0);
    assert(reg->remote);

    remote = adbusI_getTrackedRemote(c, reg->remote, reg->remoteSize);
    ii = dh_put(Reply, &c->replies.lookup, serial, &added);

    assert(added && remote);

    reply                   = NEW(adbus_ConnReply);
    reply->set              = &c->replies;
    reply->remote           = remote;
    reply->serial           = serial;
    reply->callback         = reg->callback;
    reply->cuser            = reg->cuser;
    reply->error            = reg->error;
    reply->euser            = reg->euser;
    reply->proxy            = reg->proxy;
    reply->puser            = reg->puser;
    reply->release[0]       = reg->release[0];
    reply->ruser[0]         = reg->ruser[0];
    reply->release[1]       = reg->release[1];
    reply->ruser[1]         = reg->ruser[1];
    reply->relproxy         = reg->relproxy;
    reply->relpuser         = reg->relpuser;

    dh_key(&c->replies.lookup, ii) = serial;
    dh_val(&c->replies.lookup, ii) = reply;

    dil_insert_after(Reply, &c->replies.list, reply, &reply->hl);

    return reply;
}

/* -------------------------------------------------------------------------- */

static void FreeReply(adbus_ConnReply* r)
{
    /* Disconnect from hash table */
    if (r->set) {
        d_Hash(Reply)* h = &r->set->lookup;
        dh_Iter ii = dh_get(Reply, h, r->serial);
        if (ii != dh_end(h)) {
            dh_del(Reply, h, ii);
        }
        r->set = NULL;
    }

    /* Disconnect from list */
    dil_remove(Reply, r, &r->hl);

    /* Call release callbacks */
    if (r->release[0]) {
        if (r->relproxy) {
            r->relproxy(r->relpuser, r->release[0], r->ruser[0]);
        } else {
            r->release[0](r->ruser[0]);
        }
        r->release[0] = NULL;
    }

    if (r->release[1]) {
        if (r->relproxy) {
            r->relproxy(r->relpuser, r->release[1], r->ruser[1]);
        } else {
            r->release[1](r->ruser[1]);
        }
        r->release[1] = NULL;
    }

    /* If we are in the callback, we will come back and do this once the
     * callback is complete.
     */
    if (!r->incallback) {
        /* Free data */
        adbusI_derefTrackedRemote(r->remote);
        free(r);
    }
}

/* -------------------------------------------------------------------------- */

void adbusI_freeReplies(adbus_Connection* c)
{
    adbus_ConnReply* r;
    DIL_FOREACH (Reply, r, &c->replies.list, hl) {
        r->set = NULL;
        FreeReply(r);
    }
    dh_free(Reply, &c->replies.lookup);
}

/* -------------------------------------------------------------------------- */

/** Unregisters a reply from the connection.
 *  \relates adbus_Connection
 *
 *  If in C code, you should probably use adbus_State rather than calling this
 *  directly as it manages the disconnects and thread issues.
 *
 *  \warning This should only be called on the connection thread.
 *
 *  \warning Since this should only be called if the reply is still registered
 *  and replies auto-remove on a reply or error message, you should use the
 *  adbus_Reply::release field to figure out if this needs to be called.
 */
void adbus_conn_removereply(
        adbus_Connection*       c,
        adbus_ConnReply*        reply)
{
    UNUSED(c);
    FreeReply(reply);
}

/* -------------------------------------------------------------------------- */

#define BUS "org.freedesktop.DBus"

int adbusI_dispatchReply(adbus_Connection* c, adbus_CbData* d)
{
    dh_Iter ii;
    uint32_t serial;
    adbus_ConnReply* reply;
    const char *sender, *unique;
    size_t sendsz, uniqsz;

    if (!d->msg->replySerial)
        return 0;

    /* Lookup the reply */

    serial = *d->msg->replySerial;
    ii = dh_get(Reply, &c->replies.lookup, serial);
    if (ii == dh_end(&c->replies.lookup))
        return 0;

    reply = dh_val(&c->replies.lookup, ii);

    if (reply->incallback)
        return 0;

    /* We only allow replies from the expected remote, bus server, or null for
     * security reasons
     */
    sender = d->msg->sender;
    sendsz = d->msg->senderSize;
    unique = reply->remote->unique.str;
    uniqsz = reply->remote->unique.sz;
    if (    !sender
        ||  (unique && sendsz == uniqsz && memcmp(sender, unique, sendsz) == 0)
        ||  (sendsz == sizeof(BUS) - 1 && memcmp(sender, BUS, sendsz) == 0))
    {

        /* Remove the reply from the lookup hash table. FreeReply does this if
         * reply->set is still set, but we do it now, since we know ii and the
         * callback may invalidate ii.
         */
        int ret = 0;
        adbus_MsgCallback cb;

        dh_del(Reply, &c->replies.lookup, ii);
        reply->set = NULL;
        reply->incallback = 1;

        if (d->msg->type == ADBUS_MSG_RETURN) {
            cb = reply->callback;
            d->user1 = reply->cuser;
        } else {
            cb = reply->error;
            d->user1 = reply->euser;
        }

        if (cb) {
            if (reply->proxy) {
                ret = reply->proxy(reply->puser, cb, &d);
            } else {
                ret = adbus_dispatch(cb, &d);
            }
        }

        reply->incallback = 0;
        FreeReply(reply);
        return ret;
    }
    else 
    {
        return 0;
    }
}


