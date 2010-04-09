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

#ifndef _WIN32
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <unistd.h>
#endif



static MT_Freelist* sMsgList;
static MT_Freelist* sProxyList;

/* ------------------------------------------------------------------------- */

MT_FreelistHeader* MTI_ClientMessage_New(void)
{
    MTI_ClientMessage* m = NEW(MTI_ClientMessage);
    m->ret = adbus_msg_new();
	m->msgBuffer = adbus_buf_new();
    return &m->freelistHeader;
}

void MTI_ClientMessage_Free(MT_FreelistHeader* h)
{
    MTI_ClientMessage* m = (MTI_ClientMessage*) h;
	adbus_buf_reset(m->msgBuffer);
    adbus_msg_free(m->ret);
    free(m);
}

/* ------------------------------------------------------------------------- */

MT_FreelistHeader* MTI_ProxyMessage_New(void)
{
    MTI_ProxyMessage* m = NEW(MTI_ProxyMessage);
    return &m->freelistHeader;
}

void MTI_ProxyMessage_Free(MT_FreelistHeader* h)
{
    MTI_ProxyMessage* m = (MTI_ProxyMessage*) h;
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

int MTI_Client_SendMsg(void* u, adbus_Message* m)
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

void MTI_Client_Disconnect(MTI_Client* s)
{
#ifdef _WIN32
    if (s->loop) {
        MT_Current_Unregister(s->handle);
        MT_Current_RemoveIdle(&MTI_Client_OnIdle, s);
    }
    CloseHandle(s->handle);
    closesocket(s->sock); 
    s->handle = INVALID_HANDLE_VALUE;
#else
    if (s->loop) {
        MT_Current_Unregister(s->sock);
        MT_Current_RemoveIdle(&MTI_Client_OnIdle, s);
    }
    close(s->sock); 
#endif

    s->sock = ADBUS_SOCK_INVALID;
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

static void CallMessage(void* u)
{
    MTI_ClientMessage* m = (MTI_ClientMessage*) u;
    adbus_CbData d;

    memset(&d, 0, sizeof(d));
    d.connection = m->connection;
    d.msg = &m->msg;
    d.user1 = m->user1;
    d.user2 = m->user2;

    if (m->hasReturn) {
        d.ret = m->ret;
        adbus_msg_reset(d.ret);
    }

    adbus_dispatch(m->cb, &d);
}

static void FreeMessage(void* u)
{
    MTI_ClientMessage* m = (MTI_ClientMessage*) u;
    adbus_conn_deref(m->connection);
    MT_Freelist_Push(sMsgList, &m->freelistHeader);
}

int MTI_Client_MsgProxy(void* u, adbus_MsgCallback msgcb, adbus_CbData* d)
{
    MT_EventLoop* s = (MT_EventLoop*) u;
    MTI_ClientMessage* m = (MTI_ClientMessage*) MT_Freelist_Pop(sMsgList);

    m->user1 = d->user1;
    m->user2 = d->user2;
    m->hasReturn = (d->ret != NULL);
    m->connection = d->connection;
    m->cb = msgcb;

    adbus_clonedata(m->msgBuffer, d->msg, &m->msg);
    adbus_conn_ref(m->connection);

    m->msgHeader.call = &CallMessage;
    m->msgHeader.free = &FreeMessage;
    m->msgHeader.user = m;

	MT_Message_Post(&m->msgHeader, s);
    return 0;
}

/* ------------------------------------------------------------------------- */

static void CallProxy(void* u)
{
    MTI_ProxyMessage* m = (MTI_ProxyMessage*) u;
    m->releaseCalled = 1;
    if (m->callback) {
        m->callback(m->user);
    }
    if (m->release) {
        m->release(m->user);
    }
}

static void FreeProxy(void* u)
{
    MTI_ProxyMessage* m = (MTI_ProxyMessage*) u;
    if (!m->releaseCalled && m->release) {
        m->release(m->user);
    }

    MT_Freelist_Push(sProxyList, &m->freelistHeader);
}

void MTI_Client_Proxy(void* u, adbus_Callback cb, adbus_Callback release, void* cbuser)
{ 
    MT_EventLoop* s = (MT_EventLoop*) u;
    MTI_ProxyMessage* m = (MTI_ProxyMessage*) MT_Freelist_Pop(sProxyList);

    m->callback = cb;
    m->release = release;
    m->user = cbuser;
    m->releaseCalled = 0;

    m->msgHeader.call = &CallProxy;
    m->msgHeader.free = &FreeProxy;
    m->msgHeader.user = m;

	MT_Message_Post(&m->msgHeader, s);
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

void MTI_Client_GetProxy(void* u, adbus_ProxyMsgCallback* msgcb, void** msguser, adbus_ProxyCallback* cb, void** cbuser)
{
    MTI_Client* s = (MTI_Client*) u;
    MT_EventLoop* e = MT_Current();
    (void) s;

    if (msgcb) {
        *msgcb = &MTI_Client_MsgProxy;
    }
    if (msguser) {
        *msguser = e;
    }
    if (cb) {
        *cb = &MTI_Client_Proxy;
    }
    if (cbuser) {
        *cbuser = e;
    }
}

/* ------------------------------------------------------------------------- */

void MTI_Client_Free(void* u)
{
    MTI_Client* s = (MTI_Client*) u;
    MTI_Client_SendFlush(s, 1);
    MTI_Client_Disconnect(s);
    adbus_buf_free(s->txbuf);
    MT_Freelist_Deref(&sMsgList);
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

    MT_Freelist_Ref(&sMsgList, &MTI_ClientMessage_New, &MTI_ClientMessage_Free);
    MT_Freelist_Ref(&sProxyList, &MTI_ProxyMessage_New, &MTI_ProxyMessage_Free);

#ifdef _WIN32
    s->handle = WSACreateEvent();
#endif

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

#ifdef _WIN32
	WSAEventSelect(s->sock, s->handle, FD_READ);
    MT_Current_Register(s->handle, &MTI_Client_OnReceive, s);
#else
    MT_Current_Register(s->sock, &MTI_Client_OnReceive, s);
#endif

    MT_Current_AddIdle(&MTI_Client_OnIdle, s);

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

