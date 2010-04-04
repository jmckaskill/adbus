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
#include "interface.h"
#include "message.h"
#include "state.h"
#include "proxy.h"

#include <inttypes.h>
#include <assert.h>
#include <stdio.h>

/** \struct adbus_ConnectionCallbacks
 *  \brief Structure to hold callbacks registered with adbus_conn_new(). 
 */

/** \var adbus_ConnectionCallbacks::release
    \brief Called when the connection is destroyed.

    Since the connection is internally ref counted via adbus_conn_ref() and
    adbus_conn_deref() it can occur after the call to adbus_conn_free().
 */

/** \var adbus_ConnectionCallbacks::send_message
 *  \brief This is a callback to send messages.
    The return value should be the number of bytes written or -1 on error (ie
    the return value of recv). The connection considers any value except the
    full message length to be an error.

    For example: 
    \code
    int SendMsg(void* user, adbus_Message* m)
    { return send(*(adbus_Socket*) user, m->data, m->size, 0);
    \endcode
 */

/** \var adbus_ConnectionCallbacks::proxy 
 *
 *
    Callback to send messages across to the connection thread.
    - user - User data associated with this callback.
    - cb - Function to call on the target thread.
    - release - Function to free the data in the cbuser argument.
    - cbuser - Data associated with the cb and release arguments.

    For example: 
 *  \code
    class ProxyEvent
    {
    public:
        ProxyEvent(adbus_Callback cb, adbus_Callback release, void* user)
        : m_Cb(cb), m_Release(release), m_User(user) {}
 
        ~ProxyEvent()
        { if (m_Release) m_Release(m_User); }
 
        void call()
        { if (m_Cb) m_Cb(m_User); }
 
        adbus_Callback  m_Cb;
        adbus_Callback  m_Release;
        void*           m_User;
    };
  
    void Proxy(void* user, adbus_Callback cb, adbus_Callback release, void* cbuser)
    {
        if (OnConnectionThread())
        {
            ProxyEvent e(cb, release, user);
            e.call();
        }
        else
        {
           ProxyEvent* e = new ProxyEvent(cb, release, user);
           PostEvent(e);
        }
    }
  
    // On the connection thread
    void ProcessEvent(ProxyEvent* e)
    {
        e->call();
        delete e;
    }
    \endcode
 */

/** \struct adbus_Connection
 *
    \brief Client message dispatcher

    The connection API can be split into four sections:

    -# Getting data in/out.
    -# Proxying messages from the connection to and from other threads.
    -# Registering application callbacks via:
        -# Binds: registers an interface on a path that can be introspected,
        have methods called on, properties set, etc.
        -# Replies: registers a callback listening for method return or error
        message.
        -# Matches: generic message matches that can be pushed through to the
        bus server. This is mostly used for matching signals.
    -# Connecting to the bus server.

    \note adbus_Connection does not handle the initial authentication with the
    bus server. This can be done using either the adbus_Auth module or a third
    party SASL library.

    \warning Unless otherwise specificed all connection functions must be
    called on the connection thread and may not be called in a callback.

 *  \sa adbus_Socket, adbus_Auth, adbus_Bind, adbus_Reply, adbus_Match

    \section in Getting Data In

    The connection itself has no idea how to get incoming data or messages.
    Instead the owner of the connection figures out how to get incoming data
    off of a socket and feeds it into the connection via adbus_conn_dispatch()
    or adbus_conn_parse().

    The simplest way of handing data off to the connection is to append data
    to an adbus_Buffer and then having the connection consume complete
    messages in the buffer via adbus_conn_parse(). For example:

    \code
    #define RECV_SIZE 64 * 1024
    int ReadSignalled()
    {
        // Read all the data available
        int read = RECV_SIZE;
        while (read == RECV_SIZE)
        {
            // adbus_buf_recvbuf() gives us a place at the end of the buffer
            // to hand off to recv where it can put the data
            uint8_t* buf = adbus_buf_recvbuf(my_buffer, RECV_SIZE);
            read = recv(my_socket, RECV_SIZE, 0);
            adbus_buf_recvd(my_buffer, RECV_SIZE, read);
        }
  
        // Hand the data off to the connection. If adbus_conn_parse returns 
        // an error, we should disconnect the socket.
        if (adbus_conn_parse(my_connection, my_buffer))
            return -2;
  
        // Propagate recv errors up
        if (read < 0)
            return -1;
  
        return 0;
    }
    \endcode

 *  \sa adbus_Socket, adbus_Buffer, adbus_conn_dispatch(), adbus_conn_parse()
 *
    \section out Getting Data Out

    Outgoing data is sent via adbus_ConnectionCallbacks::send_message set with
    adbus_conn_new(). This is required to be always set.

    For example:
    \code
    static int SendMessage(void* user, adbus_Message* msg)
    {
        adbus_Socket* s = (adbus_Socket*) user;
        return send(*s, msg->data, msg->size, 0);
    }

    adbus_Connection* CreateConnection(adbus_Socket sock)
    {
        adbus_ConnectionCallbacks cbs = {};
        cbs.send_message = &SendMessage;
        return adbus_conn_new(&cbs, (void*) &sock);
    }
    \endcode

    \sa adbus_ConnectionCallbacks::send_message, adbus_conn_new()

    \section proxy Proxying Messages To/From the Connection Thread

    In multithreaded applications, the connection parsing and dispatch will be
    on a given thread, but we often want to register objects and callbacks for
    all of the application's thread on the one thread. The way we get around this
    is to proxy all messages and requests for registrations to and from the connection
    thread.

    This proxying of messages to and fro is done via the proxy callbacks setup
    in adbus_conn_new(). See proxy, should_proxy, get_proxy, and block in
    adbus_ConnectionCallbacks.

    \sa adbus_ConnectionCallbacks


    \section registrations Binds, Matches, and Replies

    See adbus_Bind, adbus_Match, and adbus_Reply.

    \section bus Bus Server

    By default the connection will not connect to the bus server (specifically
    it does not send the Hello message), until adbus_conn_connect() is called.
    This is especially useful for multithreaded applications that want to
    create a connection, register all of their objects on the various app
    threads, and then finally connect to the bus. This then avoids race
    conditions when other applications try to call method for not yet
    registered objects.

    Once the server responds to the hello message adbus_conn_isconnected()
    will return true and adbus_conn_uniquename() will return the assigned
    name.

    \sa adbus_conn_connect(), adbus_conn_isconnected(), adbus_conn_uniquename()
 */

/** \struct adbus_Match
 *
 */


/* -------------------------------------------------------------------------
 * Connection management
 * -------------------------------------------------------------------------
 */

static adbusI_ConnMsg* GetNewMessage(adbus_Connection* c);

/** Creates a new connection.
 *  \relates adbus_Connection
 */
adbus_Connection* adbus_conn_new(const adbus_ConnVTable* vtable, void* obj)
{
    adbus_Member* m;
    adbus_Connection* c = NEW(adbus_Connection);

    adbusI_initlog();

    ADBUSI_LOG_1("new (connection %p)", (void*) c);

    c->vtable       = *vtable;
    c->obj          = obj;

    c->ref          = 0;
    c->nextSerial   = 1;
    c->thread       = adbusI_current_thread();

    c->state = adbus_state_new();
    c->state->refConnection = 0;

    c->parseMsg     = GetNewMessage(c);

    c->dispatchReturn = adbus_msg_new();

    c->bus = adbus_proxy_new(c->state);
    c->bus->refConnection = 0;

    adbus_proxy_init(c->bus, c, "org.freedesktop.DBus", -1, "/org/freedesktop/DBus", -1);
    adbus_proxy_setinterface(c->bus, "org.freedesktop.DBus", -1);

    c->introspectable = adbus_iface_new("org.freedesktop.DBus.Introspectable", -1);
    adbus_iface_ref(c->introspectable);

    m = adbus_iface_addmethod(c->introspectable, "Introspect", -1);
    adbus_mbr_setmethod(m, &adbusI_introspect, NULL);
    adbus_mbr_retsig(m, "s", -1);

    c->properties = adbus_iface_new("org.freedesktop.DBus.Properties", -1);
    adbus_iface_ref(c->properties);

    m = adbus_iface_addmethod(c->properties, "Get", -1);
    adbus_mbr_setmethod(m, &adbusI_getProperty, NULL);
    adbus_mbr_argsig(m, "ss", -1);
    adbus_mbr_argname(m, "interface_name", -1);
    adbus_mbr_argname(m, "property_name", -1);
    adbus_mbr_retsig(m, "v", -1);

    m = adbus_iface_addmethod(c->properties, "GetAll", -1);
    adbus_mbr_setmethod(m, &adbusI_getAllProperties, NULL);
    adbus_mbr_argsig(m, "s", -1);
    adbus_mbr_argname(m, "interface_name", -1);
    adbus_mbr_retsig(m, "a{sv}", -1);

    m = adbus_iface_addmethod(c->properties, "Set", -1);
    adbus_mbr_setmethod(m, &adbusI_setProperty, NULL);
    adbus_mbr_argsig(m, "ssv", -1);
    adbus_mbr_argname(m, "interface_name", -1);
    adbus_mbr_argname(m, "property_name", -1);
    adbus_mbr_argname(m, "value", -1);

    assert(c->ref == 0);

    return c;
}

adbusI_thread_t adbusI_conn_thread(adbus_Connection* c)
{ return c->thread; }

/** Increments the connection ref count.
 *  \relates adbus_Connection
 */
void adbus_conn_ref(adbus_Connection* c)
{ 
    long ref = adbusI_InterlockedIncrement(&c->ref); 

    ADBUSI_LOG_1("ref: %d (connection %s, %p)",
			(int) ref,
			adbus_conn_uniquename(c, NULL),
			(void*) c);
}

static void FreeMessage(adbusI_ConnMsg* m);

void adbus_conn_close(adbus_Connection* c)
{
    adbusI_ConnMsg *m, *next;

    ADBUSI_ASSERT_CONN_THREAD(c);

    ADBUSI_LOG_1("free (connection %s, %p)",
            adbus_conn_uniquename(c, NULL),
            (void*) c);

    assert(!c->closed);
    adbusI_InterlockedExchange(&c->closed, 1);

    /* Free the connection services first */
    adbusI_freeReplies(c);
    adbusI_freeMatches(c);
    adbusI_freeObjectTree(&c->binds);

    adbusI_freeRemoteTracker(c);

    adbus_proxy_free(c->bus);
    adbus_state_free(c->state);

    for (m = c->processBegin; m != NULL; m = next) {
        next = m->next;
        FreeMessage(m);
    }

    m = c->freelist;
    for (m = c->freelist; m != NULL; m = next) {
        next = m->next;
        FreeMessage(m);
    }

    FreeMessage(c->parseMsg);

    adbus_msg_free(c->dispatchReturn);

    assert(c->introspectable->ref == 1);
    assert(c->properties->ref == 1);
    adbus_iface_deref(c->introspectable);
    adbus_iface_deref(c->properties);
}

static void FreeConnection(void* u)
{
    adbus_Connection* c = (adbus_Connection*) u;

    if (c->vtable.release) {
        c->vtable.release(c->obj);
    }

    if (!c->closed) {
        adbus_conn_close(c);
    }

    adbusI_freeConnBusData(&c->connect);
    free(c);
}

/** Decrements the connection ref count.
 *  \relates adbus_Connection
 */
void adbus_conn_deref(adbus_Connection* c)
{
    long ref = adbusI_InterlockedDecrement(&c->ref);

    ADBUSI_LOG_1("deref: %d (connection %s, %p)",
			(int) ref,
			adbus_conn_uniquename(c, NULL),
			(void*) c);

    if (ref == 0) {
        adbus_conn_proxy(c, NULL, &FreeConnection, c);
    }
}

/* ------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------
 * Message
 * -------------------------------------------------------------------------
 */

struct Message
{
    adbus_Connection*   c;
    adbus_Message       m;
};

static void ProxyMessage(void* d)
{
    struct Message* msg = (struct Message*) d;
    adbus_Message* m = &msg->m;
    adbus_Connection* c = msg->c;

    ADBUSI_LOG_DATA_3(
			m->data,
			m->size,
			"sending data (connection %s, %p)",
			adbus_conn_uniquename(c, NULL),
			(void*) c);

    c->vtable.send_message(c->obj, m);
    assert(!adbus_conn_shouldproxy(c));
}

static void FreeProxiedMessage(void* d)
{
    struct Message* m = (struct Message*) d;
    adbus_freedata(&m->m);
    free(m);
}

/** Sends a message on the connection
 *  \relates adbus_Connection
 *  
 *  This function is thread safe and may be called in both callbacks and
 *  on other threads.
 *
 *  \note Do not call this is in method call callback for replies. Instead
 *  setup your response in the provided msg factory (adbus_CbData::ret).
 */
int adbus_conn_send(
        adbus_Connection* c,
        adbus_Message*    m)
{
    ADBUSI_LOG_MSG_2(
			m,
			"sending (connection %s, %p)",
			adbus_conn_uniquename(c, NULL),
			(void*) c);

    if (adbus_conn_shouldproxy(c)) {
        struct Message* msg = NEW(struct Message);
        msg->c = c;
        adbus_clonedata(m, &msg->m);
        adbus_conn_proxy(c, &ProxyMessage, &FreeProxiedMessage, msg); 
        return 0;

    } else {
        ADBUSI_LOG_DATA_3(
                m->data,
                m->size,
                "sending data (connection %s, %p)",
                adbus_conn_uniquename(c, NULL),
                (void*) c);

        return c->vtable.send_message(c->obj, m) != (int) m->size;
    }
}

/* ------------------------------------------------------------------------- */

/** Gets a serial that can be used for sending messages.
 *  \relates adbus_Connection
 *  
 *  \note This function is thread safe and may be called in both callbacks and
 *  on other threads.
 *
 */
uint32_t adbus_conn_serial(adbus_Connection* c)
{ return (uint32_t) adbusI_InterlockedIncrement(&c->nextSerial); }





/* --------------------------------------------------------------------------
 * Parsing and dispatch
 * --------------------------------------------------------------------------
 */

static adbusI_ConnMsg* GetNewMessage(adbus_Connection* c)
{
    if (c->freelist) {
        adbusI_ConnMsg* m = c->freelist;
        adbus_buf_reset(m->buf);
        m->ref = 0;
        m->matchStarted = 0;
        m->matchFinished = 0;
        m->msgFinished = 0;

        /* Remove it from the free list */
        c->freelist = m->next;
        m->next = NULL;
        return m;
    } else {
        adbusI_ConnMsg* m = NEW(adbusI_ConnMsg);
        m->ret = adbus_msg_new();
        m->buf = adbus_buf_new();
        return m;
    }
}

static void AppendMessageToProcess(adbus_Connection* c, adbusI_ConnMsg* m)
{
    assert(m->next == NULL && m->prev == NULL);
    
    /* Append the msg to the end of the to process list */
    if (c->processEnd) {
        assert(c->processEnd->next == NULL);
        m->prev = c->processEnd;
        c->processEnd->next = m;
        c->processEnd = m;
    } else {
        c->processBegin = c->processEnd = m;
    }

    /* If processCurrent has run off the end of the list set it to our new
     * message. Note processCurrent can be NULL even with items in the list
     * since we keep the items in the list around until the outer call frames
     * exit.
     */
    if (!c->processCurrent) {
        c->processCurrent = m;
    }
}

static void FreeMessage(adbusI_ConnMsg* m)
{
    adbus_freeargs(&m->msg);
    adbus_buf_free(m->buf);
    adbus_msg_free(m->ret);
    free(m);
}

static void FinishMessage(adbus_Connection* c, adbusI_ConnMsg* m)
{ 
    /* Remove message from the to process list */
    assert(m->next || m == c->processEnd);
    assert(m->prev || m == c->processBegin);
    if (m->next) {
        m->next->prev = m->prev;
    }
    if (m->prev) {
        m->prev->next = m->next;
    }
    if (m == c->processBegin) {
        c->processBegin = m->next;
    }
    if (m == c->processEnd) {
        c->processEnd = m->prev;
    }

    /* Add it to the head of the free list */
    m->next = c->freelist;
    m->prev = NULL;
    c->freelist = m;

    /* Free data in the msg we don't want to keep */
    adbus_freeargs(&m->msg);
    if (adbus_buf_reserved(m->buf) > 1024) {
        free(adbus_buf_release(m->buf));
    }
}

/* -------------------------------------------------------------------------- */

int adbus_conn_dispatch(adbus_Connection* c, adbus_Message* m)
{
    adbus_CbData d;
    adbus_ConnMatch* match;

    /* adbus_conn_dispatch doesn't support reentrancy and can't be used in
     * concurrence with adbus_conn_parse or adbus_conn_parsecb
     */
    assert(!c->closed);
    assert(!c->processBegin && !c->processEnd && !c->processCurrent);
    assert(!c->freelist);
    assert(adbus_buf_size(c->parseMsg->buf) == 0);

    ADBUSI_LOG_MSG_2(
			m,
			"received (connection %s, %p)",
			adbus_conn_uniquename(c, NULL),
			(void*) c);

    ZERO(d);
    d.connection = c;
    d.msg = m;

    DIL_FOREACH (ConnMatch, match, &c->matches.list, hl) {
        if (adbusI_dispatchMatch(match, &d)) {
            return -1;
        }
    }

    if (d.msg->type == ADBUS_MSG_ERROR || d.msg->type == ADBUS_MSG_RETURN) {

        if (adbusI_dispatchReply(c, &d)) {
            return -1;
        }

    } else if (d.msg->type == ADBUS_MSG_METHOD) {

        if ((d.msg->flags & ADBUS_MSG_NO_REPLY) == 0) {
            d.ret = c->dispatchReturn;
            adbus_msg_reset(d.ret);
        }

        if (adbusI_dispatchMethod(c, &d)) {
            return -1;
        }
    }

    return 0;
}

/* -------------------------------------------------------------------------- */

/* 
 * \return -1 on parse error
 * \return 0 on more messages to dispatch
 * \return 1 on no more messages to dispatch
 */
int adbus_conn_continue(adbus_Connection* c)
{
    adbus_CbData d;
    adbusI_ConnMsg* msg = c->processCurrent;

    if (!msg)
        return 1;

    ZERO(d);
    d.connection = c;
    d.msg = &msg->msg;

    msg->ref++;

    while (!msg->matchFinished) {

        adbus_ConnMatch* match = msg->matchStarted 
                               ? dil_getiter(&c->matches.list) 
                               : c->matches.list.next;

        msg->matchStarted = 1;

        if (!match)
            break;

        /* Set the match iter to the next match. If the next match is removed,
         * remove match will update the iter value to the next match after
         * that and so forth. Also if in the callback we call back into
         * adbus_conn_continue the inner call will update the stored iter.
         * Thus after the inner call completes we can continue on from where
         * it got up to if it did not finishing processing the matches (ie
         * matchFinished is not set).
         */
        dil_setiter(&c->matches.list, match->hl.next);
        if (adbusI_dispatchMatch(match, &d))
            return -1;
    }

    msg->matchFinished = 1;
    
    if (!msg->msgFinished) {
        if (d.msg->type == ADBUS_MSG_ERROR || d.msg->type == ADBUS_MSG_RETURN) {

            if (adbusI_dispatchReply(c, &d)) {
                return -1;
            }

        } else if (d.msg->type == ADBUS_MSG_METHOD) {

            if ((d.msg->flags & ADBUS_MSG_NO_REPLY) == 0) {
                d.ret = msg->ret;
                adbus_msg_reset(d.ret);
            }

            if (adbusI_dispatchMethod(c, &d)) {
                return -1;
            }
        }

        msg->msgFinished = 1;
    }

    /* We have finished processing this message - kick processCurrent along so
     * the next call starts processing the next message.
     */
    if (msg == c->processCurrent) {
        c->processCurrent = msg->next;
    }

    /* Don't actually remove the message from the process list until all call
     * frames that reference the message have completed.
     */
    if (--msg->ref == 0) {
        FinishMessage(c, msg);
    }

    return 0;
}

/* -------------------------------------------------------------------------- */

/* Returns 1 on success, 0 on no new messages, -1 on a parse error */
static int SplitParseMessage(adbus_Connection* c)
{
    adbus_Buffer* origbuf = c->parseMsg->buf;
    char* data = adbus_buf_data(origbuf);
    size_t size = adbus_buf_size(origbuf);
    int firstsize = adbus_parse_size(data, size);

    if (firstsize < 0)
        return -1;

    /* No complete messages */
    if (firstsize == 0 || firstsize > size)
        return 0;

    /* The first message stays in parseMsg. The excess data in parseMsg will
     * be removed after copying it across into new messages.
     */
    AppendMessageToProcess(c, c->parseMsg);

    data += firstsize;
    size -= firstsize;

    while (1) {
        adbusI_ConnMsg* next;

        /* Find the next message size */
        int msgsize = adbus_parse_size(data, size);
        if (msgsize < 0)
            return -1;
        
        /* No more complete messages */
        if (msgsize == 0 || msgsize > size)
            break;

        /* Append the next message to be parsed */
        next = GetNewMessage(c);
        adbus_buf_append(next->buf, data, msgsize);
        AppendMessageToProcess(c, next);

        data += msgsize;
        size -= msgsize;
    }

    /* Copy over the remaining data into a new parseMsg */
    c->parseMsg = GetNewMessage(c);
    adbus_buf_append(c->parseMsg->buf, data, size);

    /* Remove the extra data from the original buffer */
    adbus_buf_remove(origbuf, firstsize, adbus_buf_size(origbuf) - firstsize);

    return 1;
}

/* -------------------------------------------------------------------------- */

/* Returns 0 on success, -1 on a parse error */
static int ParseNewMessages(adbus_Connection* c, adbusI_ConnMsg* begin)
{
    /* Parse from begin until the end of the list */
    for (; begin != NULL; begin = begin->next) {
        char* data = adbus_buf_data(begin->buf);
        size_t size = adbus_buf_size(begin->buf);
        if (adbus_parse(&begin->msg, data, size)) {
            return -1;
        }

        ADBUSI_LOG_MSG_2(
				&begin->msg,
				"received (connection %s, %p)",
				adbus_conn_uniquename(c, NULL),
				(void*) c);

    }

    return 0;
}

/* -------------------------------------------------------------------------- */

int adbus_conn_parse(adbus_Connection* c, const char* data, size_t size)
{
    int ret;
    adbusI_ConnMsg* m = c->parseMsg;

    assert(!c->closed);
    adbus_buf_append(m->buf, data, size);

    ADBUSI_LOG_DATA_3(
            data,
            size,
            "received data (connection %s, %p)",
            adbus_conn_uniquename(c, NULL),
            (void*) c);

    ret = SplitParseMessage(c);
    if (ret < 0) {
        return -1;
    } else if (ret == 0) {
        /* No new messages */
        return 0;
    }

    if (ParseNewMessages(c, m))
        return -1;

    return 0;
}

#define RECV_SIZE (64*1024)
int adbus_conn_parsecb(adbus_Connection* c)
{
    int ret;
    int read = RECV_SIZE;
    adbusI_ConnMsg* m = c->parseMsg;

    assert(!c->closed);

    /* Append our new data */
    while (read == RECV_SIZE) {
        char* dest = adbus_buf_recvbuf(m->buf, RECV_SIZE);
        read = c->vtable.recv_data(c->obj, dest, RECV_SIZE);

        ADBUSI_LOG_DATA_3(
				dest,
				read,
				"received data (connection %s, %p)",
				adbus_conn_uniquename(c, NULL),
				(void*) c);

        adbus_buf_recvd(m->buf, RECV_SIZE, read);
    }

    ret = SplitParseMessage(c);
    if (ret < 0) {
        return -1;
    } else if (ret == 0) {
        /* No new messages */
        return 0;
    }

    if (ParseNewMessages(c, m))
        return -1;

    if (read < 0)
        return -1;

    return 0;
}

/* -------------------------------------------------------------------------- */

/** See if calling code should use adbus_conn_proxy()
 *  \relates adbus_Connection
 *  \sa adbus_ConnectionCallbacks::should_proxy
 */
adbus_Bool adbus_conn_shouldproxy(adbus_Connection* c)
{ return c->thread != adbusI_current_thread(); }

/* -------------------------------------------------------------------------- */

/** Proxy a call over to the connection thread or call immediately if already
 * on it.
 *  \relates adbus_Connection
 *
 * \sa adbus_conn_shouldproxy(), adbus_ConnectionCallbacks::proxy
 */
void adbus_conn_proxy(adbus_Connection* c, adbus_Callback callback, adbus_Callback release, void* user)
{ 
    if (c->vtable.proxy) {
        c->vtable.proxy(c->obj, callback, release, user); 
    } else {
        if (callback && !c->closed) {
            callback(user);
        }
        if (release) {
            release(user);
        }
    }
}

/* -------------------------------------------------------------------------- */

/** Get proxy functions for the current thread.
 *  \relates adbus_Connection
 *
 *  The arguments may be null if that value is not needed.
 *
 *  \sa adbus_ConnectionCallbacks::get_proxy
 */
void adbus_conn_getproxy(
        adbus_Connection*       c,
        adbus_ProxyMsgCallback* msgcb,
        void**                  msguser,
        adbus_ProxyCallback*    cb,
        void**                  cbuser)
{
    if (c->vtable.get_proxy && adbus_conn_shouldproxy(c))
    {
        c->vtable.get_proxy(c->obj, msgcb, msguser, cb, cbuser);
    }
    else
    {
        if (msgcb)
            *msgcb = NULL;
        if (msguser)
            *msguser = NULL;
        if (cb)
            *cb = NULL;
        if (cbuser)
            *cbuser = NULL;
    }
}

int adbus_conn_block(adbus_Connection* c, adbus_BlockType type, uintptr_t* handle, int timeoutms)
{
    if (type == ADBUS_BLOCK) {
        ADBUSI_LOG_2("block %p %#"PRIxPTR" (connection %s, %p)",
                (void*) handle,
                *handle,
				adbus_conn_uniquename(c, NULL),
                (void*) c);

    } else if (type == ADBUS_UNBLOCK) {
        ADBUSI_LOG_2("unblock %p %#"PRIxPTR" (connection %s, %p)",
                (void*) handle,
                *handle,
				adbus_conn_uniquename(c, NULL),
                (void*) c);

    } else if (type == ADBUS_WAIT_FOR_CONNECTED) {
        ADBUSI_LOG_2("wait for connected %p %#"PRIxPTR" (connection %s, %p)",
                (void*) handle,
                *handle,
				adbus_conn_uniquename(c, NULL),
                (void*) c);

    }

    if (!c->vtable.block || c->vtable.block(c->obj, type, handle, timeoutms)) {
        ADBUSI_LOG_2("block failed or timed out %p %#"PRIxPTR" (connection %s, %p)",
                (void*) handle,
                *handle,
                adbus_conn_uniquename(c, NULL),
                (void*) c);

        return -1;
    }

    return 0;
}
