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

#include "qdbusmessage_p.hxx"
#include "qdbusargument_p.hxx"
#include "qsharedfunctions_p.hxx"
#include "qdbusmetatype_p.hxx"
#include <QtCore/qthreadstorage.h>




/* ------------------------------------------------------------------------- */

bool QDBusArgumentList::init(const QMetaMethod& method)
{
    QList<QByteArray> types = method.parameterTypes();
    QList<QByteArray> names = method.parameterNames();

    m_MetacallData.resize(types.size() + 1, NULL);

    QByteArray rettype = method.typeName();
    if (rettype.isEmpty() && rettype != "void") {

        int typeId = QMetaType::type(rettype.constData());
        if (typeId < 0)
            return false;

        QDBusArgumentType* type = QDBusArgumentType::Lookup(typeId);
        if (!type)
            return false;

        m_Args += Entry(false, "", type);
    }
    else
    {
        m_Args += Entry(false, "", NULL);
    }

    for (int i = 0; i < types.size(); i++) {

        bool inarg = true;

        if (i == types.size() - 1 && (types[i] == "const QDBusMessage&" || types[i] == "QDBusMessage")) {
            m_AppendMessage = true;
            continue;

        } else if (types[i].startsWith("const ") && types[i].endsWith("&")) {
            types[i].remove(0, 6);
            types[i].remove(types[i].size() - 1, 1);

        } else if (types[i].endsWith("&")) {
            types[i].remove(types[i].size() - 1, 1);
            inarg = false;
        }

        int typeId = QMetaType::type(types[i].constData());
        if (typeId < 0)
            return false;

        QDBusArgumentType* type = QDBusArgumentType::Lookup(typeId);
        if (!type)
            return false;

        m_Args += Entry(inarg, names, type);
    }

    Q_ASSERT(m_Args.size() == m_MetacallData.size() || (m_AppendMessage && m_Args.size() + 1 == m_MetacallData.size()));

    for (int i = 0; i < m_Args.size(); i++) {
        if (!m_Args[i].inarg) {
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

void QDBusArgumentList::setupMetacall(const QDBusMessage* msg)
{
    QList<QVariant> args = msg->arguments();
    int argi = 0;

    for (int i = 0; i < m_Args.size(); i++) {
        if (m_Args[i].inarg) {
            QVariant arg = args.at(argi++);

            /* This should have already been assured by QDBusMessage::FromMessage */
            Q_ASSERT(arg.userType() == m_Args[i].type->m_TypeId);

            m_MetacallData[i] = const_cast<void*>(arg.data());
        }
    }

    if (m_AppendMessage) {
        m_MetacallData[m_MetacallData.size() - 1] = (void*) msg;
    }

    return true;
}











/* ------------------------------------------------------------------------- */

class QDBusMessageFactory
{
    QDBusMessageFactory(const QDBusMessageFactory&);
    QDBusMessageFactory& operator=(const QDBusMessageFactory&);
public:
    QDBusMessageFactory() : d(adbus_msg_new()) {}
    ~QDBusMessageFactory() {adbus_msg_free(d);}

    adbus_MsgFactory* d;
};

static QThreadStorage<QDBusMessageFactory*> factories;

adbus_MsgFactory* QDBusMessagePrivate::ToFactory(const QDBusMessage& msg)
{
    if (!factories.hasLocalData()) {
        factories.setLocalData(new QDBusMessageFactory);
    }

    QDBusMessageFactory* f = factories.localData();
    msg.d_ptr->setupFactory(f->d);
    return f->d;
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

        QDBusArgumentType* type = QDBusArgumentType::Lookup(sig);
        if (type) {
            QVariant variant(type->m_TypeId, (const void*) NULL);
            if (type->demarshall(&i, variant)) {
                goto err;
            }
            arguments.push_back(variant);
        } else {
            if (adbus_iter_value(&i)) {
                  goto err;
            }
            arguments.push_back(QVariant());
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

    adbus_Iterator i;
    adbus_iter_args(&i, msg);

    for (int i = 0; i < types.m_Args.size(); i++) {
        if (types.m_Args[i].inarg) {

            QDBusArgumentType* type = &types.m_Args[i].type;

            /* Check that we aren't expecting more types than provided */
            if (i.size == 0) {
                goto argument_error;
            }

            /* Check that we can figure out the provided type */
            const char* sigend = adbus_nextarg(i.sig);
            if (!sig) {
                goto parse_error;
            }

            /* Check that the provided type is as we expect */
            if (    sigend - i.sig != type->m_DBusSignature.size() 
                ||  qstrncmp(i.sig, type.m_DBusSignature.constData(), sigend - i.sig) != 0)
            {
                goto argument_error;
            }

            /* Demarshall the argument */
            QVariant variant(type->m_TypeId, (const void*) NULL);
            if (type->demarshall(&i, variant)) {
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

    adbus_msg_reset(msg);
    adbus_msg_settype(msg, type);
    adbus_msg_setflags(msg, flags);

    if (d->serial >= 0) {
        adbus_msg_setserial(msg, (uint32_t) serial);
    }

    if (d->replySerial >= 0) {
        adbus_msg_setreply(msg, (uint32_t) replySerial);
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

        QDBusArgumentType* type = QDBusArgumentType::Lookup(arg.userType());
        if (!type) {
            return false;
        }

        type->marshall(buf, arg.data(), true);
    }

    return true;
}

/* ------------------------------------------------------------------------- */

void QDBusMessagePrivate::GetReply(const QDBusMessage& msg, adbus_MsgFactory** ret, const QDBusArgumentList& types) const
{
    if (*ret && !d->delayedReply) {
    }
}

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

    if (msg->replySerial) {
        replySerial = *msg->replySerial;
    }

    if (msg->signature) {
        signature = QString::fromAscii(msg->signature, msg->signatureSize);
    }

    if (msg->path) {
        path = QString::fromAscii(msg->path, msg->pathSize);
    }

    if (msg->interface) {
        interface = QString::fromAscii(msg->interface, msg->interfaceSize);
    }

    if (msg->member) {
        member = QString::fromAscii(msg->member, msg->memberSize);
    }

    if (msg->error) {
        error = QString::fromAscii(msg->error, msg->errorSize);
    }

    if (msg->destination) {
        destination = QString::fromAscii(msg->destination, msg->destinationSize);
    }

    if (msg->sender) {
        sender = QString::fromAscii(msg->sender, msg->senderSize);
    }
}

/* ------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------- */

QDBusMessage::QDBusMessage()
{ qCopySharedData(d_ptr, new QDBusMessagePrivate); }

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

    ret.d_ptr->type = ADBUS_MSG_ERROR;
    ret.d_ptr->flags = ADBUS_MSG_NO_REPLY;
    ret.d_ptr->destination = d_ptr->sender;
    ret.d_ptr->replySerial = d_ptr->serial;
    ret.d_ptr->error = name;
    ret.d_ptr->arguments[0] = msg;

    return ret;
}

/* ------------------------------------------------------------------------- */

QDBusMessage QDBusMessage::createReply(const QList<QVariant> &arguments) const
{
    QDBusMessage ret;

    ret.d_ptr->type = ADBUS_MSG_RETURN;
    ret.d_ptr->flags = ADBUS_MSG_NO_REPLY;
    ret.d_ptr->replySerial = d_ptr->serial;
    ret.d_ptr->destination = d_ptr->sender;
    ret.d_ptr->arguments = arguments;

    return ret;
}

/* ------------------------------------------------------------------------- */

QList<QVariant> QDBusMessage::arguments() const
{ return d_ptr->arguments; }

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
{ return d_ptr->sender; }

QString QDBusMessage::path() const
{ return d_ptr->path; }

QString QDBusMessage::interface() const
{ return d_ptr->interface; }

QString QDBusMessage::member() const
{ return d_ptr->member; }

QDBusMessage::MessageType QDBusMessage::type() const
{ return (QDBusMessage::MessageType) d_ptr->type; }

QString QDBusMessage::signature() const
{ return d_ptr->signature; }

QString QDBusMessage::errorName() const
{ return d_ptr->error; }

bool QDBusMessage::isReplyRequired() const
{ return (d_ptr->flags & ADBUS_MSG_NO_REPLY) == 0; }

void QDBusMessage::setDelayedReply(bool enable) const
{ d_ptr->delayedReply = enable; }

bool QDBusMessage::isDelayedReply() const
{ return d_ptr->delayedReply; }

/* ------------------------------------------------------------------------- */

QString QDBusMessage::errorMessage() const
{
    if (d_ptr->type != ADBUS_MSG_ERROR || d_ptr->arguments.isEmpty()) {
        return QString();
    } else {
        return d_ptr->arguments[0].toString();
    }
}

/* ------------------------------------------------------------------------- */



