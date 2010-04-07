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

#include "qdbusmessage_p.h"
#include "qdbusargument_p.h"
#include "qsharedfunctions_p.h"
#include "qdbusreply.h"
#include "qdbusdebug.h"
#include <QtCore/qthreadstorage.h>
#include <QtCore/qmetaobject.h>




/* ------------------------------------------------------------------------- */

bool QDBusArgumentList::init(const QMetaMethod& method)
{
    QList<QByteArray> types = method.parameterTypes();
    QList<QByteArray> names = method.parameterNames();

    m_MetacallData.resize(types.size() + 1);

    QByteArray rettype = method.typeName();
    if (!rettype.isEmpty() && rettype != "void") {

        int typeId = QMetaType::type(rettype.constData());

        QDBusArgumentType* type = QDBusArgumentType::FromMetatype(typeId);

        if (!type) {
            QDBUS_LOG("failed to lookup return type '%s'", rettype.constData());
            return false;
        }

        QDBUS_LOG("looked up return: type '%s', dbus '%s', id %d",
                rettype.constData(),
                type->m_DBusSignature.constData(),
                type->m_TypeId);

        m_Args += Entry(QDBusOutArgument, "", type);

    } else {
        m_Args += Entry(QDBusOutArgument, "", NULL);
    }

    for (int i = 0; i < types.size(); i++) {

        QDBusArgumentDirection dir;

        if (i == types.size() - 1 && types[i] == "QDBusMessage") {
            m_AppendMessage = true;
            continue;
        }

        QDBusArgumentType* type = QDBusArgumentType::FromCppType(types[i], &dir);
        if (!type) {
            QDBUS_LOG("failed to lookup argument type '%s'", types[i].constData());
            return false;
        }

        QDBUS_LOG("looked up argument: name '%s', type '%s', dbus '%s', id %d, direction %s",
                names[i].constData(),
                types[i].constData(),
                type->m_DBusSignature.constData(),
                type->m_TypeId,
                (dir == QDBusInArgument) ? "in" : "out");

        m_Args += Entry(dir, names[i], type);
    }

    Q_ASSERT(m_Args.size() == m_MetacallData.size() || (m_AppendMessage && m_Args.size() + 1 == m_MetacallData.size()));

    for (int i = 0; i < m_Args.size(); i++) {
        if (m_Args[i].direction == QDBusOutArgument) {
            if (m_Args[i].type) {
                m_MetacallData[i] = QMetaType::construct(m_Args[i].type->m_TypeId);
            } else {
                Q_ASSERT(i == 0);
                m_MetacallData[i] = NULL;
            }
        }
    }

    return true;
}

/* ------------------------------------------------------------------------- */

void QDBusArgumentList::setupMetacall(const QDBusMessage& msg)
{
    m_Message = msg;
    int argi = 0;

    for (int i = 0; i < m_Args.size(); i++) {
        if (m_Args[i].type && m_Args[i].direction == QDBusInArgument) {
            /* Reference the variant inside the message directly as the
             * pointer returned by constData is only valid whilst that
             * particular QVariant is alive.
             */
            const QVariant& arg = QDBusMessagePrivate::Argument(m_Message, argi++);

            /* This should have already been assured by QDBusMessage::FromMessage */
            Q_ASSERT(arg.userType() == m_Args[i].type->m_TypeId);

            m_MetacallData[i] = const_cast<void*>(arg.constData());
        }
    }

    if (m_AppendMessage) {
        m_MetacallData[m_MetacallData.size() - 1] = (void*) &m_Message;
    }
}

/* ------------------------------------------------------------------------- */

void QDBusArgumentList::getReply(adbus_MsgFactory** ret)
{ QDBusMessagePrivate::GetReply(m_Message, ret, *this); }

/* ------------------------------------------------------------------------- */

void QDBusArgumentList::bufferReturnArguments(adbus_MsgFactory* msg) const
{
    adbus_Buffer* buf = adbus_msg_argbuffer(msg);

    for (int i = 0; i < m_Args.size(); i++) {
        QDBusArgumentType* type = m_Args[i].type;

        if (type && m_Args[i].direction == QDBusOutArgument) {
            type->marshall(buf, m_MetacallData[i], false, false);
        }
    }
}

/* ------------------------------------------------------------------------- */

void QDBusArgumentList::bufferSignalArguments(adbus_MsgFactory* msg, void** args)
{
    adbus_Buffer* buf = adbus_msg_argbuffer(msg);

    for (int i = 1; i < m_Args.size(); i++) {
        QDBusArgumentType* type = m_Args[i].type;

        Q_ASSERT(type && m_Args[i].direction == QDBusInArgument);
        type->marshall(buf, args[i], true, false);
    }
}











/* ------------------------------------------------------------------------- */

int QDBusMessagePrivate::FromMessage(QDBusMessage& q, adbus_Message* msg)
{
    qDetachSharedData(q.d_ptr);
    QDBusMessagePrivate* d = q.d_ptr;

    d->reset();
    d->getHeaders(msg);

    QByteArray sig;
    adbus_Iterator i;
    adbus_iter_args(&i, msg);
    while (i.size > 0) {
        const char* sigend = adbus_nextarg(i.sig);
        if (!sigend) {
            goto err;
        }

        sig.clear();
        sig.append(i.sig, sigend - i.sig);

        QDBusArgumentType* type = QDBusArgumentType::FromDBusType(sig);
        if (type) {
            QVariant variant(type->m_TypeId, (const void*) NULL);
            if (type->demarshall(&i, variant)) {
                goto err;
            }
            d->arguments.push_back(variant);
        } else {
            if (adbus_iter_value(&i)) {
                  goto err;
            }
            d->arguments.push_back(QVariant());
        }
    }

    return 0;

err:
    d->reset();
    return -1;
}

/* ------------------------------------------------------------------------- */

int QDBusMessagePrivate::FromMessage(QDBusMessage& q, adbus_Message* msg, const QDBusArgumentList& types)
{
    qDetachSharedData(q.d_ptr);
    QDBusMessagePrivate* d = q.d_ptr;

    d->reset();
    d->getHeaders(msg);

    adbus_Iterator iter;
    adbus_iter_args(&iter, msg);

    for (int i = 0; i < types.m_Args.size(); i++) {
        if (types.m_Args[i].direction == QDBusInArgument) {

            QDBusArgumentType* type = types.m_Args[i].type;

            /* Check that we aren't expecting more types than provided */
            if (iter.size == 0) {
                goto argument_error;
            }

            /* Check that we can figure out the provided type */
            const char* sigend = adbus_nextarg(iter.sig);
            if (!sigend) {
                goto parse_error;
            }

            /* Check that the provided type is as we expect */
            if (    sigend - iter.sig != type->m_DBusSignature.size() 
                ||  qstrncmp(iter.sig, type->m_DBusSignature.constData(), sigend - iter.sig) != 0)
            {
                goto argument_error;
            }

            /* Demarshall the argument */
            QVariant variant(type->m_TypeId, (const void*) NULL);
            if (type->demarshall(&iter, variant)) {
                goto parse_error;
            }

            d->arguments.push_back(variant);
        }
    }

    return 0;

argument_error:
    d->reset();
    return 0;

parse_error:
    d->reset();
    return -1;
}

/* ------------------------------------------------------------------------- */

bool QDBusMessagePrivate::GetMessage(const QDBusMessage& q, adbus_MsgFactory* msg)
{
    QDBusMessagePrivate* d = q.d_ptr;
    Q_ASSERT(d);

    adbus_msg_reset(msg);
    adbus_msg_settype(msg, d->type);
    adbus_msg_setflags(msg, d->flags);

    if (d->serial >= 0) {
        adbus_msg_setserial(msg, (uint32_t) d->serial);
    }

    if (d->replySerial >= 0) {
        adbus_msg_setreply(msg, (uint32_t) d->replySerial);
    }

    if (!d->path.isEmpty()) {
        QByteArray path8 = d->path.toAscii();
        adbus_msg_setpath(msg, path8.constData(), path8.size());
    }

    if (!d->interface.isEmpty()) {
        QByteArray interface8 = d->interface.toAscii();
        adbus_msg_setinterface(msg, interface8.constData(), interface8.size());
    }

    if (!d->member.isEmpty()) {
        QByteArray member8 = d->member.toAscii();
        adbus_msg_setmember(msg, member8.constData(), member8.size());
    }

    if (!d->error.isEmpty()) {
        QByteArray error8 = d->error.toAscii();
        adbus_msg_seterror(msg, error8.constData(), error8.size());
    }

    if (!d->sender.isEmpty()) {
        QByteArray sender8 = d->sender.toAscii();
        adbus_msg_setsender(msg, sender8.constData(), sender8.size());
    }

    if (!d->destination.isEmpty()) {
        QByteArray destination8 = d->destination.toAscii();
        adbus_msg_setdestination(msg, destination8.constData(), destination8.size());
    }

    if (!d->path.isEmpty()) {
        QByteArray path8 = d->path.toAscii();
        adbus_msg_setpath(msg, path8.constData(), path8.size());
    }

    adbus_Buffer* buf = adbus_msg_argbuffer(msg);
    for (int i = 0; i < d->arguments.size(); i++) {

        QVariant arg = d->arguments.at(i);

        QDBusArgumentType* type = QDBusArgumentType::FromMetatype(arg.userType());
        if (!type) {
            return false;
        }

        type->marshall(buf, arg.data(), true, false);
    }

    return true;
}

/* ------------------------------------------------------------------------- */

void QDBusMessagePrivate::GetReply(const QDBusMessage& msg, adbus_MsgFactory** ret, const QDBusArgumentList& args)
{
    QDBusMessagePrivate* d = msg.d_ptr;
    Q_ASSERT(d);

    if (*ret && !d->delayedReply) {
        args.bufferReturnArguments(*ret);
    } else {
        *ret = NULL;
    }
}



/* ------------------------------------------------------------------------- */

const QVariant& QDBusMessagePrivate::Argument(const QDBusMessage& msg, int num)
{ return msg.d_ptr->arguments.at(num); }

/* ------------------------------------------------------------------------- */

void QDBusMessagePrivate::reset()
{
    type = ADBUS_MSG_INVALID;
    flags = 0;
    serial = -1;
    replySerial = -1;
    error.clear();
    signature.clear();
    path.clear();
    interface.clear();
    member.clear();
    sender.clear();
    destination.clear();
    arguments.clear();
    delayedReply = false;
}

/* ------------------------------------------------------------------------- */

void QDBusMessagePrivate::getHeaders(adbus_Message* msg)
{
    type = msg->type;
    flags = msg->flags;
    serial = msg->serial;
    replySerial = msg->replySerial;

    if (msg->signature) {
        signature = QString::fromAscii(msg->signature, (int) msg->signatureSize);
    }

    if (msg->path) {
        path = QString::fromAscii(msg->path, (int) msg->pathSize);
    }

    if (msg->interface) {
        interface = QString::fromAscii(msg->interface, (int) msg->interfaceSize);
    }

    if (msg->member) {
        member = QString::fromAscii(msg->member, (int) msg->memberSize);
    }

    if (msg->error) {
        error = QString::fromAscii(msg->error, (int) msg->errorSize);
    }

    if (msg->destination) {
        destination = QString::fromAscii(msg->destination, (int) msg->destinationSize);
    }

    if (msg->sender) {
        sender = QString::fromAscii(msg->sender, (int) msg->senderSize);
    }
}

/* ------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------- */

QDBusMessage::QDBusMessage()
{ d_ptr = NULL; }

QDBusMessage::~QDBusMessage()
{ qDestructSharedData(d_ptr); }

QDBusMessage::QDBusMessage(const QDBusMessage& other)
{ qCopySharedData(d_ptr, other.d_ptr); }

QDBusMessage& QDBusMessage::operator=(const QDBusMessage& other)
{ qAssignSharedData(d_ptr, other.d_ptr); return *this; }

/* ------------------------------------------------------------------------- */

QDBusMessage QDBusMessage::createSignal(const QString& path,
                                        const QString& interface,
                                        const QString& name)
{
    QDBusMessage ret;

    qDetachSharedData(ret.d_ptr);
    ret.d_ptr->type = ADBUS_MSG_SIGNAL;
    ret.d_ptr->flags = ADBUS_MSG_NO_REPLY;
    ret.d_ptr->path = path;
    ret.d_ptr->interface = interface;
    ret.d_ptr->member = name;

    return ret;
}

/* ------------------------------------------------------------------------- */

QDBusMessage QDBusMessage::createMethodCall(const QString& destination,
                                            const QString& path,
                                            const QString& interface,
                                            const QString& method)
{
    QDBusMessage ret;

    qDetachSharedData(ret.d_ptr);
    ret.d_ptr->type = ADBUS_MSG_METHOD;
    ret.d_ptr->destination = destination;
    ret.d_ptr->path = path;
    ret.d_ptr->interface = interface;
    ret.d_ptr->member = method;

    return ret;
}

/* ------------------------------------------------------------------------- */

// This one is almost always pointless as the message has no destination
QDBusMessage QDBusMessage::createError(const QString &name, const QString &msg)
{
    QDBusMessage ret;

    qDetachSharedData(ret.d_ptr);
    ret.d_ptr->type = ADBUS_MSG_ERROR;
    ret.d_ptr->flags = ADBUS_MSG_NO_REPLY;
    ret.d_ptr->error = name;
    ret.d_ptr->arguments[0] = msg;

    return ret;
}

/* ------------------------------------------------------------------------- */

// Someone missed a ref on \a name
QDBusMessage QDBusMessage::createErrorReply(const QString name,
                                            const QString& msg) const
{
    QDBusMessage ret;

    qDetachSharedData(ret.d_ptr);
    ret.d_ptr->type = ADBUS_MSG_ERROR;
    ret.d_ptr->flags = ADBUS_MSG_NO_REPLY;

    if (d_ptr) {
        ret.d_ptr->destination = d_ptr->sender;
        ret.d_ptr->replySerial = d_ptr->serial;
    }

    ret.d_ptr->error = name;
    ret.d_ptr->arguments[0] = msg;

    return ret;
}

/* ------------------------------------------------------------------------- */

QDBusMessage QDBusMessage::createReply(const QList<QVariant> &arguments) const
{
    QDBusMessage ret;

    qDetachSharedData(ret.d_ptr);
    ret.d_ptr->type = ADBUS_MSG_RETURN;
    ret.d_ptr->flags = ADBUS_MSG_NO_REPLY;

    if (d_ptr) {
        ret.d_ptr->replySerial = d_ptr->serial;
        ret.d_ptr->destination = d_ptr->sender;
    }

    ret.d_ptr->arguments = arguments;

    return ret;
}

/* ------------------------------------------------------------------------- */

QList<QVariant> QDBusMessage::arguments() const
{ return d_ptr ? d_ptr->arguments : QList<QVariant>(); }

void QDBusMessage::setArguments(const QList<QVariant>& arguments)
{
    qDetachSharedData(d_ptr);
    d_ptr->arguments = arguments;
}

QDBusMessage& QDBusMessage::operator<<(const QVariant& arg)
{
    qDetachSharedData(d_ptr);
    d_ptr->arguments.push_back(arg);
    return *this;
}

/* ------------------------------------------------------------------------- */

QString QDBusMessage::service() const
{ return d_ptr ? d_ptr->sender : QString(); }

QString QDBusMessage::path() const
{ return d_ptr ? d_ptr->path : QString(); }

QString QDBusMessage::interface() const
{ return d_ptr ? d_ptr->interface : QString(); }

QString QDBusMessage::member() const
{ return d_ptr ? d_ptr->member : QString(); }

QDBusMessage::MessageType QDBusMessage::type() const
{ return d_ptr ? (MessageType) d_ptr->type : InvalidMessage; }

QString QDBusMessage::signature() const
{ return d_ptr ? d_ptr->signature : QString(); }

QString QDBusMessage::errorName() const
{ return d_ptr ? d_ptr->error : QString(); }

bool QDBusMessage::isReplyRequired() const
{ return d_ptr ? (d_ptr->flags & ADBUS_MSG_NO_REPLY) == 0 : false; }

void QDBusMessage::setDelayedReply(bool enable) const
{ 
    if (d_ptr) {
        d_ptr->delayedReply = enable; 
    }
}

bool QDBusMessage::isDelayedReply() const
{ return d_ptr ? d_ptr->delayedReply : false; }

/* ------------------------------------------------------------------------- */

QString QDBusMessage::errorMessage() const
{
    if (!d_ptr || d_ptr->type != ADBUS_MSG_ERROR || d_ptr->arguments.isEmpty()) {
        return QString();
    } else {
        return d_ptr->arguments[0].toString();
    }
}









/* ------------------------------------------------------------------------- */

void qDBusReplyFill(const QDBusMessage &reply, QDBusError &error, QVariant &data)
{
    QList<QVariant> args = reply.arguments();
    if (reply.type() == QDBusMessage::ReplyMessage && args.size() > 0) {
        data = args.at(0);
    } else if (reply.type() == QDBusMessage::ErrorMessage) {
        error = QDBusError(reply);
    } else {
        error = QDBusError(QDBusError::Other, "nz.co.foobar.adbusqt.InvalidArguments");
    }
}


