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

#define THREAD_NUM 100
#define CONCURRENT_PINGS_PER_THREAD 100
#define PINGS_PER_THREAD 1000

static QAtomicInt sCount;

PingThread::PingThread(const adbus::Connection& c)
: m_Connection(c), m_State(NULL), m_Proxy(NULL), m_Left(PINGS_PER_THREAD)
{}

void PingThread::run()
{
    m_State = new adbus::State;
    m_Proxy = new adbus::Proxy(m_State);
    m_Proxy->init(m_Connection, "nz.co.foobar.adbus.PingServer", "/");
    for (int i = 0; i < CONCURRENT_PINGS_PER_THREAD; i++) {
        ping();
    }
    exec();
    delete m_State;
    delete m_Proxy;
    m_State = NULL;
    m_Proxy = NULL;
}

void PingThread::ping()
{
    m_Proxy->method("Ping")
        .setCallback1<const char*>(&PingThread::response, this)
        .setError(&PingThread::error, this)
        .arg("str")
        .send();
}

void PingThread::error(const char* name, const char* msg)
{ qFatal("Got error %s: %s", name, msg); }

void PingThread::response(const char* str)
{
    sCount.ref();
    if (--m_Left == 0) {
        QThread::exit(0);
    } else {
        ping();
    }
}

Main::Main(const adbus::Connection& c)
{
    for (int i = 0; i < THREAD_NUM; i++) {
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
    QCoreApplication app(argc, argv);

    QDBusClient* client = new QDBusClient;
    if (!client->connectToServer(ADBUS_DEFAULT_BUS) || !client->waitForConnected()) {
        qFatal("Failed to connect");
    }

    Main m(client->base());
    return app.exec();
}

