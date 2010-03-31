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

#include "qdbusabstractinterface_p.h"
#include "qdbusconnection_p.hxx"
#include "qdbuspendingcall_p.hxx"
#include "qdbuspendingreply.h"
#include "qdbusinterface.hxx"

/* ------------------------------------------------------------------------- */

// I wonder what this was introduced for ...
QDBusAbstractInterfaceBase::QDBusAbstractInterfaceBase(QDBusAbstractInterfacePrivate& p, QObject* parent)
: QObject(p, parent)
{}

int QDBusAbstractInterfaceBase::qt_metacall(QMetaObject::Call type, int index, void** data)
{ return QObject::qt_metacall(type, index, data); }


/* ------------------------------------------------------------------------- */

QDBusAbstractInterface::QDBusAbstractInterface(const QString& service,
                                               const QString& path,
                                               const char* interface,
                                               const QDBusConnection& connection,
                                               QObject* parent)
: QDBusAbstractInterfaceBase(*new QDBusAbstractInterfacePrivate(connection), parent)
{
    Q_D(QDBusAbstractInterface);

    d->object = QDBusConnectionPrivate::GetObject(connection, this);
    d->interface = interface;
    d->remoteStr = service;
    d->pathStr = path;
    d->remote = service.toAscii();
    d->path = path.toAscii();
}

QDBusAbstractInterface::QDBusAbstractInterface(QDBusAbstractInterfacePrivate& priv, QObject* parent)
: QDBusAbstractInterfaceBase(priv, parent)
{}

QDBusAbstractInterface::~QDBusAbstractInterface()
{
    Q_D(QDBusAbstractInterface);
    delete d;
}

QDBusAbstractInterfacePrivate::QDBusAbstractInterfacePrivate(const QDBusConnection& c)
:   qconnection(c),
    connection(QDBusConnectionPrivate::Connection(c)),
    object(NULL),
    interface(NULL),
    msg(adbus_msg_new())
{}

QDBusAbstractInterfacePrivate::~QDBusAbstractInterfacePrivate()
{ 
    adbus_msg_free(msg); 
    object->destroy();
}

/* ------------------------------------------------------------------------- */

void QDBusAbstractInterface::connectNotify(const char* signal)
{
    Q_D(QDBusAbstractInterface);

    if (strcmp(signal, "destroyed(QObject*)") == 0)
        return;

    const char* nameend = strchr(signal, '(');
    if (nameend == NULL)
        return;

    QByteArray sigMethod = "2";
    sigMethod += signal;

    QByteArray member(signal, (int) (nameend - signal));

    {
        QMutexLocker lock(&d->matchLock);
        if (!d->matches.contains(member))
            return;
        d->matches.insert(member);
    }

    d->object->addMatch(d->remote, d->path, d->interface, member, this, sigMethod.constData());
}

/* ------------------------------------------------------------------------- */

void QDBusAbstractInterface::disconnectNotify(const char* signal)
{
    Q_D(QDBusAbstractInterface);

    if (strcmp(signal, "destroyed(QObject*)") == 0)
        return;

    const char* nameend = strchr(signal, '(');
    if (nameend == NULL)
        return;

    QByteArray sigMethod = "2";
    sigMethod += signal;

    QByteArray member(signal, (int) (nameend - signal));

    {
        QMutexLocker lock(&d->matchLock);
        if (!d->matches.contains(member))
            return;
        d->matches.remove(member);
    }

    d->object->removeMatch(d->remote, d->path, d->interface, member, this, sigMethod.constData());
                          
}

/* ------------------------------------------------------------------------- */

void QDBusAbstractInterface::internalPropSet(const char* propname, const QVariant& value)
{
    Q_D(QDBusAbstractInterface);

    QDBusArgumentType* type = QDBusArgumentType::Lookup(value.userType());
    if (!type)
        return;

    adbus_msg_reset(d->msg);
    adbus_msg_settype(d->msg, ADBUS_MSG_METHOD);
    adbus_msg_setflags(d->msg, ADBUS_MSG_NO_REPLY);
    adbus_msg_setdestination(d->msg, d->remote.constData(), d->remote.size());
    adbus_msg_setpath(d->msg, d->path.constData(), d->path.size());
    adbus_msg_setinterface(d->msg, "org.freedesktop.DBus.Properties", -1);
    adbus_msg_setmember(d->msg, "Set", -1);

    adbus_Buffer* b = adbus_msg_argbuffer(d->msg);
    adbus_buf_setsig(b, "ssv", 2);
    adbus_buf_string(b, d->interface, -1);
    adbus_buf_string(b, propname, -1);

    adbus_BufVariant v;
    adbus_buf_beginvariant(b, &v, type->m_DBusSignature.constData(), type->m_DBusSignature.size());
    type->marshall(b, value, false, false);
    adbus_buf_endvariant(b, &v);

    adbus_msg_send(d->msg, d->connection);
}

/* ------------------------------------------------------------------------- */

QVariant QDBusAbstractInterface::internalPropGet(const char* propname) const
{
    Q_D(const QDBusAbstractInterface);

    uint32_t serial = adbus_conn_serial(d->connection);
    QDBusPendingCall call = QDBusPendingCallPrivate::Create(d->qconnection, d->remote, serial);

    adbus_msg_reset(d->msg);
    adbus_msg_settype(d->msg, ADBUS_MSG_METHOD);
    adbus_msg_setserial(d->msg, serial);
    adbus_msg_setdestination(d->msg, d->remote.constData(), d->remote.size());
    adbus_msg_setpath(d->msg, d->path.constData(), d->path.size());
    adbus_msg_setinterface(d->msg, "org.freedesktop.DBus.Properties", -1);
    adbus_msg_setmember(d->msg, "Get", -1);

    adbus_Buffer* b = adbus_msg_argbuffer(d->msg);
    adbus_buf_setsig(b, "ss", 2);
    adbus_buf_string(b, d->interface, -1);
    adbus_buf_string(b, propname, -1);

    adbus_msg_send(d->msg, d->connection);

    QDBusPendingReply<QDBusVariant> reply(call);
    reply.waitForFinished();

    return reply.argumentAt<0>().variant();
}

/* ------------------------------------------------------------------------- */

static bool DoCall(
        QDBusAbstractInterfacePrivate*  d,
        const QString&                  method,
        const QList<QVariant>&          args,
        uint32_t                        serial)
{
    QByteArray method8 = method.toAscii();

    adbus_msg_reset(d->msg);
    adbus_msg_settype(d->msg, ADBUS_MSG_METHOD);
    adbus_msg_setserial(d->msg, serial);
    adbus_msg_setdestination(d->msg, d->remote.constData(), d->remote.size());
    adbus_msg_setpath(d->msg, d->path.constData(), d->path.size());
    adbus_msg_setinterface(d->msg, d->interface, -1);
    adbus_msg_setmember(d->msg, method8.constData(), method8.size());

    adbus_Buffer* b = adbus_msg_argbuffer(d->msg);
    for (int i = 0; i < args.size(); i++) {
        QDBusArgumentType* type = QDBusArgumentType::Lookup(args[i].userType());
        if (!type) {
            return false;
        }
        type->marshall(b, args[i], true, false);
    }

	return adbus_msg_send(d->msg, d->connection) == 0;
}

/* ------------------------------------------------------------------------- */

QDBusPendingCall QDBusAbstractInterface::asyncCallWithArgumentList(
        const QString& method,
        const QList<QVariant>& args)
{
    Q_D(QDBusAbstractInterface);

    uint32_t serial = adbus_conn_serial(d->connection);
    QDBusPendingCall call = QDBusPendingCallPrivate::Create(d->qconnection, d->remote, serial);

    DoCall(d, method, args, serial);

    return call;
}

/* ------------------------------------------------------------------------- */

QDBusPendingCall QDBusAbstractInterface::asyncCall(
        const QString &method,
        const QVariant &arg1,
        const QVariant &arg2,
        const QVariant &arg3,
        const QVariant &arg4,
        const QVariant &arg5,
        const QVariant &arg6,
        const QVariant &arg7,
        const QVariant &arg8)
{
    QList<QVariant> args;
    if (arg1.isValid()) args << arg1;
    if (arg2.isValid()) args << arg2;
    if (arg3.isValid()) args << arg3;
    if (arg4.isValid()) args << arg4;
    if (arg5.isValid()) args << arg5;
    if (arg6.isValid()) args << arg6;
    if (arg7.isValid()) args << arg7;
    if (arg8.isValid()) args << arg8;

    return asyncCallWithArgumentList(method, args);
}

/* ------------------------------------------------------------------------- */

QDBusMessage QDBusAbstractInterface::callWithArgumentList(
        QDBus::CallMode mode,
        const QString& method,
        const QList<QVariant>& args)
{
    Q_D(QDBusAbstractInterface);

    QDBusPendingCall reply = asyncCallWithArgumentList(method, args);
    if (mode == QDBus::NoBlock)
        return QDBusMessage();

    reply.waitForFinished();

    if (reply.isError()) {
        d->lastError = reply.error();
    }

    return reply.reply();
}

/* ------------------------------------------------------------------------- */

QDBusMessage QDBusAbstractInterface::call(
        QDBus::CallMode mode,
        const QString& method,
        const QVariant &arg1,
        const QVariant &arg2,
        const QVariant &arg3,
        const QVariant &arg4,
        const QVariant &arg5,
        const QVariant &arg6,
        const QVariant &arg7,
        const QVariant &arg8)
{
    QList<QVariant> args;
    if (arg1.isValid()) args << arg1;
    if (arg2.isValid()) args << arg2;
    if (arg3.isValid()) args << arg3;
    if (arg4.isValid()) args << arg4;
    if (arg5.isValid()) args << arg5;
    if (arg6.isValid()) args << arg6;
    if (arg7.isValid()) args << arg7;
    if (arg8.isValid()) args << arg8;

    return callWithArgumentList(mode, method, args);
}

/* ------------------------------------------------------------------------- */

QDBusMessage QDBusAbstractInterface::call(
        const QString& method,
        const QVariant &arg1,
        const QVariant &arg2,
        const QVariant &arg3,
        const QVariant &arg4,
        const QVariant &arg5,
        const QVariant &arg6,
        const QVariant &arg7,
        const QVariant &arg8)
{
    return call(QDBus::Block, method, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
}

/* ------------------------------------------------------------------------- */

bool QDBusAbstractInterface::callWithCallback(
        const QString&          method,
        const QList<QVariant>&  args,
        QObject*                receiver,
        const char*             member,
        const char*             errorSlot)
{
    Q_D(QDBusAbstractInterface);

    uint32_t serial = adbus_conn_serial(d->connection);

    QDBusObject* obj = QDBusConnectionPrivate::GetObject(d->qconnection, receiver);
    if (!obj->addReply(d->remote.constData(), serial, receiver, member, errorSlot))
        return false;

    DoCall(d, method, args, serial);
    return true;
}

/* ------------------------------------------------------------------------- */

bool QDBusAbstractInterface::callWithCallback(
        const QString&          method,
        const QList<QVariant>&  args,
        QObject*                receiver,
        const char*             member)
{
    return callWithCallback(method, args, receiver, member, NULL);
}

/* ------------------------------------------------------------------------- */

bool QDBusAbstractInterface::isValid() const
{
    // We have no easy means of determining this
    return true;
}

/* ------------------------------------------------------------------------- */

QDBusError QDBusAbstractInterface::lastError() const
{
    Q_D(const QDBusAbstractInterface);
    return d->lastError;
}

/* ------------------------------------------------------------------------- */

QDBusConnection QDBusAbstractInterface::connection() const
{
    Q_D(const QDBusAbstractInterface);
    return d->qconnection;
}

/* ------------------------------------------------------------------------- */

QString QDBusAbstractInterface::service() const
{
    Q_D(const QDBusAbstractInterface);
    return d->remoteStr;
}

/* ------------------------------------------------------------------------- */

QString QDBusAbstractInterface::path() const
{
    Q_D(const QDBusAbstractInterface);
    return d->pathStr;
}

/* ------------------------------------------------------------------------- */

QString QDBusAbstractInterface::interface() const
{
    Q_D(const QDBusAbstractInterface);
    return QString::fromAscii(d->interface);
}
















/* ------------------------------------------------------------------------- */

QDBusInterface::QDBusInterface(
        const QString& service,
        const QString& path,
        const QString& interface,
        const QDBusConnection& connection,
        QObject* parent)
: QDBusAbstractInterface(service, path, interface.toAscii().constData(), connection, parent)
{}

QDBusInterface::~QDBusInterface()
{}








