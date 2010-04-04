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

#include "qdbusproxy.hxx"
#include "qdbusdebug.h"

#include <QtCore/qthread.h>
#include <QtCore/qcoreapplication.h>

/* ------------------------------------------------------------------------- */

QDBusProxy::QDBusProxy(adbus_Connection* connection)
: m_Connection(connection), m_Return(adbus_msg_new())
{
    adbus_conn_ref(m_Connection);
}

QDBusProxy::~QDBusProxy()
{ 
    adbus_msg_free(m_Return); 
    adbus_conn_deref(m_Connection);
}

/* ------------------------------------------------------------------------- */

void QDBusProxy::Unregister(void* u)
{ ((QDBusProxy*) u)->unregister(); }

void QDBusProxy::Delete(void* u)
{ delete (QDBusProxy*) u; }

void QDBusProxy::destroyOnConnectionThread()
{
    // Kill all incoming events from the connection thread and stop new ones
    // from coming in. The data in those messages will still be freed since
    // the dtor is still called, which calls the supplied release callback.
    setParent(NULL);
    moveToThread(NULL);
    QCoreApplication::removePostedEvents(this);

    // Delete the object on the connection thread - this ensures that it receives all
    // of our messages up to this point safely and removing services can only be
    // done on the connection thread.
    adbus_conn_proxy(m_Connection, &Unregister, &Delete, this);
}

/* ------------------------------------------------------------------------- */

QEvent::Type QDBusProxyEvent::type = (QEvent::Type) QEvent::registerEventType();

// Called on the connection thread
void QDBusProxy::ProxyCallback(void* u, adbus_Callback cb, adbus_Callback release, void* cbuser)
{
    QDBusProxy* s = (QDBusProxy*) u;
    QThread* proxyThread = s->thread();

    if (proxyThread == NULL) {
        QDBUS_LOG("QDBusProxy %p calling %p/%p with %p at shutdown",
                s,
                cb,
                release,
                cbuser);

        if (release) {
            release(cbuser);
        }

    } else if (proxyThread == QThread::currentThread()) {
        QDBUS_LOG("QDBusProxy %p calling %p/%p with %p directly",
                s,
                cb,
                release,
                cbuser);

        if (cb) {
            cb(cbuser);
        }

        if (release) {
            release(cbuser);
        }

    } else {
        QDBUS_LOG("QDBusProxy %p posting event to call %p/%p with %p",
                s,
                cb,
                release,
                cbuser);

        QDBusProxyEvent* e = new QDBusProxyEvent;
        e->cb = cb;
        e->user = cbuser;
        e->release = release;

        QCoreApplication::postEvent(s, e);
    }
}

/* ------------------------------------------------------------------------- */

QEvent::Type QDBusProxyMsgEvent::type = (QEvent::Type)QEvent::registerEventType();

QDBusProxyMsgEvent::~QDBusProxyMsgEvent()
{
    adbus_conn_deref(connection);
    adbus_freedata(&msg);
}

// Called on the connection thread
int QDBusProxy::ProxyMsgCallback(void* user, adbus_MsgCallback cb, adbus_CbData* d)
{
    QDBusProxy* s = (QDBusProxy*) user;
    Q_ASSERT(d->connection = s->m_Connection);

    if (QThread::currentThread() == s->thread()) {
        return adbus_dispatch(cb, d);

    } else {
        QDBusProxyMsgEvent* e = new QDBusProxyMsgEvent;
        e->cb = cb;
        e->connection = d->connection;
        e->user1 = d->user1;
        e->user2 = d->user2;
        e->ret = (d->ret != NULL);


        adbus_clonedata(d->msg, &e->msg);
        adbus_conn_ref(e->connection);

        QCoreApplication::postEvent(s, e);

        // We will send the return on the other thread
        d->ret = NULL;
        return 0;
    }
}

/* ------------------------------------------------------------------------- */

// Called on the local thread
bool QDBusProxy::event(QEvent* event)
{
    if (event->type() == QDBusProxyEvent::type) {
        QDBusProxyEvent* e = (QDBusProxyEvent*) event;

        QDBUS_LOG("QDBusProxy %p received posted event for %p/%p with %p",
                this,
                e->cb,
                e->release,
                e->user);

        if (e->cb) {
            e->cb(e->user);
        }
        return true;

    } else if (event->type() == QDBusProxyMsgEvent::type) {
        QDBusProxyMsgEvent* e = (QDBusProxyMsgEvent*) event;

        adbus_CbData d = {};
        d.connection = e->connection;
        d.msg   = &e->msg;
        d.user1 = e->user1;
        d.user2 = e->user2;

        if (e->ret) {
            d.ret = m_Return;
            adbus_msg_reset(d.ret);
        }

        adbus_dispatch(e->cb, &d);
        return true;

    } else {
        return QObject::event(event);
    }
}









