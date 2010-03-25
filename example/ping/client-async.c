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

#define _BSD_SOURCE
#include "dmem/string.h"

#include <adbus.h>
#include <stdio.h>
#ifdef _WIN32
#   include <windows.h>
#else
#   include <sys/socket.h>
#   include <sys/time.h>
#endif


static int SendMsg(void* d, adbus_Message* m)
{ return (int) send(*(adbus_Socket*) d, m->data, m->size, 0); }

static int Send(void* d, const char* buf, size_t sz)
{ return (int) send(*(adbus_Socket*) d, buf, sz, 0); }

static int Recv(void* d, char* buf, size_t sz)
{ return (int) recv(*(adbus_Socket*) d, buf, sz, 0); }

static uint8_t Rand(void* d)
{ return (uint8_t) rand(); }

#define REPEAT 1000000
static int replies = REPEAT;
static adbus_ConnectionCallbacks cbs;
static adbus_AuthConnection c;
static adbus_State* state;

static int Reply(adbus_CbData* d)
{
    adbus_check_string(d, NULL);
    adbus_check_end(d);

    replies--;
    return 0;
}

static int Error(adbus_CbData* d)
{
    fprintf(stderr, "Error %s %s\n", d->msg->sender, d->msg->error);
    exit(-1);
    return 0;
}

static void Connected(void* u)
{
    int i;
    adbus_Proxy* proxy;

    (void) u;

    state = adbus_state_new();

    proxy = adbus_proxy_new(state);
    adbus_proxy_init(proxy, c.connection, "nz.co.foobar.adbus.PingServer", -1, "/", -1);

    for (i = 0; i < replies; ++i) {
        adbus_Call f;

        adbus_call_method(proxy, &f, "Ping", -1);
        f.callback = &Reply;
        f.error = &Error;

        adbus_msg_setsig(f.msg, "s", -1);
        adbus_msg_string(f.msg, "str", -1);

        adbus_call_send(proxy, &f);
    }
}

int main()
{
    adbus_Socket sock;
    uint64_t ns;

#ifdef _WIN32
    LARGE_INTEGER start, end, freq;
    WSADATA wsadata;
    int err = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (err)
        abort();

    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
#else
    struct timeval start, end, diff;
    gettimeofday(&start, NULL);
#endif

    sock = adbus_sock_connect(ADBUS_DEFAULT_BUS);
    if (sock == ADBUS_SOCK_INVALID)
        abort();

    cbs.send_message    = &SendMsg;
    cbs.recv_data       = &Recv;

    c.connection        = adbus_conn_new(&cbs, &sock);
    c.auth              = adbus_cauth_new(&Send, &Rand, &sock);
    c.recvCallback      = &Recv;
    c.user              = &sock;
    c.connectToBus      = 1;
    c.connectCallback   = &Connected;

    adbus_cauth_external(c.auth);
    adbus_aconn_connect(&c);

    while(replies > 0) {
        if (adbus_aconn_parse(&c)) {
            abort();
        }
    }

    adbus_state_free(state);
    adbus_conn_free(c.connection);
    adbus_auth_free(c.auth);

#ifdef _WIN32
    QueryPerformanceCounter(&end);
    ns = ((end.QuadPart - start.QuadPart) * 1000000000 / REPEAT) / freq.QuadPart;
#else
    gettimeofday(&end, NULL);
    timersub(&end, &start, &diff);
    ns = (diff.tv_sec * 1000000 + diff.tv_usec) * 1000 / REPEAT;
#endif

    fprintf(stderr, "Time %d ns\n", (int) ns);

    return 0;
}

