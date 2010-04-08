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


static HW_Freelist* sMsgList;
static HW_Freelist* sProxyList;

/* ------------------------------------------------------------------------- */

HW_FreelistHeader* HWI_ClientMessage_New(void)
{
	HWI_ClientMessage* m = NEW(HWI_ClientMessage);
	m->ret = adbus_msg_new();
	return &m->freelistHeader;
}

void HWI_ClientMessage_Free(HW_FreelistHeader* h)
{
	HWI_ClientMessage* m = (HWI_ClientMessage*) h;
	adbus_msg_free(m->ret);
	free(m);
}

/* ------------------------------------------------------------------------- */

HW_FreelistHeader* HWI_ProxyMessage_New(void)
{
	HWI_ProxyMessage* m = NEW(HWI_ProxyMessage);
	return &m->freelistHeader;
}

void HWI_ProxyMessage_Free(HW_FreelistHeader* h)
{
	HWI_ProxyMessage* m = (HWI_ProxyMessage*) h;
	free(m);
}

/* ------------------------------------------------------------------------- */

int HWI_Client_SendFlush(HWI_Client* s, size_t req)
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

/* ------------------------------------------------------------------------- */

int HWI_Client_SendMsg(HWI_Client* s, adbus_Message* m)
{ 
    adbus_buf_append(s->txbuf, m->data, m->size);
	HWI_Client_SendFlush(s, 16 * 1024);
    return (int) m->size;
}

/* ------------------------------------------------------------------------- */

int HWI_Client_Send(HWI_Client* s, const char* buf, size_t sz)
{ return (int) send(s->sock, buf, sz, 0); }

/* ------------------------------------------------------------------------- */

int HWI_Client_Recv(HWI_Client* s, char* buf, size_t sz)
{
    int recvd = (int) recv(s->sock, buf, sz, 0); 
    return (recvd == 0) ? -1 : recvd;
}

/* ------------------------------------------------------------------------- */

uint8_t HWI_Client_Rand(HWI_Client* s)
{ (void) s; return (uint8_t) rand(); }

/* ------------------------------------------------------------------------- */

void HWI_Client_Disconnect(HWI_Client* s)
{
#ifdef _WIN32
	if (s->loop) {
		HW_Loop_Unregister(s->loop, s->handle);
	}
	CloseHandle(s->handle);
    closesocket(s->sock); 
	s->handle = INVALID_HANDLE_VALUE;
    s->sock = ADBUS_SOCK_INVALID;

#else
	if (s->loop) {
		HW_Loop_Unregister(s->loop, s->sock);
	}
    close(s->sock); 
    s->sock = ADBUS_SOCK_INVALID;

#endif
}

/* ------------------------------------------------------------------------- */

int HWI_Client_DispatchExisting(HWI_Client* s)
{
	int ret = 0;
	while (!ret) {
		ret = adbus_conn_continue(s->connection);
	}

	if (ret < 0) {
		HWI_Client_Disconnect(s);
		return -1;
	}

	return 0;
}

/* ------------------------------------------------------------------------- */

void HWI_Client_OnReceive(HWI_Client* s)
{
	if (adbus_conn_parsecb(s->connection)) {
		HWI_Client_Disconnect(s);
	}
	HWI_Client_DispatchExisting(s);
}

/* ------------------------------------------------------------------------- */

int HWI_Client_Block(HWI_Client* s, adbus_BlockType type, uintptr_t* block, int timeoutms)
{
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

		if (HWI_Client_DispatchExisting(s))
			return -1;

        if (HWI_Client_SendFlush(s, 1))
			return -1;

        while (!*block) {
			if (s->sock == ADBUS_SOCK_INVALID) {
				return -1;
			}
			if (HW_Loop_Step(HW_Loop_Current())) {
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

		if (HWI_Client_DispatchExisting(s))
			return -1;

        if (HWI_Client_SendFlush(s, 1))
			return -1;

        while (!*block && !s->connected) {
			if (s->sock == ADBUS_SOCK_INVALID) {
				return -1;
			}
			if (HW_Loop_Step(HW_Loop_Current())) {
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

void HWI_Client_Connected(HWI_Client* s)
{ HW_AtomicInt_Set(&s->connected, 1); }


/* ------------------------------------------------------------------------- */

static void CallMessage(HWI_ClientMessage* m)
{
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

static void FreeMessage(HWI_ClientMessage* m)
{
	adbus_freedata(&m->msg);
	adbus_conn_deref(m->connection);
	HW_Freelist_Finish(sMsgList, &m->freelistHeader);
}

int HWI_Client_MsgProxy(HW_EventLoop* s, adbus_MsgCallback msgcb, adbus_CbData* d)
{
	HWI_ClientMessage* m = (HWI_ClientMessage*) HW_Freelist_Get(sMsgList);

	m->user1 = d->user1;
	m->user2 = d->user2;
	m->hasReturn = (d->ret != NULL);
	m->connection = d->connection;
	m->cb = msgcb;

	adbus_clonedata(d->msg, &m->msg);
	adbus_conn_ref(m->connection);

	m->msgHeader.call = &CallMessage;
	m->msgHeader.free = &FreeMessage;
	m->msgHeader.user = m;

	HW_Loop_Post(s, &m->msgHeader);
	return 0;
}

/* ------------------------------------------------------------------------- */

static void CallProxy(HWI_ProxyMessage* m)
{
	m->releaseCalled = 1;
	if (m->callback) {
		m->callback(m->user);
	}
	if (m->release) {
		m->release(m->user);
	}
}

static void FreeProxy(HWI_ProxyMessage* m)
{
	if (!m->releaseCalled && m->release) {
		m->release(m->user);
	}

	HW_Freelist_Finish(sProxyList, &m->freelistHeader);
}

void HWI_Client_Proxy(HW_EventLoop* s, adbus_Callback cb, adbus_Callback release, void* cbuser)
{ 
	HWI_ProxyMessage* m = (HWI_ProxyMessage*) HW_Freelist_Get(sProxyList);

	m->callback = cb;
	m->release = release;
	m->user = cbuser;
	m->releaseCalled = 0;

	m->msgHeader.call = &CallProxy;
	m->msgHeader.free = &FreeProxy;
	m->msgHeader.user = m;

	HW_Loop_Post(s, &m->msgHeader);
}

/* ------------------------------------------------------------------------- */

void HWI_Client_ConnectionProxy(HWI_Client* s, adbus_Callback cb, adbus_Callback release, void* cbuser)
{ HWI_Client_Proxy(s->loop, cb, release, cbuser); }

/* ------------------------------------------------------------------------- */

void HWI_Client_GetProxy(HWI_Client* s, adbus_ProxyMsgCallback* msgcb, void** msguser, adbus_ProxyCallback* cb, void** cbuser)
{
	HW_EventLoop* e = HW_Loop_Current();
	(void) s;

	if (msgcb) {
		*msgcb = &HWI_Client_MsgProxy;
	}
	if (msguser) {
		*msguser = e;
	}
	if (cb) {
		*cb = &HWI_Client_Proxy;
	}
	if (cbuser) {
		*cbuser = e;
	}
}

/* ------------------------------------------------------------------------- */

void HWI_Client_Free(HWI_Client* s)
{
    HWI_Client_SendFlush(s, 1);
	HWI_Client_Disconnect(s);
    adbus_buf_free(s->txbuf);
	HW_Freelist_Deref(&sMsgList);
	HW_Freelist_Deref(&sProxyList);
    free(s);
}

/* ------------------------------------------------------------------------- */

static adbus_ConnVTable sVTable = {
    &HWI_Client_Free,           /* release */
    &HWI_Client_SendMsg,        /* send_msg */
    &HWI_Client_Recv,           /* recv_data */
    &HWI_Client_Proxy,          /* proxy */
    &HWI_Client_GetProxy,       /* get_proxy */
    &HWI_Client_Block			/* block */
};

adbus_Connection* HW_CreateDBusConnection(adbus_BusType type)
{
	HWI_Client* s = NEW(HWI_Client);
    adbus_Auth* auth = NULL;
    adbus_Bool authenticated = 0;
    int recvd = 0, used = 0;
    char buf[256];
    uintptr_t handle = 0;

	HW_Freelist_Ref(&sMsgList, &HWI_ClientMessage_New, &HWI_ClientMessage_Free);
	HW_Freelist_Ref(&sProxyList, &HWI_ProxyMessage_New, &HWI_ProxyMessage_Free);

#ifdef _WIN32
	s->handle = CreateEvent(NULL, FALSE, FALSE, NULL);
#endif

    s->connection = adbus_conn_new(&sVTable, s);
    s->txbuf = adbus_buf_new();
    s->sock = adbus_sock_connect(type);
    if (s->sock == ADBUS_SOCK_INVALID)
        goto err;

    auth = adbus_cauth_new(&HWI_Client_Send, &HWI_Client_Rand, s);
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

    adbus_conn_connect(s->connection, &HWI_Client_Connected, s);

    if (adbus_conn_parse(s->connection, buf + used, recvd - used))
        goto err;

	s->loop = HW_Loop_Current();

#ifdef _WIN32
	HW_Loop_Register(s->loop, s->handle, &HWI_Client_OnReceive, s);
#else
	HW_Loop_Register(s->loop, s->sock, &HWI_Client_OnReceive, s);
#endif

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

