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

#include <adbus/adbus.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>

static int quit = 0;

static int Quit(adbus_CbData* data)
{
    quit = 1;
    return 0;
}

struct Socket
{
    adbus_User h;
    adbus_Socket s;
};

static void Send(adbus_Message* m, const adbus_User* u)
{
    const char* msg = adbus_msg_summary(m, NULL);
    fprintf(stderr, "Sending\n%s\n\n", msg);

    struct Socket* s = (struct Socket*) u;
    size_t size;
    const uint8_t* data = adbus_msg_data(m, &size);
    send(s->s, data, size, 0);
}

int main()
{
    struct Socket sdata;
    memset(&sdata, 0, sizeof(struct Socket));

    sdata.s = adbus_sock_connect(ADBUS_SESSION_BUS);
    if (sdata.s == ADBUS_SOCK_INVALID)
        return 1;

    adbus_Message* m = adbus_msg_new();
    adbus_Stream* s = adbus_stream_new();;
    adbus_Connection* c = adbus_conn_new();
    adbus_conn_setsender(c, &Send, &sdata.h);

    adbus_Interface* i = adbus_iface_new("nz.co.foobar.Test.Quit", -1);
    adbus_Member* mbr = adbus_iface_addmethod(i, "Quit", -1);
    adbus_mbr_setmethod(mbr, &Quit, NULL);

    adbus_Path* root = adbus_conn_path(c, "/", -1);
    adbus_path_bind(root, i, NULL);

    adbus_conn_connect(c, NULL, NULL);

    char buf[64 * 1024];
    while(!quit) {
        int recvd = recv(sdata.s, buf, sizeof(buf), 0);
        if (recvd < 0)
            return 2;

        const uint8_t* data = (const uint8_t*) buf;
        size_t size = recvd;
        while (size > 0) {
            if (adbus_stream_parse(s, m, &data, &size))
                return 3;

            const char* summary = adbus_msg_summary(m, NULL);
            fprintf(stderr, "Received\n%s\n\n", summary);

            if (adbus_conn_dispatch(c, m))
                return 4;
        }
    }

    return 0;
}

