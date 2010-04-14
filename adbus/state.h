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

#pragma once

#include "internal.h"

/* ------------------------------------------------------------------------- */


/* State multithreading
 *
 * Multithreaded support is achieved by having a connection specific data
 * structure, which is only modified on the connection thread. The list of
 * said connection structs is owned on the local thread, but the contents of
 * the conn struct is owned on the connection thread.
 *
 * Initially created and added to connection list on the local thread. Binds,
 * replies, and matches are added by sending a proxy message to the connection
 * thread. Likewise the connection struct is removed and freed via a proxied
 * message.
 */

/* ------------------------------------------------------------------------- */

DILIST_INIT(StateData, adbusI_StateData)

struct adbusI_StateData
{
    d_IList(StateData)      hl;
    adbusI_StateConn*       conn;
    union {
        adbus_Bind          bind;
        adbus_Reply         reply;
        adbus_Match         match;
    } u;
    void*                   data;
    int                     err;
    adbus_Callback          release[2];
    void*                   ruser[2];
};

DILIST_INIT(StateConn, adbusI_StateConn)

struct adbusI_StateConn
{
    d_IList(StateConn)      hl;
    adbus_Bool              refConnection;
    adbus_Connection*       connection;
    d_IList(StateData)      binds;
    d_IList(StateData)      matches;
    d_IList(StateData)      replies;
    adbus_ProxyCallback     proxy;
    void*                   puser;
};

struct adbus_State
{
    adbusI_thread_t         thread;
    adbus_Bool              refConnection;
    d_IList(StateConn)      connections;
};

