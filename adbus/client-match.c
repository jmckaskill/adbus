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

#include "client-match.h"
#include "connection.h"
#include "messages.h"

/** \struct adbus_Match
 *  \brief Data structure used to register general matches.
 *
 *  \note An adbus_Match structure should always be initialised using
 *  adbus_match_init().
 *
 *  \note The proxy and relproxy fields should almost always be initialised
 *  using adbus_conn_getproxy().
 *
 *  Matches are used to register for any message by specifying specific values
 *  to look for for the various message header fields. They can also be pushed
 *  through to the bus server. There are mostly used to register callbacks for
 *  signals from a specific remote object.
 *
 *  For example:
 *  \code
 *  static int Signal(adbus_CbData* d)
 *  {
 *      Object* o = (Object*) d->user1;
 *      o->OnSignal();
 *      return 0;
 *  }
 *
 *  void RegisterForSignal(adbus_Connection* c, Object* o)
 *  {
 *      adbus_Match m;
 *      adbus_match_init(&m);
 *      m.addMatchToBusDaemon = 1;
 *      m.type      = ADBUS_MSG_SIGNAL;
 *      m.sender    = "com.example.Service";
 *      m.path      = "/";
 *      m.member    = "ExampleSignal";
 *      m.callback  = &Signal;
 *      m.cuser     = o;
 *      adbus_state_addmatch(o->state(), c, &m);
 *  }
 *  \endcode
 *
 *  \note If writing C code, the adbus_State and adbus_Proxy modules \b vastly
 *  simplify the unregistering and thread jumping issues.
 *
 *  \sa adbus_Connection, adbus_conn_addmatch(), adbus_conn_addmatch(),
 *  adbus_Proxy
 */

/** \var adbus_Match::type
 *  Type of message to match or ADBUS_MSG_INVALID (default).
 */

/** \var adbus_Match::addMatchToBusDaemon
 *  Whether the match should be added to bus daemon (default not).
 */

/** \var adbus_Match::replySerial
 *  Reply serial to match or -1 (default).
 */

/** \var adbus_Match::sender
 *  Sender field to match or NULL (default).
 */

/** \var adbus_Match::senderSize
 *  Length of the adbus_Match::sender field or -1 if null terminated
 *  (default).
 */

/** \var adbus_Match::destination
 *  Destination field to match or NULL (default).
 */

/** \var adbus_Match::destinationSize
 *  Length of the adbus_Match::destination field or -1 if null terminated
 *  (default).
 */

/** \var adbus_Match::interface
 *  Interface field to match or NULL (default).
 */

/** \var adbus_Match::interfaceSize
 *  Length of the adbus_Match::interface field or -1 if null terminated
 *  (default).
 */

/** \var adbus_Match::path
 *  Object path field to match or NULL (default).
 */

/** \var adbus_Match::pathSize
 *  Length of the adbus_Match::path field or -1 if null terminated
 *  (default).
 */

/** \var adbus_Match::member
 *  Member field to match or NULL (default).
 */

/** \var adbus_Match::memberSize
 *  Length of the adbus_Match::member field or -1 if null terminated
 *  (default).
 */

/** \var adbus_Match::error
 *  Error name field to match or NULL (default).
 */

/** \var adbus_Match::errorSize
 *  Length of the adbus_Match::error field or -1 if null terminated
 *  (default).
 */

/** \var adbus_Match::arguments
 *  Array of adbus_Argument detailing what string arguments to match or NULL
 *  to not match arguments (default).
 */

/** \var adbus_Match::argumentsSize
 *  Size of the adbus_Match::argument array.
 */

/** \var adbus_Match::callback
 *  Function to call when a message matches the supplied fields.
 */

/** \var adbus_Match::cuser
 *  User data for the adbus_Match::callback function.
 */

/** \var adbus_Match::proxy
 *  Proxy function for the adbus_Match::callback function.
 *
 *  Should be default set using adbus_conn_getproxy().
 */

/** \var adbus_Match::puser
 *  User data for the adbus_Match::proxy function.
 *
 *  Should be default set using adbus_conn_getproxy().
 */

/** \var adbus_Match::release
 *  Function called when the match is removed from the connection.
 */

/** \var adbus_Match::ruser
 *  User data for the adbus_Match::release functions.
 */

/** \var adbus_Match::relproxy
 *  Proxy function for the adbus_Match::release field.
 *
 *  Should be default set using adbus_conn_getproxy().
 */

/** \var adbus_Match::relpuser
 *  User data for the adbus_Match::relproxy field.
 *
 *  Should be default set using adbus_conn_getproxy().
 */


/* -------------------------------------------------------------------------- */

/** Initialise an argument array
 *  \relates adbus_Argument
 */
void adbus_arg_init(adbus_Argument* args, size_t num)
{
    size_t i;
    memset(args, 0, sizeof(adbus_Argument) * num);
    for (i = 0; i < num; ++i) {
        args[i].size = -1;
    }
}

/* -------------------------------------------------------------------------- */

/** Initialise a match
 *  \relates adbus_Match
 */
void adbus_match_init(adbus_Match* pmatch)
{
    memset(pmatch, 0, sizeof(adbus_Match));
    pmatch->replySerial     = -1;
    pmatch->senderSize      = -1;
    pmatch->destinationSize = -1;
    pmatch->interfaceSize   = -1;
    pmatch->pathSize        = -1;
    pmatch->memberSize      = -1;
    pmatch->errorSize       = -1;
}

/* -------------------------------------------------------------------------- */

static void CloneString(const char** str, int* sz)
{
    if (*str) {
        if (*sz < 0)
            *sz = strlen(*str);
        *str = adbusI_strndup(*str, *sz);
    }
}

static void CloneMatch(const adbus_Match* from, adbus_Match* to)
{
    *to = *from;

    CloneString(&to->interface, &to->interfaceSize);
    CloneString(&to->member, &to->memberSize);
    CloneString(&to->error, &to->errorSize);

    if (to->path) {
        d_String path;
        ZERO(path);
        adbusI_sanitisePath(&path, to->path, to->pathSize);
        to->pathSize = ds_size(&path);
        to->path     = ds_release(&path);
    }

    if (to->arguments && to->argumentsSize > 0) {
        size_t i;
        to->arguments = NEW_ARRAY(adbus_Argument, to->argumentsSize);
        memcpy(to->arguments, from->arguments, to->argumentsSize * sizeof(adbus_Argument));
        for (i = 0; i < from->argumentsSize; ++i) {
            CloneString(&to->arguments[i].value, &to->arguments[i].size);
        }
    }
}

/* -------------------------------------------------------------------------- */

adbus_ConnMatch* adbus_conn_addmatch(
        adbus_Connection*       c,
        const adbus_Match*      reg)
{
    adbus_ConnMatch* m = NEW(adbus_ConnMatch);

    assert(!c->closed);
    assert(reg->callback);

    ADBUSI_LOG_MATCH_1(
			reg,
			"add match (connection %s, %p)",
			adbus_conn_uniquename(c, NULL),
			(void*) c);

    CloneMatch(reg, &m->m);

    if (m->m.sender) {
        m->sender = adbusI_getTrackedRemote(c, reg->sender, reg->senderSize);
        m->m.sender = NULL;
        m->m.senderSize = 0;
    }

    if (m->m.destination) {
        m->destination = adbusI_getTrackedRemote(c, reg->destination, reg->destinationSize);
        m->m.destination = NULL;
        m->m.destinationSize = 0;
    }

    if (m->m.addToBus) {
        adbus_Call f;

        adbusI_matchString(&m->matchString, &m->m);

        adbus_proxy_method(c->bus, &f, "AddMatch", -1);

        adbus_msg_setsig(f.msg, "s", 1);
        adbus_msg_string(f.msg, ds_cstr(&m->matchString), ds_size(&m->matchString));
        adbus_msg_end(f.msg);

        adbus_call_send(&f);
    }

    dil_insert_after(ConnMatch, &c->matches.list, m, &m->hl);

    return m;
}

/* -------------------------------------------------------------------------- */

static void FreeMatch(adbus_ConnMatch* m)
{
    size_t i;
    dil_remove(ConnMatch, m, &m->hl);

    if (m->m.release[0]) {
        if (m->m.relproxy) {
            m->m.relproxy(m->m.relpuser, NULL, m->m.release[0], m->m.ruser[0]);
        } else {
            m->m.release[0](m->m.ruser[0]);
        }
    }

    if (m->m.release[1]) {
        if (m->m.relproxy) {
            m->m.relproxy(m->m.relpuser, NULL, m->m.release[1], m->m.ruser[1]);
        } else {
            m->m.release[1](m->m.ruser[1]);
        }
    }

    adbusI_derefTrackedRemote(m->sender);
    adbusI_derefTrackedRemote(m->destination);
    ds_free(&m->matchString);
    free((char*) m->m.interface);
    free((char*) m->m.member);
    free((char*) m->m.error);
    free((char*) m->m.path);
    for (i = 0; i < m->m.argumentsSize; i++) {
        free((char*) m->m.arguments[i].value);
    }
    free(m->m.arguments);
    free(m);
}

/* -------------------------------------------------------------------------- */

void adbus_conn_removematch(
        adbus_Connection*     c,
        adbus_ConnMatch*      m)
{
    if (m) {
        ADBUSI_LOG_MATCH_1(
				&m->m,
				"remove match (connection %s, %p)",
				adbus_conn_uniquename(c, NULL),
				(void*) c);

        if (m->m.addToBus) {
            adbus_Call f;
            adbus_proxy_method(c->bus, &f, "RemoveMatch", -1);

            adbus_msg_setsig(f.msg, "s", 1);
            adbus_msg_string(f.msg, ds_cstr(&m->matchString), ds_size(&m->matchString));
            adbus_msg_end(f.msg);

            adbus_call_send(&f);
        }

        FreeMatch(m);
    }
}

/* -------------------------------------------------------------------------- */

void adbusI_freeMatches(adbus_Connection* c)
{
    adbus_ConnMatch* m;
    DIL_FOREACH (ConnMatch, m, &c->matches.list, hl) {
        FreeMatch(m);
    }
}

/* -------------------------------------------------------------------------- */

static adbus_Bool TrackedMatches(adbusI_TrackedRemote* r, const char* msg, size_t msgsz)
{
    if (r == NULL)
        return 1;
    if (r->unique.str == NULL)
        return 0;
    if (msg == NULL)
        return 0;
    if (r->unique.sz != msgsz)
        return 0;
    
    return memcmp(msg, r->unique.str, msgsz) == 0;
}

int adbusI_dispatchMatch(adbus_ConnMatch* m, adbus_CbData* d, d_Vector(Argument)* args)
{
    if (!adbusI_matchesMessage(&m->m, d->msg, args))
        return 0;

    if (!TrackedMatches(m->sender, d->msg->sender, d->msg->senderSize))
        return 0;

    if (!TrackedMatches(m->destination, d->msg->destination, d->msg->destinationSize))
        return 0;

    d->user1 = m->m.cuser;

    return adbusI_proxiedDispatch(
            m->m.proxy,
            m->m.puser,
            m->m.callback,
            d);
}


