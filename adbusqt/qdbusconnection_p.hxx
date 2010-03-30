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
#include "qdbusconnection.hxx"
#include "qdbusobject_p.hxx"
#include <adbus.h>
#include <QtCore/qstring.h>
#include <QtCore/qthreadstorage.h>
#include <QtCore/qmutex.h>
#include <QtCore/qhash.h>

class QIODevice;
class QAbstractSocket;

class QDBusClient : public QDBusProxy
{
    Q_OBJECT
public:
    QDBusClient(adbus_BusType type, bool connectToBus = true);
    QDBusClient(const char* envstr, bool connectToBus = true);

    adbus_Connection* connection() {return m_Connection;}

Q_SIGNALS:
    void connected();
    void disconnected();

private Q_SLOTS:
    void socketReadyRead();
    void socketConnected();
    void disconnect();

private:
    ~QDBusClient();

    static int              SendMsg(void* u, adbus_Message* m);
    static int              Send(void* u, const char* b, size_t sz);
    static int              Recv(void* u, char* buf, size_t sz);
    static uint8_t          Rand(void* u);
    static adbus_Bool       ShouldProxy(void* u);
    static void             GetProxy(void* u, adbus_ProxyCallback* cb, adbus_ProxyMsgCallback* msgcb, void** data);
    static int              Block(void* u, adbus_BlockType type, void** data, int timeoutms);
    static void             ConnectedToBus(void* u);
    static void             Free(void* u);

    bool connectToServer(const char* envstr);

    static QThreadStorage<QDBusProxy*>  m_Proxies;

    bool                                m_ConnectToBus;
    bool                                m_Connected;
    adbus_Bool                          m_Authenticated;
    adbus_Connection*                   m_Connection;
    adbus_Auth*                         m_Auth;
    adbus_Buffer*                       m_Buffer;
    QIODevice*                          m_IODevice;
    QString                             m_UniqueName;
};

class QDBusConnectionPrivate : public QSharedData
{
public:
    static adbus_Connection* Connection(const QDBusConnection& c);
    static QDBusObject*     GetObject(const QDBusConnection& c, QObject* object);
    static void             RemoveObject(const QDBusConnection& c, QObject* object);
    static QDBusConnection  BusConnection(adbus_BusType type);
    static void             SetLastError(const QDBusConnection& c, QDBusError err);

    QDBusConnectionPrivate();
    ~QDBusConnectionPrivate();

    adbus_Connection*                       connection;
    QDBusConnectionInterface*               interface;

    mutable QMutex                          lock;
    QHash<QObject*, QDBusObject*>           objects;
    QDBusError                              lastError;

    static adbus_MsgFactory* GetFactory();
};

