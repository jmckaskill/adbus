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
#define CONCURRENT_PINGS_PER_THREAD 10
#define PINGS_PER_THREAD 100
#define BLOCK_PINGS_PER_THREAD 10

static QAtomicInt sCount;

Pinger::Pinger(const adbus::Connection& c)
:   m_Connection(c),
    m_Proxy(this),
    m_Lua(luaL_newstate()),
    m_LeftToSend(PINGS_PER_THREAD),
    m_LeftToReceive(0)
{
    m_Proxy.init(m_Connection, "nz.co.foobar.adbus.PingServer", "/");

    // Setup the lua vm
    lua_State* L = m_Lua;
    luaL_openlibs(L);
    luaopen_adbuslua_core(L);

    lua_pushlightuserdata(L, this);
    lua_pushcclosure(L, &OnSend, 1);
    lua_setglobal(L, "on_send");

    lua_pushlightuserdata(L, this);
    lua_pushcclosure(L, &OnReply, 1);
    lua_setglobal(L, "on_reply");

    // Push the connection into lua
    lua_newtable(L);
    if (adbuslua_push_connection(L, m_Connection)) {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        abort();
    }
    lua_setglobal(L, "_CONNECTION");
    lua_remove(L, -1);

    // Load and run the lua file
    if (luaL_loadfile(L, "example/client-qt/test.lua")) {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        abort();
    }
    if (lua_pcall(L, 0, 0, 0)) {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        abort();
    }

    // Startup some pings - more will be generated in the replies to the async
    // pings
    for (int i = 0; i < CONCURRENT_PINGS_PER_THREAD; i++) {
        asyncPing();
    }
    for (int i = 0; i < BLOCK_PINGS_PER_THREAD; i++) {
        blockPing();
    }
}

Pinger::~Pinger()
{
    lua_close(m_Lua);
}

int Pinger::OnReply(lua_State* L)
{
    Pinger* p = (Pinger*) lua_touserdata(L, lua_upvalueindex(1));
    p->haveReply();
    return 0;
}

int Pinger::OnSend(lua_State* L)
{
    Pinger* p = (Pinger*) lua_touserdata(L, lua_upvalueindex(1));
    p->sendingMessage();
    return 0;
}

void Pinger::blockPing()
{
    std::string ret;
    sendingMessage();
    m_Proxy.method("Ping")
        .setError(&Pinger::error, this)
        .arg("str")
        .block(&ret);

    Q_ASSERT(ret == "str");
    haveReply();
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

void Pinger::luaPing()
{
    lua_State* L = m_Lua;

    lua_getglobal(L, "call");
    lua_pushstring(L, "str");
    if (lua_pcall(L, 1, 0, 0)) {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        abort();
    }

    lua_getglobal(L, "async_call");
    lua_pushstring(L, "str");
    if (lua_pcall(L, 1, 0, 0)) {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        abort();
    }
}

void Pinger::error(const char* name, const char* msg)
{ 
    fprintf(stderr, "Got error %s: %s\n", name, msg);
    abort();
}

void Pinger::response(const char* str)
{
    for (int i = 0; i < 2; i++) {
        blockPing();
    }

    for (int i = 0; i < 2; i++) {
        luaPing();
    }

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
    if (!p.isFinished()) {
        connect(&p, SIGNAL(finished()), this, SLOT(quit()));
        exec();
    }
}

Main::Main(const adbus::Connection& c)
#ifdef HAVE_SYNC_PINGER
: m_Pinger(c)
#endif
{
    for (int i = 0; i < THREAD_NUM; i++) {
        PingThread* p = new PingThread(c);
        p->start();
        connect(p, SIGNAL(finished()), this, SLOT(threadFinished()));
        m_Threads << p;
    }
    m_ThreadsLeft = m_Threads.size();

#ifdef HAVE_SYNC_PINGER
    if (!m_Pinger.isFinished()) {
        connect(&m_Pinger, SIGNAL(finished()), this, SLOT(threadFinished()), Qt::QueuedConnection);
        m_ThreadsLeft += 1;
    }
#endif
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

    uintptr_t block;
    adbus_Connection* c = QDBusClient::Create(ADBUS_DEFAULT_BUS);
    if (!c || adbus_conn_block(c, ADBUS_WAIT_FOR_CONNECTED, &block, -1)) {
        qFatal("Failed to connect");
    }

    Main m(c);
    return app.exec();
}

