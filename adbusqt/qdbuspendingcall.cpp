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
#include "qdbuserror.hxx"
#include "qsharedfunctions_p.h"
#include "qdbusmessage_p.hxx"
#include "qdbuspendingreply.h"

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
:   m_QConnection(c),
    m_Connection(QDBusConnectionPrivate::Connection(c)),
    m_ConnReply(NULL),
    m_Service(service),
    m_Serial(serial),
    m_Block(NULL),
    m_Finished(false)
{
    adbus_conn_proxy(m_Connection, &AddReply, NULL, this);
}

QDBusPendingCall QDBusPendingCallPrivate::Create(const QDBusConnection& c, const QByteArray& service, uint32_t serial)
{ return QDBusPendingCall(new QDBusPendingCallPrivate(c, service, serial)); }

/* ------------------------------------------------------------------------- */

void QDBusPendingCallPrivate::Unregister(void* u)
{
    QDBusPendingCallPrivate* d = (QDBusPendingCallPrivate*) u;
    adbus_conn_removereply(d->m_Connection, d->m_ConnReply);
}

void QDBusPendingCallPrivate::Delete(void* u)
{
    QDBusPendingCallPrivate* d = (QDBusPendingCallPrivate*) u;
    delete d;
}

void QDBusPendingCallPrivate::destroy()
{
    // Release to kill any incoming messages
    setParent(NULL);
    moveToThread(NULL);

    // Delete on the connection thread
    adbus_conn_proxy(m_Connection, &Unregister, &Delete, this);
}

/* ------------------------------------------------------------------------- */

void QDBusPendingCallPrivate::haveReply()
{
    adbus_conn_block(m_Connection, ADBUS_UNBLOCK, &m_Block, -1);
    m_Finished = true;
    emit finished();
}

void QDBusPendingCallPrivate::waitForFinished()
{
    adbus_conn_block(m_Connection, ADBUS_BLOCK, &m_Block, -1);
}

int QDBusPendingCallPrivate::ReplyCallback(adbus_CbData* data)
{
    QDBusPendingCallPrivate* d = (QDBusPendingCallPrivate*) data->user1;

    QDBusMessagePrivate::FromMessage(d->m_Reply, data->msg);
    d->haveReply();
    return 0;
}

/* ------------------------------------------------------------------------- */

int QDBusPendingCallPrivate::ErrorCallback(adbus_CbData* data)
{
    QDBusPendingCallPrivate* d = (QDBusPendingCallPrivate*) data->user1;

    QDBusMessagePrivate::FromMessage(d->m_ErrorMessage, data->msg);
    d->m_Error = QDBusError(d->m_ErrorMessage);

    d->haveReply();
    return 0;
}










/* ------------------------------------------------------------------------- */

inline void qDeleteSharedData(QDBusPendingCallPrivate*& d)
{ d->destroy(); }

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
{ return isFinished() && !isError(); }











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

void QDBusPendingReplyData::setMetaTypes(int count, const int* metaTypes)
{
    d->m_ReplyMetaTypes.clear();
    for (int i = 0; i < count; i++)
        d->m_ReplyMetaTypes[i] = metaTypes[i];
}










