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

#include "qdbusconnection_p.h"

#include "qdbusmessage_p.h"
#include "qdbusobject_p.hxx"
#include "qsharedfunctions_p.h"
#include "qdbusabstractadaptor_p.h"
#include "qdbuspendingcall_p.hxx"
#include "qdbusconnectioninterface.hxx"


/* ------------------------------------------------------------------------- */

adbus_Connection* QDBusConnectionPrivate::Connection(const QDBusConnection& c)
{ return c.d->connection; }

QDBusObject* QDBusConnectionPrivate::GetObject(const QDBusConnection& c, QObject* object)
{
    QMutexLocker lock(&c.d->lock);

    QHash<QObject*, QDBusObject*>::iterator ii = c.d->objects.find(object);
    if (ii != c.d->objects.end())
        return ii.value();

    QDBusObject* ret = new QDBusObject(c, object);
    c.d->objects.insert(object, ret);
    return ret;
}

void QDBusConnectionPrivate::RemoveObject(const QDBusConnection& c, QObject* object)
{
    QMutexLocker lock(&c.d->lock);
    c.d->objects.remove(object);
}

QDBusConnectionPrivate::QDBusConnectionPrivate()
: client(new QDBusClient), interface(NULL)
{
	connection = client->base();
} 

QDBusConnectionPrivate::~QDBusConnectionPrivate()
{ 
    adbus_conn_deref(connection); 
    delete interface;
}

/* ------------------------------------------------------------------------- */

static QMutex sConnectionLock;
static QHash<QString, QDBusConnectionPrivate*> sNamedConnections;
static QDBusConnectionPrivate* sConnection[ADBUS_BUS_NUM];


QDBusConnection QDBusConnectionPrivate::GetConnection(const QString& name)
{
    QMutexLocker lock(&sConnectionLock);

    QDBusConnectionPrivate*& p = sNamedConnections[name];
	if (p == NULL) {
		p = new QDBusConnectionPrivate();
		QDBusConnection c(p);
		p->interface = new QDBusConnectionInterface(c, NULL);
		// Decrement the refcount since p->interface holds a connection
		qDestructSharedData(p);
		return c;
	} else {
		return QDBusConnection(p);
	}
}

QDBusConnection QDBusConnectionPrivate::GetConnection(adbus_BusType type)
{
    QMutexLocker lock(&sConnectionLock);
    QDBusConnectionPrivate*& p = sConnection[type];
	if (p == NULL) {
		p = new QDBusConnectionPrivate();
		QDBusConnection c(p);
		p->interface = new QDBusConnectionInterface(c, NULL);
		// Decrement the refcount since p->interface holds a connection
		qDestructSharedData(p);
		return c;
	} else {
		return QDBusConnection(p);
	}
}

/* ------------------------------------------------------------------------- */

QDBusConnection::QDBusConnection(QDBusConnectionPrivate *dd)
{ qCopySharedData(d, dd); }

QDBusConnection::QDBusConnection(const QString& name)
{ *this = QDBusConnectionPrivate::GetConnection(name); }

QDBusConnection::QDBusConnection(const QDBusConnection& other)
{ qCopySharedData(d, other.d); }

QDBusConnection& QDBusConnection::operator=(const QDBusConnection& other)
{ qAssignSharedData(d, other.d); return *this; }

QDBusConnection::~QDBusConnection()
{ qDestructSharedData(d); }

/* ------------------------------------------------------------------------- */

QDBusConnection QDBusConnection::connectToBus(const QString& address, const QString &name)
{
	QDBusConnection c = QDBusConnectionPrivate::GetConnection(name);

	if (c.d->client->connectToServer(address.toLocal8Bit().constData(), true)) {
		void* block = NULL;
		adbus_conn_block(c.d->connection, ADBUS_WAIT_FOR_CONNECTED, &block, -1);
	}

    return c;
}

/* ------------------------------------------------------------------------- */

QDBusConnection QDBusConnection::connectToBus(BusType type, const QString &name)
{
	QDBusConnection c = QDBusConnectionPrivate::GetConnection(name);

	if (c.d->client->connectToServer(type == SystemBus ? ADBUS_SYSTEM_BUS : ADBUS_DEFAULT_BUS, true)) {
		void* block = NULL;
		adbus_conn_block(c.d->connection, ADBUS_WAIT_FOR_CONNECTED, &block, -1);
	}

    return c;
}

/* ------------------------------------------------------------------------- */

QDBusConnection QDBusConnectionPrivate::BusConnection(adbus_BusType type)
{
	QDBusConnection c = QDBusConnectionPrivate::GetConnection(type);

	if (c.d->client->connectToServer(type, true)) {
		void* block = NULL;
		adbus_conn_block(c.d->connection, ADBUS_WAIT_FOR_CONNECTED, &block, -1);
	}

    return c;
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
        adbus_MsgFactory* msg = d->GetFactory();
        QDBusMessagePrivate::GetMessage(message, msg);
        return adbus_msg_send(msg, d->connection) != 0;
    } else {
        return false;
    }
}

/* ------------------------------------------------------------------------- */

QDBusPendingCall QDBusConnection::asyncCall(const QDBusMessage& message, int timeout) const
{
    Q_UNUSED(timeout); // TODO

    adbus_MsgFactory* msg = d->GetFactory();
    QDBusMessagePrivate::GetMessage(message, msg);

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
    adbus_MsgFactory* msg = d->GetFactory();
    QDBusMessagePrivate::GetMessage(message, msg);

    if (adbus_msg_serial(msg) < 0)
        adbus_msg_setserial(msg, adbus_conn_serial(d->connection));

    uint32_t serial = (uint32_t) adbus_msg_serial(msg);
    QByteArray remote = message.service().toAscii();
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

    return priv->addMatch(serv8, path8, iface8, name8, receiver, slot);
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

void QDBusConnectionPrivate::SetLastError(const QDBusConnection& c, QDBusError err)
{
    QMutexLocker lock(&c.d->lock);
    c.d->lastError = err;
}

/* ------------------------------------------------------------------------- */

QDBusError QDBusConnection::lastError() const
{
    QMutexLocker lock(&d->lock);
    return d->lastError;
}

/* ------------------------------------------------------------------------- */

QDBusConnectionInterface* QDBusConnection::interface() const
{ return d->interface; }

/* ------------------------------------------------------------------------- */

bool QDBusConnection::registerService(const QString& serviceName)
{
    return interface()->registerService(serviceName).value()
        == QDBusConnectionInterface::ServiceRegistered;
}

/* ------------------------------------------------------------------------- */

bool QDBusConnection::unregisterService(const QString& serviceName)
{ return interface()->unregisterService(serviceName).value(); }

















/* ------------------------------------------------------------------------- */

struct QDBusMessageFactory
{
    QDBusMessageFactory() : d(adbus_msg_new()) {}
    ~QDBusMessageFactory() {adbus_msg_free(d);}
    adbus_MsgFactory* d;
};

static QThreadStorage<QDBusMessageFactory*> factories;

adbus_MsgFactory* QDBusConnectionPrivate::GetFactory()
{
    if (!factories.hasLocalData()) {
        factories.setLocalData(new QDBusMessageFactory);
    }

    return factories.localData()->d;
}




