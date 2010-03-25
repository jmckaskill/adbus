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

#include <adbus.h>
#ifdef _WIN32
#   include <windows.h>
#else
#   include <sys/socket.h>
#endif

#include <stdio.h>

#undef interface

static int SendMsg(void* d, adbus_Message* m)
{ return (int) send(*(adbus_Socket*) d, m->data, m->size, 0); }

static int Send(void* d, const char* buf, size_t sz)
{ return (int) send(*(adbus_Socket*) d, buf, sz, 0); }

static int Recv(void* d, char* buf, size_t sz)
{ return (int) recv(*(adbus_Socket*) d, buf, sz, 0); }

static uint8_t Rand(void* d)
{ return (uint8_t) rand(); }

static void Log(const char* str, size_t sz)
{ fwrite(str, 1, sz, stderr); }

static int quit = 0;
static adbus_ConnectionCallbacks cbs;
static adbus_AuthConnection c;
static adbus_State* state;

static int Quit(adbus_CbData* d)
{
    quit = 1;
    return 0;
}

static int Ping(adbus_CbData* d)
{
    const char* ping = adbus_check_string(d, NULL);
    adbus_check_end(d);

    if (d->ret) {
        adbus_msg_string(d->ret, ping, -1);
    }
    return 0;
}

static void Connected(void* u)
{
    adbus_Proxy* proxy = adbus_proxy_new(state);
    adbus_Call f;

    (void) u;

    adbus_proxy_init(proxy, c.connection, "org.freedesktop.DBus", -1, "/", -1);

    adbus_call_method(proxy, &f, "RequestName", -1);
    adbus_msg_setsig(f.msg, "su", -1);
    adbus_msg_string(f.msg, "nz.co.foobar.adbus.PingServer", -1);
    adbus_msg_u32(f.msg, 0);
    adbus_call_send(proxy, &f);

    adbus_proxy_free(proxy);
}

int main()
{
    adbus_Socket sock;

#ifdef _WIN32
    WSADATA wsadata;
    int err = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (err)
        abort();
#endif

    sock = adbus_sock_connect(ADBUS_SESSION_BUS);
    if (sock == ADBUS_SOCK_INVALID)
        abort();

    state = adbus_state_new();

    cbs.send_message    = &SendMsg;
    cbs.recv_data       = &Recv;

    c.connection        = adbus_conn_new(&cbs, &sock);
    c.auth              = adbus_cauth_new(&Send, &Rand, &sock);
    c.recvCallback      = &Recv;
    c.user              = &sock;
    c.connectToBus      = 1;
    c.connectCallback   = &Connected;

    {
        adbus_Member* mbr;
        adbus_Bind b;

        adbus_bind_init(&b);
        b.interface = adbus_iface_new("nz.co.foobar.adbus.PingTest", -1);

        mbr = adbus_iface_addmethod(b.interface, "Quit", -1);
        adbus_mbr_setmethod(mbr, &Quit, NULL);

        mbr = adbus_iface_addmethod(b.interface, "Ping", -1);
        adbus_mbr_setmethod(mbr, &Ping, NULL);
        adbus_mbr_argsig(mbr, "s", -1);
        adbus_mbr_retsig(mbr, "s", -1);

        b.path = "/";

        adbus_state_bind(state, c.connection, &b);
        adbus_iface_deref(b.interface);
    }

    adbus_cauth_external(c.auth);
    adbus_aconn_connect(&c);

    while(!quit) {
        if (adbus_aconn_parse(&c)) {
            abort();
        }
    }

    adbus_state_free(state);
    adbus_auth_free(c.auth);
    adbus_conn_free(c.connection);

    return 0;
}

