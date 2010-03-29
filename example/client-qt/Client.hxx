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

#include <adbuscpp.h>
#include <QtCore/QObject>

class QIODevice;

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
    i.Check(ADBUS_ARRAY_BEGIN);
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

namespace adbus {

    class QtClient : public QObject
    {
      Q_OBJECT
    public:
        QtClient(QObject* parent = NULL);
        ~QtClient();

        bool connectToServer(adbus_BusType type, bool connectToBus = true);
        bool connectToServer(const char* envstr, bool connectToBus = true);

        adbus_Connection* connection() {return m_Connection;}
        operator adbus_Connection*() {return m_Connection;}

    signals:
        void authenticated();
        void connected(QString name = QString());
        void disconnected();

    private slots:
        void socketReadyRead();
        void socketConnected();
        void disconnect();

    private:
        static int      SendMsg(void* u, adbus_Message* m);
        static int      Send(void* u, const char* b, size_t sz);
        static uint8_t  Rand(void* u);
        static int      Receive(void* u, char* buf, size_t sz);
        static void     Connected(void* u);
        static void     Authenticated(void* u);
        
        adbus_Auth*             m_Auth;
        adbus_Connection*       m_Connection;
        bool                    m_ConnectToBus;
        QIODevice*              m_Socket;
    };

}
