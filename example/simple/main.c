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

static int quit = 0;

static int Quit(adbus_CbData* data)
{
    adbus_check_end(data);
    quit = 1;
    return 0;
}

static int Send(void* d, adbus_Message* m)
{ return (int) send(*(adbus_Socket*) d, m->data, m->size, 0); }

#define RECV_SIZE 64 * 1024

int main(void)
{
    adbus_Buffer* buf = adbus_buf_new();
    adbus_Socket s = adbus_sock_connect(ADBUS_SESSION_BUS);
    if (s == ADBUS_SOCK_INVALID || adbus_sock_cauth(s, buf))
        abort();

    adbus_ConnectionCallbacks cbs = {};
    cbs.send_message = &Send;

    adbus_Connection* c = adbus_conn_new(&cbs, &s);
    adbus_conn_setbuffer(c, buf);

    adbus_Interface* i = adbus_iface_new("nz.co.foobar.adbus.SimpleTest", -1);
    adbus_Member* mbr = adbus_iface_addmethod(i, "Quit", -1);
    adbus_mbr_setmethod(mbr, &Quit, NULL);

    adbus_Bind b;
    adbus_bind_init(&b);
    b.interface = i;
    b.path      = "/";
    adbus_conn_bind(c, &b);

    adbus_conn_connect(c, NULL, NULL);

    while(!quit) {
        char* dest = adbus_buf_recvbuf(buf, RECV_SIZE);
        int recvd = recv(s, dest, RECV_SIZE, 0);
        adbus_buf_recvd(buf, RECV_SIZE, recvd);
        if (recvd < 0)
            abort();

        if (adbus_conn_parse(c))
            abort();
    }

    adbus_buf_free(buf);
    adbus_iface_free(i);
    adbus_conn_free(c);

    return 0;
}

