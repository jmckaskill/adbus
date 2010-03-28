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

void* blockhandle;

static int Quit(adbus_CbData* d)
{
    adbus_conn_block(d->connection, ADBUS_UNBLOCK, &blockhandle, -1);
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

static adbus_Interface* CreateInterface()
{
    adbus_Interface* iface = adbus_iface_new("nz.co.foobar.adbus.PingTest", -1);

    adbus_Member* mbr = adbus_iface_addmethod(iface, "Quit", -1);
    adbus_mbr_setmethod(mbr, &Quit, NULL);

    mbr = adbus_iface_addmethod(iface, "Ping", -1);
    adbus_mbr_setmethod(mbr, &Ping, NULL);
    adbus_mbr_argsig(mbr, "s", -1);
    adbus_mbr_retsig(mbr, "s", -1);

    return iface;
}

int main()
{
    adbus_Connection* connection;
    adbus_State* state;
    adbus_Proxy* bus;

    connection = adbus_sock_busconnect(ADBUS_DEFAULT_BUS, NULL);
    if (!connection)
        abort();

    state = adbus_state_new();
    bus = adbus_busproxy_new(state, connection);

    /* Bind our interface to '/' */
    {
        adbus_Bind b;
        adbus_bind_init(&b);
        b.interface = CreateInterface();
        b.path      = "/";
        adbus_state_bind(state, connection, &b);

        adbus_iface_deref(b.interface);
    }

    /* Register for nz.co.foobar.adbus.PingServer */
    {
        adbus_Call f;
        adbus_busproxy_requestname(bus, &f, "nz.co.foobar.adbus.PingServer", -1, 0);
        if (adbus_call_block(&f))
            abort();
    }

    /* Block for the quit method */
    adbus_conn_block(connection, ADBUS_BLOCK, &blockhandle, -1);

    adbus_proxy_free(bus);
    adbus_state_free(state);
    adbus_conn_free(connection);

    return 0;
}

