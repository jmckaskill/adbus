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
#include "client.h"
#include "HW/Lock.h"
#include "HW/EventLoop.h"
#include "HW/Freelist.h"
#include "HW/EventQueue.h"

#ifndef _WIN32
# include <sys/types.h>
# include <sys/socket.h>
#endif

#define NEW(type) (type*) calloc(1, sizeof(type))

typedef struct HWI_Client HWI_Client;
typedef struct HWI_ClientMessage HWI_ClientMessage;
typedef struct HWI_ProxyMessage HWI_ProxyMessage;

struct HWI_Client
{
#ifdef _WIN32
	HANDLE				handle;
#endif

	adbus_Socket		sock;
	adbus_Connection*	connection;
	HW_AtomicInt		connected;
	adbus_Buffer*		txbuf;
	HW_EventLoop*		loop;
};


struct HWI_ClientMessage
{
	HW_FreelistHeader	freelistHeader;
	HW_Message			msgHeader;
	adbus_Connection*	connection;
	adbus_Message		msg;
	adbus_MsgFactory*	ret;
	void*				user1;
	void*				user2;
	adbus_Bool			hasReturn;
	adbus_MsgCallback	cb;
};

struct HWI_ProxyMessage
{
	HW_FreelistHeader	freelistHeader;
	HW_Message			msgHeader;
	adbus_Callback		callback;
	adbus_Callback		release;
	adbus_Bool			releaseCalled;
	void*				user;
};

HW_FreelistHeader* HWI_ClientMessage_New(void);
void HWI_ClientMessage_Free(HW_FreelistHeader* h);

HW_FreelistHeader* HWI_ProxyMessage_New(void);
void HWI_ProxyMessage_Free(HW_FreelistHeader* h);

int		HWI_Client_SendFlush(HWI_Client* s, size_t req);
int		HWI_Client_SendMsg(HWI_Client* s, adbus_Message* m);
int		HWI_Client_Send(HWI_Client* s, const char* buf, size_t sz);
int		HWI_Client_Recv(HWI_Client* s, char* buf, size_t sz);
uint8_t HWI_Client_Rand(HWI_Client* s);
void	HWI_Client_Disconnect(HWI_Client* s);
int		HWI_Client_DispatchExisting(HWI_Client* s);
void	HWI_Client_OnReceive(HWI_Client* s);
int		HWI_Client_Block(HWI_Client* s, adbus_BlockType type, uintptr_t* block, int timeoutms);
void	HWI_Client_Connected(HWI_Client* s);
void	HWI_Client_ConnectionProxy(HWI_Client* s, adbus_Callback cb, adbus_Callback release, void* cbuser);
void	HWI_Client_GetProxy(HWI_Client* s, adbus_ProxyMsgCallback* msgcb, void** msguser, adbus_ProxyCallback* cb, void** cbuser);
void	HWI_Client_Free(HWI_Client* s);;

void	HWI_Client_Proxy(HW_EventLoop* s, adbus_Callback cb, adbus_Callback release, void* cbuser);
int		HWI_Client_MsgProxy(HW_EventLoop* s, adbus_MsgCallback msgcb, adbus_CbData* d);
