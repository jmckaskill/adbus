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
#include <adbus.h>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>

class TcpServer : public QObject
{
    Q_OBJECT
public:
    TcpServer(adbus_Interface* i = NULL, QObject* parent = NULL);
    ~TcpServer();

    bool listen(const QHostAddress& address = QHostAddress::Any, quint16 port = 0);

    adbus_Server* DBusServer() {return m_DBusServer;}

private slots:
    void newConnection();

private:
    QTcpServer*         m_Server;
    adbus_Server*       m_DBusServer;
};



class LocalServer : public QObject
{
    Q_OBJECT
public:
    LocalServer(adbus_Interface* i = NULL, QObject* parent = NULL);
    ~LocalServer();

    bool listen(const QString& name);

private slots:
    void newConnection();

private:
    QLocalServer*       m_Server;
    adbus_Server*       m_DBusServer;
};



class Remote : public QObject
{
    Q_OBJECT
public:
    Remote(QTcpSocket* socket, adbus_Server* server, QObject* parent = NULL);
    Remote(QLocalSocket* socket, adbus_Server* server, QObject* parent = NULL);
    ~Remote();

private slots:
    void readyRead();

private:
    static int SendMsg(void* d, const adbus_Message* m);
    static int Send(void* d, const char* b, size_t sz);
    static uint8_t Rand(void* d);

    void Init(QIODevice* socket, adbus_Server* server);

    adbus_Auth*         m_Auth;
    adbus_Remote*       m_Remote;
    adbus_Server*       m_Server;
    adbus_Buffer*      m_Buffer;
    QIODevice*          m_Socket;
};


