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
#include <string.h>
#include <stdio.h>
#ifdef _WIN32
#   include <windows.h>
#else
#   include <sys/socket.h>
#endif

#undef interface

static int SendMsg(void* d, adbus_Message* m)
{ return (int) send(*(adbus_Socket*) d, m->data, m->size, 0); }

static int Send(void* d, const char* buf, size_t sz)
{ return (int) send(*(adbus_Socket*) d, buf, sz, 0); }

static int Recv(void* d, char* buf, size_t sz)
{ return (int) recv(*(adbus_Socket*) d, buf, sz, 0); }

static uint8_t Rand(void* d)
{ return (uint8_t) rand(); }

static int quit = 0;

static int Quit(adbus_CbData* data)
{
    adbus_check_end(data);
    quit = 1;
    return 0;
}

static adbus_ConnectionCallbacks cbs;
static adbus_AuthConnection c;
static adbus_State* state;

int main(void)
{
    adbus_Socket sock = adbus_sock_connect(ADBUS_SESSION_BUS);
    if (sock == ADBUS_SOCK_INVALID)
        abort();

    state               = adbus_state_new();

    cbs.send_message    = &SendMsg;
    cbs.recv_data       = &Recv;

    c.connection        = adbus_conn_new(&cbs, &sock);
    c.auth              = adbus_cauth_new(&Send, &Rand, &sock);
    c.recvCallback      = &Recv;
    c.user              = &sock;
    c.connectToBus      = 1;

    adbus_Interface* i = adbus_iface_new("nz.co.foobar.adbus.SimpleTest", -1);
    adbus_Member* mbr = adbus_iface_addmethod(i, "Quit", -1);
    adbus_mbr_setmethod(mbr, &Quit, NULL);

    adbus_Bind b;
    adbus_bind_init(&b);
    b.interface = i;
    b.path      = "/";
    adbus_state_bind(state, c.connection, &b);
    adbus_iface_deref(i);

    adbus_aconn_connect(&c);

    while(!quit) {
        if (adbus_aconn_parse(&c)) {
            abort();
        }
    }

    adbus_conn_free(c.connection);
    adbus_auth_free(c.auth);

    return 0;
}

