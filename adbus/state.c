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

#include "state.h"

/** \struct adbus_State
 *  \brief Helper class to manage connection services (binds, matches and
 *  replies).
 *
 *  adbus_State provides two benefits over using the connection services
 *  directly:
 *  -# The connection services can only be added and removed on the connection
 *  thread (using adbus_conn_addreply(), adbus_conn_bind(), etc). An
 *  adbus_State instance acts as a proxy from the local thread to one or more
 *  connection threads managing the thread jumping needed to
 *  register/unregister services.
 *  -# Keeps track of services as the connection removes them, so that the
 *  remaining services can be removed in a single call.
 *
 *  The general idea is to keep an adbus_State associated with any callback
 *  data (ie the local object, data structure, etc). Thus when the local
 *  object gets destroyed it simply needs to reset or free the state and all
 *  remaining callbacks are cleared out.
 *
 *  Some points to note:
 *  - An adbus_State can handle services for any number of connections.
 *  - Each adbus_State is designed to be used from the thread it was created
 *  from. Specifically the client API can only be called from the thread that
 *  created the state, and all connection services will be setup to proxy
 *  messages to/from the thread it was created on.
 *  - Connection services can not set their proxy callbacks, as this is
 *  overwritten by adbus_State internally to proxy all messages to the thread
 *  it was created on.
 *  - The client API does not let you remove individual services. If you need
 *  to do this create a seperate adbus_State with only that service and then
 *  reset it to remove the service.
 *
 *  For example:
 *  \code
 *  struct Foo
 *  {
 *      adbus_State* state;
 *  };
 *
 *  struct Foo* CreateFoo()
 *  {
 *      struct Foo* f = (struct Foo*) calloc(sizeof(struct Foo), 1);
 *      f->state = adbus_state_new();
 *      return f;
 *  }
 *
 *  void DeleteFoo(struct Foo* f)
 *  {
 *      if (f) {
 *          adbus_state_free(f->state);
 *          free(f);
 *      }
 *  }
 *
 *  static int SomeCallback(adbus_CbData* d)
 *  {
 *      struct Foo* f = (struct Foo*) d->user1;
 *      // Do something
 *      return 0;
 *  }
 *
 *  void AddMatch(struct Foo* f, adbus_Connection* c)
 *  {
 *      adbus_Match m;      
 *      adbus_match_init(&m);
 *      m.sender    = "com.example.ExampleService";
 *      m.path      = "/";
 *      m.member    = "ExampleSignal";
 *      m.callback  = &SomeCallback;
 *      m.cuser     = f;
 *
 *      adbus_state_addmatch(f->state, c, &m);
 *  }
 *  \endcode
 *
 *  \sa adbus_Proxy
 *
 */

/* ------------------------------------------------------------------------- */

static void DupString(const char** str, int* sz)
{
    if (*str) {
        if (*sz < 0)
            *sz = (int) strlen(*str);
        *str = adbusI_strndup(*str, *sz);
    }
}

static void FreeString(const char* str)
{ free((char*) str); }

static void DupArguments(adbus_Argument** args, size_t sz)
{
    const adbus_Argument* from = *args;
    if (from) {
        size_t i;
        adbus_Argument* to = NEW_ARRAY(adbus_Argument, sz);
        for (i = 0; i < sz; ++i) {
            if (from[i].value) {
                int valuesz = (from[i].size >= 0) ? from[i].size : strlen(from[i].value);
                to[i].size = valuesz;
                to[i].value = adbusI_strndup(from[i].value, valuesz);
            }
        }
        *args = to;
    }
}

static void FreeArguments(adbus_Argument* args, size_t sz)
{
    if (args) {
        size_t i;
        for (i = 0; i < sz; ++i) {
            free((char*) args[i].value);
        }
        free(args);
    }
}

static void FreeData(adbusI_StateData* d)
{
    ADBUSI_LOG_3("free state data %p (state conn %p)", (void*) d, (void*) d->conn);

    dil_remove(StateData, d, &d->hl);

    if (d->release[0]) {
        if (d->conn->relproxy) {
            d->conn->relproxy(d->conn->relpuser, NULL, d->release[0], d->ruser[0]);
        } else {
            d->release[0](d->ruser[0]);
        }
    }

    if (d->release[1]) {
        if (d->conn->relproxy) {
            d->conn->relproxy(d->conn->relpuser, NULL, d->release[1], d->ruser[1]);
        } else {
            d->release[1](d->ruser[1]);
        }
    }

    free(d);
}

static void ReleaseDataCallback(void* user)
{ 
    adbusI_StateData* d = (adbusI_StateData*) user;
    ADBUSI_ASSERT_CONN_THREAD(d->conn->connection);
    FreeData(d); 
}

static void LookupConnection(adbus_State* s, adbusI_StateData* d, adbus_Connection* c);

/* ------------------------------------------------------------------------- */

/* Always called on the connection thread */
static void DoBind(void* user)
{
    adbusI_StateData* d = (adbusI_StateData*) user;

    ADBUSI_ASSERT_CONN_THREAD(d->conn->connection);
    d->data = adbus_conn_bind(d->conn->connection, &d->u.bind);

    if (d->data) {
        dil_insert_after(StateData, &d->conn->binds, d, &d->hl);
    }
}

static void FreeProxyBind(void* user)
{
    adbusI_StateData* d = (adbusI_StateData*) user;
    adbus_Bind* b = &d->u.bind;

    adbus_iface_deref(b->interface);
    FreeString(b->path);

    if (!d->data) {
        FreeData(d);
    }
}

/** Adds a binding to the supplied connection.
 *  \relates adbus_State
 *
 *  \note The proxy and relproxy fields must not be set. They will be
 *  overwritten by proxy methods to proxy messages to the local thread.
 *
 *  \warning This must be called on the state's local thread.
 *
 *  \sa adbus_conn_bind()
 */
void adbus_state_bind(
        adbus_State*        s,
        adbus_Connection*   c,
        const adbus_Bind*   b)
{
    adbusI_StateData* d = NEW(adbusI_StateData);
    adbus_Bind* b2      = &d->u.bind;

    LookupConnection(s, d, c);

    ADBUSI_ASSERT_THREAD(s->thread);
    assert(!b->proxy && !b->relproxy);

    ADBUSI_LOG_BIND_1(b, "bind %p (state %p, state conn %p)",
            (void*) d,
            (void*) s,
            (void*) d->conn);

    /* The msg callback is proxied directly to the local thread with the
     * supplied callback. We interecpt the release callback on the connection
     * thread to remove the state data. That will then call the supplied
     * release callback (if any) on the local thread.
     */

    d->release[0]   = b->release[0];
    d->ruser[0]     = b->ruser[0];
    d->release[1]   = b->release[1];
    d->ruser[1]     = b->ruser[1];

    *b2             = *b;

    b2->release[0]  = &ReleaseDataCallback;
    b2->ruser[0]    = d;
    b2->release[1]  = NULL;
    b2->ruser[1]    = NULL;

    b2->proxy       = d->conn->proxy;
    b2->puser       = d->conn->puser;
    b2->relproxy    = NULL;
    b2->relpuser    = NULL;

    if (adbus_conn_shouldproxy(c)) {
        DupString(&b2->path, &b2->pathSize);
        adbus_iface_ref(b2->interface);
        adbus_conn_proxy(c, &DoBind, &FreeProxyBind, d);
    } else {
        DoBind(d);
        if (!d->data) {
            FreeData(d);
        }
    }
}

/* ------------------------------------------------------------------------- */

static void DoAddMatch(void* user)
{
    adbusI_StateData* d  = (adbusI_StateData*) user;

    ADBUSI_ASSERT_CONN_THREAD(d->conn->connection);
    d->data = adbus_conn_addmatch(d->conn->connection, &d->u.match);

    if (d->data) {
        dil_insert_after(StateData, &d->conn->matches, d, &d->hl);
    }
}

static void FreeProxyMatch(void* user)
{
    adbusI_StateData* d  = (adbusI_StateData*) user;
    adbus_Match* m  = &d->u.match;

    FreeString(m->sender);
    FreeString(m->destination);
    FreeString(m->interface);
    FreeString(m->path);
    FreeString(m->member);
    FreeString(m->error);
    FreeArguments(m->arguments, m->argumentsSize);

    if (!d->data) {
        FreeData(d);
    }
}


/** Adds a match to the supplied connection.
 *  \relates adbus_State
 *
 *  \note The proxy and relproxy fields must not be set. They will be
 *  overwritten by proxy methods to proxy messages to the local thread.
 *
 *  \warning This must be called on the state's local thread.
 *
 *  \sa adbus_conn_addmatch()
 */
void adbus_state_addmatch(
        adbus_State*        s,
        adbus_Connection*   c,
        const adbus_Match*  m)
{
    adbusI_StateData* d = NEW(adbusI_StateData);
    adbus_Match* m2     = &d->u.match;

    LookupConnection(s, d, c);

    ADBUSI_ASSERT_THREAD(s->thread);
    assert(!m->proxy && !m->relproxy);

    ADBUSI_LOG_MATCH_1(m, "add match %p (state %p, state conn %p)",
            (void*) d,
            (void*) s,
            (void*) d->conn);

    /* The msg callback is proxied directly to the local thread with the
     * supplied callback. We interecpt the release callback on the connection
     * thread to remove the state data. That will then call the supplied
     * release callback (if any) on the local thread.
     */

    d->release[0]   = m->release[0];
    d->ruser[0]     = m->ruser[0];
    d->release[1]   = m->release[1];
    d->ruser[1]     = m->ruser[1];

    *m2             = *m;

    m2->release[0]  = &ReleaseDataCallback;
    m2->ruser[0]    = d;
    m2->release[1]  = NULL;
    m2->ruser[1]    = NULL;

    m2->proxy       = d->conn->proxy;
    m2->puser       = d->conn->puser;
    m2->relproxy    = NULL;
    m2->relpuser    = NULL;

    if (adbus_conn_shouldproxy(c)) {
        DupString(&m2->sender, &m2->senderSize);
        DupString(&m2->destination, &m2->destinationSize);
        DupString(&m2->interface, &m2->interfaceSize);
        DupString(&m2->path, &m2->pathSize);
        DupString(&m2->member, &m2->memberSize);
        DupString(&m2->error, &m2->errorSize);
        DupArguments(&m2->arguments, m2->argumentsSize);

        adbus_conn_proxy(c, &DoAddMatch, &FreeProxyMatch, d);
    } else {
        DoAddMatch(d);
        if (!d->data) {
            FreeData(d);
        }
    }
}

/* ------------------------------------------------------------------------- */

static void DoAddReply(void* user)
{
    adbusI_StateData* d = (adbusI_StateData*) user;

    ADBUSI_ASSERT_CONN_THREAD(d->conn->connection);
    d->data = adbus_conn_addreply(d->conn->connection, &d->u.reply);

    if (d->data) {
        dil_insert_after(StateData, &d->conn->replies, d, &d->hl);
    }
}

static void FreeProxyReply(void* user)
{
    adbusI_StateData* d = (adbusI_StateData*) user;
    adbus_Reply* r = &d->u.reply;

    FreeString(r->remote);

    if (!d->data) {
        FreeData(d);
    }
}

/** Adds a reply to the supplied connection.
 *  \relates adbus_State
 *
 *  \note The proxy and relproxy fields must not be set. They will be
 *  overwritten by proxy methods to proxy messages to the local thread.
 *
 *  \warning This must be called on the state's local thread.
 *
 *  \sa adbus_conn_addreply()
 */
void adbus_state_addreply(
        adbus_State*        s,
        adbus_Connection*   c,
        const adbus_Reply*  r)
{
    adbusI_StateData* d = NEW(adbusI_StateData);
    adbus_Reply* r2     = &d->u.reply;

    LookupConnection(s, d, c);

    assert(!r->proxy && !r->relproxy);
    ADBUSI_ASSERT_THREAD(s->thread);

    ADBUSI_LOG_REPLY_2(r, "add reply %p (state %p, state conn %p)",
            (void*) d,
            (void*) s,
            (void*) d->conn);

    /* The msg callback is proxied directly to the local thread with the
     * supplied callback. We interecpt the release callback on the connection
     * thread to remove the state data. That will then call the supplied
     * release callback (if any) on the local thread.
     */

    d->release[0]       = r->release[0];
    d->ruser[0]         = r->ruser[0];
    d->release[1]       = r->release[1];
    d->ruser[1]         = r->ruser[1];

    *r2                 = *r;

    r2->release[0]      = &ReleaseDataCallback;
    r2->ruser[0]        = d;
    r2->release[1]      = NULL;
    r2->ruser[1]        = NULL;

    r2->proxy           = d->conn->proxy;
    r2->puser           = d->conn->puser;
    r2->relproxy        = NULL;
    r2->relpuser        = NULL;

    if (adbus_conn_shouldproxy(c)) {
        DupString(&r2->remote, &r2->remoteSize);
        adbus_conn_proxy(c, &DoAddReply, &FreeProxyReply, d);
    } else {
        DoAddReply(d);
        if (!d->data) {
            FreeData(d);
        }
    }
}

/* ------------------------------------------------------------------------- */

/** Creates a new state object.
 *  \relates adbus_State
 */
adbus_State* adbus_state_new(void)
{
    adbus_State* s = NEW(adbus_State);
    s->refConnection = 1;
    s->thread = adbusI_current_thread();
    ADBUSI_LOG_1("new (state %p)", (void*) s);
    return s;
}

/* ------------------------------------------------------------------------- */

/* Called on the local thread */
static void LookupConnection(adbus_State* s, adbusI_StateData* d, adbus_Connection* c)
{
    adbusI_StateConn* conn;

    ADBUSI_ASSERT_THREAD(s->thread);

    DL_FOREACH(StateConn, conn, &s->connections, hl) {
        if (conn->connection == c) {
            d->conn = conn;
            return;
        }
    }

    conn = NEW(adbusI_StateConn);
    conn->connection = c;
    conn->refConnection = s->refConnection;
    dil_insert_after(StateConn, &s->connections, conn, &conn->hl);
    adbus_conn_getproxy(c, NULL, &conn->proxy, &conn->puser);
    adbus_conn_getproxy(c, &conn->relproxy, NULL, &conn->relpuser);

    if (s->refConnection) {
        adbus_conn_ref(c);
    }

    d->conn = conn;
}

/* Called on the connection thread if the connection still exists */
static void ResetConn(void* user)
{
    adbusI_StateConn* c = (adbusI_StateConn*) user;
    adbusI_StateData* d;

    ADBUSI_ASSERT_CONN_THREAD(c->connection);
    ADBUSI_LOG_3("reset (state conn %p)", (void*) c);

    DIL_FOREACH(StateData, d, &c->binds, hl) {
        adbus_conn_unbind(c->connection, (adbus_ConnBind*) d->data);
    }
    assert(dil_isempty(&c->binds));

    DIL_FOREACH(StateData, d, &c->matches, hl) {
        adbus_conn_removematch(c->connection, (adbus_ConnMatch*) d->data);
    }
    assert(dil_isempty(&c->matches));

    DIL_FOREACH(StateData, d, &c->replies, hl) {
        adbus_conn_removereply(c->connection, (adbus_ConnReply*) d->data);
    }
    assert(dil_isempty(&c->replies));
}

/* Always called after ResetConn (if its going to be called) - on indetermined
 * thread 
 */
static void FreeConn(void* user)
{
    adbusI_StateConn* c = (adbusI_StateConn*) user;

    ADBUSI_LOG_3("free (state conn %p)", (void*) c);

    if (c->refConnection) {
        adbus_conn_deref(c->connection);
    }

    free(c);
}

/** Resets the state, removing all services.
 *  \relates adbus_State
 *
 *  This will not free the state, so it is ready for reuse.
 *
 *  \warning This must be called on the state's local thread.
 */
void adbus_state_reset(adbus_State* s)
{
    adbusI_StateConn* c;

    ADBUSI_ASSERT_THREAD(s->thread);
    ADBUSI_LOG_1("reset (state %p)", (void*) s);

    DIL_FOREACH(StateConn, c, &s->connections, hl) {
        dil_remove(StateConn, c, &c->hl);
        if (adbus_conn_shouldproxy(c->connection)) {
            adbus_conn_proxy(c->connection, &ResetConn, &FreeConn, c);
        } else {
            ResetConn(c);
            FreeConn(c);
        }
    }

    assert(dil_isempty(&s->connections));
}

/* ------------------------------------------------------------------------- */

/** Frees the state, removing all services.
 *  \relates adbus_State
 */
void adbus_state_free(adbus_State* s)
{
    if (s) {
        adbusI_StateConn* c;

        ADBUSI_ASSERT_THREAD(s->thread);
        ADBUSI_LOG_1("free (state %p)", (void*) s);

        DIL_FOREACH(StateConn, c, &s->connections, hl) {
            dil_remove(StateConn, c, &c->hl);
            if (adbus_conn_shouldproxy(c->connection)) {
                adbus_conn_proxy(c->connection, &ResetConn, &FreeConn, c);
            } else {
                ResetConn(c);
                FreeConn(c);
            }
        }

        assert(dil_isempty(&s->connections));
        free(s);
    }
}

/* ------------------------------------------------------------------------- */


