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


#include "timer.h"
#include <adbus.h>
#include <stdio.h>
#include <limits.h>

#define REPEAT      1000000
static int replies = REPEAT;
static int sent    = 0;

static adbus_Proxy* proxy;
static uintptr_t blockhandle;

static int Reply(adbus_CbData* d);
static int Error(adbus_CbData* d);

static void SendPing()
{
    if (++sent <= REPEAT) {
        adbus_Call f;

        adbus_proxy_method(proxy, &f, "Ping", -1);
        f.callback = &Reply;
        f.error = &Error;

        adbus_msg_setsig(f.msg, "s", -1);
        adbus_msg_string(f.msg, "str", -1);

        adbus_call_send(&f);
    }
}

static int Reply(adbus_CbData* d)
{
    adbus_check_string(d, NULL);
    adbus_check_end(d);

    if (--replies % 10000 == 0) {
        fprintf(stderr, "Left %d\n", replies);
    }

    if (replies <= 0) {
        adbus_conn_block(d->connection, ADBUS_UNBLOCK, &blockhandle, -1);
    } else {
        /* Keep pings going until we have enough replies */
        SendPing();
    }

    return 0;
}

static int Error(adbus_CbData* d)
{
    fprintf(stderr, "Error %s %s\n", d->msg->sender, d->msg->error);
    adbus_conn_block(d->connection, ADBUS_UNBLOCK, &blockhandle, -1);
    return 0;
}

int main()
{
    struct Timer t;
    adbus_Connection* connection;
    adbus_State* state;
    int i;

    StartTimer(&t);

    connection = adbus_sock_busconnect(ADBUS_DEFAULT_BUS, NULL);
    if (!connection)
        abort();

    adbus_conn_ref(connection);
    state = adbus_state_new();
    proxy = adbus_proxy_new(state);
    adbus_proxy_init(proxy, connection, "nz.co.foobar.adbus.PingServer", -1, "/", -1);
    adbus_proxy_setinterface(proxy, "nz.co.foobar.adbus.PingTest", -1);

    /* Limit ourselves to 100000 pings at a time */
    for (i = 0; i < 100000; ++i) {
        SendPing();
    }

    /* Wait for the pings to finish */
    adbus_conn_block(connection, ADBUS_BLOCK, &blockhandle, INT_MAX);

    adbus_proxy_free(proxy);
    adbus_state_free(state);
    adbus_conn_deref(connection);

    fprintf(stderr, "Time %f ns\n", StopTimer(&t, REPEAT));

    return 0;
}

