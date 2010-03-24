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
#include "interface.h"
#include "message.h"

#include "dmem/vector.h"
#include "dmem/string.h"

#include <assert.h>

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


/* ------------------------------------------------------------------------- */
// Connection management
/* ------------------------------------------------------------------------- */

/** Creates a new connection.
 *  \relates adbus_Connection
 */
adbus_Connection* adbus_conn_new(adbus_ConnectionCallbacks* cb, void* user)
{
    adbus_Member* m;
    adbus_Connection* c = NEW(adbus_Connection);

    c->callbacks    = *cb;
    c->user         = user;

    c->ref          = 1;
    c->nextSerial   = 1;
    c->state        = adbus_state_new();

    c->bus = adbus_proxy_new(c->state);
    adbus_proxy_init(c->bus, c, "org.freedesktop.DBus", -1, "/org/freedesktop/DBus", -1);
    adbus_proxy_setinterface(c->bus, "org.freedesktop.DBus", -1);

    c->introspectable = adbus_iface_new("org.freedesktop.DBus.Introspectable", -1);

    m = adbus_iface_addmethod(c->introspectable, "Introspect", -1);
    adbus_mbr_setmethod(m, &adbusI_introspect, NULL);
    adbus_mbr_retsig(m, "s", -1);

    c->properties = adbus_iface_new("org.freedesktop.DBus.Properties", -1);

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

    adbus_conn_ref(c);

    return c;
}

/** Increments the connection ref count.
 *  \relates adbus_Connection
 */
void adbus_conn_ref(adbus_Connection* connection)
{ adbusI_InterlockedIncrement(&connection->ref); }

/* ------------------------------------------------------------------------- */

/** \def adbus_conn_free(c)
 *  \brief Decrements the connection ref count.
 *  \relates adbus_Connection
 *  
 *  Since the connection is ref counted, this may not actually free the
 *  connection.
 *
 *  \sa adbus_conn_new(), adbus_conn_ref(), adbus_conn_deref()
 */

/** Decrements the connection ref count.
 *  \relates adbus_Connection
 */
void adbus_conn_deref(adbus_Connection* c)
{
    if (c && adbusI_InterlockedDecrement(&c->ref) == 0) {
        size_t i;

        /* Free the connection services first */
        adbusI_freeReplies(c);
        adbusI_freeMatches(c);
        adbusI_freeObjectTree(&c->binds);

        adbusI_freeRemoteTracker(c);
        adbusI_freeConnBusData(&c->connect);

        adbus_proxy_free(c->bus);
        adbus_state_free(c->state);

        for (i = 0; i < dv_size(&c->returnMessages); i++) {
            adbus_msg_free(dv_a(&c->returnMessages, i));
        }
        dv_free(MsgFactory, &c->returnMessages);

        adbus_msg_free(c->errorMessage);
        adbus_iface_free(c->introspectable);
        adbus_iface_free(c->properties);
        adbus_buf_free(c->parseBuffer);

        if (c->callbacks.release) {
            c->callbacks.release(c->user);
        }

        free(c);
    }
}

/* ------------------------------------------------------------------------- */
// Message
/* ------------------------------------------------------------------------- */

/** Sends a message on the connection
 *  \relates adbus_Connection
 *  
 *  This function is thread safe and may be called in both callbacks and
 *  on other threads.
 *
 *  \note Do not call this is in method call callback for replies. Instead
 *  setup your response in the provided msg factory (adbus_CbData::ret).
 *
 *  \todo Actually make this thread safe.
 */
int adbus_conn_send(
        adbus_Connection* c,
        adbus_Message*    message)
{
    ADBUSI_LOG_MSG("Sending", message);

    if (!c->callbacks.send_message)
        return -1;

    if (c->callbacks.send_message(c->user, message) != (int) message->size)
        return -1;

    return 0;
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
static void SendReply(adbus_Connection* c, adbus_Message* msg, adbus_MsgFactory* ret)
{
    if (ret) {
        if (adbus_msg_type(ret) == ADBUS_MSG_INVALID) {
            adbus_msg_settype(ret, ADBUS_MSG_RETURN);
        }

        if (msg->sender) {
            adbus_msg_setdestination(ret, msg->sender, msg->senderSize);
        }

        adbus_msg_setreply(ret, msg->serial);
        adbus_msg_setflags(ret, adbus_msg_flags(ret) | ADBUS_MSG_NO_REPLY);
        adbus_msg_send(ret, c);
    }
}

static int Dispatchstep(
        adbus_Connection* c,
        adbus_Message*    msg)
{
    adbus_CbData d;
    adbus_ConnMatch* nextmatch = dil_getiter(&c->matches.list);
    adbus_Bool called = 0;
    int err = 0;

    c->depth++;
    if (c->depth >= dv_size(&c->returnMessages)) {
        adbus_MsgFactory** pmsg = dv_push(MsgFactory, &c->returnMessages, 1);
        *pmsg = adbus_msg_new();
    }

    ZERO(d);
    d.connection    = c;
    d.msg           = msg;

    if (nextmatch == NULL) {
        ADBUSI_LOG_MSG("Received", msg);

        switch (msg->type) 
        {
        case ADBUS_MSG_METHOD:
            {
                adbus_ConnBind* bind;
                adbus_Member* mbr = adbusI_getMethod(c, &d, &bind);
                if (mbr) {
                    called = 1;

                    if ((msg->flags & ADBUS_MSG_NO_REPLY) == 0) {
                        d.ret = dv_a(&c->returnMessages, c->depth);
                        adbus_msg_reset(d.ret);
                    }

                    err = adbus_mbr_call(mbr, bind, &d);

                    if (!err) {
                        SendReply(c, msg, d.ret);
                    }
                }
            }
            break;
        case ADBUS_MSG_RETURN:
            {
                adbus_ConnReply* reply = adbusI_getReply(c, &d);
                if (reply) {
                    called = 1;
                    d.user1 = reply->cuser;

                    if (reply->proxy) {
                        err = reply->proxy(reply->puser, reply->callback, &d);
                    } else {
                        err = adbus_dispatch(reply->callback, &d);
                    }
                }
                adbusI_finishReply(reply);
            }
            break;
        case ADBUS_MSG_ERROR:
            {
                adbus_ConnReply* reply = adbusI_getReply(c, &d);
                if (reply) {
                    called = 1;
                    d.user1 = reply->euser;

                    if (reply->proxy) {
                        err = reply->proxy(reply->puser, reply->error, &d);
                    } else {
                        err = adbus_dispatch(reply->error, &d);
                    }
                }
                adbusI_finishReply(reply);
            }
            break;
        default:
            break;
        }
    }

    d.ret = NULL;

    if (err) {
        return -1;
    }

    if (called) {
        /* Setup the match iter to begin with the match list on the next step */
        dil_setiter(&c->matches.list, c->matches.list.next);
        return 0;
    }

    /* Dispatch matches */
    {
        adbus_ConnMatch* match = adbusI_getNextMatch(c, &d);

        if (match) {
            d.user1 = match->m.cuser;

            if (match->m.proxy) {
                err = match->m.proxy(match->m.puser, match->m.callback, &d);
            } else {
                err = adbus_dispatch(match->m.callback, &d);
            }

            if (err) {
                return -1;
            }

            return 0;
        }
    }

    /* We've finished processing this message */
    return 1;
}

/* -------------------------------------------------------------------------- */

/** Dispatch a pre-parsed message
 *  \relates adbus_Connection
 */
int adbus_conn_dispatch(
        adbus_Connection* c,
        adbus_Message*    message)
{
    int ret = 0;

    while (ret == 0)
        ret = Dispatchstep(c, message);

    if (ret < 0)
        return -1;

    return 0;
}

/* -------------------------------------------------------------------------- */

/** Consume messages from the supplied buffer.
 *  \relates adbus_Connection
 *
 *  This will remove all complete messages from the beginning of the buffer,
 *  but it will leave incomplete messages in the buffer. These should be
 *  appended to once more data comes in and then recall this function.
 */
int adbus_conn_parse(
        adbus_Connection*   c,
        adbus_Buffer*      buf)

{
    char* data = adbus_buf_data(buf);
    size_t size = adbus_buf_size(buf);

    adbus_Message m;

    while (1) {
        size_t msgsize = adbus_parse_size(data, size);
        if (msgsize == 0 || msgsize > size)
            break;

        if (ADBUS_ALIGN(data, 8) == (uintptr_t) data) {
            if (adbus_parse(&m, data, msgsize))
                return -1;
            if (adbus_conn_dispatch(c, &m))
                return -1;

        } else {
            adbus_buf_reset(c->parseBuffer);
            adbus_buf_append(c->parseBuffer, data, msgsize);

            if (adbus_parse(&m, adbus_buf_data(c->parseBuffer), msgsize))
                return -1;
            if (adbus_conn_dispatch(c, &m))
                return -1;
        }

        data += msgsize;
        size -= msgsize;
    }

    adbus_buf_remove(buf, 0, adbus_buf_size(buf) - size);
    return 0;
}

/* -------------------------------------------------------------------------- */

/** See if calling code should use adbus_conn_proxy()
 *  \relates adbus_Connection
 *  \sa adbus_ConnectionCallbacks::should_proxy
 */
adbus_Bool adbus_conn_shouldproxy(adbus_Connection* c)
{ return c->callbacks.should_proxy && c->callbacks.should_proxy(c->user) && c->callbacks.proxy; }

/* -------------------------------------------------------------------------- */

/** Proxy a call over to the connection thread or call immediately if already
 * on it.
 *  \relates adbus_Connection
 *
 * \sa adbus_conn_shouldproxy(), adbus_ConnectionCallbacks::proxy
 */
void adbus_conn_proxy(adbus_Connection* c, adbus_Callback callback, void* user)
{ 
    if (c->callbacks.proxy) {
        c->callbacks.proxy(c->user, callback, user); 
    } else {
        callback(user);
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
        adbus_ProxyCallback*    cb,
        adbus_ProxyMsgCallback* msgcb,
        void**                  user)
{
    if (c->callbacks.get_proxy)
    {
        c->callbacks.get_proxy(c->user, cb, msgcb, user);
    }
    else
    {
        if (cb)
            *cb = NULL;
        if (msgcb)
            *msgcb = NULL;
        if (user)
            *user = NULL;
    }
}
