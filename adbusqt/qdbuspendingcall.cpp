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

#include "qdbuspendingcall_p.hxx"
#include "qdbusconnection_p.hxx"
#include "qdbuserror_p.hxx"

// Unfortunately QDBusPendingCall uses 
// QExplicitlySharedDataPointer<QDBusPendingCallPrivate> d; as its private
// pointer. This is a pain since we want to detach the private object and free
// it on a different thread. So we are going to use our own definition of
// QDBusPendingCall that uses a plain pointer instead of
// QExplicitelySharedDataPointer as d.

class QDBusPendingCallPrivate;
class QDBUS_EXPORT QDBusPendingCall
{
public:
    QDBusPendingCall(const QDBusPendingCall &other);
    ~QDBusPendingCall();
    QDBusPendingCall &operator=(const QDBusPendingCall &other);

#ifndef Q_QDOC
    // pretend that they aren't here
    bool isFinished() const;
    void waitForFinished();

    bool isError() const;
    bool isValid() const;
    QDBusError error() const;
    QDBusMessage reply() const;
#endif

    static QDBusPendingCall fromError(const QDBusError &error);
    static QDBusPendingCall fromCompletedCall(const QDBusMessage &message);

protected:
    QDBusPendingCallPrivate* d; // << notice the difference
    friend class QDBusPendingCallPrivate;
    friend class QDBusPendingCallWatcher;
    friend class QDBusConnection;

    QDBusPendingCall(QDBusPendingCallPrivate *dd);

private:
    QDBusPendingCall();         // not defined
};

class QDBusPendingCallWatcherPrivate;
class QDBUS_EXPORT QDBusPendingCallWatcher: public QObject, public QDBusPendingCall
{
    Q_OBJECT
public:
    QDBusPendingCallWatcher(const QDBusPendingCall &call, QObject *parent = 0);
    ~QDBusPendingCallWatcher();

#ifdef Q_QDOC
    // trick qdoc into thinking this method is here
    bool isFinished() const;
#endif
    void waitForFinished();     // non-virtual override

Q_SIGNALS:
    void finished(QDBusPendingCallWatcher *self);

private:
    Q_DECLARE_PRIVATE(QDBusPendingCallWatcher)
    Q_PRIVATE_SLOT(d_func(), void _q_finished())
};

/* ------------------------------------------------------------------------- */

void QDBusPendingCallPrivate::AddReply(void* u)
{
    QDBusPendingCallPrivate* d = (QDBusPendingCallPrivate*) u;

    adbus_Reply r;
    adbus_reply_init(&r);

    r.callback = &QDBusPendingCallPrivate::ReplyCallback;
    r.cuser = d;

    r.error = &QDBusPendingCallPrivate::ErrorCallback;
    r.euser = d;

    r.proxy = &QDBusProxy::ProxyMsgCallback;
    r.puser = (QDBusProxy*) d;

    r.remote = d->service.constData();
    r.remoteSize = d->service.size();

    r.release[0] = &QDBusPendingCallPrivate::ReplyReceived;
    r.ruser[0] = d;

    r.serial = d->serial;

    d->connReply = adbus_conn_addreply(d->connection, &r);
}

void QDBusPendingCallPrivate::ReplyReceived(void* u)
{ 
    QDBusPendingCallPrivate* d = (QDBusPendingCallPrivate*) u;
    d->connReply = NULL; 
}

QDBusPendingCallPrivate::QDBusPendingCallPrivate(const QDBusConnection& c)
: qconnection(c),
  connection(QDBusConnectionPrivate::Connection(c)),
  connReply(NULL)
{}

QDBusPendingCall QDBusPendingCallPrivate::Create(const QDBusConnection& c, const QByteArray& service, uint32_t serial)
{
    QDBusPendingCallPrivate* d = new QDBusPendingCallPrivate(c);
    d->service = service;
    d->serial = serial;
    adbus_conn_proxy(d->connection, &AddReply, d);

    return QDBusPendingCall(d);
}

/* ------------------------------------------------------------------------- */

void QDBusPendingCallPrivate::Delete(void* u)
{
    QDBusPendingCallPrivate* d = (QDBusPendingCallPrivate*) u;

    if (d->connReply) {
        adbus_conn_removereply(d->connection, d->connReply);
    }

    delete d;
}

void QDBusPendingCallPrivate::destroy()
{
    // Release to kill any incoming messages
    setParent(NULL);
    moveToThread(NULL);

    // Delete on the connection thread
    adbus_conn_proxy(connection, &QDBusPendingCallPrivate::Delete, this);
}

/* ------------------------------------------------------------------------- */

void QDBusConnectionPrivate::haveReply()
{
    if (waiting) {
        adbus_conn_block(connection, ADBUS_UNBLOCK, -1);
    }
    emit finished();
}

int QDBusPendingCallPrivate::ReplyCallback(adbus_CbData* data)
{
    QDBusPendingCallPrivate* d = (QDBusPendingCallPrivate*) data->user1;

    adbus_Iterator i;
    adbus_iter_args(&i, data->msg);

    QByteArray sig;
    QDBusArgumentType type;
    while (i.size > 0) {
        const char* sigend = adbus_nextarg(i.sig);
        if (!sigend) {
            goto err;
        }

        sig.clear();
        sig.append(i.sig, sigend - i.sig);

        if (!qDBusLookupDBusSignature(sig, &type) && type.typeId > 0) {
            void* arg = QMetaType::construct(type.typeId);
            if (QDBusArgumentPrivate::Demarshall(&i, type.demarshall, arg)) {
                goto err;
            }
            d->replyArgs.push_back(QVariant(type.typeId, arg));
            QMetaType::destroy(type.typeId, arg);

            Q_ASSERT(sigend == i.sig);

        } else {
            if (adbus_iter_value(&i)) {
                  goto err;
            }
            d->replyArgs.push_back(QVariant());
            
        }
    }

    d->haveReply();
    return 0;

err:
    d->replyArgs.clear();
    d->error = QDBusError(QDBusError::Other, QLatin1String("nz.co.foobar.ParseError"));
    d->haveReply();
    return -1;
}

/* ------------------------------------------------------------------------- */

int QDBusPendingCallPrivate::ErrorCallback(adbus_CbData* data)
{
    QDBusPendingCallPrivate* d = (QDBusPendingCallPrivate*) data->user1;

    DBusError err = {data->msg};
    d->error = QDBusError(&err);

    d->haveReply();
    return 0;
}

/* ------------------------------------------------------------------------- */

void QDBusPendingCall::waitForFinished()
{
    Q_ASSERT(!d->waiting);
    d->waiting = true;
    adbus_conn_block(d->connection, ADBUS_BLOCK, -1);    
}







