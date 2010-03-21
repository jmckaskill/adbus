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

/* ------------------------------------------------------------------------- */

int QDBusMessagePrivate::Copy(const adbus_Message* from, QDBusMessage* to)
{
    adbus_MsgFactory* m = to->d_ptr->msg;

    adbus_msg_reset(m);
    adbus_msg_settype(m, from->type);
    adbus_msg_setflags(m, from->flags);
    adbus_msg_setserial(m, from->serial);
    if (from->replySerial)
        adbus_msg_setreply(m, *from->replySerial);
    if (from->path)
        adbus_msg_setpath(m, from->path, from->pathSize);
    if (from->interface)
        adbus_msg_setinterface(m, from->interface, from->interfaceSize);
    if (from->member)
        adbus_msg_setmember(m, from->member, from->memberSize);
    if (from->error)
        adbus_msg_seterror(m, from->error, from->errorSize);
    if (from->destination)
        adbus_msg_setdestination(m, from->destination, from->destinationSize);
    if (from->sender)
        adbus_msg_setsender(m, from->sender, from->senderSize);

    adbus_msg_setsig(m, from->signature, from->signatureSize);
    adbus_msg_append(m, from->argdata, from->argsize);

    return 0;
}

/* ------------------------------------------------------------------------- */

QDBusMessagePrivate::QDBusMessagePrivate(const QDBusMessagePrivate& other)
: QSharedData()
{
    msg = adbus_msg_new();

    adbus_MsgFactory* from = other.msg;
    adbus_MsgFactory* to = msg;

    adbus_msg_settype(to, adbus_msg_type(from));
    adbus_msg_setflags(to, adbus_msg_flags(from));
    adbus_msg_setserial(to, adbus_msg_serial(from));

    uint32_t serial;
    if (adbus_msg_reply(from, &serial))
        adbus_msg_setreply(to, serial);

    const char* str;
    size_t sz;

    if (NULL != (str = adbus_msg_path(from, &sz)))
        adbus_msg_setpath(to, str, sz);
    if (NULL != (str = adbus_msg_interface(from, &sz)))
        adbus_msg_setinterface(to, str, sz);
    if (NULL != (str = adbus_msg_sender(from, &sz)))
        adbus_msg_setsender(to, str, sz);
    if (NULL != (str = adbus_msg_destination(from, &sz)))
        adbus_msg_setdestination(to, str, sz);
    if (NULL != (str = adbus_msg_member(from, &sz)))
        adbus_msg_setmember(to, str, sz);
    if (NULL != (str = adbus_msg_error(from, &sz)))
        adbus_msg_seterror(to, str, sz);
    if (NULL != (str = adbus_msg_destination(from, &sz)))
        adbus_msg_setdestination(to, str, sz);

    const adbus_Buffer* args = adbus_msg_argbuffer_c(from);
    str = adbus_buf_sig(args, &sz);
    adbus_msg_setsig(to, str, sz);
    adbus_msg_append(to, adbus_buf_data(args), adbus_buf_size(args));
}

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

    QByteArray path8 = path.toAscii();
    QByteArray iface8 = interface.toAscii();
    QByteArray name8 = name.toAscii();

    adbus_MsgFactory* m = ret.d_ptr->msg;
    adbus_msg_settype(m, ADBUS_MSG_SIGNAL);
    adbus_msg_setflags(m, ADBUS_MSG_NO_REPLY);
    adbus_msg_setpath(m, path8.constData(), path8.size());
    adbus_msg_setinterface(m, iface8.constData(), iface8.size());
    adbus_msg_setmember(m, name8.constData(), name8.size());

    return ret;
}

/* ------------------------------------------------------------------------- */

QDBusMessage QDBusMessage::createMethodCall(const QString& destination,
                                            const QString& path,
                                            const QString& interface,
                                            const QString& method)
{
    QDBusMessage ret;

    QByteArray dest8 = destination.toAscii();
    QByteArray path8 = path.toAscii();
    QByteArray iface8 = interface.toAscii();
    QByteArray method8 = method.toAscii();

    adbus_MsgFactory* m = ret.d_ptr->msg;
    adbus_msg_settype(m, ADBUS_MSG_METHOD);
    adbus_msg_setdestination(m, dest8.constData(), dest8.size());
    adbus_msg_setpath(m, path8.constData(), path8.size());
    adbus_msg_setinterface(m, iface8.constData(), iface8.size());
    adbus_msg_setmember(m, method8.constData(), method8.size());

    return ret;
}

/* ------------------------------------------------------------------------- */

// This one is almost always pointless as the message has no destination
QDBusMessage QDBusMessage::createError(const QString &name, const QString &msg)
{
    QDBusMessage ret;

    adbus_MsgFactory* m = ret.d_ptr->msg;
    adbus_msg_settype(m, ADBUS_MSG_ERROR);

    QByteArray name8 = name.toAscii();
    adbus_msg_seterror(m, name8.constData(), name8.size());

    if (!msg.isEmpty()) {
        QByteArray msg8 = msg.toUtf8();
        adbus_msg_setsig(m, "s", 1);
        adbus_msg_string(m, msg8.constData(), msg8.size());
    }

    return ret;
}

/* ------------------------------------------------------------------------- */

// Someone missed a ref on \a name
QDBusMessage QDBusMessage::createErrorReply(const QString name,
                                            const QString& msg) const
{
    QDBusMessage ret;

    adbus_MsgFactory* m = ret.d_ptr->msg;
    adbus_msg_settype(m, ADBUS_MSG_ERROR);

    size_t sendersz;
    const char* sender = adbus_msg_sender(d_ptr->msg, &sendersz);
    adbus_msg_setdestination(m, sender, (int) sendersz);
    adbus_msg_setreply(m, adbus_msg_serial(d_ptr->msg));

    QByteArray name8 = name.toAscii();
    adbus_msg_seterror(m, name8.constData(), name8.size());

    if (!msg.isEmpty()) {
        QByteArray msg8 = msg.toUtf8();
        adbus_msg_setsig(m, "s", 1);
        adbus_msg_string(m, msg8.constData(), msg8.size());
    }

    return ret;
}

/* ------------------------------------------------------------------------- */

QDBusMessage QDBusMessage::createReply(const QList<QVariant> &arguments) const
{
    QDBusMessage ret;

    adbus_MsgFactory* m = ret.d_ptr->msg;
    adbus_msg_settype(m, ADBUS_MSG_RETURN);

    size_t sendersz;
    const char* sender = adbus_msg_sender(d_ptr->msg, &sendersz);
    adbus_msg_setdestination(m, sender, (int) sendersz);
    adbus_msg_setreply(m, adbus_msg_serial(d_ptr->msg));

    for (int i = 0; i < arguments.size(); i++)
        ret << arguments[i];

    return ret;
}

/* ------------------------------------------------------------------------- */

QDBusMessage& QDBusMessage::operator<<(const QVariant& arg)
{
    qDetachSharedData(d_ptr);
    adbus_Buffer* buf = adbus_msg_argbuffer(d_ptr->msg);
    // This will also tack on the signature
    QDBusArgumentPrivate::Marshall(buf, arg);
    return *this;
}

/* ------------------------------------------------------------------------- */

QString QDBusMessage::errorName() const
{
    size_t sz;
    const char* str = adbus_msg_error(d_ptr->msg, &sz);
    return str ? QString::fromUtf8(str, sz) : QString();
}

/* ------------------------------------------------------------------------- */

QString QDBusMessage::errorMessage() const
{
    adbus_Iterator iter = {};
    adbus_iter_buffer(&iter, adbus_msg_argbuffer(d_ptr->msg));

    if (strncmp(iter.sig, "s", 1) != 0)
        return QString();

    const char* str;
    size_t sz;
    if (adbus_iter_string(&iter, &str, &sz))
        return QString();

    return QString::fromUtf8(str, sz);
}

