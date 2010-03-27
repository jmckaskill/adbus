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

#include "Server.hxx"
#include <errno.h>

TcpServer::TcpServer(adbus_Interface* iface, QObject* parent)
:   QObject(parent)
{
    adbus_Interface* i = iface ? iface : adbus_iface_new("org.freedesktop.DBus", -1);
    m_DBusServer = adbus_serv_new(i);

    m_Server = new QTcpServer;
    connect(m_Server, SIGNAL(newConnection()), SLOT(newConnection()));

    if (i != iface)
        adbus_iface_deref(i);
}

TcpServer::~TcpServer()
{
    adbus_serv_free(m_DBusServer);
    m_Server->deleteLater();
}

bool TcpServer::listen(const QHostAddress& address, quint16 port)
{ return m_Server->listen(address, port); }

void TcpServer::newConnection()
{
    QTcpSocket* sock = m_Server->nextPendingConnection();
    new Remote(sock, m_DBusServer, this);
}




LocalServer::LocalServer(adbus_Interface* iface, QObject* parent)
:   QObject(parent)
{
    adbus_Interface* i = iface ? iface : adbus_iface_new("org.freedesktop.DBus", -1);
    m_DBusServer = adbus_serv_new(i);

    m_Server = new QLocalServer;
    connect(m_Server, SIGNAL(newConnection()), SLOT(newConnection()));

    if (i != iface)
        adbus_iface_deref(i);
}

LocalServer::~LocalServer()
{
    adbus_serv_free(m_DBusServer);
    m_Server->deleteLater();
}

bool LocalServer::listen(const QString& name)
{ return m_Server->listen(name); }

void LocalServer::newConnection()
{
    QLocalSocket* sock = m_Server->nextPendingConnection();
    new Remote(sock, m_DBusServer, this);
}




Remote::Remote(QTcpSocket* socket, adbus_Server* server, QObject* parent /* = NULL */)
:   QObject(parent)
{
    Init(socket, server);
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(deleteLater()));
}

Remote::Remote(QLocalSocket* socket, adbus_Server* server, QObject* parent /* = NULL */)
:   QObject(parent)
{
    Init(socket, server);
    connect(socket, SIGNAL(error(QLocalSocket::LocalSocketError)), SLOT(deleteLater()));
}

void Remote::Init(QIODevice* socket, adbus_Server* server)
{
    m_Auth   = NULL;
    m_Remote = NULL;
    m_Socket = socket;
    m_Server = server;
    m_Buffer = adbus_buf_new();
    connect(m_Socket, SIGNAL(disconnected()), this, SLOT(deleteLater()));
    connect(m_Socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
}

Remote::~Remote()
{
    adbus_buf_free(m_Buffer);
    adbus_auth_free(m_Auth);
    adbus_remote_disconnect(m_Remote);
    m_Socket->deleteLater();
}

int Remote::SendMsg(void* d, adbus_Message* m)
{
    Remote* c = (Remote*) d;
    return c->m_Socket->write(m->data, m->size);
}

int Remote::Send(void* d, const char* b, size_t sz)
{
    Remote* c = (Remote*) d;
    return c->m_Socket->write(b, sz);
}

uint8_t Remote::Rand(void* d)
{
    (void) d;
    return (uint8_t) rand();
}

#define RECV_SIZE 64 * 1024

void Remote::readyRead()
{
    qint64 read;
    do {
        char* dest = adbus_buf_recvbuf(m_Buffer, RECV_SIZE);
        read = m_Socket->read(dest, RECV_SIZE);
        adbus_buf_recvd(m_Buffer, RECV_SIZE, read);
    } while (read == RECV_SIZE);

    while (adbus_buf_size(m_Buffer) > 0) {
        if (m_Remote) {
            if (adbus_remote_parse(m_Remote, m_Buffer)) {
                return deleteLater();
            }
            break;
        } else if (m_Auth) {
            adbus_Bool finished;
            char* data = adbus_buf_data(m_Buffer);
            size_t size = adbus_buf_size(m_Buffer);
            int used = adbus_auth_parse(m_Auth, data, size, &finished);

            if (used < 0) {
                return deleteLater();
            }

            adbus_buf_remove(m_Buffer, 0, used);

            if (finished) {
                adbus_auth_free(m_Auth);
                m_Auth = NULL;
                m_Remote = adbus_serv_connect(m_Server, &SendMsg, this);
            } else {
                break;
            }

        } else {
            char* d = adbus_buf_data(m_Buffer);
            if (*d != '\0')
                return deleteLater();
            adbus_buf_remove(m_Buffer, 0, 1);
            m_Auth = adbus_sauth_new(&Send, &Rand, this);
            adbus_sauth_external(m_Auth, NULL);
        }
    }

    if (read < 0) {
        return deleteLater();
    }
}

