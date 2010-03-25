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

DILIST_INIT(Reply, adbus_ConnReply)

struct adbus_ConnReply
{
    d_IList(Reply)              hl;

    adbusI_ReplySet*            set;

    adbusI_TrackedRemote*       remote;
    uint32_t                    serial;
    adbus_Bool                  incallback;

    adbus_MsgCallback           callback;
    void*                       cuser;

    adbus_MsgCallback           error;
    void*                       euser;

    adbus_ProxyMsgCallback      proxy;
    void*                       puser;

    adbus_Callback              release[2];
    void*                       ruser[2];

    adbus_ProxyCallback         relproxy;
    void*                       relpuser;
};

DHASH_MAP_INIT_UINT32(Reply, adbus_ConnReply*)

struct adbusI_ReplySet
{
    d_IList(Reply)              list;
    d_Hash(Reply)               lookup;
};

ADBUSI_FUNC void adbusI_freeReplies(adbus_Connection* c);
ADBUSI_FUNC int adbusI_dispatchReply(adbus_Connection* c, adbus_CbData* d);
ADBUSI_FUNC void adbusI_finishReply(adbus_ConnReply* reply);

ADBUS_API void adbus_reply_init(adbus_Reply* reply);

ADBUS_API adbus_ConnReply* adbus_conn_addreply(
        adbus_Connection*       connection,
        const adbus_Reply*      reply);

ADBUS_API void adbus_conn_removereply(
        adbus_Connection*       connection,
        adbus_ConnReply*        reply);
