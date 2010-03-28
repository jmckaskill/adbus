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
#include <stdio.h>

void* blockhandle;

static int Quit(adbus_CbData* d)
{
    adbus_check_end(d);
    adbus_conn_block(d->connection, ADBUS_UNBLOCK, &blockhandle, -1);
    return 0;
}

static adbus_Interface* CreateInterface()
{
    adbus_Interface* i = adbus_iface_new("nz.co.foobar.adbus.SimpleTest", -1);

    adbus_Member* mbr = adbus_iface_addmethod(i, "Quit", -1);
    adbus_mbr_setmethod(mbr, &Quit, NULL);

    return i;
}

int main(int argc, char* argv[])
{
    adbus_State* state = adbus_state_new();
    adbus_Interface* iface = CreateInterface();
    adbus_Connection* conn;
    adbus_Proxy* bus;

    if (argc > 1) {
        fprintf(stderr, "Connecting to %s\n", argv[1]);
        conn = adbus_sock_busconnect_s(argv[1], -1, NULL);
    } else {
        fprintf(stderr, "Connecting to the default bus\n");
        conn = adbus_sock_busconnect(ADBUS_DEFAULT_BUS, NULL);
    }

    if (conn == NULL) {
        fprintf(stderr, "Failed to connect\n");
        return 1;
    }

    {
        adbus_Bind b;

        adbus_bind_init(&b);
        b.interface = iface;
        b.path      = "/";
        adbus_state_bind(state, conn, &b);
    }

    bus = adbus_busproxy_new(state, conn);

    {
        adbus_Call f;

        adbus_busproxy_requestname(bus, &f, "nz.co.foobar.adbus.TestService", -1, 0);
        adbus_call_send(&f);
    }

    /* Wait for the quit call */
    adbus_conn_block(conn, ADBUS_BLOCK, &blockhandle, -1);

    adbus_state_free(state);
    adbus_conn_free(conn);
    adbus_iface_free(iface);
    adbus_proxy_free(bus);

    return 0;
}

