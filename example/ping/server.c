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

#ifdef _MSC_VER
#   define strdup _strdup
#endif

static int quit = 0;

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

static int Send(void* d, adbus_Message* m)
{ return (int) send(*(adbus_Socket*) d, m->data, m->size, 0); }

static void Log(const char* str, size_t sz)
{ fwrite(str, 1, sz, stderr); }

#define RECV_SIZE 64 * 1024

int main()
{
    adbus_Buffer* buffer;
    adbus_Socket sock;
    adbus_Connection* connection;
    adbus_Interface* interface;
    adbus_Member* mbr;
    adbus_ConnectionCallbacks cbs;
    adbus_State* state;
    adbus_Proxy* proxy;

#ifdef _WIN32
    WSADATA wsadata;
    int err = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (err)
        abort();
#endif

    adbus_set_logger(&Log);

    buffer = adbus_buf_new();
    sock = adbus_sock_connect(ADBUS_SESSION_BUS);
    if (sock == ADBUS_SOCK_INVALID || adbus_sock_cauth(sock, buffer))
        abort();

    memset(&cbs, 0, sizeof(cbs));
    cbs.send_message = &Send;

    state = adbus_state_new();
    connection = adbus_conn_new(&cbs, &sock);
    adbus_conn_setbuffer(connection, buffer);

    interface = adbus_iface_new("nz.co.foobar.adbus.PingTest", -1);

    mbr = adbus_iface_addmethod(interface, "Quit", -1);
    adbus_mbr_setmethod(mbr, &Quit, NULL);

    mbr = adbus_iface_addmethod(interface, "Ping", -1);
    adbus_mbr_setmethod(mbr, &Ping, NULL);
    adbus_mbr_argsig(mbr, "s", -1);
    adbus_mbr_retsig(mbr, "s", -1);

    {
        adbus_Bind b;
        adbus_bind_init(&b);
        b.interface = interface;
        b.path = "/";
        adbus_state_bind(state, connection, &b);
    }

    adbus_conn_connect(connection, NULL, NULL);

    proxy = adbus_proxy_new(state);
    adbus_proxy_init(proxy, connection, "org.freedesktop.DBus", -1, "/", -1);

    {
        adbus_Call f;
        adbus_call_method(proxy, &f, "RequestName", -1);
        adbus_msg_setsig(f.msg, "su", -1);
        adbus_msg_string(f.msg, "nz.co.foobar.adbus.PingServer", -1);
        adbus_msg_u32(f.msg, 0);
        adbus_call_send(proxy, &f);
    }

    adbus_proxy_free(proxy);

    while(!quit) {
        char* dest = adbus_buf_recvbuf(buffer, RECV_SIZE);
        int recvd = recv(sock, dest, RECV_SIZE, 0);
        adbus_buf_recvd(buffer, RECV_SIZE, recvd);
        if (recvd < 0)
            abort();

        if (adbus_conn_parse(connection))
            abort();
    }

    adbus_conn_free(connection);
    adbus_iface_free(interface);
    adbus_buf_free(buffer);

    return 0;
}

