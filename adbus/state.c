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
    if (*sz < 0)
        *sz = (int) strlen(*str);
    *str = adbusI_strndup(*str, *sz);
}

static void FreeData(adbusI_StateData* d)
{
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
{ FreeData((adbusI_StateData*) user); }

static void LookupConnection(adbus_State* s, adbusI_StateData* d, adbus_Connection* c);

/* ------------------------------------------------------------------------- */

/* Always called on the connection thread */
static int DoBind(adbusI_StateData* d)
{
    adbus_Bind* b = &d->u.bind;

    d->release[0]   = b->release[0];
    d->ruser[0]     = b->ruser[0];
    d->release[1]   = b->release[1];
    d->ruser[1]     = b->ruser[1];

    b->release[0]   = &ReleaseDataCallback;
    b->ruser[0]     = d;
    b->release[1]   = NULL;
    b->ruser[1]     = NULL;

    d->data = adbus_conn_bind(d->conn->connection, b);

    if (!d->data)
        return -1;

    dil_insert_after(StateData, &d->conn->binds, d, &d->hl);
    return 0;
}

static void ProxyBind(void* user)
{
    adbusI_StateData* d = (adbusI_StateData*) user;
    d->err = DoBind(d);
}

static void FreeProxyBind(void* user)
{
    adbusI_StateData* d = (adbusI_StateData*) user;
    adbus_iface_deref(d->u.bind.interface);
    free((char*) d->u.bind.path);
    if (d->err) {
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
    LookupConnection(s, d, c);
    d->u.bind = *b;
    d->err = -1;

    assert(!b->proxy && !b->relproxy);

    if (adbus_conn_shouldproxy(c)) {
        adbus_Bind* b2 = &d->u.bind;

        DupString(&b2->path, &b2->pathSize);

        b2->proxy = d->conn->proxy;
        b2->puser = d->conn->puser;
        b2->relproxy = d->conn->relproxy;
        b2->relpuser = d->conn->relpuser;

        adbus_iface_ref(b2->interface);
        adbus_conn_proxy(c, &ProxyBind, &FreeProxyBind, d);
    } else {
        if (DoBind(d)) {
            FreeData(d);
        }
    }
}

/* ------------------------------------------------------------------------- */

static int DoAddMatch(adbusI_StateData* d)
{
    adbus_Match* m = &d->u.match;

    d->release[0]   = m->release[0];
    d->ruser[0]     = m->ruser[0];
    d->release[1]   = m->release[1];
    d->ruser[1]     = m->ruser[1];

    m->release[0]   = &ReleaseDataCallback;
    m->ruser[0]     = d;
    m->release[1]   = NULL;
    m->ruser[1]     = NULL;

    d->data = adbus_conn_addmatch(d->conn->connection, m);

    if (!d->data)
        return -1;

    dil_insert_after(StateData, &d->conn->matches, d, &d->hl);
    return 0;
}

static void ProxyMatch(void* user)
{
    adbusI_StateData* d  = (adbusI_StateData*) user;
    d->err = DoAddMatch(d);
}

static void FreeProxyMatch(void* user)
{
    adbusI_StateData* d  = (adbusI_StateData*) user;

    adbus_Match* m  = &d->u.match;
    free((char*) m->sender);
    free((char*) m->destination);
    free((char*) m->interface);
    free((char*) m->path);
    free((char*) m->member);
    free((char*) m->error);
    free(m->arguments);

    if (d->err) {
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
    LookupConnection(s, d, c);
    d->u.match = *m;
    d->err = -1;

    assert(!m->proxy && !m->relproxy);

    if (adbus_conn_shouldproxy(c)) {
        adbus_Match* m2 = &d->u.match;

        DupString(&m2->sender, &m2->senderSize);
        DupString(&m2->destination, &m2->destinationSize);
        DupString(&m2->interface, &m2->interfaceSize);
        DupString(&m2->path, &m2->pathSize);
        DupString(&m2->member, &m2->memberSize);
        DupString(&m2->error, &m2->errorSize);

        m2->proxy = d->conn->proxy;
        m2->puser = d->conn->puser;
        m2->relproxy = d->conn->relproxy;
        m2->relpuser = d->conn->relpuser;

        if (m->arguments) {
            m2->arguments = (adbus_Argument*) malloc(sizeof(adbus_Argument) * m->argumentsSize);
            memcpy(m2->arguments, m->arguments, m->argumentsSize);
        }

        adbus_conn_proxy(c, &ProxyMatch, &FreeProxyMatch, d);
    } else {
        if (DoAddMatch(d)) {
            FreeData(d);
        }
    }
}

/* ------------------------------------------------------------------------- */

static int DoAddReply(adbusI_StateData* d)
{
    adbus_Reply* r = &d->u.reply;

    d->release[0]   = r->release[0];
    d->ruser[0]     = r->ruser[0];
    d->release[1]   = r->release[1];
    d->ruser[1]     = r->ruser[1];

    r->release[0]   = &ReleaseDataCallback;
    r->ruser[0]     = d;
    r->release[1]   = NULL;
    r->ruser[1]     = NULL;

    d->data = adbus_conn_addreply(d->conn->connection, r);

    if (!d->data)
        return -1;

    dil_insert_after(StateData, &d->conn->replies, d, &d->hl);
    return 0;
}

static void ProxyReply(void* user)
{
    adbusI_StateData* d = (adbusI_StateData*) user;
    d->err = DoAddReply(d);
}

static void FreeProxyReply(void* user)
{
    adbusI_StateData* d = (adbusI_StateData*) user;

    free((char*) d->u.reply.remote);
    if (d->err) {
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
    LookupConnection(s, d, c);
    d->u.reply = *r;
    d->err = -1;

    assert(!r->proxy && !r->relproxy);

    if (adbus_conn_shouldproxy(c)) {
        adbus_Reply* r2 = &d->u.reply;

        DupString(&r2->remote, &r2->remoteSize);

        r2->proxy = d->conn->proxy;
        r2->puser = d->conn->puser;
        r2->relproxy = d->conn->relproxy;
        r2->relpuser = d->conn->relpuser;

        adbus_conn_proxy(c, &ProxyReply, &FreeProxyReply, d);
    } else {
        if (DoAddReply(d)) {
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
    return s;
}

/* ------------------------------------------------------------------------- */

/* Called on the local thread */
static void LookupConnection(adbus_State* s, adbusI_StateData* d, adbus_Connection* c)
{
    adbusI_StateConn* conn;
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
        adbus_state_reset(s);
        free(s);
    }
}

/* ------------------------------------------------------------------------- */


