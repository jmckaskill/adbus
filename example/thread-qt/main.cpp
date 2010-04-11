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

#include "main.hxx"
#include <QtCore/qcoreapplication.h>
#include <QtCore/qtimer.h>

static QAtomicInt sCount;

Pinger::Pinger(const adbus::Connection& c)
:   m_Connection(c),
    m_Proxy(this),
    m_LeftToSend(1000000),
    m_LeftToReceive(0)
{
    m_Proxy.init(m_Connection, "nz.co.foobar.adbus.PingServer", "/");
	QTimer::singleShot(0, this, SLOT(start()));
}

void Pinger::start()
{
    for (int i = 0; i < 10000; i++) {
        asyncPing();
    }
}

void Pinger::asyncPing()
{
    if (m_LeftToSend-- > 0) {
        sendingMessage();
        m_Proxy.method("Ping")
            .setCallback1<const char*>(&Pinger::response, this)
            .setError(&Pinger::error, this)
            .arg("str")
            .send();
    }
}

void Pinger::error(const char* name, const char* msg)
{ 
    fprintf(stderr, "Got error %s: %s\n", name, msg);
    abort();
}

void Pinger::response(const char* str)
{
    asyncPing();
    haveReply();
}

void Pinger::sendingMessage()
{
    m_LeftToReceive++;
}

void Pinger::haveReply()
{
    sCount.ref();
    if (--m_LeftToReceive == 0) {
        emit finished();
    }
}

void PingThread::run()
{
    Pinger p(m_Connection);
    connect(&p, SIGNAL(finished()), this, SLOT(quit()));
    exec();
}

Main::Main(const adbus::Connection& c)
{
    for (int i = 0; i < 1; i++) {
        PingThread* p = new PingThread(c);
        p->start();
        connect(p, SIGNAL(finished()), this, SLOT(threadFinished()));
        m_Threads << p;
    }
    m_ThreadsLeft = m_Threads.size();
}

Main::~Main()
{
    for (int i = 0; i < m_Threads.size(); i++) {
        delete m_Threads[i];
    }
    fprintf(stderr, "%d\n", (int) sCount);
}

void Main::threadFinished()
{
    if (--m_ThreadsLeft == 0) {
        QCoreApplication::quit();
    }
}

int main(int argc, char* argv[])
{
    static char path[] = "LUA_PATH=example/client-qt/?.lua;include/lua/?/init.lua;include/lua/?.lua";
    putenv(path);

    QCoreApplication app(argc, argv);

	adbus::Connection c = QDBusClient::Create(ADBUS_DEFAULT_BUS);
    if (!c.isValid() || c.waitForConnected()) {
        qFatal("Failed to connect");
    }

    Main m(c);
    return app.exec();
}

