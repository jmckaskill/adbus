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

#include "Client.hxx"

#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QLocalSocket>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QMap>

using namespace adbus;

QtClient::QtClient(QObject* parent)
:   QObject(parent),
    m_ConnectToBus(false),
    m_Auth(NULL),
    m_Socket(NULL)
{
    adbus_ConnectionCallbacks cbs = {};
    cbs.send_message = &SendMsg;
    m_Connection = adbus_conn_new(&cbs, this);
    m_Buffer = adbus_buf_new();
}

QtClient::~QtClient()
{
    if (m_Socket) {
        m_Socket->deleteLater();
        m_Socket = NULL;
        emit disconnected();
    }
    adbus_conn_free(m_Connection);
    adbus_auth_free(m_Auth);
    adbus_buf_free(m_Buffer);
}

adbus_ssize_t QtClient::SendMsg(void* d, adbus_Message* m)
{
    QtClient* c = (QtClient*) d;
    return (adbus_ssize_t) c->m_Socket->write(m->data, m->size);
}

adbus_ssize_t QtClient::Send(void* d, const char* b, size_t sz)
{
    QtClient* c = (QtClient*) d;
    return (adbus_ssize_t) c->m_Socket->write(b, sz);
}

uint8_t QtClient::Rand(void* d)
{
    (void) d;
    return (uint8_t) rand();
}

bool QtClient::connectToServer(adbus_BusType type, bool connectToBus)
{
    char buf[255];
    if (adbus_connect_address(type, buf, sizeof(buf)))
        return false;

    return connectToServer(buf, connectToBus);
}

bool QtClient::connectToServer(const char* envstr, bool connectToBus)
{
    disconnect();
    m_ConnectToBus = connectToBus;

    QString s = QString::fromAscii(envstr);
    QStringList l1 = s.split(':');
    if (l1.size() != 2)
        return false;
    QString type = l1[0];
    QMap<QString, QString> fields;
    QStringList l2 = l1[1].split(',');
    for (int i = 0; i < l2.size(); i++) {
        QStringList l3 = l2[i].split('=');
        if (l3.size() != 2)
            return false;
        fields[l3[0]] = l3[1];
    }

    if (type == "tcp" && fields.contains("host") && fields.contains("port")) {
        QTcpSocket* s = new QTcpSocket;
        s->connectToHost(fields["host"], fields["port"].toUInt());
        connect(s, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(disconnect()));
        connect(s, SIGNAL(disconnected()), this, SLOT(disconnect()));
        connect(s, SIGNAL(readyRead()), this, SLOT(socketReadyRead()));
        connect(s, SIGNAL(connected()), this, SLOT(socketConnected()));
        m_Socket = s;
        return true;

#ifndef _WIN32
    } else if (type == "unix" && fields.contains("file")) {
        QLocalSocket* s = new QLocalSocket;
        s->connectToServer(fields["file"]);
        connect(s, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(disconnect()));
        connect(s, SIGNAL(disconnected()), this, SLOT(disconnect()));
        connect(s, SIGNAL(readyRead()), this, SLOT(socketReadyRead()));
        connect(s, SIGNAL(connected()), this, SLOT(socketConnected()));
        m_Socket = s;
        return true;
#endif
    } else {
        return false;
    }
}

void QtClient::disconnect()
{
    if (m_Socket) {
        m_Socket->deleteLater();
        m_Socket = NULL;
        emit disconnected();
    }
    adbus_buf_reset(m_Buffer);
    adbus_auth_free(m_Auth);
    m_Auth = NULL;
}

void QtClient::socketConnected()
{
    m_Socket->write("\0", 1);
    if (m_Auth) {
        adbus_auth_free(m_Auth);
    }
    m_Auth = adbus_cauth_new(&Send, &Rand, this);
    adbus_cauth_external(m_Auth);
    adbus_cauth_start(m_Auth);
}

#define RECV_SIZE 64 * 1024

void QtClient::socketReadyRead()
{
    qint64 read;
    do {
        char* dest = adbus_buf_recvbuf(m_Buffer, RECV_SIZE);
        read = m_Socket->read(dest, RECV_SIZE);
        adbus_buf_recvd(m_Buffer, RECV_SIZE, read);
    } while (read == RECV_SIZE);

    if (read < 0) {
        return disconnect();
    }

    while (adbus_buf_size(m_Buffer) > 0) {
        if (m_Auth) {
            int ret = adbus_auth_parse(m_Auth, m_Buffer);
            if (ret < 0) {
                return disconnect();
            } else if (ret == 0) {
                break;
            } else if (ret > 0) {
                adbus_auth_free(m_Auth);
                m_Auth = NULL;
                if (m_ConnectToBus) {
                    adbus_conn_connect(m_Connection, &ConnectedToBus, this);
                } else {
                    emit connected();
                }
            }
        } else {
            if (adbus_conn_parse(m_Connection, m_Buffer)) {
                return disconnect();
            }
            break;
        }
    }
}

void QtClient::ConnectedToBus(void* u)
{
    QtClient* q = (QtClient*) u;
    QString qname = QString::fromUtf8(adbus_conn_uniquename(q->m_Connection, NULL));
    emit q->connected(qname);
}
    
