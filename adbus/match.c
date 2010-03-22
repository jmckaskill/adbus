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

#define ADBUS_LIBRARY
#include "connection.h"

#include <string.h>

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


// ----------------------------------------------------------------------------

/** Initialise an argument array
 *  \relates adbus_Argument
 */
void adbus_arg_init(adbus_Argument* args, size_t num)
{
    memset(args, 0, sizeof(adbus_Argument) * num);
    for (size_t i = 0; i < num; ++i) {
        args[i].size = -1;
    }
}

// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------

static void CloneString(const char* from, int fsize, const char** to, int* tsize)
{
    if (from) {
        if (fsize < 0)
            fsize = strlen(from);
        *to = adbusI_strndup(from, fsize);
        *tsize = fsize;
    }
}

static void CloneMatch(const adbus_Match* from, adbus_Match* to)
{
    to->type                = from->type;
    to->addMatchToBusDaemon = from->addMatchToBusDaemon;
    to->replySerial         = from->replySerial;
    to->callback            = from->callback;
    to->cuser               = from->cuser;
    to->proxy               = from->proxy;
    to->puser               = from->puser;
    to->release[0]          = from->release[0];
    to->ruser[0]            = from->ruser[0];
    to->release[1]          = from->release[1];
    to->ruser[1]            = from->ruser[1];
    to->relproxy            = from->relproxy;
    to->relpuser            = from->relpuser;

    CloneString(from->destination, from->destinationSize, &to->destination, &to->destinationSize);
    CloneString(from->interface, from->interfaceSize, &to->interface, &to->interfaceSize);
    CloneString(from->member, from->memberSize, &to->member, &to->memberSize);
    CloneString(from->error, from->errorSize, &to->error, &to->errorSize);

    if (from->path) {
        d_String sanitised;
        ZERO(&sanitised);
        adbusI_relativePath(&sanitised, from->path, from->pathSize, NULL, 0);
        to->pathSize = ds_size(&sanitised);
        to->path     = ds_release(&sanitised);
    }

    if (from->arguments && from->argumentsSize > 0) {
        to->arguments = NEW_ARRAY(adbus_Argument, from->argumentsSize);
        to->argumentsSize = from->argumentsSize;
        for (size_t i = 0; i < from->argumentsSize; ++i) {
            CloneString(from->arguments[i].value,
                        from->arguments[i].size,
                        &to->arguments[i].value,
                        &to->arguments[i].size);
        }
    }
}

// ----------------------------------------------------------------------------

adbus_ConnMatch* adbus_conn_addmatch(
        adbus_Connection*       c,
        const adbus_Match*      reg)
{
    assert(reg->callback);

    if (ADBUS_TRACE_MATCH) {
        adbusI_logmatch("add match", reg);
    }

    adbus_ConnMatch* m = NEW(adbus_ConnMatch);
    memset(m, 0, sizeof(adbus_ConnMatch));
    CloneMatch(reg, &m->m);
    m->service = adbusI_lookupService(c, reg->sender, reg->senderSize);

    if (!m->service) {
        CloneString(reg->sender, reg->senderSize, &m->m.sender, &m->m.senderSize);
    }

    if (m->m.addMatchToBusDaemon) {
        adbus_Proxy* proxy = c->bus;

        adbus_Call f;
        adbus_call_method(proxy, &f, "AddMatch", -1);

        adbus_msg_setsig(f.msg, "s", 1);

        d_String s;
        ZERO(&s);
        adbusI_matchString(&s, &m->m);
        adbus_msg_string(f.msg, ds_cstr(&s), ds_size(&s));
        ds_free(&s);

        adbus_call_send(proxy, &f);
    }

    dil_insert_after(Match, &c->matches, m, &m->hl);

    return m;
}

// ----------------------------------------------------------------------------

void adbus_conn_removematch(
        adbus_Connection*     c,
        adbus_ConnMatch*      m)
{
    if (ADBUS_TRACE_MATCH) {
        adbusI_logmatch("rm match", &m->m);
    }

    if (m->m.addMatchToBusDaemon) {
        adbus_Proxy* proxy = m->proxy ? m->proxy : c->bus;

        adbus_Call f;
        adbus_call_method(proxy, &f, "RemoveMatch", -1);

        adbus_msg_setsig(f.msg, "s", 1);

        d_String s;
        ZERO(&s);
        adbusI_matchString(&s, &m->m);
        adbus_msg_string(f.msg, ds_cstr(&s), ds_size(&s));
        ds_free(&s);

        adbus_call_send(proxy, &f);
    }

    adbusI_freeMatch(m);
}

// ----------------------------------------------------------------------------

void adbusI_freeMatch(adbus_ConnMatch* m)
{
    dil_remove(Match, m, &m->hl);

    if (m->m.release[0]) {
        if (m->m.relproxy) {
            m->m.relproxy(m->m.relpuser, m->m.release[0], m->m.ruser[0]);
        } else {
            m->m.release[0](m->m.ruser[0]);
        }
    }

    if (m->m.release[1]) {
        if (m->m.relproxy) {
            m->m.relproxy(m->m.relpuser, m->m.release[1], m->m.ruser[1]);
        } else {
            m->m.release[1](m->m.ruser[1]);
        }
    }

    adbus_state_free(m->state);
    adbus_proxy_free(m->proxy);
    free((char*) m->m.sender);
    free((char*) m->m.destination);
    free((char*) m->m.interface);
    free((char*) m->m.member);
    free((char*) m->m.error);
    free((char*) m->m.path);
    for (size_t i = 0; i < m->m.argumentsSize; i++) {
        free((char*) m->m.arguments[i].value);
    }
    free(m->m.arguments);
    free(m);
}

// ----------------------------------------------------------------------------

static adbus_Bool Matches(
        const char*     matchstr,
        size_t          matchsz,
        const char*     msgstr,
        size_t          msgsz)
{
    // Should this field be ignored
    if (!matchstr)
        return 1;

    // Does the message have this field
    if (!msgstr)
        return 0;

    if (matchsz != msgsz)
        return 0;

    return memcmp(matchstr, msgstr, matchsz) == 0;
}

static adbus_Bool ArgsMatch(
        adbus_Match*    match,
        adbus_Message*  msg)
{
    if (msg->argumentsSize < match->argumentsSize)
        return 0;

    for (size_t i = 0; i < match->argumentsSize; i++) {
        adbus_Argument* matcharg = &match->arguments[i];
        adbus_Argument* msgarg = &msg->arguments[i];

        if (!matcharg->value)
            continue;

        if (msgarg->value == NULL)
            return 0;

        if (msgarg->size != matcharg->size)
            return 0;

        if (memcmp(msgarg->value, matcharg->value, matcharg->size) != 0)
            return 0;

    }
    return 1;
}

int adbusI_dispatchMatch(adbus_CbData* d)
{
    adbus_Connection* c = d->connection;
    adbus_ConnMatch* m;
    DIL_FOREACH(Match, m, &c->matches, hl) {

        if (m->m.type != ADBUS_MSG_INVALID && d->msg->type != m->m.type) {
            continue;
        }

        if (    m->m.replySerial >= 0
            && (    !d->msg->replySerial 
                ||  *d->msg->replySerial != m->m.replySerial))
        {
            continue;
        }

        if (    (m->service && !Matches(m->service->unique.str, m->service->unique.sz, d->msg->sender, d->msg->senderSize))
            ||  !Matches(m->m.sender, m->m.senderSize, d->msg->sender, d->msg->senderSize)
            ||  !Matches(m->m.destination, m->m.destinationSize, d->msg->destination, d->msg->destinationSize)
            ||  !Matches(m->m.interface, m->m.interfaceSize, d->msg->interface, d->msg->interfaceSize)
            ||  !Matches(m->m.path, m->m.pathSize, d->msg->path, d->msg->pathSize)
            ||  !Matches(m->m.member, m->m.memberSize, d->msg->member, d->msg->memberSize)
            ||  !Matches(m->m.error, m->m.errorSize, d->msg->error, d->msg->errorSize))
        {
            continue;
        }

        if (m->m.arguments) {
            if (adbus_parseargs(d->msg))
                return -1;
            if (!ArgsMatch(&m->m, d->msg))
                continue;
        }

        d->user1 = m->m.cuser;

        if (m->m.proxy) {
            return m->m.proxy(m->m.puser, m->m.callback, d);
        } else {
            return adbus_dispatch(m->m.callback, d);
        }
    }

    return 0;
}


