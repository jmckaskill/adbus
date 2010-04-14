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

#include "qdbusproxy.hxx"
#include <adbus.h>
#include <adbuscpp.h>
#include <dmem/list.h>
#include <QtCore/qthreadstorage.h>
#include <QtCore/qstring.h>
#include <QtCore/qeventloop.h>

class QIODevice;
class QAbstractSocket;

/** Qt wrapper around adbus_Connection.
 *
 *  Most functionality can be gotten through adbus_Connection, except for
 *  choosing the initial connection address and kicking off the initial
 *  connect (connectToServer).
 *
 *  All functions except for the three proxing function (Proxy, GetProxy, and
 *  Block) should only be called on the connection thread.
 *
 *  adbus_Connection is an atypical base class which calls a number of virtual
 *  functions through a manual vtable (these are all the private state
 *  functions).
 */
class QDBUS_EXPORT QDBusClient : public QObject 
{
    Q_OBJECT
public:
    static adbus_Connection* Create(adbus_BusType type, bool connectToBus = true);
    static adbus_Connection* Create(const char* envstr, bool connectToBus = true);

    QDBusClient();

    bool connectToServer(adbus_BusType type, bool connectToBus = true);
    bool connectToServer(const char* envstr, bool connectToBus = true);

    adbus_Connection* base() {return m_Connection;}

Q_SIGNALS:
    void connected();
    void disconnected();

private Q_SLOTS:
    void socketReadyRead();
    void socketConnected();
    void disconnect();
    void close();

private:
    /* Called on the local thread */
    static int          SendMsg(void* u, const adbus_Message* m);
    static int          Send(void* u, const char* b, size_t sz);
    static int          Recv(void* u, char* buf, size_t sz);
    static uint8_t      Rand(void* u);
    static void         ConnectedToBus(void* u);

    bool event(QEvent* e);
    int  dispatchExisting();


    /* Called on any thread */
    static void         Proxy(void* u, adbus_Callback cb, adbus_Callback release, void* cbuser);
    static void         GetProxy(void* u, adbus_ProxyCallback* cb, void** cbuser);
    static int          Block(void* u, adbus_BlockType type, uintptr_t* data, int timeoutms);
    static void         Free(void* u);

    ~QDBusClient();


    static const adbus_ConnVTable s_VTable;

    adbus_Connection*   m_Connection;
    bool                m_ConnectToBus;
    bool                m_Connected;
    adbus_Bool          m_Authenticated;
    adbus_Auth*         m_Auth;
    adbus_Buffer*       m_Buffer;
    QIODevice*          m_IODevice;
    QString             m_UniqueName;
    bool                m_Closed;
};

/* We need a special event loop that can be timed out without running exit
 * multiple times.
 */
class QDBusEventLoop : public QEventLoop
{
    Q_OBJECT
public:
    QDBusEventLoop(int timeoutms);

    int exec();

public Q_SLOTS:
    void success();
    void failure();

private:
    bool m_Finished;
    int m_Return;
};

inline int operator<<(QString& v, adbus::Iterator& i)
{
  i.Check(ADBUS_STRING);

  const char* str;
  size_t sz;
  if (adbus_iter_string(i, &str, &sz))
    return -1;

  v = QString::fromUtf8(str, sz);
  return 0;
}

inline void operator>>(const QString& v, adbus::Buffer& b)
{ 
    QByteArray utf8 = v.toUtf8();
    adbus_buf_string(b.b, utf8.constData(), utf8.size());
}

template<class T>
inline int operator<<(QList<T>& v, adbus::Iterator& i)
{
    adbus_IterArray a;
    i.Check(ADBUS_ARRAY);
    if (adbus_iter_beginarray(i, &a))
        return -1;
    while (adbus_iter_inarray(i, &a)) {
        T t;
        if (t << i)
            return -1;
        v.push_back(t);
    }
    return adbus_iter_endarray(i, &a);
}

template<class T>
inline void operator>>(const QList<T>& v, adbus::Buffer& b)
{ 
    adbus_BufArray a;
    adbus_buf_beginarray(b, &a);

    for (int i = 0; i < v.size(); ++i) {
        adbus_buf_arrayentry(b, &a);
        v[i] >> b;
    }

    adbus_buf_endarray(b, &a);
}

