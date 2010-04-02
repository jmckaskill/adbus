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

#include "qdbusclient.hxx"
#include "qdbusdebug.h"
#include <QtNetwork/qtcpsocket.h>
#include <QtNetwork/qlocalsocket.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qthread.h>
#include <QtCore/qeventloop.h>
#include <QtCore/qtimer.h>
#include <QtCore/qstringlist.h>

#ifdef _WIN32
#   include <windows.h>
#else
#   include <unistd.h>
#endif

/* ------------------------------------------------------------------------- */

const adbus_ConnVTable QDBusClient::s_VTable = {
    &QDBusClient::Free,
    &QDBusClient::SendMsg,
    &QDBusClient::Recv,
    &QDBusClient::Proxy,
    &QDBusClient::ShouldProxy,
    &QDBusClient::GetProxy,
    &QDBusClient::Block
};

QDBusClient::QDBusClient()
:   m_Connection(adbus_conn_new(&s_VTable, this)),
    m_ConnectToBus(false),
    m_Connected(false),
    m_AppHasQuit(false),
    m_Authenticated(0),
    m_Auth(NULL),
    m_IODevice(NULL)
{
    m_Buffer = adbus_buf_new();
    /* Bit of a special hack - since messages can contain open refs to the
     * connection, the client can easily be deleted in the real late stages of
     * app shutdown (when its flushing the event queue). At this point
     * deleteLater doesn't work and deleting the socket results in a deadlock.
     * So we detect this earlier and simply delay the actual delete until the
     * refs have been cleared up.
     */
    connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(appQuitting()));
}

/* ------------------------------------------------------------------------- */

QDBusClient::~QDBusClient()
{
    if (m_IODevice) {
        m_IODevice->deleteLater();
        m_IODevice = NULL;
    }
    adbus_auth_free(m_Auth);
    adbus_buf_free(m_Buffer);
    adbus_conn_free(m_Connection);
}

/* ------------------------------------------------------------------------- */

void QDBusClient::Free(void* u)
{
    QDBusClient* d = (QDBusClient*) u;
    if (d->m_AppHasQuit) {
        delete d;
    } else {
        d->deleteLater();
    }
}

/* ------------------------------------------------------------------------- */

void QDBusClient::appQuitting()
{
    setParent(NULL);
    moveToThread(NULL);
    QCoreApplication::removePostedEvents(this);
    delete m_IODevice;
    m_IODevice = NULL;
    m_AppHasQuit = true;
}

/* ------------------------------------------------------------------------- */

int QDBusClient::SendMsg(void* u, adbus_Message* m)
{
    QDBusClient* d = (QDBusClient*) u;
    if (!d->m_IODevice)
        return -1;

    return d->m_IODevice->write(m->data, m->size);
}

/* ------------------------------------------------------------------------- */

int QDBusClient::Send(void* u, const char* b, size_t sz)
{
    QDBusClient* d = (QDBusClient*) u;
    if (!d->m_IODevice)
        return -1;

    return d->m_IODevice->write(b, sz);
}

/* ------------------------------------------------------------------------- */

int QDBusClient::Recv(void* u, char* buf, size_t sz)
{
    QDBusClient* d = (QDBusClient*) u;
    if (!d->m_IODevice)
        return -1;

    return d->m_IODevice->read(buf, sz);
}

/* ------------------------------------------------------------------------- */

uint8_t QDBusClient::Rand(void* u)
{
    (void) u;
    return (uint8_t) qrand();
}

/* ------------------------------------------------------------------------- */

adbus_Bool QDBusClient::ShouldProxy(void* u)
{
    QDBusClient* c = (QDBusClient*) u;
    return QThread::currentThread() != c->thread();
}

/* ------------------------------------------------------------------------- */

struct QDBusClientThreadData
{
    ~QDBusClientThreadData();
    QHash<QDBusClient*, QDBusProxy*> proxies;
};

QDBusClientThreadData::~QDBusClientThreadData()
{
    QHash<QDBusClient*, QDBusProxy*>::iterator ii;
    for (ii = proxies.begin(); ii != proxies.end(); ++ii) {
        ii.value()->destroyOnConnectionThread();
    }
}

static QThreadStorage<QDBusClientThreadData*> s_ThreadData;

void QDBusClient::GetProxy(void* u, adbus_ProxyCallback* cb, adbus_ProxyMsgCallback* msgcb, void** data)
{
    QDBusClient* c = (QDBusClient*) u;

    Q_ASSERT(QThread::currentThread() != c->thread());

    if (cb) {
        *cb = QDBusProxy::ProxyCallback;
    }

    if (msgcb) {
        *msgcb = QDBusProxy::ProxyMsgCallback;
    }

    if (!s_ThreadData.hasLocalData()) {
        s_ThreadData.setLocalData(new QDBusClientThreadData);
    }

    QDBusClientThreadData* threadData = s_ThreadData.localData();
    QDBusProxy*& p = threadData->proxies[c];

    if (p == NULL) {
        p = new QDBusProxy(c->base());
    }

    *data = p;
}

/* ------------------------------------------------------------------------- */

// Called on the connection thread
void QDBusClient::Proxy(void* u, adbus_Callback cb, adbus_Callback release, void* cbuser)
{
    QDBusClient* s = (QDBusClient*) u;

    if (QThread::currentThread() == s->thread()) {
        QDBUS_LOG("QDBusClient %p calling %p/%p with %p directly",
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
        QDBUS_LOG("QDBusClient %p posting event to call %p/%p with %p",
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

// Called on the local thread
bool QDBusClient::event(QEvent* event)
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

    } else {
        return QObject::event(event);
    }
}

/* ------------------------------------------------------------------------- */

#define UNBLOCK_CODE 6

int QDBusClient::Block(void* u, adbus_BlockType type, void** data, int timeoutms)
{
    QDBusClient* d = (QDBusClient*) u;

    switch (type)
    {
    case ADBUS_WAIT_FOR_CONNECTED:
        {
			if (d->m_Connected)
				return 0;

            QEventLoop loop;
            *data = &loop;

            connect(d, SIGNAL(connected()), &loop, SLOT(quit()));
            connect(d, SIGNAL(disconnected()), &loop, SLOT(quit()));

            if (timeoutms > 0) {
                QTimer::singleShot(timeoutms, &loop, SLOT(quit()));
            }

            if (loop.exec() == UNBLOCK_CODE)
                return 0;

            if (!d->m_Connected)
                return -1; /* timeout or error */

            return 0;
        }

    case ADBUS_BLOCK:
        {
            QEventLoop loop;
            *data = &loop;

            connect(d, SIGNAL(disconnected()), &loop, SLOT(quit()));
            if (timeoutms > 0) {
                QTimer::singleShot(timeoutms, &loop, SLOT(quit()));
            }

            if (loop.exec() != UNBLOCK_CODE)
                return -1; // timeout or error

            return 0;
        }

    case ADBUS_UNBLOCK:
        {
            QEventLoop* loop = (QEventLoop*) *data;
            if (!loop)
                return -1;

            Q_ASSERT(loop->thread() == QThread::currentThread());
            loop->exit(UNBLOCK_CODE);
            *data = NULL;
            return 0;
        }

    default:
        return -1;
    }
}

/* ------------------------------------------------------------------------- */

void QDBusClient::ConnectedToBus(void* u)
{
    QDBusClient* d = (QDBusClient*) u;
    d->m_UniqueName = QString::fromUtf8(adbus_conn_uniquename(d->m_Connection, NULL));
    d->m_Connected = true;
    emit d->connected();
}

/* ------------------------------------------------------------------------- */

bool QDBusClient::waitForConnected()
{
    void* block = NULL;
    return adbus_conn_block(m_Connection, ADBUS_WAIT_FOR_CONNECTED, &block, -1) == 0;
}

/* ------------------------------------------------------------------------- */

bool QDBusClient::connectToServer(adbus_BusType type, bool connectToBus /* = true */)
{
    char buf[255];
    if (adbus_connect_address(type, buf, sizeof(buf))) {
		return false;
    }

    return connectToServer(buf, connectToBus);
}

/* ------------------------------------------------------------------------- */

bool QDBusClient::connectToServer(const char* envstr, bool connectToBus)
{
    disconnect();
	m_ConnectToBus = connectToBus;

    QString s = QString::fromAscii(envstr);
    QStringList l1 = s.split(':');
    if (l1.size() != 2)
        return false;
    QString type = l1[0];
    QMap<QString, QString> fields;
    QStringList l2 = l1[1].split(',');
    for (int i = 0; i < l2.size(); i++) {
        QStringList l3 = l2[i].split('=');
        if (l3.size() != 2)
            return false;
        fields[l3[0]] = l3[1];
    }

    if (type == "tcp" && fields.contains("host") && fields.contains("port")) {
        QTcpSocket* s = new QTcpSocket;
        s->connectToHost(fields["host"], fields["port"].toUInt());
        connect(s, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(disconnect()));
        connect(s, SIGNAL(disconnected()), this, SLOT(disconnect()));
        connect(s, SIGNAL(readyRead()), this, SLOT(socketReadyRead()));
        connect(s, SIGNAL(connected()), this, SLOT(socketConnected()));
        m_IODevice = s;
        return true;

#ifdef Q_OS_UNIX
    } else if (type == "unix") {
        // Use a socket opened by adbus_sock* so we can handle abstract
        // sockets. adbus_sock_connect is normally blocking (hence why we
        // don't use it for tcp), but unix sockets don't block on connect.

        adbus_Socket sock = adbus_sock_connect_s(envstr, -1);
        if (sock == ADBUS_SOCK_INVALID)
            return false;

        QLocalSocket* s = new QLocalSocket;
        s->setSocketDescriptor(sock, QLocalSocket::ConnectedState);
        connect(s, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(disconnect()));
        connect(s, SIGNAL(disconnected()), this, SLOT(disconnect()));
        connect(s, SIGNAL(readyRead()), this, SLOT(socketReadyRead()));
        m_IODevice = s;
        socketConnected();
        return true;
#endif

    } else {
        return false;
    }
}

/* ------------------------------------------------------------------------- */

void QDBusClient::disconnect()
{
    if (m_IODevice) {
        m_IODevice->deleteLater();
        m_IODevice = NULL;
    }
    adbus_buf_reset(m_Buffer);
    adbus_auth_free(m_Auth);
    m_Auth = NULL;
    m_Connected = false;
    m_Authenticated = 0;
    emit disconnected();
}

/* ------------------------------------------------------------------------- */

void QDBusClient::socketConnected()
{
    m_IODevice->write("\0", 1);
    adbus_auth_free(m_Auth);
    m_Auth = adbus_cauth_new(&Send, &Rand, this);
    adbus_cauth_external(m_Auth);
    adbus_cauth_start(m_Auth);
    m_Authenticated = 0;
}

/* ------------------------------------------------------------------------- */

void QDBusClient::socketReadyRead()
{
    if (!m_Authenticated) {
        QByteArray data = m_IODevice->readAll();

        int used = adbus_auth_parse(m_Auth, data.constData(), data.size(), &m_Authenticated);
        if (used < 0) {
            return disconnect();
        }

        if (m_Authenticated) {
            if (adbus_conn_parse(m_Connection, data.constData() + used, data.size() - used)) {
                return disconnect();
            }
            if (m_ConnectToBus) {
                adbus_conn_connect(m_Connection, &ConnectedToBus, this);
            } else {
                m_Connected = true;
                emit connected();
            }
        }
    } else {
        if (adbus_conn_parsecb(m_Connection)) {
            return disconnect();
        }
    }
}





