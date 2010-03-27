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

static int quit = 0;

static int Quit(adbus_CbData* data)
{
    adbus_check_end(data);
    quit = 1;
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
    adbus_Proxy* proxy = adbus_proxy_new(state);
    adbus_Interface* iface = CreateInterface();
    adbus_Connection* conn;

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

    {
        adbus_Call f;

        adbus_proxy_init(proxy, conn, "org.freedesktop.DBus", -1, "/", -1);
        adbus_call_method(proxy, &f, "RequestName", -1);

        adbus_msg_setsig(f.msg, "su", -1);
        adbus_msg_string(f.msg, "nz.co.foobar.adbus.TestService", -1);
        adbus_msg_u32(f.msg, 0);

        adbus_call_send(proxy, &f);
    }

    while(!quit) {
        if (adbus_conn_parsecb(conn)) {
            return 2;
        }
    }

    adbus_state_free(state);
    adbus_conn_free(conn);
    adbus_iface_free(iface);
    adbus_proxy_free(proxy);

    return 0;
}

