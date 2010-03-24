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
#include "server-match.h"
#include "server-parse.h"

DVECTOR_INIT(ServiceQueue, adbusI_ServiceQueue*)
DLIST_INIT(Remote, adbus_Remote)

struct adbus_Remote
{
    d_List(Remote)          hl;

    adbus_Server*           server;
    d_String                unique;

    adbus_SendMsgCallback   send;
    void*                   user;

    adbusI_ServerMatchList  matches;
    adbusI_ServerParser     parser;

    /* The first message on connecting a new remote needs to be a method call
     * to "Hello" on the bus, if not we kick the connection
     */
    adbus_Bool              haveHello;

    /* Only use from the service queue code */
    d_Vector(ServiceQueue)  services;
};

struct adbusI_RemoteSet
{
    d_List(Remote)          async;
    d_List(Remote)          sync;
    unsigned int            nextRemote;
};

ADBUSI_FUNC adbus_Remote* adbusI_serv_createRemote(
        adbus_Server*           s,
        adbus_SendMsgCallback   send,
        void*                   data,
        const char*             unique, /* or NULL if you want a default name */
        adbus_Bool              needhello);

ADBUS_API void adbus_remote_disconnect(
        adbus_Remote*           r);

ADBUS_API void adbus_remote_setsynchronous(
        adbus_Remote*           r,
        adbus_Bool              sync);

ADBUS_API adbus_Remote* adbus_serv_connect(
        adbus_Server*           s,
        adbus_SendMsgCallback   send,
        void*                   user);
