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

// Don't include qdbuspendingcall.h here as we need to overload the class
// definition in the cpp file. 

// #include <qdbuspendingcall.h>
#include "qdbusobject_p.hxx"
#include <qshareddata.h>
#include <qdbuserror.h>
#include <adbus.h>

class QDBusPendingCall;

class QDBusPendingCallPrivate : public QDBusProxy, public QSharedData
{
    Q_OBJECT
public:
    static QDBusPendingCall Create(const QDBusConnection& c, const QByteArray& service, uint32_t serial);

    void destroy();
    void waitForFinished();

    QList<QVariant>   replyArgs;
    QDBusError        error;

signals:
    void finished();

private:
    friend class QDBusPendingCall;

    QDBusPendingCallPrivate(const QDBusConnection& c);
    ~QDBusPendingCallPrivate() {}

    // Called on the local thread
    static int ReplyCallback(adbus_CbData* d);
    static int ErrorCallback(adbus_CbData* d);

    // Called on the connection thread
    static void Delete(void* u);
    static void AddReply(void* u);
    static void ReplyReceived(void* u);

    void haveReply();

    QDBusConnection   qconnection;
    adbus_Connection* connection;
    adbus_ConnReply*  connReply;
    QByteArray        service;
    uint32_t          serial;
    bool              waiting;
};


