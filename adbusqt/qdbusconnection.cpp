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

#include "qdbusconnection_p.hxx"
#include "qdbusmessage_p.hxx"
#include "qdbusobject_p.hxx"
#include "qsharedfunctions_p.hxx"
#include "qdbusabstractadaptor_p.hxx"
#include "qdbuspendingcall_p.hxx"

#include <qdbuspendingcall.h>
#include <QHash>
#include <QMutex>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QLocalSocket>
#include <QThread>
#include <QTimer>

/* ------------------------------------------------------------------------- */

QThreadStorage<QDBusProxy*> QDBusClient::m_Proxies;

/* ------------------------------------------------------------------------- */

QDBusClient::QDBusClient(adbus_BusType type, bool connectToBus)
:   m_ConnectToBus(connectToBus),
    m_Connected(false),
    m_Auth(NULL),
    m_IODevice(NULL)
{
    adbus_ConnectionCallbacks cbs = {};
    cbs.release         = &QDBusClient::Free;
    cbs.send_message    = &QDBusClient::SendMsg;
    cbs.recv_data       = &QDBusClient::Recv;
    cbs.proxy           = &QDBusProxy::ProxyCallback;
    cbs.should_proxy    = &QDBusClient::ShouldProxy;
    cbs.get_proxy       = &QDBusClient::GetProxy;
    cbs.block           = &QDBusClient::Block;

    m_Connection = adbus_conn_new(&cbs, this);
    m_Buffer = adbus_buf_new();

    char buf[255];
    if (!adbus_connect_address(type, buf, sizeof(buf))) {
        connectToServer(buf);
    }
}

/* ------------------------------------------------------------------------- */

QDBusClient::QDBusClient(const char* envstr, bool connectToBus)
:   m_ConnectToBus(connectToBus),
    m_Connected(false),
    m_Auth(NULL),
    m_IODevice(NULL)
{
    adbus_ConnectionCallbacks cbs = {};
    cbs.release       = &QDBusClient::Free;
    cbs.send_message  = &QDBusClient::SendMsg;
    cbs.proxy         = &QDBusProxy::ProxyCallback;
    cbs.should_proxy  = &QDBusClient::ShouldProxy;
    cbs.get_proxy     = &QDBusClient::GetProxy;
    cbs.block         = &QDBusClient::Block;

    m_Connection = adbus_conn_new(&cbs, this);
    m_Buffer = adbus_buf_new();

    connectToServer(envstr);
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

void QDBusClient::GetProxy(void* u, adbus_ProxyCallback* cb, adbus_ProxyMsgCallback* msgcb, void** data)
{
    (void) u;
    if (!m_Proxies.hasLocalData()) {
        m_Proxies.setLocalData(new QDBusProxy);
    }

    if (cb) {
        *cb = QDBusProxy::ProxyCallback;
    }

    if (msgcb) {
        *msgcb = QDBusProxy::ProxyMsgCallback;
    }

    *data = m_Proxies.localData();
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

void QDBusClient::Free(void* u)
{
    QDBusClient* d = (QDBusClient*) u;
    d->deleteLater();
}

/* ------------------------------------------------------------------------- */

bool QDBusClient::connectToServer(const char* envstr)
{
    disconnect();

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







/* ------------------------------------------------------------------------- */

adbus_Connection* QDBusConnectionPrivate::Connection(const QDBusConnection& c)
{ return c.d->connection; }

QDBusObject* QDBusConnectionPrivate::GetObject(const QDBusConnection& c, QObject* object)
{
    QMutexLocker lock(&c.d->objectLock);

    QHash<QObject*, QDBusObject*>::iterator ii = c.d->objects.find(object);
    if (ii != c.d->objects.end())
        return ii.value();

    QDBusObject* ret = new QDBusObject(c, object);
    c.d->objects.insert(object, ret);
    return ret;
}

void QDBusConnectionPrivate::RemoveObject(const QDBusConnection& c, QObject* object)
{
    QMutexLocker lock(&c.d->objectLock);
    c.d->objects.remove(object);
}

QDBusConnectionPrivate::QDBusConnectionPrivate()
: connection(NULL)
{} 

QDBusConnectionPrivate::~QDBusConnectionPrivate()
{ adbus_conn_deref(connection); }

/* ------------------------------------------------------------------------- */

static QMutex sConnectionLock;
static QHash<QString, QDBusConnectionPrivate*> sNamedConnections;
static QDBusConnectionPrivate* sConnection[ADBUS_BUS_NUM];


QDBusConnectionPrivate* GetConnection(const QString& name)
{
    QMutexLocker lock(&sConnectionLock);

    QDBusConnectionPrivate*& p = sNamedConnections[name];
    if (p == NULL) {
        p = new QDBusConnectionPrivate;
    }
    return p;
}

QDBusConnectionPrivate* GetConnection(adbus_BusType type)
{
    QMutexLocker lock(&sConnectionLock);
    QDBusConnectionPrivate*& p = sConnection[type];
    if (p == NULL) {
        p = new QDBusConnectionPrivate;
    }
    return p;
}

/* ------------------------------------------------------------------------- */

QDBusConnection::QDBusConnection(QDBusConnectionPrivate *dd)
{ 
    void* handle = NULL;
    qCopySharedData(d, dd); 
    if (d->connection) {
        adbus_conn_block(d->connection, ADBUS_WAIT_FOR_CONNECTED, &handle, -1);
    }
}

QDBusConnection::QDBusConnection(const QString& name)
{
    void* handle = NULL;
    qCopySharedData(d, GetConnection(name));
    if (d->connection) {
        adbus_conn_block(d->connection, ADBUS_WAIT_FOR_CONNECTED, &handle, -1);
    }
}

QDBusConnection::QDBusConnection(const QDBusConnection& other)
{ qCopySharedData(d, other.d); }

QDBusConnection& QDBusConnection::operator=(const QDBusConnection& other)
{ qAssignSharedData(d, other.d); return *this; }

QDBusConnection::~QDBusConnection()
{ qDestructSharedData(d); }

/* ------------------------------------------------------------------------- */

QDBusConnection QDBusConnection::connectToBus(const QString& address, const QString &name)
{
    QDBusConnectionPrivate* p = GetConnection(name);
    if (!p->connection) {
        QDBusClient* c = new QDBusClient(address.toLocal8Bit().constData());
        p->connection = c->connection();
    }

    return QDBusConnection(p);
}

/* ------------------------------------------------------------------------- */

QDBusConnection QDBusConnection::connectToBus(BusType type, const QString &name)
{
    QDBusConnectionPrivate* p = GetConnection(name);
    if (!p->connection) {
        QDBusClient* c = new QDBusClient(type == SystemBus ? ADBUS_SYSTEM_BUS : ADBUS_DEFAULT_BUS);
        p->connection = c->connection();
    }

    return QDBusConnection(p);
}

/* ------------------------------------------------------------------------- */

QDBusConnection QDBusConnectionPrivate::BusConnection(adbus_BusType type)
{
    QDBusConnectionPrivate* p = GetConnection(type);
    if (!p->connection) {
        p->connection = adbus_conn_get(type);
    }

    if (!p->connection) {
        QDBusClient* c = new QDBusClient(type);
        p->connection = c->connection();
        adbus_conn_set(type, p->connection);
    }

    return QDBusConnection(p);
}

QDBusConnection QDBusConnection::sessionBus()
{ return QDBusConnectionPrivate::BusConnection(ADBUS_DEFAULT_BUS); }

QDBusConnection QDBusConnection::systemBus()
{ return QDBusConnectionPrivate::BusConnection(ADBUS_SYSTEM_BUS); }

/* ------------------------------------------------------------------------- */

bool QDBusConnection::isConnected() const
{ return d->connection && adbus_conn_isconnected(d->connection); }

QString QDBusConnection::baseService() const
{ 
    if (d->connection) {
        return QString::fromUtf8(adbus_conn_uniquename(d->connection, NULL));
    } else {
        return QString(); 
    }
}

bool QDBusConnection::send(const QDBusMessage &message) const
{ 
    if (isConnected()) {
        return adbus_msg_send(QDBusMessagePrivate::ToFactory(message), d->connection) != 0;
    } else {
        return false;
    }
}

/* ------------------------------------------------------------------------- */

QDBusPendingCall QDBusConnection::asyncCall(const QDBusMessage& message, int timeout) const
{
    Q_UNUSED(timeout); // TODO

    adbus_MsgFactory* msg = QDBusMessagePrivate::ToFactory(message);

    const char* service = adbus_msg_destination(msg, NULL);
    uint32_t serial = adbus_conn_serial(d->connection);
    adbus_msg_setserial(msg, serial);

    QDBusPendingCall ret = QDBusPendingCallPrivate::Create(*this, QByteArray(service), serial);
    send(message);
    return ret;
}

/* ------------------------------------------------------------------------- */

bool QDBusConnection::callWithCallback(const QDBusMessage &message,
                                       QObject *receiver,
                                       const char *slot,
                                       int timeout /* = -1 */) const
{
    return callWithCallback(message, receiver, slot, NULL, timeout);
}

/* ------------------------------------------------------------------------- */

bool QDBusConnection::callWithCallback(const QDBusMessage& message,
                                       QObject* receiver,
                                       const char* returnMethod,
                                       const char* errorMethod,
                                       int timeout) const
{
    Q_UNUSED(timeout);
    QDBusObject* priv = QDBusConnectionPrivate::GetObject(*this, receiver);
    adbus_MsgFactory* msg = QDBusMessagePrivate::ToFactory(message);

    if (adbus_msg_serial(msg) < 0)
        adbus_msg_setserial(msg, adbus_conn_serial(d->connection));

    uint32_t serial = (uint32_t) adbus_msg_serial(msg);
    const char* remote = adbus_msg_destination(msg, NULL);
    if (!priv->addReply(remote, serial, receiver, returnMethod, errorMethod))
        return false;

    return send(message);
}

/* ------------------------------------------------------------------------- */

bool QDBusConnection::connect(const QString &service,
                              const QString &path,
                              const QString &interface,
                              const QString &name,
                              QObject *receiver,
                              const char *slot)
{
    QDBusObject* priv = QDBusConnectionPrivate::GetObject(*this, receiver);

    QByteArray serv8 = service.toAscii();
    QByteArray path8 = path.toAscii();
    QByteArray iface8 = interface.toAscii();
    QByteArray name8 = name.toAscii();

    return priv->addMatch(serv8.constData(), path8.constData(), iface8.isEmpty() ? NULL : iface8.constData(), name8.constData(), receiver, slot);
}

/* ------------------------------------------------------------------------- */

bool QDBusConnection::connect(const QString &service,
                              const QString &path,
                              const QString& interface,
                              const QString &name,
                              const QString& signature,
                              QObject *receiver,
                              const char *slot)
{
    // TODO check the signature
    Q_UNUSED(signature);
    return connect(service, path, interface, name, receiver, slot);
}

/* ------------------------------------------------------------------------- */

bool QDBusConnection::registerObject(const QString &path, QObject *object, RegisterOptions options)
{
    QByteArray path8 = path.toAscii();
    QDBusObject* priv = QDBusConnectionPrivate::GetObject(*this, object);
    if ((options & ExportAllContents) != 0) {
        if (!priv->bindFromMetaObject(path8.constData(), object, options)) {
            return false;
        }
    }

    if ((options & (ExportAdaptors | ExportChildObjects)) != 0) {
        QString nodepath = path + "/";
        QObjectList children = object->children();
        for (int i = 0; i < children.size(); i++) {
            QObject* child = children[i];
            if (!child)
                continue;

            QDBusAbstractAdaptor* adaptor = qobject_cast<QDBusAbstractAdaptor*>(child);

            if (adaptor && (options & ExportAdaptors)) {
                const char* xml = QDBusAbstractAdaptorPrivate::IntrospectionXml(adaptor);
                if (!xml)
                    continue;

                if (!priv->bindFromXml(path8.constData(), adaptor, xml)) {
                    return false;
                }

            } else if (!adaptor && (options & ExportChildObjects)) {
                QString name = child->objectName();
                if (!name.isEmpty())
                    continue;

                if (!registerObject(nodepath + name, child, options)) {
                    return false;
                }
            }
        }
    }

    return true;
}

/* ------------------------------------------------------------------------- */

