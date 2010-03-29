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

#pragma once

#include "qdbuspendingcall.hxx"
#include "qdbusobject_p.hxx"

#include "qdbuserror.hxx"

#include <Qt/private/qobject_p.h>
#include <QtCore/qshareddata.h>

#include <adbus.h>

class QDBusPendingCall;

class QDBusPendingCallPrivate : public QDBusProxy, public QSharedData
{
    Q_OBJECT
public:
    static QDBusPendingCall Create(const QDBusConnection& c, const QByteArray& service, uint32_t serial);

    void destroy();
    void waitForFinished();
    bool isFinished() const {return m_Finished;}

    bool                m_CheckTypes;
    bool                m_TypeCheckFailure;
    QList<int>          m_MetaTypes;
    QDBusMessage        m_Reply;
    QDBusMessage        m_ErrorMessage;
    QDBusError          m_Error;

signals:
    void finished();

private:
    QDBusPendingCallPrivate(const QDBusConnection& c, const QByteArray& service, uint32_t serial);
    ~QDBusPendingCallPrivate() {}

    // Called on the local thread
    static int ReplyCallback(adbus_CbData* d);
    static int ErrorCallback(adbus_CbData* d);

    // Called on the connection thread
    static void Delete(void* u);
    static void Unregister(void* u);
    static void AddReply(void* u);
    static void ReplyReceived(void* u);

    void haveReply();

    QDBusConnection     m_QConnection;
    adbus_Connection*   m_Connection;
    adbus_ConnReply*    m_ConnReply;
    QByteArray          m_Service;
    uint32_t            m_Serial;
    void*               m_Block;
    bool                m_Finished;
};


class QDBusPendingCallWatcherPrivate: public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QDBusPendingCallWatcher)
public:
    void _q_finished();
};

