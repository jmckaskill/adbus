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
#include "qdbusconnection_p.h"
#include "qdbuserror.h"
#include "qsharedfunctions_p.h"
#include "qdbusmessage_p.h"
#include "qdbuspendingreply.h"
#include "qdbusdebug.h"

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

    r.proxy = &QDBusProxy::ProxyCallback;
    r.puser = (QDBusProxy*) d;

    r.remote = d->m_Service.constData();
    r.remoteSize = d->m_Service.size();

    r.release[0] = &QDBusPendingCallPrivate::ReplyReceived;
    r.ruser[0] = d;

    r.serial = d->m_Serial;

    d->m_ConnReply = adbus_conn_addreply(d->m_Connection, &r);
}

void QDBusPendingCallPrivate::ReplyReceived(void* u)
{ 
    QDBusPendingCallPrivate* d = (QDBusPendingCallPrivate*) u;
    /* Reset m_ConnReply to NULL so that we dont try and remove the reply in
     * QDBusPendingCallPrivate::Unregister
     */
    d->m_ConnReply = NULL; 
}

QDBusPendingCallPrivate::QDBusPendingCallPrivate(const QDBusConnection& c, const QByteArray& service, uint32_t serial)
:   QDBusProxy(QDBusConnectionPrivate::Connection(c)),
    m_CheckTypes(false),
    m_QConnection(c),
    m_ConnReply(NULL),
    m_Service(service),
    m_Serial(serial),
    m_Block(0),
    m_Finished(false)
{
    adbus_conn_proxy(m_Connection, &AddReply, NULL, this);
}

QDBusPendingCall QDBusPendingCallPrivate::Create(const QDBusConnection& c, const QByteArray& service, uint32_t serial)
{ return QDBusPendingCall(new QDBusPendingCallPrivate(c, service, serial)); }

void QDBusPendingCallPrivate::unregister()
{ adbus_conn_removereply(m_Connection, m_ConnReply); }

/* ------------------------------------------------------------------------- */

void QDBusPendingCallPrivate::haveReply()
{
    adbus_conn_block(m_Connection, ADBUS_UNBLOCK, &m_Block, -1);
    m_Finished = true;

    QList<QVariant> args = m_Reply.arguments();
    if (!m_Error.isValid() && m_CheckTypes && args.size() == m_MetaTypes.size()) {
        bool success = true;
        for (int i = 0; i < args.size(); i++) {
            success = success && args.at(i).canConvert((QVariant::Type) m_MetaTypes.at(i));
        }
        m_TypeCheckFailure = !success;
    }

    emit finished();
}

void QDBusPendingCallPrivate::waitForFinished()
{
    adbus_conn_block(m_Connection, ADBUS_BLOCK, &m_Block, -1);
}

int QDBusPendingCallPrivate::ReplyCallback(adbus_CbData* data)
{
    QDBusPendingCallPrivate* d = (QDBusPendingCallPrivate*) data->user1;

    QDBUS_LOG("PendingReplyCallback: Remote '%s', Serial %d",
            d->m_Service.constData(),
            (int) d->m_Serial);

    QDBusMessagePrivate::FromMessage(d->m_Reply, data->msg);
    d->haveReply();
    return 0;
}

/* ------------------------------------------------------------------------- */

int QDBusPendingCallPrivate::ErrorCallback(adbus_CbData* data)
{
    QDBusPendingCallPrivate* d = (QDBusPendingCallPrivate*) data->user1;

    QDBUS_LOG("PendingErrorCallback: Remote '%s', Serial %d, Error '%s'",
            d->m_Service.constData(),
            (int) d->m_Serial,
            data->msg->error);

    QDBusMessagePrivate::FromMessage(d->m_Reply, data->msg);
    d->m_Error = QDBusError(d->m_Reply);
    QDBusConnectionPrivate::SetLastError(d->m_QConnection, d->m_Error);

    d->haveReply();
    return 0;
}










/* ------------------------------------------------------------------------- */

inline void qDeleteSharedData(QDBusPendingCallPrivate*& d)
{ d->destroyOnConnectionThread(); }

QDBusPendingCall::QDBusPendingCall(QDBusPendingCallPrivate* dd)
{ qCopySharedData(d, dd); }

QDBusPendingCall::QDBusPendingCall(const QDBusPendingCall& r)
{ qCopySharedData(d, r.d); }

QDBusPendingCall& QDBusPendingCall::operator=(const QDBusPendingCall& r)
{ qAssignSharedData(d, r.d); return *this; }

QDBusPendingCall::~QDBusPendingCall()
{ qDestructSharedData(d); }

/* ------------------------------------------------------------------------- */

bool QDBusPendingCall::isFinished() const
{ return d->isFinished(); }

void QDBusPendingCall::waitForFinished()
{ d->waitForFinished(); }

bool QDBusPendingCall::isError() const
{ return d->m_Error.isValid(); }

QDBusError QDBusPendingCall::error() const
{ return d->m_Error; }

QDBusMessage QDBusPendingCall::reply() const
{ return d->m_Reply; }

bool QDBusPendingCall::isValid() const
{ 
    if (d->m_CheckTypes && d->m_TypeCheckFailure)
        return false;

    return isFinished() && !isError();
}











/* ------------------------------------------------------------------------- */

void QDBusPendingCallWatcherPrivate::_q_finished()
{
    Q_Q(QDBusPendingCallWatcher);
    emit q->finished(q);
}

QDBusPendingCallWatcher::QDBusPendingCallWatcher(const QDBusPendingCall& call, QObject* parent)
:   QObject(*new QDBusPendingCallWatcherPrivate, parent),
    QDBusPendingCall(call)
{
    QObject::connect(QDBusPendingCall::d, SIGNAL(finished()), this, SLOT(_q_finished()));
}

QDBusPendingCallWatcher::~QDBusPendingCallWatcher()
{}

void QDBusPendingCallWatcher::waitForFinished()
{ QDBusPendingCall::waitForFinished(); }









/* ------------------------------------------------------------------------- */

QDBusPendingReplyData::QDBusPendingReplyData()
: QDBusPendingCall(NULL)
{}

QDBusPendingReplyData::~QDBusPendingReplyData()
{}

void QDBusPendingReplyData::assign(const QDBusPendingCall& call)
{ *((QDBusPendingCall*) this) = call; }

// Is this actually needed?
#if 0
void QDBusPendingReplyData::assign(const QDBusMessage& message)
{ }
#endif

QVariant QDBusPendingReplyData::argumentAt(int i) const
{
    QList<QVariant> args = d->m_Reply.arguments();
    if (i >= args.size() || i >= d->m_MetaTypes.size() || !isValid())
        return QVariant();

    QVariant arg = args.at(i);
    if (!arg.convert((QVariant::Type) d->m_MetaTypes.at(i)))
        return QVariant();

    return arg;
}

void QDBusPendingReplyData::setMetaTypes(int count, const int* metaTypes)
{
    d->m_MetaTypes.clear();
    for (int i = 0; i < count; i++) {
        d->m_MetaTypes << metaTypes[i];
    }
    d->m_CheckTypes = true;
    d->m_TypeCheckFailure = true;
}










