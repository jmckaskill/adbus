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

#include "dmem/string.h"

#include <adbus.h>
#include <stdio.h>
#ifdef _WIN32
#   include <windows.h>
#else
#   include <sys/socket.h>
#   include <sys/time.h>
#endif


static int replies = 0;

static adbus_ssize_t Send(void* d, adbus_Message* m)
{ return send(*(adbus_Socket*) d, m->data, m->size, 0); }

static int Reply(adbus_CbData* d)
{
    const char* rep = adbus_check_string(d, NULL);
    adbus_check_end(d);

    replies--;
    return 0;
}

#define RECV_SIZE 64 * 1024
#define REPEAT 1000000
int main()
{

#ifdef _WIN32
    WSADATA wsadata;
    int err = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (err)
        abort();

    LARGE_INTEGER start, end, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
#else
    struct timeval start, end, diff;
    gettimeofday(&start, NULL);
#endif

    adbus_Buffer* buf = adbus_buf_new();
    adbus_Socket sock = adbus_sock_connect(ADBUS_SESSION_BUS);
    if (sock == ADBUS_SOCK_INVALID || adbus_sock_cauth(sock, buf))
        abort();

    adbus_Connection* c = adbus_conn_new();
    adbus_conn_setsender(c, &Send, &sock);

    adbus_conn_connect(c, NULL, NULL);

    adbus_State* s = adbus_state_new();
    adbus_Proxy* p = adbus_proxy_new(s);
    adbus_proxy_init(p, c, "nz.co.foobar.adbus.PingServer", -1, "/", -1);

    for (int i = 0; i < REPEAT; ++i) {
        replies++;

        adbus_Call f;
        adbus_call_method(p, &f, "Ping", -1);
        f.callback = &Reply;

        adbus_msg_setsig(f.msg, "s", -1);
        adbus_msg_string(f.msg, "str", -1);

        adbus_call_send(p, &f);
    }

    while(replies > 0) {
        char* dest = adbus_buf_recvbuf(buf, RECV_SIZE);
        adbus_ssize_t recvd = recv(sock, dest, RECV_SIZE, 0);
        adbus_buf_recvd(buf, RECV_SIZE, recvd);
        if (recvd <= 0)
            abort();

        if (adbus_conn_parse(c, buf))
            abort();
    }

    adbus_proxy_free(p);
    adbus_state_free(s);
    adbus_conn_free(c);
    adbus_buf_free(buf);

#ifdef _WIN32
    QueryPerformanceCounter(&end);
    uint64_t ns = ((end.QuadPart - start.QuadPart) * 1000000000 / REPEAT) / freq.QuadPart;
#else
    gettimeofday(&end, NULL);
    timersub(&end, &start, &diff);
    uint64_t us = (uint64_t) diff.tv_sec * 1000000 + diff.tv_usec;
    uint64_t ns = us * 1000 / REPEAT;
#endif
    fprintf(stderr, "Time %d ns\n", (int) ns);

    return 0;
}

