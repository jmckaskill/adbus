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

#include "client_p.h"
#include <limits.h>
#include <stddef.h>

#ifndef _WIN32
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <unistd.h>
#endif



static MT_Freelist* sProxyList;

/* ------------------------------------------------------------------------- */

MT_Header* MTI_ProxyMessage_New(void)
{
    MTI_ProxyMessage* m = NEW(MTI_ProxyMessage);
    return &m->header;
}

void MTI_ProxyMessage_Free(MT_Header* h)
{
    MTI_ProxyMessage* m = (MTI_ProxyMessage*) ((char*) h - offsetof(MTI_ProxyMessage, header));
    free(m);
}

/* ------------------------------------------------------------------------- */

int MTI_Client_SendFlush(MTI_Client* s, size_t req)
{
    size_t sz = adbus_buf_size(s->txbuf);
    if (sz > req) {
        int sent = send(s->sock, adbus_buf_data(s->txbuf), sz, 0); 
        if (sent > 0) {
            adbus_buf_remove(s->txbuf, 0, sent);
        }
        if (sent < 0) {
            return -1;
        }
    }
    return 0;
}

void MTI_Client_OnIdle(void* u)
{
    MTI_Client* s = (MTI_Client*) u;
    MTI_Client_SendFlush(s, 1);
}

/* ------------------------------------------------------------------------- */

int MTI_Client_SendMsg(void* u, const adbus_Message* m)
{ 
    MTI_Client* s = (MTI_Client*) u;
    adbus_buf_append(s->txbuf, m->data, m->size);
    MTI_Client_SendFlush(s, 16 * 1024);
    return (int) m->size;
}

/* ------------------------------------------------------------------------- */

int MTI_Client_Send(void* u, const char* buf, size_t sz)
{ 
    MTI_Client* s = (MTI_Client*) u;
    return (int) send(s->sock, buf, sz, 0); 
}

/* ------------------------------------------------------------------------- */

int MTI_Client_Recv(void* u, char* buf, size_t sz)
{
    MTI_Client* s = (MTI_Client*) u;
    int recvd = (int) recv(s->sock, buf, sz, 0); 
    return (recvd == 0) ? -1 : recvd;
}

/* ------------------------------------------------------------------------- */

uint8_t MTI_Client_Rand(void* u)
{ (void) u; return (uint8_t) rand(); }

/* ------------------------------------------------------------------------- */

#ifndef _WIN32
#   define closesocket(x) close(x)
#endif

void MTI_Client_Disconnect(void* u)
{
    MTI_Client* s = (MTI_Client*) u;
    MT_Current_Remove(s->reg);
    MT_Current_Remove(s->idlereg);

    closesocket(s->sock); 
    s->sock = ADBUS_SOCK_INVALID;

    adbus_conn_close(s->connection);
}

/* ------------------------------------------------------------------------- */

int MTI_Client_DispatchExisting(MTI_Client* s)
{
    int ret = 0;
    while (!ret) {
        ret = adbus_conn_continue(s->connection);
    }

    if (ret < 0) {
        MTI_Client_Disconnect(s);
        return -1;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

void MTI_Client_OnReceive(void* u)
{
    MTI_Client* s = (MTI_Client*) u;
    if (adbus_conn_parsecb(s->connection)) {
        MTI_Client_Disconnect(s);
    }
    MTI_Client_DispatchExisting(s);
}

/* ------------------------------------------------------------------------- */

int MTI_Client_Block(void* u, adbus_BlockType type, uintptr_t* block, int timeoutms)
{
    MTI_Client* s = (MTI_Client*) u;
    if (s->sock == ADBUS_SOCK_INVALID) {
        return -1;
    }

    if (timeoutms < 0) {
        timeoutms = INT_MAX;
    }

    /* This block function doesn't support timeouts */
    assert(timeoutms == INT_MAX);

    if (type == ADBUS_BLOCK) {
        assert(*block == 0);

        if (MTI_Client_DispatchExisting(s))
            return -1;

        while (!*block) {
            if (s->sock == ADBUS_SOCK_INVALID) {
                return -1;
            }
            if (MT_Current_Step()) {
                return -1;
            }
        }

        return 0;

    } else if (type == ADBUS_UNBLOCK) {
        /* Just set it to something */
        *block = 1;
        return 0;

    } else if (type == ADBUS_WAIT_FOR_CONNECTED) {
        assert(*block == 0);

        if (MTI_Client_DispatchExisting(s))
            return -1;

        while (!*block && !s->connected) {
            if (s->sock == ADBUS_SOCK_INVALID) {
                return -1;
            }
            if (MT_Current_Step()) {
                return -1;
            }
        }

        return 0;

    } else {
        assert(0);
        return -1;
    }
}

/* ------------------------------------------------------------------------- */

void MTI_Client_Connected(void* u)
{ 
    MTI_Client* s = (MTI_Client*) u;
    MT_AtomicInt_Set(&s->connected, 1); 
}


/* ------------------------------------------------------------------------- */

static void CallProxy(MT_Message* m)
{
    MTI_ProxyMessage* cm = (MTI_ProxyMessage*) m->user;
    cm->callback(cm->user);
}

static void FreeProxy(MT_Message* m)
{
    MTI_ProxyMessage* cm = (MTI_ProxyMessage*) m->user;
    if (cm->release) {
        cm->release(cm->user);
    }

    MT_Freelist_Push(sProxyList, &cm->header);
}

void MTI_Client_Proxy(void* u, adbus_Callback cb, adbus_Callback release, void* cbuser)
{ 
    MT_MainLoop* s = (MT_MainLoop*) u;
    MTI_ProxyMessage* cm = (MTI_ProxyMessage*) MT_Freelist_Pop(sProxyList);

    cm->callback = cb;
    cm->release = release;
    cm->user = cbuser;

    cm->msgHeader.call = cb ? &CallProxy : NULL;
    cm->msgHeader.free = &FreeProxy;
    cm->msgHeader.user = cm;

    MT_Loop_Post(s, &cm->msgHeader);
}

/* ------------------------------------------------------------------------- */

void MTI_Client_ConnectionProxy(void* u, adbus_Callback cb, adbus_Callback release, void* cbuser)
{ 
    MTI_Client* s = (MTI_Client*) u;

    if (MT_Current() == s->loop) {
        if (cb) {
            cb(cbuser);
        }
        if (release) {
            release(cbuser);
        }

    } else {
        MTI_Client_Proxy(s->loop, cb, release, cbuser); 
    }
}

/* ------------------------------------------------------------------------- */

void MTI_Client_GetProxy(void* u, adbus_ProxyCallback* cb, void** cbuser)
{
    MTI_Client* s = (MTI_Client*) u;
    MT_MainLoop* loop = MT_Current();
    (void) s;

    *cb = &MTI_Client_Proxy;
    *cbuser = loop;
}

/* ------------------------------------------------------------------------- */

void MTI_Client_Free(void* u)
{
    MTI_Client* s = (MTI_Client*) u;
    MTI_Client_SendFlush(s, 1);
    MTI_Client_Disconnect(s);
    adbus_buf_free(s->txbuf);
    MT_Freelist_Deref(&sProxyList);
    free(s);
}

/* ------------------------------------------------------------------------- */

static adbus_ConnVTable sVTable = {
    &MTI_Client_Free,               /* release */
    &MTI_Client_SendMsg,            /* send_msg */
    &MTI_Client_Recv,               /* recv_data */
    &MTI_Client_ConnectionProxy,    /* proxy */
    &MTI_Client_GetProxy,           /* get_proxy */
    &MTI_Client_Block               /* block */
};

adbus_Connection* MT_CreateDBusConnection(adbus_BusType type)
{
    MTI_Client* s = NEW(MTI_Client);
    adbus_Auth* auth = NULL;
    adbus_Bool authenticated = 0;
    int recvd = 0, used = 0;
    char buf[256];
    uintptr_t handle = 0;

    MT_Freelist_Ref(&sProxyList, &MTI_ProxyMessage_New, &MTI_ProxyMessage_Free);

    s->connection = adbus_conn_new(&sVTable, s);
    s->txbuf = adbus_buf_new();
    s->sock = adbus_sock_connect(type);
    s->loop = MT_Current();
    if (s->sock == ADBUS_SOCK_INVALID)
        goto err;

    auth = adbus_cauth_new(&MTI_Client_Send, &MTI_Client_Rand, s);
    adbus_cauth_external(auth);

    if (send(s->sock, "\0", 1, 0) != 1)
        goto err;

    if (adbus_cauth_start(auth))
        goto err;

    while (!authenticated) {
        recvd = recv(s->sock, buf, 256, 0);
        if (recvd < 0)
            goto err;

        used = adbus_auth_parse(auth, buf, recvd, &authenticated);
        if (used < 0)
            goto err;

    }

    adbus_conn_connect(s->connection, &MTI_Client_Connected, s);

    if (adbus_conn_parse(s->connection, buf + used, recvd - used))
        goto err;

    s->reg = MT_Current_AddClientSocket(s->sock, &MTI_Client_OnReceive, NULL, &MTI_Client_Disconnect, s);
    s->idlereg = MT_Current_AddIdle(&MTI_Client_OnIdle, s);

    if (adbus_conn_block(s->connection, ADBUS_WAIT_FOR_CONNECTED, &handle, -1))
        goto err;

    adbus_auth_free(auth);
    return s->connection;

err:
    adbus_auth_free(auth);
    adbus_conn_ref(s->connection);
    adbus_conn_deref(s->connection);
    return NULL;
}

