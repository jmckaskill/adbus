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
#include "client-bind.h"
#include "client-bus.h"
#include "client-match.h"
#include "client-reply.h"
#include "tracker.h"

DLIST_INIT(ConnMsg, adbusI_ConnMsg)

struct adbusI_ConnMsg
{
    d_List(ConnMsg)             hl;
    int                         ref;
    adbus_Message               msg;
    adbus_Buffer*               buf;
    adbus_MsgFactory*           ret;
    adbus_Bool                  matchStarted;
    adbus_Bool                  matchFinished;
    adbus_Bool                  msgFinished;
};


struct adbus_Connection
{
    /** \privatesection */
    volatile long               ref;
    volatile long               nextSerial;

    adbus_ConnectionCallbacks   callbacks;
    void*                       user;

    adbus_State*                state;
    adbus_Proxy*                bus;

    adbus_Interface*            introspectable;
    adbus_Interface*            properties;

    adbus_MsgFactory*           dispatchReturn;
    adbusI_ConnMsg*             next;
    d_List(ConnMsg)             extra;

    adbusI_ConnMsg*             current;
    adbusI_ConnMsg              toprocess;

    adbusI_ConnBusData          connect;
    adbusI_ObjectTree           binds;
    adbusI_ConnMatchList        matches;
    adbusI_ReplySet             replies;
    adbusI_RemoteTracker        tracker;
};





