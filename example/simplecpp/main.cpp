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

#include <adbuscpp.h>

#include <string.h>
#include <stdio.h>
#ifdef _WIN32
#   include <windows.h>
#else
#   include <sys/socket.h>
#endif

static int SendMsg(void* d, adbus_Message* m)
{ return (int) send(*(adbus_Socket*) d, m->data, m->size, 0); }

static int Send(void* d, const char* b, size_t sz)
{ return (int) send(*(adbus_Socket*) d, b, sz, 0); }

static uint8_t Rand(void* d)
{ return (uint8_t) rand(); }

static int Recv(void* d, char* buf, size_t sz)
{ return (int) recv(*(adbus_Socket*) d, buf, sz, 0); }

class Quitter : public adbus::State
{
public:
    Quitter() :m_Quit(false) {}

    void quit()
    { m_Quit = true; }

    bool m_Quit;
};

static adbus::Interface<Quitter>* Interface()
{
    static adbus::Interface<Quitter>* i = NULL;
    if (i == NULL) {
        i = new adbus::Interface<Quitter>("nz.co.foobar.Test.Quit");
        i->addMethod0("Quit", &Quitter::quit);

    }

    return i;
}

static adbus_ConnectionCallbacks cbs;
static adbus_AuthConnection c;
static adbus_State* state;

static void Connected(void*)
{
    state = adbus_state_new();

    adbus::Proxy bus(state);
    bus.init(c.connection, "org.freedesktop.DBus", "/");
    bus.call("RequestName", "nz.co.foobar.adbus.SimpleCppTest", uint32_t(0));

}

int main()
{
    adbus_Socket s = adbus_sock_connect(ADBUS_SESSION_BUS);
    if (s == ADBUS_SOCK_INVALID)
        abort();

    cbs.send_message    = &SendMsg;
    cbs.recv_data       = &Recv;

    c.connection        = adbus_conn_new(&cbs, &s);
    c.auth              = adbus_cauth_new(&Send, &Rand, &s);
    c.recvCallback      = &Recv;
    c.user              = &s;
    c.connectToBus      = 1;
    c.connectCallback   = &Connected;

    Quitter q;
    q.bind(c.connection, "/", Interface(), &q);

    adbus_cauth_external(c.auth);
    adbus_aconn_connect(&c);

    while(!q.m_Quit) {
        if (adbus_aconn_parse(&c)) {
            abort();
        }
    }

    return 0;
}

